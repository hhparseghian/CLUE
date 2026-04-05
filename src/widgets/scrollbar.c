#define CLUE_IMPL
#include "clue/scrollbar.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/event.h"
#include "clue/widget.h"

/* Compute scrollbar thumb geometry */
static bool thumb_geom(int area_y, int area_h, int content_h, int scroll_y,
                       int area_x, int area_w,
                       int *out_x, int *out_y, int *out_h)
{
    if (content_h <= area_h) return false;

    float ratio = (float)area_h / (float)content_h;
    int bar_h = (int)(ratio * area_h);
    if (bar_h < 20) bar_h = 20;

    int track = area_h - bar_h - 4;  /* 2px inset top and bottom */
    if (track < 0) track = 0;
    int max_scroll = content_h - area_h;
    float pos = (max_scroll > 0) ? (float)scroll_y / (float)max_scroll : 0.0f;

    *out_x = area_x + area_w - CLUE_SCROLLBAR_W - 4;
    *out_y = area_y + 2 + (int)(pos * track);
    *out_h = bar_h;
    return true;
}

static void clamp(int *scroll_y, int area_h, int content_h)
{
    int max_y = content_h - area_h;
    if (max_y < 0) max_y = 0;
    if (*scroll_y > max_y) *scroll_y = max_y;
    if (*scroll_y < 0) *scroll_y = 0;
}

void clue_scrollbar_draw(const ClueScrollbar *sb,
                         int area_x, int area_y, int area_w, int area_h,
                         int content_h, int scroll_y)
{
    int tx, ty, th;
    if (!thumb_geom(area_y, area_h, content_h, scroll_y,
                    area_x, area_w, &tx, &ty, &th))
        return;

    UIColor col = sb->dragging
        ? UI_RGBA(180, 185, 195, 220)
        : sb->hovered
            ? UI_RGBA(160, 165, 175, 180)
            : UI_RGBA(130, 135, 145, 150);
    clue_fill_rounded_rect(tx, ty, CLUE_SCROLLBAR_W, th,
                           CLUE_SCROLLBAR_W / 2.0f, col);
}

bool clue_scrollbar_handle_event(ClueScrollbar *sb,
                                 int area_x, int area_y, int area_w, int area_h,
                                 int content_h, int *scroll_y,
                                 void *event_ptr, void *widget_base)
{
    UIEvent *event = (UIEvent *)event_ptr;
    UIWidget *wb = (UIWidget *)widget_base;

    if (content_h <= area_h) {
        sb->dragging = false;
        return false;
    }

    int tx, ty, th;
    bool has_thumb = thumb_geom(area_y, area_h, content_h, *scroll_y,
                                area_x, area_w, &tx, &ty, &th);

    /* Track hover state + set cursor */
    if (event->type == UI_EVENT_MOUSE_MOVE && has_thumb && !sb->dragging) {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        sb->hovered = mx >= tx - 4 && mx <= tx + CLUE_SCROLLBAR_W + 4 &&
                      my >= area_y && my < area_y + area_h;
        if (sb->hovered && event->window)
            clue_window_set_cursor(event->window, UI_CURSOR_POINTER);
    }

    /* Drag in progress — track mouse move */
    if (sb->dragging && event->type == UI_EVENT_MOUSE_MOVE) {
        int my = event->mouse_move.y;
        int track = area_h - th;
        if (track > 0) {
            int thumb_top = my - sb->drag_offset - area_y;
            float frac = (float)thumb_top / (float)track;
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;
            *scroll_y = (int)(frac * (content_h - area_h));
            clamp(scroll_y, area_h, content_h);
        }
        return true;
    }

    /* Mouse button */
    if (event->type == UI_EVENT_MOUSE_BUTTON && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;

        if (event->mouse_button.pressed) {
            /* Start drag if clicking on thumb */
            if (has_thumb &&
                mx >= tx - 4 && mx <= tx + CLUE_SCROLLBAR_W + 4 &&
                my >= ty && my <= ty + th) {
                sb->dragging = true;
                sb->drag_offset = my - ty;
                clue_capture_mouse(wb);
                return true;
            }

            /* Click on track (above or below thumb) — jump scroll */
            if (has_thumb &&
                mx >= tx - 4 && mx <= tx + CLUE_SCROLLBAR_W + 4 &&
                my >= area_y && my < area_y + area_h) {
                int track = area_h - th;
                if (track > 0) {
                    int thumb_top = my - th / 2 - area_y;
                    float frac = (float)thumb_top / (float)track;
                    if (frac < 0.0f) frac = 0.0f;
                    if (frac > 1.0f) frac = 1.0f;
                    *scroll_y = (int)(frac * (content_h - area_h));
                    clamp(scroll_y, area_h, content_h);
                }
                /* Start drag from the new position */
                sb->dragging = true;
                sb->drag_offset = th / 2;
                clue_capture_mouse(wb);
                return true;
            }
        } else {
            /* Release drag */
            if (sb->dragging) {
                sb->dragging = false;
                clue_release_mouse();
                if (event->window)
                    clue_window_set_cursor(event->window, UI_CURSOR_DEFAULT);
                return true;
            }
        }
    }

    return false;
}
