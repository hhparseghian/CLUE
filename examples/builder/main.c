#include <stdio.h>
#include <string.h>
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

static char g_current_path[1024] = "";

static ClueFileFilter layout_filters[] = {
    { "Layout files", ".json" },
};

static void on_save_as(void *w, void *d)
{
    ClueFileDialogResult r = clue_file_dialog_save("Save Layout As",
                                                    NULL, "layout.json",
                                                    layout_filters, 1);
    if (r.ok) {
        strncpy(g_current_path, r.path, sizeof(g_current_path) - 1);
        g_current_path[sizeof(g_current_path) - 1] = '\0';
        builder_save(g_current_path);
    }
}

static void on_save(void *w, void *d)
{
    if (g_current_path[0]) {
        builder_save(g_current_path);
    } else {
        on_save_as(NULL, NULL);
    }
}

static void on_load(void *w, void *d)
{
    ClueFileDialogResult r = clue_file_dialog_open("Open Layout",
                                                    NULL,
                                                    layout_filters, 1);
    if (r.ok) {
        strncpy(g_current_path, r.path, sizeof(g_current_path) - 1);
        g_current_path[sizeof(g_current_path) - 1] = '\0';
        builder_load(g_current_path);
    }
}

static void on_undo(void *w, void *d)
{
    builder_undo();
}

static void on_redo(void *w, void *d)
{
    builder_redo();
}

static void on_preview(void *w, void *d)
{
    builder_preview_show();
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
    clue_menu_add_separator(file_menu);
    clue_menu_add_item(file_menu, "Save Layout", on_save, NULL);
    clue_menu_add_item(file_menu, "Save Layout As...", on_save_as, NULL);
    clue_menu_add_item(file_menu, "Load Layout...", on_load, NULL);
    clue_menu_add_separator(file_menu);
    clue_menu_add_item(file_menu, "Copy Code", on_copy_code, NULL);
    clue_menu_add_separator(file_menu);
    clue_menu_add_item(file_menu, "Quit", on_quit, NULL);

    ClueMenu *edit_menu = clue_menu_new();
    clue_menu_add_item(edit_menu, "Undo", on_undo, NULL);
    clue_menu_add_item(edit_menu, "Redo", on_redo, NULL);
    clue_menu_add_separator(edit_menu);
    clue_menu_add_item(edit_menu, "Delete Selected", on_delete, NULL);
    clue_menu_add_item(edit_menu, "Duplicate Selected", on_duplicate, NULL);

    ClueMenu *view_menu = clue_menu_new();
    clue_menu_add_item(view_menu, "Preview Layout", on_preview, NULL);

    clue_menubar_add(menubar, "File", file_menu);
    clue_menubar_add(menubar, "Edit", edit_menu);
    clue_menubar_add(menubar, "View", view_menu);

    /* Shortcuts */
    clue_shortcut_add("Ctrl+N", on_new, NULL);
    clue_shortcut_add("Ctrl+S", on_save, NULL);
    clue_shortcut_add("Ctrl+Shift+S", on_save_as, NULL);
    clue_shortcut_add("Ctrl+O", on_load, NULL);
    clue_shortcut_add("Ctrl+Z", on_undo, NULL);
    clue_shortcut_add("Ctrl+Y", on_redo, NULL);
    clue_shortcut_add("Ctrl+P", on_preview, NULL);
    clue_shortcut_add("Ctrl+Q", on_quit, NULL);
    clue_shortcut_add("Ctrl+D", on_duplicate, NULL);

    /* Create panels */
    ClueScroll *palette = builder_palette_create();
    ClueScroll *canvas = builder_canvas_create();
    ClueScroll *properties = builder_properties_create();
    ClueTreeView *tree = builder_tree_create();

    /* Wrap tree in a titled box */
    ClueBox *tree_panel = clue_box_new(CLUE_VERTICAL, 0);
    tree_panel->base.style.hexpand = true;
    tree_panel->base.style.vexpand = true;

    ClueLabel *tree_title = clue_label_new("Hierarchy");
    tree_title->base.style.fg_color = CLUE_RGB(180, 180, 195);
    clue_style_set_padding(&tree_title->base.style, 6);

    tree->base.style.hexpand = true;
    tree->base.style.vexpand = true;

    clue_container_add(tree_panel, tree_title);
    clue_container_add(tree_panel, tree);

    /* Left panel: hierarchy on the left, palette on the right */
    ClueSplitter *left_panel = clue_splitter_new(CLUE_HORIZONTAL);
    left_panel->base.style.vexpand = true;
    clue_container_add(left_panel, tree_panel);
    clue_container_add(left_panel, palette);
    clue_splitter_set_ratio(left_panel, 0.55f);

    /* 3-pane layout: left_panel | canvas | properties */
    ClueSplitter *right_split = clue_splitter_new(CLUE_HORIZONTAL);
    right_split->base.style.hexpand = true;
    right_split->base.style.vexpand = true;
    clue_container_add(right_split, canvas);
    clue_container_add(right_split, properties);
    clue_splitter_set_ratio(right_split, 0.70f);

    ClueSplitter *left_split = clue_splitter_new(CLUE_HORIZONTAL);
    left_split->base.style.hexpand = true;
    left_split->base.style.vexpand = true;
    clue_container_add(left_split, left_panel);
    clue_container_add(left_split, right_split);
    clue_splitter_set_ratio(left_split, 0.30f);

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

    /* Initial state */
    builder_codegen_update();
    builder_properties_refresh();

    clue_app_run(app);
    clue_app_destroy(app);
    return 0;
}
