#include "demo.h"

static ClueProgress *g_progress;
static ClueLabel *g_timer_label;
static int g_seconds = 0;
static int g_progress_ms = 0;

static bool on_tick_clock(void *data)
{
    g_seconds++;
    char buf[32];
    int m = g_seconds / 60, s = g_seconds % 60;
    snprintf(buf, sizeof(buf), "Uptime: %02d:%02d", m, s);
    clue_label_set_text(g_timer_label, buf);
    return true;
}

static bool on_tick_progress(void *data)
{
    g_progress_ms += 16;
    float v = (float)g_progress_ms / 10000.0f;
    if (v >= 1.0f) {
        clue_progress_set_value(g_progress, 1.0f);
        return false;
    }
    clue_progress_set_value(g_progress, v);
    return true;
}

static void on_reset_progress(ClueButton *button, void *data)
{
    g_progress_ms = 0;
    clue_progress_set_value(g_progress, 0.0f);
    clue_timer_repeat(16, on_tick_progress, NULL);
}

static void on_hello(ClueButton *button, void *data)
{
    clue_label_set_text(g_status, "Hello button clicked!");
}

static void on_quit(ClueButton *button, void *data)
{
    ClueApp *app = data;
    clue_app_quit(app);
}

static void on_checkbox(ClueCheckbox *checkbox, void *data)
{
    clue_label_set_text(g_status, clue_checkbox_is_checked(checkbox)
        ? "Checkbox: ON" : "Checkbox: OFF");
}

static void on_radio(ClueRadio *radio, void *data)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "Radio: %s", radio->label);
    clue_label_set_text(g_status, buf);
}

static void on_slider(ClueSlider *slider, void *data)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "Slider: %.0f%%", clue_slider_get_value(slider));
    clue_label_set_text(g_status, buf);
}

static void on_dropdown(ClueDropdown *dropdown, void *data)
{
    const char *s = clue_dropdown_get_selected_text(dropdown);
    char buf[64];
    snprintf(buf, sizeof(buf), "Selected: %s", s ? s : "none");
    clue_label_set_text(g_status, buf);
}

static void on_input(ClueTextInput *input, void *data)
{
    char buf[300];
    snprintf(buf, sizeof(buf), "Input: %s", clue_text_input_get_text(input));
    clue_label_set_text(g_status, buf);
}

static void on_toggle(ClueToggle *toggle, void *data)
{
    clue_label_set_text(g_status, clue_toggle_is_on(toggle)
        ? "Toggle: ON" : "Toggle: OFF");
}

static void on_spinner(ClueSpinbox *spinbox, void *data)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "Spinner: %.1f", clue_spinbox_get_value(spinbox));
    clue_label_set_text(g_status, buf);
}

static void on_show_modal(ClueButton *button, void *data)
{
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

static void on_show_ontop(ClueButton *button, void *data)
{
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

static void on_show_free(ClueButton *button, void *data)
{
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

static void on_overlay_open_result(const ClueFileDialogResult *r, void *data)
{
    if (r->ok) {
        const char *name = strrchr(r->path, '/');
        name = name ? name + 1 : r->path;
        char buf[256];
        snprintf(buf, sizeof(buf), "Overlay open: %.200s", name);
        clue_label_set_text(g_status, buf);
    } else {
        clue_label_set_text(g_status, "Overlay open: cancelled");
    }
}

static void on_overlay_save_result(const ClueFileDialogResult *r, void *data)
{
    if (r->ok) {
        const char *name = strrchr(r->path, '/');
        name = name ? name + 1 : r->path;
        char buf[256];
        snprintf(buf, sizeof(buf), "Overlay save: %.200s", name);
        clue_label_set_text(g_status, buf);
    } else {
        clue_label_set_text(g_status, "Overlay save: cancelled");
    }
}

static void on_overlay_open(ClueButton *button, void *data)
{
    ClueFileFilter filters[] = {
        {"C Source",   ".c .h"},
        {"Text Files", ".txt .md"},
        {"Images",     ".png .jpg .bmp"},
    };
    clue_file_dialog_open_overlay("Open File (Overlay)", NULL,
                                  filters, 3,
                                  on_overlay_open_result, NULL);
}

static void on_overlay_save(ClueButton *button, void *data)
{
    ClueFileFilter filters[] = {
        {"Text Files", ".txt"},
        {"C Source",    ".c"},
    };
    clue_file_dialog_save_overlay("Save File (Overlay)", NULL,
                                  "untitled.txt",
                                  filters, 2,
                                  on_overlay_save_result, NULL);
}

static void on_show_qwerty(ClueButton *button, void *data)
{
    clue_osk_show(CLUE_OSK_QWERTY);
}

static void on_show_numpad(ClueButton *button, void *data)
{
    clue_osk_show(CLUE_OSK_NUMPAD);
}

static void on_osk_auto(ClueToggle *toggle, void *data)
{
    clue_osk_set_auto(clue_toggle_is_on(toggle), CLUE_OSK_QWERTY);
}

static void on_date_picked(const ClueDateTimeResult *r, bool ok, void *data)
{
    if (ok) {
        char buf[128];
        if (r->has_time)
            snprintf(buf, sizeof(buf), "Picked: %04d-%02d-%02d %02d:%02d",
                     r->year, r->month, r->day, r->hour, r->minute);
        else
            snprintf(buf, sizeof(buf), "Picked: %04d-%02d-%02d",
                     r->year, r->month, r->day);
        clue_label_set_text(g_status, buf);
    } else {
        clue_label_set_text(g_status, "Date picker cancelled");
    }
}

static void on_show_datepicker(ClueButton *button, void *data)
{
    clue_datepicker_show(on_date_picked, NULL);
}

static void on_show_timepicker(ClueButton *button, void *data)
{
    clue_timepicker_show(on_date_picked, NULL);
}

static void on_show_datetimepicker(ClueButton *button, void *data)
{
    clue_datetimepicker_show(on_date_picked, NULL);
}

static void on_drop(void *widget, void *data)
{
    const ClueDragEvent *ev = clue_drag_get_event();
    ClueDragEvent *mut = (ClueDragEvent *)ev;
    mut->accepted = true;
    clue_label_set_text(g_status, "Widget dropped!");
}

static ClueLabel *section_title(const char *text)
{
    ClueLabel *lbl = clue_label_new(text);
    lbl->base.style.fg_color = CLUE_RGB(255, 255, 255);
    lbl->base.style.margin_top = 6;
    return lbl;
}

ClueScroll *build_widgets_page(ClueApp *app)
{
    ClueScroll *scroll = clue_scroll_new();
    scroll->base.style.hexpand = true;
    scroll->base.style.vexpand = true;

    ClueBox *page = clue_box_new(CLUE_VERTICAL, 6);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueBox *btn_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_hello = clue_button_new("Say Hello");
    clue_signal_connect(btn_hello, "clicked", on_hello, NULL);
    ClueButton *btn_quit = clue_button_new("Quit");
    btn_quit->base.style.bg_color = CLUE_RGB(180, 50, 50);
    clue_signal_connect(btn_quit, "clicked", on_quit, app);
    clue_container_add(btn_row, btn_hello);
    clue_container_add(btn_row, btn_quit);

    ClueTextInput *input = clue_text_input_new("Type here...");
    input->base.base.w = 280;
    clue_signal_connect(input, "changed", on_input, NULL);

    /* Checkbox + toggle on same row */
    ClueBox *check_row = clue_box_new(CLUE_HORIZONTAL, 16);
    ClueCheckbox *cb = clue_checkbox_new("Enable feature");
    clue_signal_connect(cb, "toggled", on_checkbox, NULL);
    ClueToggle *toggle = clue_toggle_new("Notifications");
    clue_signal_connect(toggle, "toggled", on_toggle, NULL);
    clue_container_add(check_row, cb);
    clue_container_add(check_row, toggle);

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

    ClueBox *slider_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueLabel *sl = clue_label_new("Volume:");
    sl->base.style.fg_color = CLUE_RGB(180, 180, 190);
    ClueSlider *slider = clue_slider_new(0, 100, 50);
    slider->base.base.w = 200;
    clue_signal_connect(slider, "changed", on_slider, NULL);
    clue_container_add(slider_row, sl);
    clue_container_add(slider_row, slider);

    ClueDropdown *dd = clue_dropdown_new("Pick a colour...");
    dd->base.base.w = 180;
    clue_dropdown_add_item(dd, "Red");
    clue_dropdown_add_item(dd, "Green");
    clue_dropdown_add_item(dd, "Blue");
    clue_signal_connect(dd, "changed", on_dropdown, NULL);

    g_progress = clue_progress_new();
    g_progress->base.base.w = 220;
    ClueButton *btn_reset = clue_button_new("Reset");
    clue_signal_connect(btn_reset, "clicked", on_reset_progress, NULL);
    ClueBox *progress_row = clue_box_new(CLUE_HORIZONTAL, 16);
    clue_container_add(progress_row, g_progress);
    clue_container_add(progress_row, btn_reset);

    g_timer_label = clue_label_new("Uptime: 00:00");
    g_timer_label->base.style.fg_color = CLUE_RGB(180, 180, 190);

    ClueBox *dlg_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_modal = clue_button_new("Modal");
    clue_signal_connect(btn_modal, "clicked", on_show_modal, NULL);
    ClueButton *btn_ontop = clue_button_new("On Top");
    clue_signal_connect(btn_ontop, "clicked", on_show_ontop, NULL);
    ClueButton *btn_free = clue_button_new("Free");
    clue_signal_connect(btn_free, "clicked", on_show_free, NULL);
    ClueButton *btn_ov_open = clue_button_new("Open (Overlay)");
    clue_signal_connect(btn_ov_open, "clicked", on_overlay_open, NULL);
    ClueButton *btn_ov_save = clue_button_new("Save (Overlay)");
    clue_signal_connect(btn_ov_save, "clicked", on_overlay_save, NULL);

    clue_container_add(dlg_row, btn_modal);
    clue_container_add(dlg_row, btn_ontop);
    clue_container_add(dlg_row, btn_free);
    clue_container_add(dlg_row, btn_ov_open);
    clue_container_add(dlg_row, btn_ov_save);

    clue_tooltip_set(btn_hello, "Prints hello to status bar");
    clue_tooltip_set(btn_quit, "Exits the application");
    clue_tooltip_set(btn_modal, "Modal + on top, blocks parent");
    clue_tooltip_set(btn_ontop, "Stays on top, parent is usable");
    clue_tooltip_set(btn_free, "Independent window, no blocking");
    clue_tooltip_set(g_progress, "Animated progress bar (timer)");

    ClueBox *spin_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueLabel *spin_lbl = clue_label_new("Quantity:");
    spin_lbl->base.style.fg_color = CLUE_RGB(180, 180, 190);
    ClueSpinbox *spinner = clue_spinbox_new(0, 100, 1, 5);
    spinner->base.base.w = 130;
    clue_signal_connect(spinner, "changed", on_spinner, NULL);
    clue_container_add(spin_row, spin_lbl);
    clue_container_add(spin_row, spinner);

    ClueLabel *wrap_lbl = clue_label_new(
        "CLUE is a C99 GUI toolkit for Linux with OpenGL ES 2 rendering. "
        "It supports Wayland, X11, and DRM/KMS backends.\n\n"
        "Try Ctrl+O to open a file, or right-click a text field for a context menu.");
    wrap_lbl->base.base.w = 350;
    wrap_lbl->base.style.fg_color = CLUE_RGB(180, 180, 190);
    clue_label_set_wrap(wrap_lbl, true);

    /* Layout with section titles */
    clue_container_add(page, section_title("Buttons"));
    clue_container_add(page, btn_row);

    clue_container_add(page, section_title("Text Input"));
    clue_container_add(page, input);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("Toggles"));
    clue_container_add(page, check_row);
    clue_container_add(page, radio_row);

    clue_container_add(page, section_title("Range"));
    clue_container_add(page, slider_row);
    clue_container_add(page, spin_row);
    clue_container_add(page, dd);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("Progress & Timer"));
    clue_container_add(page, progress_row);
    clue_container_add(page, g_timer_label);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("Dialogs"));
    clue_container_add(page, dlg_row);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("On-Screen Keyboard"));
    ClueBox *osk_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_qwerty = clue_button_new("QWERTY");
    clue_signal_connect(btn_qwerty, "clicked", on_show_qwerty, NULL);
    ClueButton *btn_numpad = clue_button_new("Numpad");
    clue_signal_connect(btn_numpad, "clicked", on_show_numpad, NULL);
    ClueToggle *osk_auto = clue_toggle_new("Auto-show OSK");
    clue_signal_connect(osk_auto, "toggled", on_osk_auto, NULL);
    clue_container_add(osk_row, btn_qwerty);
    clue_container_add(osk_row, btn_numpad);
    clue_container_add(osk_row, osk_auto);
    clue_container_add(page, osk_row);
    ClueTextInput *osk_input = clue_text_input_new("Tap here to type with OSK...");
    osk_input->base.base.w = 350;
    clue_container_add(page, osk_input);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("Date/Time Picker"));
    ClueBox *dp_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_date = clue_button_new("Date");
    clue_signal_connect(btn_date, "clicked", on_show_datepicker, NULL);
    ClueButton *btn_datetime = clue_button_new("Date + Time");
    clue_signal_connect(btn_datetime, "clicked", on_show_datetimepicker, NULL);
    ClueButton *btn_time = clue_button_new("Time");
    clue_signal_connect(btn_time, "clicked", on_show_timepicker, NULL);
    clue_container_add(dp_row, btn_date);
    clue_container_add(dp_row, btn_time);
    clue_container_add(dp_row, btn_datetime);
    clue_container_add(page, dp_row);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("Drag and Drop"));
    ClueBox *dnd_row = clue_box_new(CLUE_HORIZONTAL, 12);
    dnd_row->base.style.hexpand = true;

    ClueBox *dnd_src = clue_box_new(CLUE_VERTICAL, 4);
    clue_style_set_padding(&dnd_src->base.style, 8);
    dnd_src->base.style.bg_color = CLUE_RGBA(255, 255, 255, 20);
    dnd_src->base.base.w = 140;
    dnd_src->base.base.h = 100;
    ClueLabel *src_lbl = clue_label_new("Source");
    src_lbl->base.style.fg_color = CLUE_RGB(180, 180, 190);
    clue_container_add(dnd_src, src_lbl);
    ClueButton *drag1 = clue_button_new("Drag me");
    clue_widget_set_draggable(CLUE_W(drag1), true);
    ClueButton *drag2 = clue_button_new("Me too");
    clue_widget_set_draggable(CLUE_W(drag2), true);
    clue_container_add(dnd_src, drag1);
    clue_container_add(dnd_src, drag2);

    ClueBox *dnd_tgt = clue_box_new(CLUE_VERTICAL, 4);
    clue_style_set_padding(&dnd_tgt->base.style, 8);
    dnd_tgt->base.style.bg_color = CLUE_RGBA(100, 200, 100, 20);
    dnd_tgt->base.base.w = 140;
    dnd_tgt->base.base.h = 100;
    clue_widget_set_drop_target(CLUE_W(dnd_tgt), true);
    clue_signal_connect(dnd_tgt, "drop", on_drop, NULL);
    ClueLabel *tgt_lbl = clue_label_new("Drop here");
    tgt_lbl->base.style.fg_color = CLUE_RGB(180, 180, 190);
    clue_container_add(dnd_tgt, tgt_lbl);

    clue_container_add(dnd_row, dnd_src);
    clue_container_add(dnd_row, dnd_tgt);
    clue_container_add(page, dnd_row);

    clue_container_add(page, clue_separator_new(CLUE_HORIZONTAL));
    clue_container_add(page, section_title("About"));
    clue_container_add(page, wrap_lbl);

    clue_timer_repeat(1000, on_tick_clock, NULL);
    clue_timer_repeat(16, on_tick_progress, NULL);

    clue_container_add(scroll, page);
    return scroll;
}
