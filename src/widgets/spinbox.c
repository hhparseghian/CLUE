#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "clue/spinbox.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/timer.h"

#define PAD_H           8
#define PAD_V           6
#define BTN_W           24
#define REPEAT_MS       80   /* interval between auto-repeat ticks */
#define REPEAT_DELAY    6    /* ticks before repeat starts (~480ms) */

static UIFont *spinbox_font(ClueSpinbox *s)
{
    return s->base.style.font ? s->base.style.font : clue_app_default_font();
}

static void format_value(ClueSpinbox *s, char *buf, int buflen)
{
    if (s->decimals <= 0)
        snprintf(buf, buflen, "%d", (int)s->value);
    else
        snprintf(buf, buflen, "%.*f", s->decimals, s->value);
}

static void clamp_value(ClueSpinbox *s)
{
    if (s->value < s->min_val) s->value = s->min_val;
    if (s->value > s->max_val) s->value = s->max_val;
}

static void stop_repeat(ClueSpinbox *s)
{
    if (s->repeat_timer) {
        clue_timer_cancel(s->repeat_timer);
        s->repeat_timer = 0;
    }
    s->repeat_count = 0;
}

static bool repeat_tick(void *data)
{
    ClueSpinbox *s = data;
    s->repeat_count++;
    /* Initial delay before repeating */
    if (s->repeat_count < REPEAT_DELAY) return true;

    if (s->btn_up_pressed) s->value += s->step;
    else if (s->btn_dn_pressed) s->value -= s->step;
    else { stop_repeat(s); return false; }

    clamp_value(s);
    clue_signal_emit(s, "changed");
    return true;
}

static void spinbox_draw(ClueWidget *w)
{
    ClueSpinbox *s = (ClueSpinbox *)w;
    const ClueTheme *th = clue_theme_get();
    UIFont *font = spinbox_font(s);
    if (!font) return;

    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;
    float radius = th->corner_radius;

    /* Background */
    clue_fill_rounded_rect(x, y, bw, bh, radius, th->input.bg);
    clue_draw_rounded_rect(x, y, bw, bh, radius, 1.5f, th->input.border);

    /* Value text */
    char buf[64];
    format_value(s, buf, sizeof(buf));
    int tw = clue_font_text_width(font, buf);
    int text_area = bw - BTN_W * 2;
    int text_x = x + BTN_W + (text_area - tw) / 2;
    int text_y = y + (bh - clue_font_line_height(font)) / 2;
    clue_draw_text(text_x, text_y, buf, font, th->input.fg);

    /* Down button (left) */
    UIColor dn_bg = s->btn_dn_pressed ? th->button.bg_pressed :
                    s->btn_dn_hover   ? th->button.bg_hover : th->surface;
    clue_fill_rounded_rect(x + 1, y + 1, BTN_W - 1, bh - 2, radius, dn_bg);
    /* Minus sign */
    int mid_y = y + bh / 2;
    clue_fill_rect(x + 7, mid_y - 1, BTN_W - 14, 2, th->fg);

    /* Up button (right) */
    int rx = x + bw - BTN_W;
    UIColor up_bg = s->btn_up_pressed ? th->button.bg_pressed :
                    s->btn_up_hover   ? th->button.bg_hover : th->surface;
    clue_fill_rounded_rect(rx, y + 1, BTN_W - 1, bh - 2, radius, up_bg);
    /* Plus sign */
    clue_fill_rect(rx + 7, mid_y - 1, BTN_W - 14, 2, th->fg);
    int mid_x = rx + BTN_W / 2;
    clue_fill_rect(mid_x - 1, y + bh / 2 - (BTN_W - 14) / 2, 2, BTN_W - 14, th->fg);
}

static void spinbox_layout(ClueWidget *w)
{
    ClueSpinbox *s = (ClueSpinbox *)w;
    UIFont *font = spinbox_font(s);
    if (!font) return;
    if (w->base.w == 0) w->base.w = 120;
    w->base.h = clue_font_line_height(font) + PAD_V * 2;
}

static int spinbox_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueSpinbox *s = (ClueSpinbox *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    if (event->type == UI_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x, my = event->mouse_move.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;
        s->btn_dn_hover = inside && mx < x + BTN_W;
        s->btn_up_hover = inside && mx >= x + bw - BTN_W;
        return 0;
    }

    if (event->type == UI_EVENT_MOUSE_BUTTON && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;
        if (!inside) return 0;

        if (event->mouse_button.pressed) {
            if (mx < x + BTN_W) {
                s->btn_dn_pressed = true;
                s->value -= s->step;
                clamp_value(s);
                clue_signal_emit(s, "changed");
                stop_repeat(s);
                s->repeat_timer = clue_timer_repeat(REPEAT_MS, repeat_tick, s);
                return 1;
            }
            if (mx >= x + bw - BTN_W) {
                s->btn_up_pressed = true;
                s->value += s->step;
                clamp_value(s);
                clue_signal_emit(s, "changed");
                stop_repeat(s);
                s->repeat_timer = clue_timer_repeat(REPEAT_MS, repeat_tick, s);
                return 1;
            }
        } else {
            s->btn_up_pressed = false;
            s->btn_dn_pressed = false;
            stop_repeat(s);
        }
        return 1;
    }

    if (event->type == UI_EVENT_MOUSE_SCROLL) {
        int mx = event->mouse_scroll.x, my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            if (event->mouse_scroll.dy > 0) s->value += s->step;
            else if (event->mouse_scroll.dy < 0) s->value -= s->step;
            clamp_value(s);
            clue_signal_emit(s, "changed");
            return 1;
        }
    }

    return 0;
}

static const ClueWidgetVTable spinbox_vtable = {
    .draw         = spinbox_draw,
    .layout       = spinbox_layout,
    .handle_event = spinbox_handle_event,
    .destroy      = NULL,
};

ClueSpinbox *clue_spinbox_new(double min_val, double max_val,
                              double step, double value)
{
    ClueSpinbox *s = calloc(1, sizeof(ClueSpinbox));
    if (!s) return NULL;

    clue_cwidget_init(&s->base, &spinbox_vtable);
    s->base.type_id = CLUE_WIDGET_SPINBOX;
    s->min_val  = min_val;
    s->max_val  = max_val;
    s->step     = step;
    s->value    = value;
    s->decimals = 0;
    clamp_value(s);

    spinbox_layout(&s->base);
    return s;
}

double clue_spinbox_get_value(ClueSpinbox *s)
{
    return s ? s->value : 0.0;
}

void clue_spinbox_set_value(ClueSpinbox *s, double value)
{
    if (!s) return;
    s->value = value;
    clamp_value(s);
}

void clue_spinbox_set_decimals(ClueSpinbox *s, int decimals)
{
    if (s) s->decimals = decimals < 0 ? 0 : decimals;
}
