#include <stdlib.h>
#include "clue/scroll.h"
#include "clue/draw.h"
#include "clue/clue_widget.h"

#define SCROLLBAR_W 6
#define SCROLLBAR_PAD 2

static void clamp_scroll(ClueScroll *s)
{
    int max_y = s->content_h - s->base.base.h;
    if (max_y < 0) max_y = 0;
    if (s->scroll_y > max_y) s->scroll_y = max_y;
    if (s->scroll_y < 0) s->scroll_y = 0;

    int max_x = s->content_w - s->base.base.w;
    if (max_x < 0) max_x = 0;
    if (s->scroll_x > max_x) s->scroll_x = max_x;
    if (s->scroll_x < 0) s->scroll_x = 0;
}

static void offset_tree(ClueWidget *w, int dx, int dy)
{
    w->base.x += dx;
    w->base.y += dy;
    for (int i = 0; i < w->base.child_count; i++)
        offset_tree((ClueWidget *)w->base.children[i], dx, dy);
}

static void scroll_draw(ClueWidget *w)
{
    ClueScroll *s = (ClueScroll *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    if (w->style.bg_color.a > 0.001f) {
        clue_fill_rounded_rect(x, y, bw, bh,
                               w->style.corner_radius, w->style.bg_color);
    }

    /* Clip and draw children with scroll offset applied to all descendants */
    clue_set_clip_rect(x, y, bw, bh);

    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        offset_tree(child, -s->scroll_x, -s->scroll_y);
        clue_cwidget_draw_tree(child);
        offset_tree(child, s->scroll_x, s->scroll_y);
    }

    clue_reset_clip_rect();

    /* Vertical scrollbar */
    if (s->content_h > bh) {
        float ratio = (float)bh / (float)s->content_h;
        int bar_h = (int)(ratio * bh);
        if (bar_h < 20) bar_h = 20;
        float pos_ratio = (float)s->scroll_y / (float)(s->content_h - bh);
        int bar_y = y + (int)(pos_ratio * (bh - bar_h));
        int bar_x = x + bw - SCROLLBAR_W - SCROLLBAR_PAD;

        clue_fill_rounded_rect(bar_x, bar_y, SCROLLBAR_W, bar_h,
                               SCROLLBAR_W / 2.0f, UI_RGBA(150, 150, 160, 120));
    }
}

/* Recursively compute the bounding box of all descendants */
static void measure_content(ClueWidget *root, ClueWidget *w, int *max_w, int *max_h)
{
    int r = w->base.x + w->base.w - root->base.x;
    int b = w->base.y + w->base.h - root->base.y;
    if (r > *max_w) *max_w = r;
    if (b > *max_h) *max_h = b;
    for (int i = 0; i < w->base.child_count; i++)
        measure_content(root, (ClueWidget *)w->base.children[i], max_w, max_h);
}

static void scroll_layout(ClueWidget *w)
{
    ClueScroll *s = (ClueScroll *)w;

    /* Give children the scroll's position and width, but reset height
     * so they compute their natural (unconstrained) content height. */
    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        child->base.x = w->base.x;
        child->base.y = w->base.y;
        if (child->style.hexpand)
            child->base.w = w->base.w;
        child->base.h = 0;
        clue_cwidget_layout_tree(child);
    }

    /* Measure full content bounds from all descendants */
    int max_w = 0, max_h = 0;
    for (int i = 0; i < w->base.child_count; i++)
        measure_content(w, (ClueWidget *)w->base.children[i], &max_w, &max_h);

    s->content_w = max_w;
    s->content_h = max_h;
    clamp_scroll(s);
}

static int scroll_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueScroll *s = (ClueScroll *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    if (event->type == UI_EVENT_MOUSE_SCROLL) {
        int mx = event->mouse_scroll.x;
        int my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            s->scroll_y -= (int)(event->mouse_scroll.dy * s->scroll_speed);
            clamp_scroll(s);
            return 1;
        }
    }

    /* Translate mouse events for children */
    if (event->type == UI_EVENT_MOUSE_MOVE) {
        event->mouse_move.x += s->scroll_x;
        event->mouse_move.y += s->scroll_y;
    } else if (event->type == UI_EVENT_MOUSE_BUTTON) {
        event->mouse_button.x += s->scroll_x;
        event->mouse_button.y += s->scroll_y;
    }

    return 0; /* let children handle */
}

static const ClueWidgetVTable scroll_vtable = {
    .draw         = scroll_draw,
    .layout       = scroll_layout,
    .handle_event = scroll_handle_event,
    .destroy      = NULL,
};

ClueScroll *clue_scroll_new(void)
{
    ClueScroll *s = calloc(1, sizeof(ClueScroll));
    if (!s) return NULL;

    clue_cwidget_init(&s->base, &scroll_vtable);
    s->base.base.skip_child_draw = true;
    s->base.base.intercept_events = true;
    s->scroll_speed = 30;

    return s;
}

void clue_scroll_to(ClueScroll *scroll, int x, int y)
{
    if (!scroll) return;
    scroll->scroll_x = x;
    scroll->scroll_y = y;
    clamp_scroll(scroll);
}
