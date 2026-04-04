#ifndef CLUE_CANVAS_H
#define CLUE_CANVAS_H

#include "clue_widget.h"

/* Canvas draw callback -- called each frame to let user draw custom content.
 * x, y, w, h are the canvas bounds in window coordinates.
 * Safe to make raw OpenGL ES 2 calls -- GL state is saved/restored. */
typedef void (*ClueCanvasDrawCb)(int x, int y, int w, int h, void *user_data);

/* Canvas event types */
#define CLUE_CANVAS_PRESS    1
#define CLUE_CANVAS_RELEASE  2
#define CLUE_CANVAS_MOTION   3
#define CLUE_CANVAS_SCROLL   4
#define CLUE_CANVAS_KEY      5

/* Canvas event data */
typedef struct {
    int   type;         /* CLUE_CANVAS_PRESS/RELEASE/MOTION/SCROLL/KEY */
    int   x, y;        /* mouse position relative to canvas origin */
    int   dx, dy;       /* mouse delta since last motion event */
    int   button;       /* mouse button (0=left, 1=middle, 2=right) */
    float scroll_x;     /* scroll delta (CLUE_CANVAS_SCROLL) */
    float scroll_y;
    int   keycode;      /* xkb keysym (CLUE_CANVAS_KEY) */
    int   modifiers;    /* UI_MOD_* flags */
    int   pressed;      /* key pressed (1) or released (0) */
} ClueCanvasEvent;

/* Canvas event callback */
typedef void (*ClueCanvasEventCb)(const ClueCanvasEvent *event, void *user_data);

/* Canvas -- user-drawable area with optional GL state protection */
typedef struct {
    ClueWidget        base;       /* MUST be first */
    ClueCanvasDrawCb  draw_cb;
    void             *draw_data;
    ClueCanvasEventCb event_cb;
    void             *event_data;
    bool              painting;   /* mouse button held */
    int               last_mx;    /* for delta tracking */
    int               last_my;
    bool              save_gl;    /* save/restore GL state around draw (default true) */
    bool              focusable_canvas; /* receive keyboard events (default false) */
} ClueCanvas;

/* Create a new canvas */
ClueCanvas *clue_canvas_new(int w, int h);

/* Set the draw callback */
void clue_canvas_set_draw(ClueCanvas *c, ClueCanvasDrawCb cb, void *user_data);

/* Set the event callback */
void clue_canvas_set_event(ClueCanvas *c, ClueCanvasEventCb cb, void *user_data);

/* Enable keyboard events on the canvas (makes it focusable, click to focus) */
void clue_canvas_set_focusable(ClueCanvas *c, bool focusable);

/* Enable/disable GL state save/restore around draw callback (default: true) */
void clue_canvas_set_save_gl(ClueCanvas *c, bool save);

#endif /* CLUE_CANVAS_H */
