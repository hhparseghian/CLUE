#ifndef CLUE_BACKEND_H
#define CLUE_BACKEND_H

#include "event.h"

/* Forward declarations */
struct UIWindow;

/* Backend interface -- all platform backends implement this vtable */
typedef struct {
    int   (*init)(void);
    void  (*shutdown)(void);
    int   (*create_window)(struct UIWindow *win);
    void  (*destroy_window)(struct UIWindow *win);
    void  (*set_window_type)(struct UIWindow *win, int type);
    void  (*set_window_parent)(struct UIWindow *win, struct UIWindow *parent);
    void  (*set_window_position)(struct UIWindow *win, int x, int y);
    void  (*make_current)(struct UIWindow *win);
    void  (*swap_buffers)(struct UIWindow *win);
    int   (*poll_events)(UIEvent *events, int max);
    void  *native_display;
    void  *native_window;
} UIBackend;

/* Select the best available backend at runtime.
 * Checks WAYLAND_DISPLAY to prefer Wayland, falls back to DRM/KMS. */
UIBackend *clue_select_backend(void);

#endif /* CLUE_BACKEND_H */
