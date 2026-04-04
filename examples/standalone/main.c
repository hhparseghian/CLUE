#include <clue/clue.h>

static void on_clicked(void *widget, void *data)
{
    (void)widget;
    clue_label_set_text((ClueLabel *)data, "Button clicked!");
}

static void on_quit(void *widget, void *data)
{
    (void)widget;
    clue_app_quit((ClueApp *)data);
}

int main(void)
{
    ClueApp *app = clue_app_new("My App", 400, 300);
    if (!app) return 1;

    ClueBox *vbox = clue_box_new(CLUE_VERTICAL, 12);
    clue_style_set_padding(&vbox->base.style, 20);

    ClueLabel *title = clue_label_new("My CLUE Application");
    title->base.style.fg_color = UI_RGB(255, 255, 255);

    ClueLabel *status = clue_label_new("Ready");
    status->base.style.fg_color = UI_RGB(100, 200, 100);

    ClueButton *btn = clue_button_new("Click Me");
    clue_signal_connect(btn, "clicked", on_clicked, status);

    ClueButton *quit = clue_button_new("Quit");
    quit->base.style.bg_color = UI_RGB(180, 50, 50);
    clue_signal_connect(quit, "clicked", on_quit, app);

    ClueBox *btn_row = clue_box_new(CLUE_HORIZONTAL, 8);
    clue_container_add((ClueWidget *)btn_row, (ClueWidget *)btn);
    clue_container_add((ClueWidget *)btn_row, (ClueWidget *)quit);

    clue_container_add((ClueWidget *)vbox, (ClueWidget *)title);
    clue_container_add((ClueWidget *)vbox, (ClueWidget *)status);
    clue_container_add((ClueWidget *)vbox, (ClueWidget *)btn_row);

    clue_app_set_root(app, (ClueWidget *)vbox);
    clue_app_run(app);
    clue_app_destroy(app);
    return 0;
}
