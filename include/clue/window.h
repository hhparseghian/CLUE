#ifndef CLUE_WINDOW_H
#define CLUE_WINDOW_H

#include <stdbool.h>
#include "backend.h"

/* Window types */
typedef enum {
    UI_WINDOW_NORMAL = 0,
    UI_WINDOW_DIALOG,
    UI_WINDOW_POPUP,
} UIWindowType;

/* A single application window */
typedef struct UIWindow {
    int              x, y, w, h;
    const char      *title;
    bool             visible;
    bool             dirty;
    UIWindowType     type;
    struct UIWindow *parent;        /* non-NULL for dialogs and popups */
    void            *backend_data;  /* opaque per-backend state */
    void            *widget_root;   /* root UIWidget* for this window */
} UIWindow;

/* Manages all open windows and owns the active backend */
typedef struct {
    UIWindow   **windows;
    int          count;
    int          capacity;
    UIWindow    *focused;
    UIBackend   *backend;
} UIWindowManager;

/* Get the global window manager singleton */
UIWindowManager *clue_wm_get(void);

/* Create a new window with the given dimensions and title.
 * Returns NULL on failure. */
UIWindow *clue_window_create(int w, int h, const char *title);

/* Destroy a window and free its resources */
void clue_window_destroy(UIWindow *win);

/* Set the window type (NORMAL, DIALOG, POPUP) */
void clue_window_set_type(UIWindow *win, UIWindowType type);

/* Set the parent window (required for DIALOG and POPUP types) */
void clue_window_set_parent(UIWindow *win, UIWindow *parent);

/* Set the window position on screen */
void clue_window_set_position(UIWindow *win, int x, int y);

/* Show or hide a window */
void clue_window_set_visible(UIWindow *win, bool visible);

/* Mark a window as needing a redraw */
void clue_window_mark_dirty(UIWindow *win);

/* Make a window's GL context current for rendering */
void clue_window_make_current(UIWindow *win);

/* Swap the front/back buffers for a window */
void clue_window_swap_buffers(UIWindow *win);

#endif /* CLUE_WINDOW_H */
