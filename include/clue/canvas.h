#ifndef CLUE_CANVAS_H
#define CLUE_CANVAS_H

#include "clue_widget.h"

/* Canvas draw callback -- called each frame to let user draw custom content.
 * x, y, w, h are the canvas bounds in window coordinates. */
typedef void (*ClueCanvasDrawCb)(int x, int y, int w, int h, void *user_data);

/* Canvas event callback -- receives mouse events with coordinates
 * relative to the canvas origin (0,0 = top-left of canvas). */
typedef void (*ClueCanvasEventCb)(int type, int x, int y, int button,
                                  void *user_data);

/* Event types passed to ClueCanvasEventCb */
#define CLUE_CANVAS_PRESS   1
#define CLUE_CANVAS_RELEASE 2
#define CLUE_CANVAS_MOTION  3

/* Canvas -- user-drawable area */
typedef struct {
    ClueWidget        base;       /* MUST be first */
    ClueCanvasDrawCb  draw_cb;
    void             *draw_data;
    ClueCanvasEventCb event_cb;
    void             *event_data;
    bool              painting;   /* mouse button held */
} ClueCanvas;

/* Create a new canvas */
ClueCanvas *clue_canvas_new(int w, int h);

/* Set the draw callback */
void clue_canvas_set_draw(ClueCanvas *c, ClueCanvasDrawCb cb, void *user_data);

/* Set the event callback for mouse interaction */
void clue_canvas_set_event(ClueCanvas *c, ClueCanvasEventCb cb, void *user_data);

#endif /* CLUE_CANVAS_H */
