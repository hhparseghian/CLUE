#include <stdlib.h>
#include <string.h>
#include "builder.h"

/* Create a preview widget tree by re-creating the builder nodes as live widgets.
 * Returns the root widget (caller must destroy). */
static ClueWidget *create_preview_widget(const char *type_name, const char *label)
{
    if (strcmp(type_name, "Button") == 0) {
        ClueButton *b = clue_button_new(label);
        b->base.style.hexpand = true;
        return (ClueWidget *)b;
    }
    if (strcmp(type_name, "Label") == 0) {
        ClueLabel *l = clue_label_new(label);
        return (ClueWidget *)l;
    }
    if (strcmp(type_name, "Text Input") == 0) {
        ClueTextInput *t = clue_text_input_new(label);
        t->base.style.hexpand = true;
        return (ClueWidget *)t;
    }
    if (strcmp(type_name, "Checkbox") == 0) {
        return (ClueWidget *)clue_checkbox_new(label);
    }
    if (strcmp(type_name, "Toggle") == 0) {
        return (ClueWidget *)clue_toggle_new(label);
    }
    if (strcmp(type_name, "Slider") == 0) {
        ClueSlider *s = clue_slider_new(0, 100, 50);
        s->base.style.hexpand = true;
        return (ClueWidget *)s;
    }
    if (strcmp(type_name, "Dropdown") == 0) {
        ClueDropdown *d = clue_dropdown_new(label);
        d->base.base.w = 150;
        return (ClueWidget *)d;
    }
    if (strcmp(type_name, "Separator") == 0) {
        ClueSeparator *s = clue_separator_new(CLUE_HORIZONTAL);
        s->base.style.hexpand = true;
        return (ClueWidget *)s;
    }
    if (strcmp(type_name, "Progress") == 0) {
        ClueProgress *p = clue_progress_new();
        p->base.style.hexpand = true;
        clue_progress_set_value(p, 0.5f);
        return (ClueWidget *)p;
    }
    if (strcmp(type_name, "Spinbox") == 0) {
        ClueSpinbox *s = clue_spinbox_new(0, 100, 1, 0);
        return (ClueWidget *)s;
    }
    if (strcmp(type_name, "Box (V)") == 0) {
        ClueBox *b = clue_box_new(CLUE_VERTICAL, 4);
        b->base.style.hexpand = true;
        clue_style_set_padding(&b->base.style, 8);
        return (ClueWidget *)b;
    }
    if (strcmp(type_name, "Box (H)") == 0) {
        ClueBox *b = clue_box_new(CLUE_HORIZONTAL, 4);
        b->base.style.hexpand = true;
        clue_style_set_padding(&b->base.style, 8);
        return (ClueWidget *)b;
    }
    return (ClueWidget *)clue_label_new(label);
}

static bool g_preview_running = false;
static ClueOverlay *g_preview_overlay = NULL;

static void on_preview_close(ClueOverlayResult r, void *data)
{
    g_preview_overlay = NULL;
    g_preview_running = false;
}

void builder_preview_show(void)
{
    if (g_preview_running) return;
    g_preview_running = true;

    /* Build a mirror tree of the builder state */
    ClueWidget *preview_widgets[MAX_NODES] = {0};
    ClueBox *preview_root = clue_box_new(CLUE_VERTICAL, 8);
    clue_style_set_padding(&preview_root->base.style, 12);
    preview_root->base.style.hexpand = true;
    preview_root->base.style.vexpand = true;

    /* Create all widgets in order */
    for (int i = 0; i < g_state.count; i++) {
        BuilderNode *n = &g_state.nodes[i];
        ClueWidget *w = create_preview_widget(n->type_name, n->label);
        preview_widgets[i] = w;
        if (n->w > 0) w->base.w = n->w;
        if (n->h > 0) w->base.h = n->h;
        w->style.hexpand = n->hexpand;
        w->style.vexpand = n->vexpand;
    }

    /* Parent them */
    for (int i = 0; i < g_state.count; i++) {
        BuilderNode *n = &g_state.nodes[i];
        if (!preview_widgets[i]) continue;

        ClueWidget *parent = (ClueWidget *)preview_root;
        if (n->parent_id >= 0) {
            for (int j = 0; j < g_state.count; j++) {
                if (g_state.nodes[j].id == n->parent_id) {
                    parent = preview_widgets[j];
                    break;
                }
            }
        }
        clue_container_add(parent, preview_widgets[i]);
    }

    /* Show as overlay */
    g_preview_overlay = clue_overlay_new("Preview", 600, 500);
    clue_overlay_set_content(g_preview_overlay, (ClueWidget *)preview_root);
    clue_overlay_add_button(g_preview_overlay, "Close", CLUE_OVERLAY_OK);
    clue_overlay_set_callback(g_preview_overlay, on_preview_close, NULL);
    clue_overlay_show(g_preview_overlay);
}