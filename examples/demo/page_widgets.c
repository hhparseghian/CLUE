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

ClueBox *build_widgets_page(ClueApp *app)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 10);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;
    page->base.style.h_align = CLUE_ALIGN_CENTER;

    ClueBox *btn_row = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueButton *btn_hello = clue_button_new("Say Hello");
    clue_signal_connect(btn_hello, "clicked", on_hello, NULL);
    ClueButton *btn_quit = clue_button_new("Quit");
    btn_quit->base.style.bg_color = UI_RGB(180, 50, 50);
    clue_signal_connect(btn_quit, "clicked", on_quit, app);
    clue_container_add(btn_row, btn_hello);
    clue_container_add(btn_row, btn_quit);

    ClueTextInput *input = clue_text_input_new("Type here...");
    input->base.base.w = 280;
    clue_signal_connect(input, "changed", on_input, NULL);

    ClueCheckbox *cb = clue_checkbox_new("Enable feature");
    clue_signal_connect(cb, "toggled", on_checkbox, NULL);

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
    sl->base.style.fg_color = UI_RGB(180, 180, 190);
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
    g_timer_label->base.style.fg_color = UI_RGB(180, 180, 190);

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

    clue_tooltip_set(btn_hello, "Prints hello to status bar");
    clue_tooltip_set(btn_quit, "Exits the application");
    clue_tooltip_set(btn_modal, "Modal + on top, blocks parent");
    clue_tooltip_set(btn_ontop, "Stays on top, parent is usable");
    clue_tooltip_set(btn_free, "Independent window, no blocking");
    clue_tooltip_set(g_progress, "Animated progress bar (timer)");

    input->base.style.margin_top = 4;
    cb->base.style.margin_top = 4;
    radio_row->base.style.margin_top = 4;
    slider_row->base.style.margin_top = 8;
    dd->base.style.margin_top = 4;
    progress_row->base.style.margin_top = 8;
    g_timer_label->base.style.margin_top = 8;
    dlg_row->base.style.margin_top = 8;

    ClueToggle *toggle = clue_toggle_new("Dark mode");
    clue_signal_connect(toggle, "toggled", on_toggle, NULL);
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

    /* Wrapped label */
    ClueSeparator *sep4 = clue_separator_new(CLUE_HORIZONTAL);
    ClueLabel *wrap_lbl = clue_label_new(
        "This is a multi-line label with word wrapping enabled. "
        "It automatically breaks long text to fit within the width.\n\n"
        "Newlines also work.");
    wrap_lbl->base.base.w = 350;
    wrap_lbl->base.style.fg_color = UI_RGB(180, 180, 190);
    clue_label_set_wrap(wrap_lbl, true);
    clue_container_add(page, sep4);
    clue_container_add(page, wrap_lbl);

    clue_timer_repeat(1000, on_tick_clock, NULL);
    clue_timer_repeat(16, on_tick_progress, NULL);

    return page;
}
