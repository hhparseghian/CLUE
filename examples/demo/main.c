#include "demo.h"

/* --- Shared globals --- */
ClueLabel *g_status;

const char *fruits[] = {
    "Apple", "Banana", "Cherry", "Date", "Elderberry",
    "Fig", "Grape", "Honeydew", "Kiwi", "Lemon",
    "Mango", "Nectarine", "Orange", "Papaya", "Quince",
    "Raspberry", "Strawberry", "Tangerine", "Watermelon", "Yuzu",
};

const char *fruit_item(int index, void *user_data)
{
    (void)user_data;
    return fruits[index];
}

/* --- Theme toggle --- */

static void on_theme_toggle(void *w, void *d)
{
    (void)d;
    static int idx = 0;
    idx = (idx + 1) % 4;

    static const char *names[] = {
        "Dark", "Light", "Purple", "Ocean"
    };

    switch (idx) {
    case 0:
        clue_theme_set(clue_theme_dark());
        break;
    case 1:
        clue_theme_set(clue_theme_light());
        break;
    case 2: {
        ClueTheme purple = *clue_theme_dark();
        purple.bg = UI_RGB(25, 15, 35);
        purple.accent = UI_RGB(140, 60, 200);
        purple.accent_hover = UI_RGB(160, 80, 220);
        purple.accent_pressed = UI_RGB(110, 40, 170);
        purple.button.bg = UI_RGB(140, 60, 200);
        purple.button.bg_hover = UI_RGB(160, 80, 220);
        purple.button.bg_pressed = UI_RGB(110, 40, 170);
        purple.input.focus_border = UI_RGB(160, 80, 220);
        purple.input.cursor = UI_RGB(180, 120, 255);
        purple.slider.fill = UI_RGB(140, 60, 200);
        purple.checkbox.box_checked = UI_RGB(140, 60, 200);
        purple.tabs.indicator = UI_RGB(140, 60, 200);
        purple.list.selected_bg = UI_RGB(140, 60, 200);
        purple.dropdown.selected_bg = UI_RGBA(140, 60, 200, 80);
        clue_theme_set(&purple);
        break;
    }
    case 3: {
        ClueTheme ocean = *clue_theme_dark();
        ocean.bg = UI_RGB(15, 25, 35);
        ocean.surface = UI_RGB(20, 35, 50);
        ocean.surface_hover = UI_RGB(25, 45, 60);
        ocean.surface_border = UI_RGB(40, 65, 85);
        ocean.accent = UI_RGB(0, 180, 160);
        ocean.accent_hover = UI_RGB(0, 200, 180);
        ocean.accent_pressed = UI_RGB(0, 140, 120);
        ocean.button.bg = UI_RGB(0, 180, 160);
        ocean.button.bg_hover = UI_RGB(0, 200, 180);
        ocean.button.bg_pressed = UI_RGB(0, 140, 120);
        ocean.input.bg = UI_RGB(15, 30, 45);
        ocean.input.border = UI_RGB(40, 65, 85);
        ocean.input.focus_border = UI_RGB(0, 180, 160);
        ocean.input.cursor = UI_RGB(0, 220, 200);
        ocean.slider.fill = UI_RGB(0, 180, 160);
        ocean.slider.track = UI_RGB(30, 50, 65);
        ocean.checkbox.box_checked = UI_RGB(0, 180, 160);
        ocean.tabs.bar_bg = UI_RGB(20, 35, 50);
        ocean.tabs.active_bg = UI_RGB(15, 25, 35);
        ocean.tabs.indicator = UI_RGB(0, 180, 160);
        ocean.list.bg = UI_RGB(20, 35, 50);
        ocean.list.selected_bg = UI_RGB(0, 180, 160);
        ocean.list.hover_bg = UI_RGB(25, 45, 60);
        ocean.dropdown.bg = UI_RGB(15, 30, 45);
        ocean.dropdown.border = UI_RGB(40, 65, 85);
        ocean.dropdown.list_bg = UI_RGB(20, 35, 50);
        ocean.dropdown.hover_bg = UI_RGB(25, 50, 65);
        clue_theme_set(&ocean);
        break;
    }
    }

    char label[32];
    snprintf(label, sizeof(label), "Theme: %s", names[(idx + 1) % 4]);
    clue_button_set_label(w, label);
}

/* --- Menu callbacks --- */

static void on_menu_open(void *w, void *d)
{
    (void)w; (void)d;
    ClueFileFilter filters[] = {
        {"C Source",  ".c .h"},
        {"Text Files", ".txt .md"},
        {"Images",    ".png .jpg .bmp"},
    };
    ClueFileDialogResult r = clue_file_dialog_open("Open File", NULL,
                                                    filters, 3);
    if (r.ok) {
        const char *name = strrchr(r.path, '/');
        name = name ? name + 1 : r.path;
        char buf[256];
        snprintf(buf, sizeof(buf), "Opened: %.200s", name);
        clue_label_set_text(g_status, buf);
    }
}

static void on_menu_save(void *w, void *d)
{
    (void)w; (void)d;
    ClueFileFilter save_filters[] = {
        {"Text Files", ".txt"},
        {"C Source",   ".c"},
    };
    ClueFileDialogResult r = clue_file_dialog_save("Save File", NULL,
                                                    "untitled.txt",
                                                    save_filters, 2);
    if (r.ok) {
        const char *name = strrchr(r.path, '/');
        name = name ? name + 1 : r.path;
        char buf[256];
        snprintf(buf, sizeof(buf), "Saved: %.200s", name);
        clue_label_set_text(g_status, buf);
    }
}

static void on_menu_about(void *w, void *d)
{
    (void)w; (void)d;
    clue_dialog_message("About", "CLUE UI Engine v0.1");
}

static void on_menu_quit(void *w, void *d)
{
    (void)w; (void)d;
    clue_app_quit(clue_app_get());
}

/* --- Main --- */

int main(void)
{
    ClueApp *app = clue_app_new("CLUE Demo", 750, 800);
    if (!app) return 1;

    ClueBox *root = clue_box_new(CLUE_VERTICAL, 0);
    root->base.style.hexpand = true;
    root->base.style.vexpand = true;

    /* Menu bar */
    ClueMenuBar *menubar = clue_menubar_new();

    ClueMenu *file_menu = clue_menu_new();
    clue_menu_add_item_shortcut(file_menu, "New", "Ctrl+N", NULL, NULL);
    clue_menu_add_item_shortcut(file_menu, "Open", "Ctrl+O", on_menu_open, NULL);
    clue_menu_add_item_shortcut(file_menu, "Save", "Ctrl+S", on_menu_save, NULL);
    clue_menu_add_separator(file_menu);
    clue_menu_add_item_shortcut(file_menu, "Quit", "Ctrl+Q", on_menu_quit, NULL);

    ClueMenu *help_menu = clue_menu_new();
    clue_menu_add_item(help_menu, "About CLUE", on_menu_about, NULL);

    clue_menubar_add(menubar, "File", file_menu);
    clue_menubar_add(menubar, "Help", help_menu);

    /* Keyboard shortcuts */
    clue_shortcut_add("Ctrl+O", on_menu_open, NULL);
    clue_shortcut_add("Ctrl+S", on_menu_save, NULL);
    clue_shortcut_add("Ctrl+Q", on_menu_quit, NULL);

    /* Header */
    ClueBox *header = clue_box_new(CLUE_HORIZONTAL, 10);
    clue_style_set_padding(&header->base.style, 12);

    ClueLabel *title = clue_label_new("CLUE Demo");
    title->base.style.fg_color = UI_RGB(255, 255, 255);
    title->base.style.font = clue_font_load(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20);

    g_status = clue_label_new("Ready");
    g_status->base.style.fg_color = UI_RGB(100, 200, 100);

    ClueButton *theme_btn = clue_button_new("Theme: Light");
    clue_signal_connect(theme_btn, "clicked", on_theme_toggle, NULL);
    clue_tooltip_set(theme_btn, "Cycle through 4 themes");

    clue_container_add(header, title);
    clue_container_add(header, theme_btn);

    /* Tabs */
    ClueTabs *tabs = clue_tabs_new();
    tabs->base.style.hexpand = true;
    tabs->base.style.vexpand = true;
    tabs->page_bg = UI_RGB(45, 45, 50);
    clue_tabs_add(tabs, "Widgets", build_widgets_page(app));
    clue_tabs_add(tabs, "List", build_list_page());
    clue_tabs_add(tabs, "Form", build_grid_page());
    clue_tabs_add(tabs, "Tree", build_tree_page());
    clue_tabs_add(tabs, "Table", build_table_page());
    clue_tabs_add(tabs, "Editor", build_editor_page());
    clue_tabs_add(tabs, "Canvas", build_canvas_page());
    clue_tabs_add(tabs, "Splitter", build_splitter_page());
    clue_tabs_add(tabs, "3D", build_3d_page());

    clue_container_add(root, menubar);
    clue_container_add(root, header);
    clue_container_add(root, g_status);
    clue_container_add(root, tabs);

    clue_app_set_root(app, root);

    clue_app_run(app);
    clue_app_destroy(app);
    return 0;
}
