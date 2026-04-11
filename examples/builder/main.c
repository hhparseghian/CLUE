#include <stdio.h>
#include "builder.h"

BuilderState g_state = { .selected = -1 };

/* --- Menu callbacks --- */

static void on_new(void *w, void *d)
{
    while (g_state.count > 0) {
        g_state.selected = g_state.count - 1;
        builder_canvas_delete_selected();
    }
    builder_codegen_update();
}

static void on_copy_code(void *w, void *d)
{
    if (g_state.code_view) {
        const char *code = clue_text_editor_get_text(g_state.code_view);
        if (code) clue_clipboard_set(code);
    }
}

static void on_delete(void *w, void *d)
{
    builder_canvas_delete_selected();
}

static void on_duplicate(void *w, void *d)
{
    int sel = g_state.selected;
    if (sel >= 0 && sel < g_state.count)
        builder_canvas_add_widget(g_state.nodes[sel].type_name);
}

static void on_quit(void *w, void *d)
{
    clue_app_quit(clue_app_get());
}

/* --- Main --- */

int main(void)
{
    ClueApp *app = clue_app_new("CLUE Builder", 1000, 700);
    if (!app) return 1;

    /* Root layout */
    ClueBox *root = clue_box_new(CLUE_VERTICAL, 0);
    root->base.style.hexpand = true;
    root->base.style.vexpand = true;

    /* Menu bar */
    ClueMenuBar *menubar = clue_menubar_new();

    ClueMenu *file_menu = clue_menu_new();
    clue_menu_add_item(file_menu, "New Layout", on_new, NULL);
    clue_menu_add_item(file_menu, "Copy Code", on_copy_code, NULL);
    clue_menu_add_separator(file_menu);
    clue_menu_add_item(file_menu, "Quit", on_quit, NULL);

    ClueMenu *edit_menu = clue_menu_new();
    clue_menu_add_item(edit_menu, "Delete Selected", on_delete, NULL);
    clue_menu_add_item(edit_menu, "Duplicate Selected", on_duplicate, NULL);

    clue_menubar_add(menubar, "File", file_menu);
    clue_menubar_add(menubar, "Edit", edit_menu);

    /* Shortcuts */
    clue_shortcut_add("Ctrl+N", on_new, NULL);
    clue_shortcut_add("Ctrl+Q", on_quit, NULL);
    clue_shortcut_add("Ctrl+D", on_duplicate, NULL);

    /* Create panels */
    ClueScroll *palette = builder_palette_create();
    ClueScroll *canvas = builder_canvas_create();
    ClueScroll *properties = builder_properties_create();
    ClueTreeView *tree = builder_tree_create();

    /* Left panel: palette on top, widget tree below */
    ClueBox *left_panel = clue_box_new(CLUE_VERTICAL, 0);
    left_panel->base.style.vexpand = true;
    left_panel->base.base.w = 160;

    /* Palette takes natural height, tree fills the rest */
    tree->base.style.vexpand = true;

    ClueLabel *tree_title = clue_label_new("Hierarchy");
    tree_title->base.style.fg_color = CLUE_RGB(180, 180, 195);
    clue_style_set_padding(&tree_title->base.style, 6);

    clue_container_add(left_panel, palette);
    clue_container_add(left_panel, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(left_panel, tree_title);
    clue_container_add(left_panel, tree);

    /* 3-pane layout: left_panel | canvas | properties */
    ClueSplitter *right_split = clue_splitter_new(CLUE_HORIZONTAL);
    right_split->base.style.hexpand = true;
    right_split->base.style.vexpand = true;
    clue_container_add(right_split, canvas);
    clue_container_add(right_split, properties);
    clue_splitter_set_ratio(right_split, 0.75f);

    ClueSplitter *left_split = clue_splitter_new(CLUE_HORIZONTAL);
    left_split->base.style.hexpand = true;
    left_split->base.style.vexpand = true;
    clue_container_add(left_split, left_panel);
    clue_container_add(left_split, right_split);
    clue_splitter_set_ratio(left_split, 0.18f);

    /* Code output at bottom */
    ClueSplitter *vert_split = clue_splitter_new(CLUE_VERTICAL);
    vert_split->base.style.hexpand = true;
    vert_split->base.style.vexpand = true;

    g_state.code_view = clue_text_editor_new();
    g_state.code_view->base.style.hexpand = true;
    g_state.code_view->base.style.vexpand = true;

    clue_container_add(vert_split, left_split);
    clue_container_add(vert_split, g_state.code_view);
    clue_splitter_set_ratio(vert_split, 0.65f);

    /* Assemble */
    clue_container_add(root, menubar);
    clue_container_add(root, vert_split);

    clue_app_set_root(app, root);

    /* Initial code generation */
    builder_codegen_update();

    clue_app_run(app);
    clue_app_destroy(app);
    return 0;
}
