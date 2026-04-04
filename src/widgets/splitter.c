#include <stdlib.h>
#include "clue/splitter.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/theme.h"
#include "clue/window.h"

#define DIVIDER_PX 6
#define MIN_PANE   30

static void splitter_draw(ClueWidget *w)
{
    ClueSplitter *s = (ClueSplitter *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Divider */
    UIColor div_col = s->dragging ? th->accent : th->surface_border;
    if (s->orientation == CLUE_HORIZONTAL) {
        int dx = x + (int)(bw * s->ratio) - s->divider_px / 2;
        clue_fill_rect(dx, y, s->divider_px, bh, div_col);
        /* Grip dots */
        int mid = dx + s->divider_px / 2;
        int cy = y + bh / 2;
        for (int i = -1; i <= 1; i++)
            clue_fill_circle(mid, cy + i * 8, 2, th->fg_dim);
    } else {
        int dy = y + (int)(bh * s->ratio) - s->divider_px / 2;
        clue_fill_rect(x, dy, bw, s->divider_px, div_col);
        int mid = dy + s->divider_px / 2;
        int cx = x + bw / 2;
        for (int i = -1; i <= 1; i++)
            clue_fill_circle(cx + i * 8, mid, 2, th->fg_dim);
    }
}

static void splitter_layout(ClueWidget *w)
{
    ClueSplitter *s = (ClueSplitter *)w;
    if (w->base.child_count < 2) return;

    ClueWidget *a = (ClueWidget *)w->base.children[0];
    ClueWidget *b = (ClueWidget *)w->base.children[1];
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;
    int div = s->divider_px;

    if (s->orientation == CLUE_HORIZONTAL) {
        int aw = (int)(bw * s->ratio) - div / 2;
        if (aw < MIN_PANE) aw = MIN_PANE;
        int bww = bw - aw - div;
        if (bww < MIN_PANE) bww = MIN_PANE;

        a->base.x = x;         a->base.y = y;
        a->base.w = aw;        a->base.h = bh;
        b->base.x = x + aw + div; b->base.y = y;
        b->base.w = bww;       b->base.h = bh;
    } else {
        int ah = (int)(bh * s->ratio) - div / 2;
        if (ah < MIN_PANE) ah = MIN_PANE;
        int bhh = bh - ah - div;
        if (bhh < MIN_PANE) bhh = MIN_PANE;

        a->base.x = x; a->base.y = y;
        a->base.w = bw; a->base.h = ah;
        b->base.x = x; b->base.y = y + ah + div;
        b->base.w = bw; b->base.h = bhh;
    }
}

static bool on_divider(ClueSplitter *s, int mx, int my)
{
    int x = s->base.base.x, y = s->base.base.y;
    int bw = s->base.base.w, bh = s->base.base.h;
    if (s->orientation == CLUE_HORIZONTAL) {
        int dx = x + (int)(bw * s->ratio) - s->divider_px / 2;
        return mx >= dx && mx < dx + s->divider_px &&
               my >= y && my < y + bh;
    } else {
        int dy = y + (int)(bh * s->ratio) - s->divider_px / 2;
        return my >= dy && my < dy + s->divider_px &&
               mx >= x && mx < x + bw;
    }
}

static void set_resize_cursor(ClueSplitter *s, bool active)
{
    ClueApp *app = clue_app_get();
    if (!app || !app->window) return;
    if (active) {
        UICursorShape shape = s->orientation == CLUE_HORIZONTAL
            ? UI_CURSOR_RESIZE_H : UI_CURSOR_RESIZE_V;
        clue_window_set_cursor(app->window, shape);
    } else {
        clue_window_set_cursor(app->window, UI_CURSOR_DEFAULT);
    }
}

static int splitter_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueSplitter *s = (ClueSplitter *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    if (event->type == UI_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x, my = event->mouse_move.y;
        if (s->dragging) {
            float new_ratio;
            if (s->orientation == CLUE_HORIZONTAL) {
                new_ratio = (float)(mx - x) / (float)bw;
            } else {
                new_ratio = (float)(my - y) / (float)bh;
            }
            if (new_ratio < 0.05f) new_ratio = 0.05f;
            if (new_ratio > 0.95f) new_ratio = 0.95f;
            if (new_ratio != s->ratio) {
                s->ratio = new_ratio;
                clue_signal_emit(s, "changed");
            }
            return 1;
        }
        /* Update cursor when hovering over divider */
        set_resize_cursor(s,on_divider(s, mx, my));
        return 0;
    }

    if (event->type == UI_EVENT_MOUSE_BUTTON && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;
        if (event->mouse_button.pressed) {
            if (on_divider(s, mx, my)) {
                s->dragging = true;
                set_resize_cursor(s,true);
                clue_capture_mouse(&w->base);
                return 1;
            }
        } else if (s->dragging) {
            s->dragging = false;
            set_resize_cursor(s,on_divider(s, mx, my));
            clue_release_mouse();
            return 1;
        }
    }

    return 0;
}

static const ClueWidgetVTable splitter_vtable = {
    .draw         = splitter_draw,
    .layout       = splitter_layout,
    .handle_event = splitter_handle_event,
};

ClueSplitter *clue_splitter_new(ClueBoxOrientation orientation)
{
    ClueSplitter *s = calloc(1, sizeof(ClueSplitter));
    if (!s) return NULL;

    clue_cwidget_init(&s->base, &splitter_vtable);
    s->base.type_id = CLUE_WIDGET_SPLITTER;
    s->base.base.intercept_events = true;
    s->base.style.hexpand = true;
    s->base.style.vexpand = true;
    s->orientation = orientation;
    s->ratio = 0.5f;
    s->divider_px = DIVIDER_PX;

    return s;
}

void clue_splitter_set_ratio(ClueSplitter *s, float ratio)
{
    if (!s) return;
    if (ratio < 0.05f) ratio = 0.05f;
    if (ratio > 0.95f) ratio = 0.95f;
    s->ratio = ratio;
}

float clue_splitter_get_ratio(ClueSplitter *s)
{
    return s ? s->ratio : 0.5f;
}
