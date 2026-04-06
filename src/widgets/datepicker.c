#define CLUE_IMPL
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "clue/datepicker.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/draw.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/button.h"
#include "clue/spinbox.h"
#include "clue/clue_widget.h"
#include "clue/timer.h"
#include "clue/signal.h"

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */

#define DP_PANEL_W    320
#define DP_PANEL_H    360
#define DP_PANEL_H_T  440   /* with time picker */
#define DP_PAD        12
#define DP_NAV_H      36
#define DP_HDR_H      24
#define DP_CELL_PAD   2
#define DP_BTN_H      36
#define DP_TIME_H     44
#define DP_ROWS       6
#define DP_COLS       7

static const char *month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const char *day_headers[] = {
    "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"
};

/* ------------------------------------------------------------------ */
/* Calendar math                                                       */
/* ------------------------------------------------------------------ */

static int is_leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(int y, int m)
{
    static const int d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 30;
    if (m == 2 && is_leap(y)) return 29;
    return d[m - 1];
}

/* Day of week for any date (0=Sunday). Tomohiko Sakamoto's algorithm. */
static int day_of_week(int y, int m, int d)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    ClueWidget          base;       /* MUST be first */
    bool                visible;
    bool                with_time;
    bool                time_only;

    /* Current view */
    int                 view_year;
    int                 view_month; /* 1-12 */

    /* Selection */
    int                 sel_year, sel_month, sel_day;
    int                 sel_hour, sel_minute;

    /* Today */
    int                 today_year, today_month, today_day;

    /* Hover */
    int                 hover_day;  /* 1-31 or -1 */

    /* Panel geometry (calculated in draw) */
    int                 panel_x, panel_y, panel_w, panel_h;

    /* Buttons */
    ClueButton         *btn_ok;
    ClueButton         *btn_cancel;
    ClueButton         *btn_prev;
    ClueButton         *btn_next;
    ClueSpinbox        *spin_hour;
    ClueSpinbox        *spin_minute;

    /* Callback */
    ClueDateTimeCallback callback;
    void               *user_data;

    bool                handling_event;
} DatePicker;

static DatePicker *g_dp = NULL;

/* Initial date/time set before show */
static int g_init_year = 0, g_init_month = 0, g_init_day = 0;
static int g_init_hour = 0, g_init_minute = 0;

/* ------------------------------------------------------------------ */
/* Forward declarations                                                */
/* ------------------------------------------------------------------ */

static void dp_finish(DatePicker *dp, bool ok);

/* ------------------------------------------------------------------ */
/* Button callbacks                                                    */
/* ------------------------------------------------------------------ */

static void on_prev_month(void *w, void *d)
{
    DatePicker *dp = (DatePicker *)d;
    dp->view_month--;
    if (dp->view_month < 1) {
        dp->view_month = 12;
        dp->view_year--;
    }
}

static void on_next_month(void *w, void *d)
{
    DatePicker *dp = (DatePicker *)d;
    dp->view_month++;
    if (dp->view_month > 12) {
        dp->view_month = 1;
        dp->view_year++;
    }
}

static void on_ok(void *w, void *d)
{
    dp_finish((DatePicker *)d, true);
}

static void on_cancel(void *w, void *d)
{
    dp_finish((DatePicker *)d, false);
}

/* ------------------------------------------------------------------ */
/* Finish / cleanup                                                    */
/* ------------------------------------------------------------------ */

static bool dp_deferred_cleanup(void *data)
{
    DatePicker *dp = (DatePicker *)data;

    ClueDateTimeResult result = {0};
    result.year   = dp->sel_year;
    result.month  = dp->sel_month;
    result.day    = dp->sel_day;
    result.hour   = dp->sel_hour;
    result.minute = dp->sel_minute;
    result.has_time = dp->with_time;

    ClueDateTimeCallback cb = dp->callback;
    void *ud = dp->user_data;
    bool ok = dp->visible; /* reused as ok flag after dismiss */

    clue_cwidget_destroy((ClueWidget *)dp->btn_ok);
    clue_cwidget_destroy((ClueWidget *)dp->btn_cancel);
    if (dp->btn_prev) clue_cwidget_destroy((ClueWidget *)dp->btn_prev);
    if (dp->btn_next) clue_cwidget_destroy((ClueWidget *)dp->btn_next);
    if (dp->spin_hour)   clue_cwidget_destroy((ClueWidget *)dp->spin_hour);
    if (dp->spin_minute) clue_cwidget_destroy((ClueWidget *)dp->spin_minute);
    free(dp);
    g_dp = NULL;

    if (cb) cb(&result, ok, ud);
    return false;
}

static void dp_finish(DatePicker *dp, bool ok)
{
    if (!dp || dp != g_dp) return;

    ClueApp *app = clue_app_get();

    /* Read spinbox values */
    if (dp->with_time) {
        dp->sel_hour   = (int)clue_spinbox_get_value(dp->spin_hour);
        dp->sel_minute = (int)clue_spinbox_get_value(dp->spin_minute);
    }

    /* Stash ok in visible flag for deferred callback */
    dp->visible = ok;

    if (app && app->modal_widget == (ClueWidget *)dp)
        app->modal_widget = NULL;

    g_dp = NULL; /* prevent re-entry */
    clue_timer_once(0, dp_deferred_cleanup, dp);
}

/* ------------------------------------------------------------------ */
/* Draw                                                                */
/* ------------------------------------------------------------------ */

static void dp_draw(ClueWidget *w)
{
    DatePicker *dp = (DatePicker *)w;
    if (!dp->visible) return;

    ClueApp *app = clue_app_get();
    if (!app) return;

    const ClueTheme *th = clue_theme_get();
    ClueFont *font = clue_app_default_font();
    if (!font) return;

    int win_w = app->window->w;
    int win_h = app->window->h;

    dp->panel_w = dp->time_only ? 260 : DP_PANEL_W;
    dp->panel_h = dp->time_only ? 140 : (dp->with_time ? DP_PANEL_H_T : DP_PANEL_H);
    dp->panel_x = (win_w - dp->panel_w) / 2;
    dp->panel_y = (win_h - dp->panel_h) / 2;

    int px = dp->panel_x;
    int py = dp->panel_y;
    int pw = dp->panel_w;
    int ph = dp->panel_h;

    /* Dim background */
    clue_fill_rect(0, 0, win_w, win_h, CLUE_RGBA(0, 0, 0, 120));

    /* Panel */
    clue_fill_rounded_rect(px, py, pw, ph, 8.0f, th->surface);
    clue_draw_rounded_rect(px, py, pw, ph, 8.0f, 1.0f, th->surface_border);

    int cx = px + DP_PAD;
    int cy = py + DP_PAD;
    int cw = pw - DP_PAD * 2;
    int lh = clue_font_line_height(font);

    /* --- Calendar --- */
    if (!dp->time_only) {

    /* --- Navigation bar: < Month Year > --- */
    dp->btn_prev->base.base.x = cx;
    dp->btn_prev->base.base.y = cy;
    dp->btn_prev->base.base.w = 36;
    dp->btn_prev->base.base.h = DP_NAV_H;
    clue_cwidget_draw_tree((ClueWidget *)dp->btn_prev);

    dp->btn_next->base.base.x = cx + cw - 36;
    dp->btn_next->base.base.y = cy;
    dp->btn_next->base.base.w = 36;
    dp->btn_next->base.base.h = DP_NAV_H;
    clue_cwidget_draw_tree((ClueWidget *)dp->btn_next);

    /* Month/year title centered */
    char title[64];
    snprintf(title, sizeof(title), "%s %d",
             month_names[dp->view_month - 1], dp->view_year);
    int tw = clue_font_text_width(font, title);
    clue_draw_text(cx + (cw - tw) / 2, cy + (DP_NAV_H - lh) / 2,
                   title, font, th->fg);

    cy += DP_NAV_H + 4;

    /* --- Day-of-week headers --- */
    int cell_w = cw / DP_COLS;
    for (int i = 0; i < DP_COLS; i++) {
        int hx = cx + i * cell_w;
        int htw = clue_font_text_width(font, day_headers[i]);
        clue_draw_text(hx + (cell_w - htw) / 2, cy + (DP_HDR_H - lh) / 2,
                       day_headers[i], font, th->fg_dim);
    }
    cy += DP_HDR_H;

    /* --- Calendar grid --- */
    int first_dow = day_of_week(dp->view_year, dp->view_month, 1);
    int dim = days_in_month(dp->view_year, dp->view_month);
    int cell_h = (ph - (cy - py) - DP_PAD - DP_BTN_H - 8 -
                  (dp->with_time ? DP_TIME_H + 8 : 0)) / DP_ROWS;
    if (cell_h < 20) cell_h = 20;

    for (int row = 0; row < DP_ROWS; row++) {
        for (int col = 0; col < DP_COLS; col++) {
            int idx = row * DP_COLS + col;
            int day = idx - first_dow + 1;

            int gx = cx + col * cell_w + DP_CELL_PAD;
            int gy = cy + row * cell_h + DP_CELL_PAD;
            int gw = cell_w - DP_CELL_PAD * 2;
            int gh = cell_h - DP_CELL_PAD * 2;

            if (day < 1 || day > dim) continue;

            bool selected = (day == dp->sel_day &&
                             dp->view_month == dp->sel_month &&
                             dp->view_year == dp->sel_year);
            bool today = (day == dp->today_day &&
                          dp->view_month == dp->today_month &&
                          dp->view_year == dp->today_year);
            bool hovered = (day == dp->hover_day);

            if (selected) {
                clue_fill_rounded_rect(gx, gy, gw, gh, 4.0f, th->accent);
            } else if (hovered) {
                clue_fill_rounded_rect(gx, gy, gw, gh, 4.0f, th->surface_hover);
            }

            if (today && !selected) {
                clue_draw_rounded_rect(gx, gy, gw, gh, 4.0f, 1.5f, th->accent);
            }

            char buf[12];
            snprintf(buf, sizeof(buf), "%d", day);
            int dtw = clue_font_text_width(font, buf);
            ClueColor fg = selected ? (ClueColor){1,1,1,1} : th->fg;
            clue_draw_text(gx + (gw - dtw) / 2, gy + (gh - lh) / 2,
                           buf, font, fg);
        }
    }

    cy += DP_ROWS * cell_h + 4;

    } /* end if (!dp->time_only) */

    /* --- Time picker --- */
    if (dp->with_time || dp->time_only) {
        /* Label */
        clue_draw_text(cx, cy + (DP_TIME_H - lh) / 2, "Time:", font, th->fg);

        int spin_w = 80;
        int sx = cx + cw / 2 - spin_w - 10;

        dp->spin_hour->base.base.x = sx;
        dp->spin_hour->base.base.y = cy + 4;
        dp->spin_hour->base.base.w = spin_w;
        dp->spin_hour->base.base.h = DP_TIME_H - 8;
        clue_cwidget_layout_tree((ClueWidget *)dp->spin_hour);
        clue_cwidget_draw_tree((ClueWidget *)dp->spin_hour);

        /* ":" separator */
        clue_draw_text(sx + spin_w + 2, cy + (DP_TIME_H - lh) / 2,
                       ":", font, th->fg);

        dp->spin_minute->base.base.x = sx + spin_w + 16;
        dp->spin_minute->base.base.y = cy + 4;
        dp->spin_minute->base.base.w = spin_w;
        dp->spin_minute->base.base.h = DP_TIME_H - 8;
        clue_cwidget_layout_tree((ClueWidget *)dp->spin_minute);
        clue_cwidget_draw_tree((ClueWidget *)dp->spin_minute);

        cy += DP_TIME_H + 4;
    }

    /* --- OK / Cancel buttons --- */
    int btn_w = 80;
    int bx = px + pw - DP_PAD - btn_w;
    int by = py + ph - DP_PAD - DP_BTN_H;

    dp->btn_ok->base.base.x = bx;
    dp->btn_ok->base.base.y = by;
    dp->btn_ok->base.base.w = btn_w;
    dp->btn_ok->base.base.h = DP_BTN_H;
    clue_cwidget_draw_tree((ClueWidget *)dp->btn_ok);

    dp->btn_cancel->base.base.x = bx - btn_w - 8;
    dp->btn_cancel->base.base.y = by;
    dp->btn_cancel->base.base.w = btn_w;
    dp->btn_cancel->base.base.h = DP_BTN_H;
    clue_cwidget_draw_tree((ClueWidget *)dp->btn_cancel);
}

/* ------------------------------------------------------------------ */
/* Event handling                                                      */
/* ------------------------------------------------------------------ */

static int dp_handle_event(ClueWidget *w, ClueEvent *event)
{
    DatePicker *dp = (DatePicker *)w;
    if (!dp->visible) return 0;

    /* Route to buttons/spinboxes */
    dp->handling_event = true;

    if (clue_widget_dispatch_event(&dp->btn_ok->base.base, event) ||
        clue_widget_dispatch_event(&dp->btn_cancel->base.base, event)) {
        dp->handling_event = false;
        return 1;
    }

    if (!dp->time_only) {
        if (clue_widget_dispatch_event(&dp->btn_prev->base.base, event) ||
            clue_widget_dispatch_event(&dp->btn_next->base.base, event)) {
            dp->handling_event = false;
            return 1;
        }
    }

    if ((dp->with_time || dp->time_only) && dp->spin_hour && dp->spin_minute) {
        if (clue_widget_dispatch_event(&dp->spin_hour->base.base, event) ||
            clue_widget_dispatch_event(&dp->spin_minute->base.base, event)) {
            dp->handling_event = false;
            return 1;
        }
    }
    dp->handling_event = false;

    /* Calendar grid click/hover detection */
    int mx = 0, my = 0;
    bool is_click = false;
    bool is_move = false;

    if (event->type == CLUE_EVENT_MOUSE_BUTTON && event->mouse_button.pressed) {
        mx = event->mouse_button.x;
        my = event->mouse_button.y;
        is_click = true;
    } else if (event->type == CLUE_EVENT_MOUSE_MOVE) {
        mx = event->mouse_move.x;
        my = event->mouse_move.y;
        is_move = true;
    }

    if (!dp->time_only && (is_click || is_move)) {
        int cx = dp->panel_x + DP_PAD;
        int cy = dp->panel_y + DP_PAD + DP_NAV_H + 4 + DP_HDR_H;
        int cw = dp->panel_w - DP_PAD * 2;
        int cell_w = cw / DP_COLS;
        int ph = dp->with_time ? DP_PANEL_H_T : DP_PANEL_H;
        int cell_h = (ph - (cy - dp->panel_y) - DP_PAD - DP_BTN_H - 8 -
                      (dp->with_time ? DP_TIME_H + 8 : 0)) / DP_ROWS;
        if (cell_h < 20) cell_h = 20;

        int first_dow = day_of_week(dp->view_year, dp->view_month, 1);
        int dim = days_in_month(dp->view_year, dp->view_month);

        if (mx >= cx && mx < cx + cw && my >= cy && my < cy + DP_ROWS * cell_h) {
            int col = (mx - cx) / cell_w;
            int row = (my - cy) / cell_h;
            int day = row * DP_COLS + col - first_dow + 1;

            if (day >= 1 && day <= dim) {
                if (is_move) {
                    dp->hover_day = day;
                } else {
                    dp->sel_year  = dp->view_year;
                    dp->sel_month = dp->view_month;
                    dp->sel_day   = day;
                    dp->hover_day = -1;
                }
                return 1;
            }
        }

        if (is_move) dp->hover_day = -1;
    }

    /* Consume events inside the panel */
    if (event->type == CLUE_EVENT_MOUSE_BUTTON ||
        event->type == CLUE_EVENT_MOUSE_MOVE) {
        int ex = 0, ey = 0;
        if (event->type == CLUE_EVENT_MOUSE_BUTTON) {
            ex = event->mouse_button.x;
            ey = event->mouse_button.y;
        } else {
            ex = event->mouse_move.x;
            ey = event->mouse_move.y;
        }
        if (ex >= dp->panel_x && ex < dp->panel_x + dp->panel_w &&
            ey >= dp->panel_y && ey < dp->panel_y + dp->panel_h) {
            return 1;
        }
    }

    /* Let events outside the panel through to the app */
    ClueApp *app = clue_app_get();
    if (app && app->root)
        return clue_widget_dispatch_event(&app->root->base, event);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Layout / destroy                                                    */
/* ------------------------------------------------------------------ */

static void dp_layout(ClueWidget *w) { (void)w; }

static void dp_destroy(ClueWidget *w) { (void)w; }

static const ClueWidgetVTable dp_vtable = {
    .draw         = dp_draw,
    .layout       = dp_layout,
    .handle_event = dp_handle_event,
    .destroy      = dp_destroy,
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void clue_datepicker_set_date(int year, int month, int day)
{
    g_init_year  = year;
    g_init_month = month;
    g_init_day   = day;
}

void clue_datepicker_set_time(int hour, int minute)
{
    g_init_hour   = hour;
    g_init_minute = minute;
}

static void dp_show(bool with_time, bool time_only,
                    ClueDateTimeCallback callback, void *user_data)
{
    ClueApp *app = clue_app_get();
    if (!app || g_dp) return;

    DatePicker *dp = calloc(1, sizeof(DatePicker));
    if (!dp) return;

    clue_cwidget_init(&dp->base, &dp_vtable);
    dp->visible   = true;
    dp->with_time = with_time || time_only;
    dp->time_only = time_only;
    dp->callback  = callback;
    dp->user_data = user_data;
    dp->hover_day = -1;

    /* Get today */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    dp->today_year  = t->tm_year + 1900;
    dp->today_month = t->tm_mon + 1;
    dp->today_day   = t->tm_mday;

    /* Initial selection */
    if (g_init_year > 0) {
        dp->sel_year  = g_init_year;
        dp->sel_month = g_init_month;
        dp->sel_day   = g_init_day;
    } else {
        dp->sel_year  = dp->today_year;
        dp->sel_month = dp->today_month;
        dp->sel_day   = dp->today_day;
    }
    dp->view_year  = dp->sel_year;
    dp->view_month = dp->sel_month;

    if (with_time || time_only) {
        dp->sel_hour   = g_init_hour;
        dp->sel_minute = g_init_minute;
    }

    /* Reset initial values */
    g_init_year = g_init_month = g_init_day = 0;
    g_init_hour = g_init_minute = 0;

    /* Create buttons */
    if (!time_only) {
        dp->btn_prev = clue_button_new("<");
        dp->btn_prev->base.base.focusable = false;
        clue_signal_connect(dp->btn_prev, "clicked", on_prev_month, dp);

        dp->btn_next = clue_button_new(">");
        dp->btn_next->base.base.focusable = false;
        clue_signal_connect(dp->btn_next, "clicked", on_next_month, dp);
    }

    dp->btn_ok = clue_button_new("OK");
    dp->btn_ok->base.base.focusable = false;
    dp->btn_ok->base.style.bg_color = clue_theme_get()->accent;
    clue_signal_connect(dp->btn_ok, "clicked", on_ok, dp);

    dp->btn_cancel = clue_button_new("Cancel");
    dp->btn_cancel->base.base.focusable = false;
    clue_signal_connect(dp->btn_cancel, "clicked", on_cancel, dp);

    if (dp->with_time) {
        dp->spin_hour = clue_spinbox_new(0, 23, 1, dp->sel_hour);
        dp->spin_hour->base.base.focusable = false;
        dp->spin_minute = clue_spinbox_new(0, 59, 1, dp->sel_minute);
        dp->spin_minute->base.base.focusable = false;
    }

    g_dp = dp;
    app->modal_widget = (ClueWidget *)dp;
}

void clue_datepicker_show(ClueDateTimeCallback callback, void *user_data)
{
    dp_show(false, false, callback, user_data);
}

void clue_datetimepicker_show(ClueDateTimeCallback callback, void *user_data)
{
    dp_show(true, false, callback, user_data);
}

void clue_timepicker_show(ClueDateTimeCallback callback, void *user_data)
{
    dp_show(false, true, callback, user_data);
}
