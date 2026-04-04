#include <stdlib.h>
#include "clue/canvas.h"
#include "clue/draw.h"
#include "clue/theme.h"
#include "clue/app.h"
#include "clue/window.h"

static void canvas_draw(ClueWidget *w)
{
    ClueCanvas *c = (ClueCanvas *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    clue_fill_rect(x, y, bw, bh, th->surface);
    clue_draw_rect(x, y, bw, bh, 1.0f, th->surface_border);

    /* User draw callback */
    if (c->draw_cb) {
        clue_set_clip_rect(x, y, bw, bh);
        c->draw_cb(x, y, bw, bh, c->draw_data);
        clue_reset_clip_rect();
    }
}

static void canvas_layout(ClueWidget *w)
{
    if (w->base.w == 0) w->base.w = 200;
    if (w->base.h == 0) w->base.h = 150;
}

static void fire_event(ClueCanvas *c, int type, int mx, int my, int btn)
{
    if (!c->event_cb) return;
    int rx = mx - c->base.base.x;
    int ry = my - c->base.base.y;
    c->event_cb(type, rx, ry, btn, c->event_data);
}

static int canvas_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueCanvas *c = (ClueCanvas *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    if (event->type == UI_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x, my = event->mouse_move.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            if (event->window)
                clue_window_set_cursor(event->window, UI_CURSOR_CROSSHAIR);
        }
        if (c->painting) {
            fire_event(c, CLUE_CANVAS_MOTION, mx, my, 0);
            return 1;
        }
        return 0;
    }

    if (event->type == UI_EVENT_MOUSE_BUTTON) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;

        if (event->mouse_button.pressed && inside) {
            c->painting = true;
            clue_capture_mouse(&w->base);
            fire_event(c, CLUE_CANVAS_PRESS, mx, my, event->mouse_button.btn);
            return 1;
        }
        if (!event->mouse_button.pressed && c->painting) {
            c->painting = false;
            clue_release_mouse();
            fire_event(c, CLUE_CANVAS_RELEASE, mx, my, event->mouse_button.btn);
            return 1;
        }
    }

    return 0;
}

static const ClueWidgetVTable canvas_vtable = {
    .draw         = canvas_draw,
    .layout       = canvas_layout,
    .handle_event = canvas_handle_event,
};

ClueCanvas *clue_canvas_new(int w, int h)
{
    ClueCanvas *c = calloc(1, sizeof(ClueCanvas));
    if (!c) return NULL;

    clue_cwidget_init(&c->base, &canvas_vtable);
    c->base.type_id = CLUE_WIDGET_CANVAS;
    c->base.base.w = w;
    c->base.base.h = h;

    return c;
}

void clue_canvas_set_draw(ClueCanvas *c, ClueCanvasDrawCb cb, void *user_data)
{
    if (!c) return;
    c->draw_cb   = cb;
    c->draw_data = user_data;
}

void clue_canvas_set_event(ClueCanvas *c, ClueCanvasEventCb cb, void *user_data)
{
    if (!c) return;
    c->event_cb   = cb;
    c->event_data = user_data;
}
