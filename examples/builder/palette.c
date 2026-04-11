#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builder.h"

typedef struct {
    const char *heading;      /* NULL = widget entry, non-NULL = section label */
    const char *type;
} PaletteEntry;

static const PaletteEntry palette[] = {
    { "Layout",     NULL },
    { NULL, "Box (V)" },
    { NULL, "Box (H)" },
    { NULL, "Separator" },

    { "Input",      NULL },
    { NULL, "Button" },
    { NULL, "Text Input" },
    { NULL, "Checkbox" },
    { NULL, "Toggle" },
    { NULL, "Slider" },
    { NULL, "Dropdown" },
    { NULL, "Spinbox" },

    { "Display",    NULL },
    { NULL, "Label" },
    { NULL, "Progress" },
    { NULL, "Image" },
};
#define PALETTE_COUNT (int)(sizeof(palette) / sizeof(palette[0]))

static void on_palette_click(void *w, void *data)
{
    const char *type = (const char *)data;
    builder_canvas_add_widget(type);
}

ClueScroll *builder_palette_create(void)
{
    ClueScroll *scroll = clue_scroll_new();
    scroll->base.base.w = 130;
    scroll->base.style.vexpand = true;

    ClueBox *box = clue_box_new(CLUE_VERTICAL, 4);
    clue_style_set_padding(&box->base.style, 6);
    box->base.style.padding_right = 16; /* room for scrollbar */
    box->base.style.hexpand = true;

    for (int i = 0; i < PALETTE_COUNT; i++) {
        if (palette[i].heading) {
            /* Section header */
            ClueLabel *hdr = clue_label_new(palette[i].heading);
            hdr->base.style.fg_color = CLUE_RGB(180, 180, 195);
            hdr->base.style.margin_top = (i > 0) ? 8 : 0;
            hdr->base.style.margin_bottom = 2;
            clue_container_add(box, hdr);
            clue_container_add(box, clue_separator_new(CLUE_HORIZONTAL));
        } else {
            /* Widget button */
            ClueButton *btn = clue_button_new(palette[i].type);
            btn->base.style.hexpand = true;
            clue_signal_connect(btn, "clicked", on_palette_click,
                                (void *)palette[i].type);
            clue_container_add(box, btn);
        }
    }

    clue_container_add(scroll, box);
    return scroll;
}

/* ------------------------------------------------------------------ */
/* Widget tree (hierarchy list)                                        */
/* ------------------------------------------------------------------ */

static ClueTreeView *g_tree_view = NULL;

/* Find tree node for a given builder node index */
static ClueTreeNode *find_tree_node(ClueTreeNode *tn, int node_idx)
{
    if (!tn) return NULL;
    for (int i = 0; i < tn->child_count; i++) {
        int idx = (int)(intptr_t)tn->children[i]->user_data;
        if (idx == node_idx) return tn->children[i];
        ClueTreeNode *found = find_tree_node(tn->children[i], node_idx);
        if (found) return found;
    }
    return NULL;
}

static void on_tree_selected(void *w, void *data)
{
    ClueTreeNode *sel = clue_treeview_get_selected(g_tree_view);
    if (sel && sel->user_data) {
        int idx = (int)(intptr_t)sel->user_data;
        if (idx >= 0 && idx < g_state.count) {
            if (g_state.selected != idx)
                builder_canvas_select(idx);
        }
    }
}

static void on_tree_context(void *w, void *data)
{
    ClueTreeView *tv = g_tree_view;
    if (!tv || !tv->selected) return;
    clue_context_menu_show(builder_canvas_get_context_menu(), tv->click_x, tv->click_y);
}

ClueTreeView *builder_tree_create(void)
{
    g_tree_view = clue_treeview_new();
    g_tree_view->base.style.hexpand = true;
    g_tree_view->base.style.vexpand = true;
    clue_signal_connect(g_tree_view, "selected", on_tree_selected, NULL);
    clue_signal_connect(g_tree_view, "context", on_tree_context, NULL);
    return g_tree_view;
}

static void build_tree_nodes(ClueTreeNode *parent, int parent_id)
{
    for (int i = 0; i < g_state.count; i++) {
        if (g_state.nodes[i].parent_id != parent_id) continue;

        BuilderNode *node = &g_state.nodes[i];
        ClueTreeNode *tn = clue_tree_node_new(node->type_name);
        tn->user_data = (void *)(intptr_t)i;
        tn->expanded = true;
        clue_tree_node_add(parent, tn);

        /* Recurse for containers */
        bool is_container = (strcmp(node->type_name, "Box (V)") == 0 ||
                             strcmp(node->type_name, "Box (H)") == 0);
        if (is_container)
            build_tree_nodes(tn, node->id);
    }
}

static void free_tree_children(ClueTreeNode *node)
{
    for (int i = 0; i < node->child_count; i++) {
        free_tree_children(node->children[i]);
        free(node->children[i]->label);
        free(node->children[i]);
    }
    node->child_count = 0;
}

void builder_tree_refresh(void)
{
    if (!g_tree_view || !g_tree_view->root) return;

    /* Clear existing tree nodes (keep root) */
    free_tree_children(g_tree_view->root);

    /* Rebuild from builder state */
    build_tree_nodes(g_tree_view->root, -1);

    /* Sync selection: highlight the selected node in the tree */
    if (g_state.selected >= 0 && g_state.selected < g_state.count) {
        ClueTreeNode *tn = find_tree_node(g_tree_view->root, g_state.selected);
        g_tree_view->selected = tn;
    } else {
        g_tree_view->selected = NULL;
    }
}
