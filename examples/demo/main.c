#include <stdio.h>
#include <string.h>
#include "clue/clue.h"

/* --- List view data --- */
static const char *fruits[] = {
    "Apple", "Banana", "Cherry", "Date", "Elderberry",
    "Fig", "Grape", "Honeydew", "Kiwi", "Lemon",
    "Mango", "Nectarine", "Orange", "Papaya", "Quince",
    "Raspberry", "Strawberry", "Tangerine", "Watermelon", "Yuzu",
};

static const char *fruit_item(int index, void *user_data)
{
    (void)user_data;
    return fruits[index];
}

/* --- Globals --- */
static ClueLabel *g_status;
static ClueProgress *g_progress;
static ClueLabel *g_timer_label;
static int g_seconds = 0;

static bool on_tick_clock(void *data)
{
    (void)data;
    g_seconds++;
    char buf[32];
    int m = g_seconds / 60, s = g_seconds % 60;
    snprintf(buf, sizeof(buf), "Uptime: %02d:%02d", m, s);
    clue_label_set_text(g_timer_label, buf);
    return true;
}

static int g_progress_ms = 0;

static bool on_tick_progress(void *data)
{
    (void)data;
    g_progress_ms += 16;
    float v = (float)g_progress_ms / 10000.0f;
    if (v >= 1.0f) {
        clue_progress_set_value(g_progress, 1.0f);
        return false; /* stop timer */
    }
    clue_progress_set_value(g_progress, v);
    return true;
}

static void on_reset_progress(void *w, void *d)
{
    (void)w; (void)d;
    g_progress_ms = 0;
    clue_progress_set_value(g_progress, 0.0f);
    clue_timer_repeat(16, on_tick_progress, NULL);
}

static void on_hello(void *w, void *d)
{
    (void)w; (void)d;
    clue_label_set_text(g_status, "Hello button clicked!");
}

static void on_quit(void *w, void *d)
{
    (void)w;
    clue_app_quit((ClueApp *)d);
}

static void on_checkbox(void *w, void *d)
{
    (void)d;
    ClueCheckbox *cb = (ClueCheckbox *)w;
    clue_label_set_text(g_status, clue_checkbox_is_checked(cb)
        ? "Checkbox: ON" : "Checkbox: OFF");
}

static void on_radio(void *w, void *d)
{
    (void)d;
    ClueRadio *r = (ClueRadio *)w;
    char buf[64];
    snprintf(buf, sizeof(buf), "Radio: %s", r->label);
    clue_label_set_text(g_status, buf);
}

static void on_slider(void *w, void *d)
{
    (void)d;
    char buf[64];
    snprintf(buf, sizeof(buf), "Slider: %.0f%%", clue_slider_get_value(w));
    clue_label_set_text(g_status, buf);
}

static void on_dropdown(void *w, void *d)
{
    (void)d;
    const char *s = clue_dropdown_get_selected_text(w);
    char buf[64];
    snprintf(buf, sizeof(buf), "Selected: %s", s ? s : "none");
    clue_label_set_text(g_status, buf);
}

static void on_input(void *w, void *d)
{
    (void)d;
    char buf[300];
    snprintf(buf, sizeof(buf), "Input: %s", clue_text_input_get_text(w));
    clue_label_set_text(g_status, buf);
}

static void on_list_selected(void *w, void *d)
{
    (void)d;
    int idx = clue_listview_get_selected(w);
    char buf[64];
    snprintf(buf, sizeof(buf), "Fruit: %s", idx >= 0 ? fruits[idx] : "none");
    clue_label_set_text(g_status, buf);
}

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
        /* Custom purple theme -- start from dark, tweak accent + widgets */
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
        /* Custom ocean theme -- teal/cyan tones */
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

static void on_show_modal(void *w, void *d)
{
    (void)w; (void)d;
    ClueDialog *dlg = clue_dialog_new("Modal + On Top", 380, 160);
    ClueLabel *lbl = clue_label_new("Blocks parent, stays on top.");
    lbl->base.style.fg_color = clue_theme_get()->fg;
    clue_dialog_set_content(dlg, lbl);
    clue_dialog_add_button(dlg, "Cancel", CLUE_DIALOG_CANCEL);
    clue_dialog_add_button(dlg, "OK", CLUE_DIALOG_OK);
    clue_dialog_set_flags(dlg, CLUE_DIALOG_FLAG_MODAL | CLUE_DIALOG_FLAG_ON_TOP);
    ClueDialogResult r = clue_dialog_run(dlg);
    char buf[64];
    snprintf(buf, sizeof(buf), "Modal: %s",
             r == CLUE_DIALOG_OK ? "OK" : "Cancel");
    clue_label_set_text(g_status, buf);
    clue_dialog_destroy(dlg);
}

static void on_show_ontop(void *w, void *d)
{
    (void)w; (void)d;
    ClueDialog *dlg = clue_dialog_new("On Top Only", 380, 160);
    ClueLabel *lbl = clue_label_new("Stays on top, parent is usable.");
    lbl->base.style.fg_color = clue_theme_get()->fg;
    clue_dialog_set_content(dlg, lbl);
    clue_dialog_add_button(dlg, "Close", CLUE_DIALOG_OK);
    clue_dialog_set_flags(dlg, CLUE_DIALOG_FLAG_ON_TOP);
    clue_dialog_run(dlg);
    clue_label_set_text(g_status, "On-top closed");
    clue_dialog_destroy(dlg);
}

static void on_show_free(void *w, void *d)
{
    (void)w; (void)d;
    ClueDialog *dlg = clue_dialog_new("Free Dialog", 380, 160);
    ClueLabel *lbl = clue_label_new("Independent window, no blocking.");
    lbl->base.style.fg_color = clue_theme_get()->fg;
    clue_dialog_set_content(dlg, lbl);
    clue_dialog_add_button(dlg, "Close", CLUE_DIALOG_OK);
    clue_dialog_set_flags(dlg, 0);
    clue_dialog_run(dlg);
    clue_label_set_text(g_status, "Free dialog closed");
    clue_dialog_destroy(dlg);
}

/* --- Widget callbacks --- */

static void on_toggle(void *w, void *d)
{
    (void)d;
    ClueToggle *t = (ClueToggle *)w;
    clue_label_set_text(g_status, clue_toggle_is_on(t)
        ? "Toggle: ON" : "Toggle: OFF");
}

static void on_spinner(void *w, void *d)
{
    (void)d;
    char buf[64];
    snprintf(buf, sizeof(buf), "Spinner: %.1f", clue_spinbox_get_value(w));
    clue_label_set_text(g_status, buf);
}

/* --- Canvas painting --- */

#define PAINT_MAX 4096

typedef struct {
    int x, y;
    UIColor color;
} PaintPoint;

static PaintPoint g_paint_points[PAINT_MAX];
static int g_paint_count = 0;
static int g_paint_last_x = -1, g_paint_last_y = -1;
static UIColor g_paint_color = {0.86f, 0.86f, 1.0f, 1.0f};

static void canvas_draw_cb(int x, int y, int w, int h, void *data)
{
    (void)data; (void)w; (void)h;
    for (int i = 1; i < g_paint_count; i++) {
        if (g_paint_points[i].x < 0 || g_paint_points[i - 1].x < 0)
            continue;
        clue_draw_line(x + g_paint_points[i - 1].x,
                       y + g_paint_points[i - 1].y,
                       x + g_paint_points[i].x,
                       y + g_paint_points[i].y,
                       3.0f, g_paint_points[i].color);
    }
    if (g_paint_count == 0) {
        clue_draw_text_default(x + 10, y + 10, "Draw here with the mouse!",
                               UI_RGB(100, 100, 120));
    }
}

static void canvas_event_cb(int type, int cx, int cy, int button, void *data)
{
    (void)data; (void)button;
    if (type == CLUE_CANVAS_PRESS) {
        if (g_paint_count > 0 && g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){-1, -1, {0}};
            g_paint_count++;
        }
        if (g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){cx, cy, g_paint_color};
            g_paint_count++;
        }
        g_paint_last_x = cx;
        g_paint_last_y = cy;
    } else if (type == CLUE_CANVAS_MOTION) {
        if (g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){cx, cy, g_paint_color};
            g_paint_count++;
        }
        g_paint_last_x = cx;
        g_paint_last_y = cy;
    } else if (type == CLUE_CANVAS_RELEASE) {
        g_paint_last_x = -1;
        g_paint_last_y = -1;
    }
}

static void on_canvas_clear(void *w, void *d)
{
    (void)w; (void)d;
    g_paint_count = 0;
    clue_label_set_text(g_status, "Canvas cleared");
}

static void on_color_changed(void *w, void *d)
{
    (void)d;
    g_paint_color = clue_colorpicker_get_color((ClueColorPicker *)w);
}

/* --- Build tab pages --- */

static ClueBox *build_widgets_page(ClueApp *app)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    /* Buttons */
    ClueBox *btn_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_hello = clue_button_new("Say Hello");
    clue_signal_connect(btn_hello, "clicked", on_hello, NULL);
    ClueButton *btn_quit = clue_button_new("Quit");
    btn_quit->base.style.bg_color = UI_RGB(180, 50, 50);
    clue_signal_connect(btn_quit, "clicked", on_quit, app);
    clue_container_add(btn_row, btn_hello);
    clue_container_add(btn_row, btn_quit);

    /* Text input */
    ClueTextInput *input = clue_text_input_new("Type here...");
    input->base.base.w = 280;
    clue_signal_connect(input, "changed", on_input, NULL);

    /* Checkbox */
    ClueCheckbox *cb = clue_checkbox_new("Enable feature");
    clue_signal_connect(cb, "toggled", on_checkbox, NULL);

    /* Radio buttons */
    ClueRadioGroup *rg = clue_radio_group_new();
    ClueRadio *r1 = clue_radio_new("Small", rg);
    ClueRadio *r2 = clue_radio_new("Medium", rg);
    ClueRadio *r3 = clue_radio_new("Large", rg);
    clue_radio_set_selected(r2);
    clue_signal_connect(r1, "changed", on_radio, NULL);
    clue_signal_connect(r2, "changed", on_radio, NULL);
    clue_signal_connect(r3, "changed", on_radio, NULL);
    ClueBox *radio_row = clue_box_new(CLUE_HORIZONTAL, 16);
    clue_container_add(radio_row, r1);
    clue_container_add(radio_row, r2);
    clue_container_add(radio_row, r3);

    /* Slider */
    ClueBox *slider_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueLabel *sl = clue_label_new("Volume:");
    sl->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueSlider *slider = clue_slider_new(0, 100, 50);
    slider->base.base.w = 200;
    clue_signal_connect(slider, "changed", on_slider, NULL);
    clue_container_add(slider_row, sl);
    clue_container_add(slider_row, slider);

    /* Dropdown */
    ClueDropdown *dd = clue_dropdown_new("Pick a colour...");
    dd->base.base.w = 180;
    clue_dropdown_add_item(dd, "Red");
    clue_dropdown_add_item(dd, "Green");
    clue_dropdown_add_item(dd, "Blue");
    clue_signal_connect(dd, "changed", on_dropdown, NULL);

    /* Progress bar (animated by timer) */
    g_progress = clue_progress_new();
    g_progress->base.base.w = 220;

    ClueButton *btn_reset = clue_button_new("Reset");
    clue_signal_connect(btn_reset, "clicked", on_reset_progress, NULL);

    ClueBox *progress_row = clue_box_new(CLUE_HORIZONTAL, 16);
    clue_container_add(progress_row, g_progress);
    clue_container_add(progress_row, btn_reset);

    /* Timer label */
    g_timer_label = clue_label_new("Uptime: 00:00");
    g_timer_label->base.style.fg_color = UI_RGB(180, 180, 190);

    /* Dialog buttons */
    ClueBox *dlg_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_modal = clue_button_new("Modal");
    clue_signal_connect(btn_modal, "clicked", on_show_modal, NULL);
    ClueButton *btn_ontop = clue_button_new("On Top");
    clue_signal_connect(btn_ontop, "clicked", on_show_ontop, NULL);
    ClueButton *btn_free = clue_button_new("Free");
    clue_signal_connect(btn_free, "clicked", on_show_free, NULL);
    clue_container_add(dlg_row, btn_modal);
    clue_container_add(dlg_row, btn_ontop);
    clue_container_add(dlg_row, btn_free);

    /* Tooltips */
    clue_tooltip_set(btn_hello, "Prints hello to status bar");
    clue_tooltip_set(btn_quit, "Exits the application");
    clue_tooltip_set(btn_modal, "Modal + on top, blocks parent");
    clue_tooltip_set(btn_ontop, "Stays on top, parent is usable");
    clue_tooltip_set(btn_free, "Independent window, no blocking");
    clue_tooltip_set(g_progress, "Animated progress bar (timer)");

    /* Add spacing between widget groups */
    input->base.style.margin_top = 4;
    cb->base.style.margin_top = 4;
    radio_row->base.style.margin_top = 4;
    slider_row->base.style.margin_top = 8;
    dd->base.style.margin_top = 4;
    progress_row->base.style.margin_top = 8;
    g_timer_label->base.style.margin_top = 8;
    dlg_row->base.style.margin_top = 8;

    /* Toggle switch */
    ClueToggle *toggle = clue_toggle_new("Dark mode");
    clue_signal_connect(toggle, "toggled", on_toggle, NULL);

    /* Spinner */
    ClueBox *spin_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueLabel *spin_lbl = clue_label_new("Quantity:");
    spin_lbl->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueSpinbox *spinner = clue_spinbox_new(0, 100, 1, 5);
    spinner->base.base.w = 130;
    clue_signal_connect(spinner, "changed", on_spinner, NULL);
    clue_container_add(spin_row, spin_lbl);
    clue_container_add(spin_row, spinner);

    toggle->base.style.margin_top = 4;
    spin_row->base.style.margin_top = 4;

    ClueSeparator *sep1 = clue_separator_new(CLUE_HORIZONTAL);
    ClueSeparator *sep2 = clue_separator_new(CLUE_HORIZONTAL);
    ClueSeparator *sep3 = clue_separator_new(CLUE_HORIZONTAL);
    ClueSeparator *sep4 = clue_separator_new(CLUE_HORIZONTAL);

    clue_container_add(page, btn_row);
    clue_container_add(page, input);
    clue_container_add(page, sep1);
    clue_container_add(page, cb);
    clue_container_add(page, toggle);
    clue_container_add(page, radio_row);
    clue_container_add(page, slider_row);
    clue_container_add(page, dd);
    clue_container_add(page, sep2);
    clue_container_add(page, spin_row);
    clue_container_add(page, progress_row);
    clue_container_add(page, g_timer_label);
    clue_container_add(page, sep3);
    clue_container_add(page, dlg_row);

    return page;
}

static ClueBox *build_list_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueLabel *lbl = clue_label_new("Fruit list (virtual scrolling):");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueListView *lv = clue_listview_new();
    lv->base.base.w = 350;
    lv->base.base.h = 280;
    clue_listview_set_data(lv, 20, fruit_item, NULL);
    clue_signal_connect(lv, "selected", on_list_selected, NULL);

    clue_container_add(page, lbl);
    clue_container_add(page, lv);

    return page;
}

static ClueBox *build_grid_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueLabel *lbl = clue_label_new("Form grid layout:");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueGrid *grid = clue_grid_new(2, 8, 12);
    grid->base.base.w = 360;
    clue_grid_set_col_width(grid, 0, 80);

    ClueLabel *l1 = clue_label_new("Name:");
    l1->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueTextInput *i1 = clue_text_input_new("Enter name");
    i1->base.base.w = 240;

    ClueLabel *l2 = clue_label_new("Email:");
    l2->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueTextInput *i2 = clue_text_input_new("Enter email");
    i2->base.base.w = 240;

    ClueLabel *l3 = clue_label_new("Role:");
    l3->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueDropdown *dd = clue_dropdown_new("Select...");
    dd->base.base.w = 240;
    clue_dropdown_add_item(dd, "Developer");
    clue_dropdown_add_item(dd, "Designer");
    clue_dropdown_add_item(dd, "Manager");

    clue_container_add(grid, l1);
    clue_container_add(grid, i1);
    clue_container_add(grid, l2);
    clue_container_add(grid, i2);
    clue_container_add(grid, l3);
    clue_container_add(grid, dd);

    clue_container_add(page, lbl);
    clue_container_add(page, grid);

    return page;
}

/* --- Table data --- */
static const char *people[][3] = {
    {"Alice",   "alice@example.com",   "Developer"},
    {"Bob",     "bob@example.com",     "Designer"},
    {"Charlie", "charlie@example.com", "Manager"},
    {"Diana",   "diana@example.com",   "Developer"},
    {"Eve",     "eve@example.com",     "Analyst"},
    {"Frank",   "frank@example.com",   "Developer"},
    {"Grace",   "grace@example.com",   "Designer"},
    {"Hank",    "hank@example.com",    "Manager"},
};

static const char *table_cell(int row, int col, void *user_data)
{
    (void)user_data;
    if (row < 0 || row >= 8 || col < 0 || col >= 3) return "";
    return people[row][col];
}

static void on_tree_selected(void *w, void *d)
{
    (void)d;
    ClueTreeNode *node = clue_treeview_get_selected(w);
    if (node && node->label) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Tree: %s", node->label);
        clue_label_set_text(g_status, buf);
    }
}

static void on_table_selected(void *w, void *d)
{
    (void)d;
    int row = clue_table_get_selected(w);
    if (row >= 0 && row < 8) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Table: %s (%s)", people[row][0], people[row][2]);
        clue_label_set_text(g_status, buf);
    }
}

static ClueBox *build_tree_page(void)
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

    /* Build a sample tree */
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

static ClueBox *build_table_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueTable *tbl = clue_table_new();
    clue_table_add_column(tbl, "Name", 120);
    clue_table_add_column(tbl, "Email", 200);
    clue_table_add_column(tbl, "Role", 100);
    tbl->base.base.h = 280;
    clue_table_set_data(tbl, 8, table_cell, NULL);
    clue_signal_connect(tbl, "selected", on_table_selected, NULL);

    clue_container_add(page, tbl);
    return page;
}

/* --- Editor page --- */

static ClueBox *build_editor_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    ClueLabel *lbl = clue_label_new("Multi-line text editor:");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);

    ClueTextEditor *ed = clue_text_editor_new();
    ed->base.base.w = 500;
    ed->base.base.h = 300;
    ed->base.style.hexpand = true;
    ed->base.style.vexpand = true;
    clue_text_editor_set_line_numbers(ed, true);
    clue_text_editor_set_text(ed,
        "// Welcome to the CLUE text editor\n"
        "#include <stdio.h>\n"
        "\n"
        "int main(void) {\n"
        "    printf(\"Hello, world!\\n\");\n"
        "    return 0;\n"
        "}\n");

    clue_container_add(page, lbl);
    clue_container_add(page, ed);

    return page;
}

/* --- Canvas page --- */

static ClueBox *build_canvas_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 8);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    /* Toolbar row */
    ClueBox *top = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueLabel *lbl = clue_label_new("Paint:");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueButton *clear_btn = clue_button_new("Clear");
    clue_signal_connect(clear_btn, "clicked", on_canvas_clear, NULL);
    clue_container_add(top, lbl);
    clue_container_add(top, clear_btn);

    /* Canvas + color picker side by side */
    ClueBox *body = clue_box_new(CLUE_HORIZONTAL, 8);
    body->base.style.hexpand = true;
    body->base.style.vexpand = true;

    ClueCanvas *canvas = clue_canvas_new(500, 300);
    canvas->base.style.hexpand = true;
    canvas->base.style.vexpand = true;
    clue_canvas_set_draw(canvas, canvas_draw_cb, NULL);
    clue_canvas_set_event(canvas, canvas_event_cb, NULL);

    ClueColorPicker *cp = clue_colorpicker_new();
    clue_colorpicker_set_color(cp, g_paint_color);
    clue_signal_connect(cp, "changed", on_color_changed, NULL);

    clue_container_add(body, canvas);
    clue_container_add(body, cp);

    clue_container_add(page, top);
    clue_container_add(page, body);

    return page;
}

/* --- Splitter page --- */

static ClueBox *build_splitter_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 0);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    ClueSplitter *split = clue_splitter_new(CLUE_HORIZONTAL);
    split->base.base.w = 600;
    split->base.base.h = 400;
    split->base.style.hexpand = true;
    split->base.style.vexpand = true;
    clue_splitter_set_ratio(split, 0.35f);

    /* Left pane: a list */
    ClueBox *left = clue_box_new(CLUE_VERTICAL, 6);
    clue_style_set_padding(&left->base.style, 8);
    ClueLabel *left_title = clue_label_new("Left Pane");
    left_title->base.style.fg_color = UI_RGB(255, 255, 255);
    ClueListView *lv = clue_listview_new();
    lv->base.base.w = 200;
    lv->base.base.h = 300;
    lv->base.style.hexpand = true;
    lv->base.style.vexpand = true;
    clue_listview_set_data(lv, 20, fruit_item, NULL);
    clue_container_add(left, left_title);
    clue_container_add(left, lv);

    /* Right pane: an editor */
    ClueBox *right = clue_box_new(CLUE_VERTICAL, 6);
    clue_style_set_padding(&right->base.style, 8);
    ClueLabel *right_title = clue_label_new("Right Pane");
    right_title->base.style.fg_color = UI_RGB(255, 255, 255);
    ClueTextEditor *ed = clue_text_editor_new();
    ed->base.base.w = 300;
    ed->base.base.h = 300;
    ed->base.style.hexpand = true;
    ed->base.style.vexpand = true;
    clue_text_editor_set_line_numbers(ed, true);
    clue_text_editor_set_text(ed, "// Drag the divider to resize panes\n");
    clue_container_add(right, right_title);
    clue_container_add(right, ed);

    clue_container_add(split, left);
    clue_container_add(split, right);
    clue_container_add(page, split);

    return page;
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

int main(void)
{
    ClueApp *app = clue_app_new("CLUE Demo", 750, 800);
    if (!app) return 1;

    /* Main layout */
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

    /* Register keyboard shortcuts */
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

    clue_container_add(root, menubar);
    clue_container_add(root, header);
    clue_container_add(root, g_status);
    clue_container_add(root, tabs);

    clue_app_set_root(app, root);

    /* 1-second timer for uptime clock */
    clue_timer_repeat(1000, on_tick_clock, NULL);
    /* ~60fps timer for smooth progress bar */
    clue_timer_repeat(16, on_tick_progress, NULL);

    clue_app_run(app);
    clue_app_destroy(app);
    return 0;
}
