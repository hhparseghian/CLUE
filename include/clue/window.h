#ifndef CLUE_WINDOW_H
#define CLUE_WINDOW_H

#include <stdbool.h>
#include "backend.h"

/* Cursor shapes */
typedef enum {
    CLUE_CURSOR_DEFAULT = 0,
    CLUE_CURSOR_POINTER,       /* hand, for clickable items */
    CLUE_CURSOR_TEXT,           /* I-beam, for text fields */
    CLUE_CURSOR_RESIZE_H,      /* horizontal resize (left-right) */
    CLUE_CURSOR_RESIZE_V,      /* vertical resize (up-down) */
    CLUE_CURSOR_MOVE,          /* move/drag */
    CLUE_CURSOR_CROSSHAIR,
} ClueCursorShape;

/* Window types */
typedef enum {
    CLUE_WINDOW_NORMAL = 0,
    CLUE_WINDOW_DIALOG,
    CLUE_WINDOW_POPUP,
} ClueWindowType;

/* A single application window */
typedef struct ClueWindow {
    int              x, y, w, h;
    const char      *title;
    bool             visible;
    bool             dirty;
    ClueWindowType     type;
    struct ClueWindow *parent;        /* non-NULL for dialogs and popups */
    void            *backend_data;  /* opaque per-backend state */
    void            *widget_root;   /* root UIWidget* for this window */
} ClueWindow;

/* Manages all open windows and owns the active backend */
typedef struct {
    ClueWindow   **windows;
    int          count;
    int          capacity;
    ClueWindow    *focused;
    UIBackend   *backend;
} ClueWindowManager;

/* Get the global window manager singleton */
ClueWindowManager *clue_wm_get(void);

/* Create a new window with the given dimensions and title.
 * Returns NULL on failure. */
ClueWindow *clue_window_create(int w, int h, const char *title);

/* Destroy a window and free its resources */
void clue_window_destroy(ClueWindow *win);

/* Set the window type (NORMAL, DIALOG, POPUP) */
void clue_window_set_type(ClueWindow *win, ClueWindowType type);

/* Set the parent window (required for DIALOG and POPUP types) */
void clue_window_set_parent(ClueWindow *win, ClueWindow *parent);

/* Set the window position on screen */
void clue_window_set_position(ClueWindow *win, int x, int y);

/* Show or hide a window */
void clue_window_set_visible(ClueWindow *win, bool visible);

/* Mark a window as needing a redraw */
void clue_window_mark_dirty(ClueWindow *win);

/* Make a window's GL context current for rendering */
void clue_window_make_current(ClueWindow *win);

/* Swap the front/back buffers for a window */
void clue_window_swap_buffers(ClueWindow *win);

/* Set the mouse cursor shape for a window */
void clue_window_set_cursor(ClueWindow *win, ClueCursorShape cursor);

#endif /* CLUE_WINDOW_H */
