#ifndef CLUE_BACKEND_H
#define CLUE_BACKEND_H

#include "event.h"

/* Forward declarations */
struct ClueWindow;

/* Backend interface -- all platform backends implement this vtable */
typedef struct {
    int   (*init)(void);
    void  (*shutdown)(void);
    int   (*create_window)(struct ClueWindow *win);
    void  (*destroy_window)(struct ClueWindow *win);
    void  (*set_window_type)(struct ClueWindow *win, int type);
    void  (*set_window_parent)(struct ClueWindow *win, struct ClueWindow *parent);
    void  (*set_window_position)(struct ClueWindow *win, int x, int y);
    void  (*make_current)(struct ClueWindow *win);
    void  (*swap_buffers)(struct ClueWindow *win);
    int   (*poll_events)(ClueEvent *events, int max);
    void  (*set_cursor)(struct ClueWindow *win, int cursor_shape);
    void  *native_display;
    void  *native_window;
} UIBackend;

/* Select the best available backend at runtime.
 * Checks WAYLAND_DISPLAY to prefer Wayland, falls back to DRM/KMS. */
UIBackend *clue_select_backend(void);

#endif /* CLUE_BACKEND_H */
