#include <stdio.h>
#include <string.h>
#include "builder.h"

/* ------------------------------------------------------------------ */
/* Selection via canvas-level click detection                          */
/* ------------------------------------------------------------------ */

/* Find which builder node a widget belongs to */
static int find_node_for_widget(ClueWidget *w)
{
    for (int i = 0; i < g_state.count; i++) {
        if (g_state.nodes[i].widget == w)
            return i;
    }
    return -1;
}

/* Walk up the widget tree to find a builder-managed widget */
static int find_node_at(ClueWidget *w, int x, int y)
{
    if (!w || !w->base.visible) return -1;
    if (x < w->base.x || x >= w->base.x + w->base.w ||
        y < w->base.y || y >= w->base.y + w->base.h)
        return -1;

    /* Check children first (reverse order = topmost first) */
    for (int i = w->base.child_count - 1; i >= 0; i--) {
        int found = find_node_at((ClueWidget *)w->base.children[i], x, y);
        if (found >= 0) return found;
    }

    /* Check if this widget is a builder node */
    int idx = find_node_for_widget(w);
    if (idx >= 0) return idx;

    return -1;
}

static void on_drop(void *w, void *data)
{
    const ClueDragEvent *ev = clue_drag_get_event();
    ClueDragEvent *mut = (ClueDragEvent *)ev;
    mut->accepted = true;

    /* Update parent_id for the dragged widget */
    ClueWidget *target = (ClueWidget *)w;
    int src_idx = find_node_for_widget(ev->source);
    int tgt_idx = find_node_for_widget(target);
    if (src_idx >= 0) {
        g_state.nodes[src_idx].parent_id = (tgt_idx >= 0) ? g_state.nodes[tgt_idx].id : -1;
        builder_tree_refresh();
    }
}

/* --- Context menu callbacks --- */

static void on_ctx_delete(void *w, void *d)
{
    builder_canvas_delete_selected();
}

static void on_ctx_duplicate(void *w, void *d)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;
    BuilderNode *node = &g_state.nodes[sel];
    builder_canvas_add_widget(node->type_name);
}

static void on_ctx_move_up(void *w, void *d)
{
    int sel = g_state.selected;
    if (sel <= 0 || sel >= g_state.count) return;

    BuilderNode *node = &g_state.nodes[sel];
    ClueWidget *cw = node->widget;
    if (!cw || !cw->base.parent) return;

    /* Swap in node array */
    BuilderNode tmp = g_state.nodes[sel - 1];
    g_state.nodes[sel - 1] = g_state.nodes[sel];
    g_state.nodes[sel] = tmp;

    /* Reorder in parent: remove and re-add all children in new order */
    UIWidget *parent = cw->base.parent;
    int count = parent->child_count;
    UIWidget *children[MAX_NODES];
    for (int i = 0; i < count; i++)
        children[i] = parent->children[i];

    /* Find this widget in parent and swap with previous */
    for (int i = 1; i < count; i++) {
        if (children[i] == &cw->base) {
            UIWidget *tmp_w = children[i - 1];
            children[i - 1] = children[i];
            children[i] = tmp_w;
            break;
        }
    }

    /* Rebuild child array */
    for (int i = 0; i < count; i++)
        parent->children[i] = children[i];

    g_state.selected = sel - 1;
    builder_properties_refresh();
    builder_codegen_update();
    builder_tree_refresh();
}

static void on_ctx_move_down(void *w, void *d)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count - 1) return;

    BuilderNode *node = &g_state.nodes[sel];
    ClueWidget *cw = node->widget;
    if (!cw || !cw->base.parent) return;

    /* Swap in node array */
    BuilderNode tmp = g_state.nodes[sel + 1];
    g_state.nodes[sel + 1] = g_state.nodes[sel];
    g_state.nodes[sel] = tmp;

    /* Reorder in parent */
    UIWidget *parent = cw->base.parent;
    int count = parent->child_count;
    for (int i = 0; i < count - 1; i++) {
        if (parent->children[i] == &cw->base) {
            UIWidget *tmp_w = parent->children[i + 1];
            parent->children[i + 1] = parent->children[i];
            parent->children[i] = tmp_w;
            break;
        }
    }

    g_state.selected = sel + 1;
    builder_properties_refresh();
    builder_codegen_update();
    builder_tree_refresh();
}

ClueMenu *builder_canvas_get_context_menu(void)
{
    static ClueMenu *menu = NULL;
    if (!menu) {
        menu = clue_menu_new();
        clue_menu_add_item(menu, "Delete", on_ctx_delete, NULL);
        clue_menu_add_item(menu, "Duplicate", on_ctx_duplicate, NULL);
        clue_menu_add_separator(menu);
        clue_menu_add_item(menu, "Move Up", on_ctx_move_up, NULL);
        clue_menu_add_item(menu, "Move Down", on_ctx_move_down, NULL);
    }
    return menu;
}

/* --- Event interception on canvas root --- */

static ClueWidgetEventHandler g_orig_canvas_event = NULL;

static int canvas_event_intercept(UIWidget *w, ClueEvent *event)
{
    if (event->type == CLUE_EVENT_MOUSE_BUTTON && event->mouse_button.pressed) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;

        /* Bounds check — only handle clicks inside the canvas */
        if (mx < w->x || mx >= w->x + w->w || my < w->y || my >= w->y + w->h)
            goto passthrough;

        int btn = event->mouse_button.btn;
        builder_canvas_handle_click(mx, my, btn);
        if (btn == 1) return 1; /* consume right-click */
    }
    passthrough:

    /* Call original handler for normal widget behavior */
    if (g_orig_canvas_event)
        return g_orig_canvas_event(w, event);
    return 0;
}

ClueScroll *builder_canvas_create(void)
{
    ClueScroll *scroll = clue_scroll_new();
    scroll->base.style.hexpand = true;
    scroll->base.style.vexpand = true;

    ClueBox *root = clue_box_new(CLUE_VERTICAL, 6);
    clue_style_set_padding(&root->base.style, 12);
    root->base.style.hexpand = true;
    root->base.style.vexpand = true;
    root->base.style.bg_color = CLUE_RGBA(40, 42, 48, 255);
    root->base.base.intercept_events = true;

    /* Intercept events for selection and context menu */
    g_orig_canvas_event = root->base.base.on_event;
    root->base.base.on_event = canvas_event_intercept;

    clue_widget_set_drop_target(CLUE_W(root), true);
    clue_signal_connect(root, "drop", on_drop, NULL);

    g_state.canvas_root = root;

    /* Add a helper label when empty */
    ClueLabel *hint = clue_label_new("Click a widget in the palette to add it here");
    hint->base.style.fg_color = CLUE_RGB(120, 120, 130);
    hint->base.style.h_align = CLUE_ALIGN_CENTER;
    clue_container_add(root, hint);

    clue_container_add(scroll, root);
    return scroll;
}

/* Called from the app's event loop intercept — detect clicks on canvas widgets.
 * btn: 0 = left click (select), 1 = right click (context menu) */
void builder_canvas_handle_click(int mx, int my, int btn)
{
    if (!g_state.canvas_root) return;

    int found = find_node_at((ClueWidget *)g_state.canvas_root, mx, my);

    if (btn == 1 && found >= 0) {
        /* Right-click: select and show context menu */
        builder_canvas_select(found);
        clue_context_menu_show(builder_canvas_get_context_menu(), mx, my);
    } else if (btn == 0) {
        /* Left-click: select */
        builder_canvas_select(found);
    }
}

static ClueWidget *create_widget(const char *type, const char *var)
{
    if (strcmp(type, "Button") == 0) {
        ClueButton *b = clue_button_new("Button");
        b->base.style.hexpand = true;
        return (ClueWidget *)b;
    }
    if (strcmp(type, "Label") == 0) {
        ClueLabel *l = clue_label_new("Label");
        l->base.style.fg_color = CLUE_RGB(200, 200, 210);
        return (ClueWidget *)l;
    }
    if (strcmp(type, "Text Input") == 0) {
        ClueTextInput *t = clue_text_input_new("Placeholder...");
        t->base.style.hexpand = true;
        return (ClueWidget *)t;
    }
    if (strcmp(type, "Checkbox") == 0) {
        return (ClueWidget *)clue_checkbox_new("Checkbox");
    }
    if (strcmp(type, "Toggle") == 0) {
        return (ClueWidget *)clue_toggle_new("Toggle");
    }
    if (strcmp(type, "Slider") == 0) {
        ClueSlider *s = clue_slider_new(0, 100, 50);
        s->base.style.hexpand = true;
        return (ClueWidget *)s;
    }
    if (strcmp(type, "Dropdown") == 0) {
        ClueDropdown *d = clue_dropdown_new("Select...");
        d->base.base.w = 150;
        clue_dropdown_add_item(d, "Option 1");
        clue_dropdown_add_item(d, "Option 2");
        return (ClueWidget *)d;
    }
    if (strcmp(type, "Separator") == 0) {
        ClueSeparator *s = clue_separator_new(CLUE_HORIZONTAL);
        s->base.style.hexpand = true;
        return (ClueWidget *)s;
    }
    if (strcmp(type, "Progress") == 0) {
        ClueProgress *p = clue_progress_new();
        p->base.style.hexpand = true;
        clue_progress_set_value(p, 0.5f);
        return (ClueWidget *)p;
    }
    if (strcmp(type, "Spinbox") == 0) {
        ClueSpinbox *s = clue_spinbox_new(0, 100, 1, 0);
        s->base.base.w = 120;
        return (ClueWidget *)s;
    }
    if (strcmp(type, "Box (V)") == 0) {
        ClueBox *b = clue_box_new(CLUE_VERTICAL, 4);
        b->base.style.hexpand = true;
        b->base.base.h = 80;
        b->base.style.bg_color = CLUE_RGBA(55, 58, 65, 255);
        clue_style_set_padding(&b->base.style, 8);
        clue_widget_set_drop_target(CLUE_W(b), true);
        clue_signal_connect(b, "drop", on_drop, NULL);
        return (ClueWidget *)b;
    }
    if (strcmp(type, "Box (H)") == 0) {
        ClueBox *b = clue_box_new(CLUE_HORIZONTAL, 4);
        b->base.style.hexpand = true;
        b->base.base.h = 60;
        b->base.style.bg_color = CLUE_RGBA(55, 58, 65, 255);
        clue_style_set_padding(&b->base.style, 8);
        clue_widget_set_drop_target(CLUE_W(b), true);
        clue_signal_connect(b, "drop", on_drop, NULL);
        return (ClueWidget *)b;
    }
    if (strcmp(type, "Image") == 0) {
        ClueLabel *placeholder = clue_label_new("[Image]");
        placeholder->base.style.fg_color = CLUE_RGB(140, 140, 150);
        placeholder->base.base.w = 100;
        placeholder->base.base.h = 80;
        placeholder->base.style.bg_color = CLUE_RGBA(60, 62, 68, 255);
        placeholder->base.style.h_align = CLUE_ALIGN_CENTER;
        return (ClueWidget *)placeholder;
    }
    /* Fallback */
    return (ClueWidget *)clue_label_new(type);
}

void builder_canvas_add_widget(const char *type_name)
{
    if (g_state.count >= MAX_NODES) return;

    /* Remove the hint label on first add */
    if (g_state.count == 0 && g_state.canvas_root->base.base.child_count > 0) {
        UIWidget *hint = g_state.canvas_root->base.base.children[0];
        clue_widget_remove_child(&g_state.canvas_root->base.base, hint);
        clue_cwidget_destroy((ClueWidget *)hint);
    }

    int idx = g_state.count;
    BuilderNode *node = &g_state.nodes[idx];
    memset(node, 0, sizeof(*node));

    node->id = g_state.next_id++;
    node->parent_id = -1;
    node->type_name = type_name;
    snprintf(node->var_name, sizeof(node->var_name), "%s_%d",
             type_name, node->id);
    /* Sanitize var name: lowercase, replace spaces/parens */
    for (char *p = node->var_name; *p; p++) {
        if (*p == ' ' || *p == '(') *p = '_';
        if (*p == ')') { *p = '\0'; break; }
        if (*p >= 'A' && *p <= 'Z') *p += 32;
    }

    strncpy(node->label, type_name, sizeof(node->label) - 1);

    ClueWidget *w = create_widget(type_name, node->var_name);
    node->widget = w;
    node->w = w->base.w;
    node->h = w->base.h;
    node->hexpand = w->style.hexpand;
    node->vexpand = w->style.vexpand;

    clue_widget_set_draggable(w, true);

    clue_container_add(g_state.canvas_root, w);
    g_state.count++;

    builder_canvas_select(idx);
    builder_codegen_update();
    builder_tree_refresh();
}

void builder_canvas_delete_selected(void)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;

    BuilderNode *node = &g_state.nodes[sel];
    if (node->widget && node->widget->base.parent) {
        clue_widget_remove_child(node->widget->base.parent, &node->widget->base);
    }
    if (node->widget) {
        clue_cwidget_destroy(node->widget);
    }

    /* Shift remaining nodes */
    for (int i = sel; i < g_state.count - 1; i++)
        g_state.nodes[i] = g_state.nodes[i + 1];
    g_state.count--;
    g_state.selected = -1;

    builder_properties_refresh();
    builder_codegen_update();
    builder_tree_refresh();
}

void builder_canvas_select(int index)
{
    g_state.selected = index;
    builder_properties_refresh();
    builder_tree_refresh();
}

/* Draw selection highlight around the selected widget */
void builder_canvas_draw_selection(void)
{
    int sel = g_state.selected;
    if (sel < 0 || sel >= g_state.count) return;

    ClueWidget *w = g_state.nodes[sel].widget;
    if (!w || !w->base.visible) return;

    const ClueTheme *th = clue_theme_get();
    clue_draw_rounded_rect(w->base.x - 2, w->base.y - 2,
                           w->base.w + 4, w->base.h + 4,
                           4.0f, 2.0f, th->accent);
}
