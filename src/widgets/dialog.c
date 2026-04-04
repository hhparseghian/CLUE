#define _POSIX_C_SOURCE 200809L
/*
 * CLUE -- Dialog widget
 *
 * Opens a real OS window for the dialog (X11/Wayland/DRM).
 * Modal dialogs run a nested event loop that blocks the caller.
 */

#include <stdlib.h>
#include <string.h>
#include "clue/dialog.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/button.h"
#include "clue/label.h"
#include "clue/box.h"
#include "clue/clue_widget.h"
#include "clue/clue.h"

#define TITLE_PAD 12
#define BODY_PAD  16
#define BTN_PAD   12
#define BTN_H     36

/* ------------------------------------------------------------------ */
/* Internal: render dialog content into its window                     */
/* ------------------------------------------------------------------ */

static void dialog_render(ClueDialog *d, UIWindow *win, UIRenderer *r)
{
    const ClueTheme *th = clue_theme_get();
    UIFont *font = clue_app_default_font();

    /* Make the dialog window's EGL surface current */
    clue_window_make_current(win);
    r->begin_frame(win);
    r->clear(th->surface.r, th->surface.g, th->surface.b, th->surface.a);

    int w = win->w, h = win->h;

    /* Title */
    if (font && d->title) {
        r->draw_text(TITLE_PAD, TITLE_PAD, d->title, font, th->fg_bright);
    }

    int title_h = font ? clue_font_line_height(font) + TITLE_PAD * 2 : 40;

    /* Separator line */
    r->draw_line(0, title_h, w, title_h, 1.0f, th->surface_border);

    /* Content */
    if (d->content) {
        d->content->base.x = BODY_PAD;
        d->content->base.y = title_h + BODY_PAD;
        d->content->base.w = w - BODY_PAD * 2;
        clue_cwidget_layout_tree(d->content);
        clue_cwidget_draw_tree(d->content);
    }

    /* Buttons at bottom, right-aligned */
    int btn_y = h - BTN_PAD - BTN_H;
    int btn_x = w - BTN_PAD;
    for (int i = d->button_count - 1; i >= 0; i--) {
        ClueWidget *bw = (ClueWidget *)d->buttons[i];
        clue_cwidget_layout_tree(bw);
        btn_x -= bw->base.w;
        bw->base.x = btn_x;
        bw->base.y = btn_y;
        clue_cwidget_draw_tree(bw);
        btn_x -= BTN_PAD;
    }

    r->end_frame(win);
    clue_window_swap_buffers(win);
}

/* ------------------------------------------------------------------ */
/* Button click handler                                                */
/* ------------------------------------------------------------------ */

static void btn_clicked(void *widget, void *user_data)
{
    ClueDialog *d = (ClueDialog *)user_data;

    for (int i = 0; i < d->button_count; i++) {
        if ((void *)d->buttons[i] == widget) {
            d->result = d->btn_results[i];
            d->running = false;
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

ClueDialog *clue_dialog_new(const char *title, int w, int h)
{
    ClueDialog *d = calloc(1, sizeof(ClueDialog));
    if (!d) return NULL;

    d->title    = title ? strdup(title) : NULL;
    d->dialog_w = w;
    d->dialog_h = h;
    d->result   = CLUE_DIALOG_NONE;
    d->running  = false;

    return d;
}

void clue_dialog_destroy(ClueDialog *dlg)
{
    if (!dlg) return;
    free(dlg->title);
    if (dlg->content) clue_cwidget_destroy(dlg->content);
    for (int i = 0; i < dlg->button_count; i++) {
        clue_cwidget_destroy((ClueWidget *)dlg->buttons[i]);
    }
    free(dlg);
}

void clue_dialog_set_content(ClueDialog *dlg, ClueWidget *content)
{
    if (dlg) dlg->content = content;
}

void clue_dialog_add_button(ClueDialog *dlg, const char *label,
                            ClueDialogResult result)
{
    if (!dlg || dlg->button_count >= CLUE_DIALOG_MAX_BUTTONS) return;

    ClueButton *btn = clue_button_new(label);
    if (!btn) return;

    int idx = dlg->button_count;
    dlg->buttons[idx] = btn;
    dlg->btn_results[idx] = result;
    dlg->button_count++;

    clue_signal_connect(btn, "clicked", btn_clicked, dlg);
}

void clue_dialog_set_callback(ClueDialog *dlg, ClueDialogCallback cb,
                              void *user_data)
{
    if (!dlg) return;
    dlg->callback = cb;
    dlg->callback_data = user_data;
}

ClueDialogResult clue_dialog_run(ClueDialog *dlg)
{
    if (!dlg) return CLUE_DIALOG_NONE;

    ClueApp *app = clue_app_get();
    if (!app) return CLUE_DIALOG_NONE;

    /* Create a real OS window for the dialog */
    UIWindow *win = clue_window_create(dlg->dialog_w, dlg->dialog_h,
                                       dlg->title ? dlg->title : "Dialog");
    if (!win) return CLUE_DIALOG_NONE;

    clue_window_set_type(win, UI_WINDOW_DIALOG);
    clue_window_set_parent(win, app->window);

    dlg->result  = CLUE_DIALOG_NONE;
    dlg->running = true;

    /* Nested event loop -- blocks until a button is clicked or window closed */
    while (dlg->running) {
        UIEvent events[32];
        int count = clue_poll_events(events, 32);

        for (int i = 0; i < count; i++) {
            /* Close event on the dialog window */
            if (events[i].type == UI_EVENT_CLOSE &&
                events[i].window == win) {
                dlg->result = CLUE_DIALOG_CANCEL;
                dlg->running = false;
                break;
            }

            /* Close event on the main window -- propagate quit */
            if (events[i].type == UI_EVENT_CLOSE &&
                events[i].window == app->window) {
                dlg->result = CLUE_DIALOG_CANCEL;
                dlg->running = false;
                app->running = false;
                break;
            }

            /* Route events that target the dialog window to buttons */
            if (events[i].window == win) {
                for (int b = 0; b < dlg->button_count; b++) {
                    if (clue_widget_dispatch_event(
                            &dlg->buttons[b]->base.base, &events[i]))
                        break;
                }
                if (dlg->content) {
                    clue_widget_dispatch_event(
                        &dlg->content->base, &events[i]);
                }
            }
        }

        /* Render the main app window (keep it alive) */
        if (app->root) {
            clue_window_make_current(app->window);
            app->renderer->begin_frame(app->window);
            const ClueTheme *th = clue_theme_get();
            app->renderer->clear(th->bg.r, th->bg.g, th->bg.b, th->bg.a);
            clue_cwidget_draw_tree(app->root);
            app->renderer->end_frame(app->window);
            clue_window_swap_buffers(app->window);
        }

        /* Render the dialog window */
        dialog_render(dlg, win, app->renderer);
    }

    clue_window_destroy(win);

    if (dlg->callback) {
        dlg->callback(dlg->result, dlg->callback_data);
    }

    return dlg->result;
}

void clue_dialog_show(ClueDialog *dlg)
{
    /* Non-blocking: just run in the same way, but the caller
     * should call this from an event handler. The nested loop
     * handles modality. */
    clue_dialog_run(dlg);
}

ClueDialogResult clue_dialog_message(const char *title, const char *message)
{
    ClueDialog *d = clue_dialog_new(title, 350, 160);
    if (!d) return CLUE_DIALOG_NONE;

    ClueLabel *lbl = clue_label_new(message);
    if (lbl) {
        lbl->base.style.fg_color = clue_theme_get()->fg;
        clue_dialog_set_content(d, (ClueWidget *)lbl);
    }

    clue_dialog_add_button(d, "OK", CLUE_DIALOG_OK);

    ClueDialogResult r = clue_dialog_run(d);
    clue_dialog_destroy(d);
    return r;
}

ClueDialogResult clue_dialog_confirm(const char *title, const char *message)
{
    ClueDialog *d = clue_dialog_new(title, 380, 160);
    if (!d) return CLUE_DIALOG_NONE;

    ClueLabel *lbl = clue_label_new(message);
    if (lbl) {
        lbl->base.style.fg_color = clue_theme_get()->fg;
        clue_dialog_set_content(d, (ClueWidget *)lbl);
    }

    clue_dialog_add_button(d, "Cancel", CLUE_DIALOG_CANCEL);
    clue_dialog_add_button(d, "OK", CLUE_DIALOG_OK);

    ClueDialogResult r = clue_dialog_run(d);
    clue_dialog_destroy(d);
    return r;
}

void clue_dialog_message_async(const char *title, const char *message,
                               ClueDialogCallback cb, void *user_data)
{
    ClueDialog *d = clue_dialog_new(title, 350, 160);
    if (!d) return;

    ClueLabel *lbl = clue_label_new(message);
    if (lbl) {
        lbl->base.style.fg_color = clue_theme_get()->fg;
        clue_dialog_set_content(d, (ClueWidget *)lbl);
    }

    clue_dialog_add_button(d, "OK", CLUE_DIALOG_OK);
    clue_dialog_set_callback(d, cb, user_data);

    clue_dialog_run(d);
    clue_dialog_destroy(d);
}

void clue_dialog_confirm_async(const char *title, const char *message,
                               ClueDialogCallback cb, void *user_data)
{
    ClueDialog *d = clue_dialog_new(title, 380, 160);
    if (!d) return;

    ClueLabel *lbl = clue_label_new(message);
    if (lbl) {
        lbl->base.style.fg_color = clue_theme_get()->fg;
        clue_dialog_set_content(d, (ClueWidget *)lbl);
    }

    clue_dialog_add_button(d, "Cancel", CLUE_DIALOG_CANCEL);
    clue_dialog_add_button(d, "OK", CLUE_DIALOG_OK);
    clue_dialog_set_callback(d, cb, user_data);

    clue_dialog_run(d);
    clue_dialog_destroy(d);
}
