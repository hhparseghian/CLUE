#include "demo.h"

static void on_tree_selected(void *widget, void *data)
{
    (void)data;
    ClueTreeNode *node = clue_treeview_get_selected(widget);
    if (node && node->label) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Tree: %s", node->label);
        clue_label_set_text(g_status, buf);
    }
}

ClueBox *build_tree_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueTreeView *tv = clue_treeview_new();
    tv->base.base.w = 350;
    tv->base.base.h = 300;
    clue_signal_connect(tv, "selected", on_tree_selected, NULL);

    ClueTreeNode *src = clue_tree_node_add(tv->root, clue_tree_node_new("src"));
    src->expanded = true;
    ClueTreeNode *core = clue_tree_node_add(src, clue_tree_node_new("core"));
    core->expanded = true;
    clue_tree_node_add(core, clue_tree_node_new("window.c"));
    clue_tree_node_add(core, clue_tree_node_new("event.c"));
    clue_tree_node_add(core, clue_tree_node_new("widget.c"));

    ClueTreeNode *widgets = clue_tree_node_add(src, clue_tree_node_new("widgets"));
    clue_tree_node_add(widgets, clue_tree_node_new("button.c"));
    clue_tree_node_add(widgets, clue_tree_node_new("label.c"));
    clue_tree_node_add(widgets, clue_tree_node_new("slider.c"));
    clue_tree_node_add(widgets, clue_tree_node_new("treeview.c"));

    ClueTreeNode *inc = clue_tree_node_add(tv->root, clue_tree_node_new("include"));
    ClueTreeNode *clue_dir = clue_tree_node_add(inc, clue_tree_node_new("clue"));
    clue_tree_node_add(clue_dir, clue_tree_node_new("clue.h"));
    clue_tree_node_add(clue_dir, clue_tree_node_new("widget.h"));

    clue_tree_node_add(tv->root, clue_tree_node_new("CMakeLists.txt"));
    clue_tree_node_add(tv->root, clue_tree_node_new("README.md"));

    clue_container_add(page, tv);
    return page;
}
