#ifndef CLUE_SCROLLBAR_H
#define CLUE_SCROLLBAR_H

#include <stdbool.h>
#include "renderer.h"

/*
 * Shared scrollbar helper for all scrollable widgets.
 * Handles drawing, hit testing, and mouse drag.
 *
 * Usage:
 *   1. Embed ClueScrollbar in your widget struct
 *   2. Call clue_scrollbar_draw() in your draw function
 *   3. Call clue_scrollbar_handle_event() in your event handler
 *      BEFORE processing other mouse events — it returns true
 *      if the event was consumed by the scrollbar
 *   4. Read scroll_y after events to get the new scroll position
 */

#define CLUE_SCROLLBAR_W 8

typedef struct {
    bool  dragging;
    bool  hovered;         /* mouse is over the scrollbar area */
    int   drag_offset;     /* mouse offset from thumb top */
} ClueScrollbar;

/* Draw a vertical scrollbar.
 * area_x/y/w/h: the scrollable widget bounds.
 * content_h: total content height.
 * scroll_y: current scroll offset.
 */
void clue_scrollbar_draw(const ClueScrollbar *sb,
                         int area_x, int area_y, int area_w, int area_h,
                         int content_h, int scroll_y);

/* Handle a mouse event for the scrollbar.
 * Returns true if the event was consumed (scrollbar click/drag).
 * Updates *scroll_y in place if dragging.
 * widget_base: the UIWidget* for capture/release calls.
 */
bool clue_scrollbar_handle_event(ClueScrollbar *sb,
                                 int area_x, int area_y, int area_w, int area_h,
                                 int content_h, int *scroll_y,
                                 void *event, void *widget_base);

#endif /* CLUE_SCROLLBAR_H */
