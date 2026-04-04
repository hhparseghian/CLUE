#include <stdlib.h>
#include "clue/canvas.h"
#include "clue/draw.h"
#include "clue/theme.h"

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
    /* Size is set by the user or by expand flags */
    if (w->base.w == 0) w->base.w = 200;
    if (w->base.h == 0) w->base.h = 150;
}

static int canvas_handle_event(ClueWidget *w, UIEvent *event)
{
    if (event->type == UI_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed &&
        event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;
        if (mx >= w->base.x && mx < w->base.x + w->base.w &&
            my >= w->base.y && my < w->base.y + w->base.h) {
            ClueCanvas *c = (ClueCanvas *)w;
            clue_signal_emit(c, "clicked");
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
