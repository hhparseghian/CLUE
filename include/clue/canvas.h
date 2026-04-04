#ifndef CLUE_CANVAS_H
#define CLUE_CANVAS_H

#include "clue_widget.h"

/* Canvas draw callback -- called each frame to let user draw custom content.
 * x, y, w, h are the canvas bounds in window coordinates. */
typedef void (*ClueCanvasDrawCb)(int x, int y, int w, int h, void *user_data);

/* Canvas -- user-drawable area */
typedef struct {
    ClueWidget       base;       /* MUST be first */
    ClueCanvasDrawCb draw_cb;
    void            *draw_data;
} ClueCanvas;

/* Create a new canvas */
ClueCanvas *clue_canvas_new(int w, int h);

/* Set the draw callback */
void clue_canvas_set_draw(ClueCanvas *c, ClueCanvasDrawCb cb, void *user_data);

/* Signals: "clicked" -- emitted on mouse click (mouse_button event) */

#endif /* CLUE_CANVAS_H */
