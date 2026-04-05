/*
 * CLUE -- Wayland backend
 *
 * Implements the UIBackend interface for Wayland compositors using
 * libwayland-client, xdg-shell, EGL, and xkbcommon.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include "xdg-shell-client-protocol.h"

#include "wayland.h"
#include "clue/window.h"

/* ------------------------------------------------------------------ */
/* Internal types                                                      */
/* ------------------------------------------------------------------ */

#define EVENT_QUEUE_SIZE 64

/* Per-window Wayland state, stored in win->backend_data */
typedef struct {
    struct wl_surface      *wl_surface;
    struct wl_egl_window   *egl_window;
    struct xdg_surface     *xdg_surface;
    struct xdg_toplevel    *xdg_toplevel;   /* NORMAL / DIALOG */
    struct xdg_popup       *xdg_popup;      /* POPUP */
    EGLSurface              egl_surface;
    bool                    configured;
    bool                    close_requested;
} WaylandWindowData;

/* Global Wayland connection state (file-scope singleton) */
typedef struct {
    struct wl_display      *display;
    struct wl_registry     *registry;
    struct wl_compositor   *compositor;
    struct wl_seat         *seat;
    struct wl_shm          *shm;
    struct wl_pointer      *pointer;
    struct wl_keyboard     *keyboard;
    struct xdg_wm_base     *xdg_wm_base;

    /* EGL */
    EGLDisplay              egl_display;
    EGLContext              egl_context;
    EGLConfig               egl_config;

    /* xkbcommon keyboard state */
    struct xkb_context     *xkb_ctx;
    struct xkb_keymap      *xkb_keymap;
    struct xkb_state       *xkb_state;

    /* Input tracking */
    ClueWindow               *pointer_window;
    int                     pointer_x;
    int                     pointer_y;

    /* Cursor */
    struct wl_cursor_theme  *cursor_theme;
    struct wl_surface       *cursor_surface;
    uint32_t                 pointer_serial;

    /* Circular event queue: written by listeners, drained by poll_events */
    ClueEvent                 event_queue[EVENT_QUEUE_SIZE];
    int                     event_head;
    int                     event_tail;
} WaylandState;

static WaylandState wl = {0};

/* ------------------------------------------------------------------ */
/* Event queue helpers                                                 */
/* ------------------------------------------------------------------ */

/* Push an event into the circular queue. Drops silently if full. */
static void push_event(const ClueEvent *ev)
{
    int next = (wl.event_head + 1) % EVENT_QUEUE_SIZE;
    if (next == wl.event_tail) {
        return; /* queue full -- drop */
    }
    wl.event_queue[wl.event_head] = *ev;
    wl.event_head = next;
}

/* Pop one event. Returns false if the queue is empty. */
static bool pop_event(ClueEvent *out)
{
    if (wl.event_tail == wl.event_head) return false;
    *out = wl.event_queue[wl.event_tail];
    wl.event_tail = (wl.event_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

/* ------------------------------------------------------------------ */
/* Window lookup helper                                                */
/* ------------------------------------------------------------------ */

/* Find which ClueWindow owns a given wl_surface */
static ClueWindow *find_window_by_surface(struct wl_surface *surface)
{
    ClueWindowManager *wm = clue_wm_get();
    for (int i = 0; i < wm->count; i++) {
        WaylandWindowData *d = wm->windows[i]->backend_data;
        if (d && d->wl_surface == surface) {
            return wm->windows[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* xdg_wm_base listener (ping/pong keep-alive)                        */
/* ------------------------------------------------------------------ */

static void xdg_wm_base_handle_ping(void *data,
                                     struct xdg_wm_base *shell,
                                     uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_handle_ping,
};

/* ------------------------------------------------------------------ */
/* wl_seat listener                                                    */
/* ------------------------------------------------------------------ */

static void seat_handle_capabilities(void *data, struct wl_seat *seat,
                                     uint32_t caps);
static void seat_handle_name(void *data, struct wl_seat *seat,
                             const char *name);

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name         = seat_handle_name,
};

/* ------------------------------------------------------------------ */
/* Pointer listener                                                    */
/* ------------------------------------------------------------------ */

static void pointer_handle_enter(void *data, struct wl_pointer *pointer,
                                 uint32_t serial, struct wl_surface *surface,
                                 wl_fixed_t sx, wl_fixed_t sy)
{
    (void)data; (void)pointer;
    wl.pointer_serial = serial;
    wl.pointer_window = find_window_by_surface(surface);
    wl.pointer_x = wl_fixed_to_int(sx);
    wl.pointer_y = wl_fixed_to_int(sy);
}

static void pointer_handle_leave(void *data, struct wl_pointer *pointer,
                                 uint32_t serial, struct wl_surface *surface)
{
    (void)data; (void)pointer; (void)serial; (void)surface;
    wl.pointer_window = NULL;
}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer,
                                  uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    (void)data; (void)pointer; (void)time;
    wl.pointer_x = wl_fixed_to_int(sx);
    wl.pointer_y = wl_fixed_to_int(sy);

    if (wl.pointer_window) {
        ClueEvent ev = {0};
        ev.type = CLUE_EVENT_MOUSE_MOVE;
        ev.window = wl.pointer_window;
        ev.mouse_move.x = wl.pointer_x;
        ev.mouse_move.y = wl.pointer_y;
        push_event(&ev);
    }
}

static void pointer_handle_button(void *data, struct wl_pointer *pointer,
                                  uint32_t serial, uint32_t time,
                                  uint32_t button, uint32_t state)
{
    (void)data; (void)pointer; (void)serial; (void)time;
    if (!wl.pointer_window) return;

    int btn;
    switch (button) {
    case BTN_LEFT:   btn = 0; break;
    case BTN_RIGHT:  btn = 1; break;
    case BTN_MIDDLE: btn = 2; break;
    default:         btn = (int)button; break;
    }

    ClueEvent ev = {0};
    ev.type = CLUE_EVENT_MOUSE_BUTTON;
    ev.window = wl.pointer_window;
    ev.mouse_button.x = wl.pointer_x;
    ev.mouse_button.y = wl.pointer_y;
    ev.mouse_button.btn = btn;
    ev.mouse_button.pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    push_event(&ev);
}

static void pointer_handle_axis(void *data, struct wl_pointer *pointer,
                                uint32_t time, uint32_t axis,
                                wl_fixed_t value)
{
    (void)data; (void)pointer; (void)time; (void)axis; (void)value;
    /* TODO: Scroll / axis events -- extend ClueEventType if needed */
}

static void pointer_handle_frame(void *data, struct wl_pointer *pointer)
{
    (void)data; (void)pointer;
}

static void pointer_handle_axis_source(void *data,
                                       struct wl_pointer *pointer,
                                       uint32_t axis_source)
{
    (void)data; (void)pointer; (void)axis_source;
}

static void pointer_handle_axis_stop(void *data,
                                     struct wl_pointer *pointer,
                                     uint32_t time, uint32_t axis)
{
    (void)data; (void)pointer; (void)time; (void)axis;
}

static void pointer_handle_axis_discrete(void *data,
                                         struct wl_pointer *pointer,
                                         uint32_t axis, int32_t discrete)
{
    (void)data; (void)pointer; (void)axis; (void)discrete;
}

static const struct wl_pointer_listener pointer_listener = {
    .enter         = pointer_handle_enter,
    .leave         = pointer_handle_leave,
    .motion        = pointer_handle_motion,
    .button        = pointer_handle_button,
    .axis          = pointer_handle_axis,
    .frame         = pointer_handle_frame,
    .axis_source   = pointer_handle_axis_source,
    .axis_stop     = pointer_handle_axis_stop,
    .axis_discrete = pointer_handle_axis_discrete,
};

/* ------------------------------------------------------------------ */
/* Keyboard listener                                                   */
/* ------------------------------------------------------------------ */

static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                                   uint32_t format, int32_t fd,
                                   uint32_t size)
{
    (void)data; (void)keyboard;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char *map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }

    /* Tear down previous keymap if any */
    if (wl.xkb_state)  { xkb_state_unref(wl.xkb_state);   wl.xkb_state  = NULL; }
    if (wl.xkb_keymap) { xkb_keymap_unref(wl.xkb_keymap); wl.xkb_keymap = NULL; }

    wl.xkb_keymap = xkb_keymap_new_from_string(wl.xkb_ctx, map_str,
                                                 XKB_KEYMAP_FORMAT_TEXT_V1,
                                                 XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_str, size);
    close(fd);

    if (!wl.xkb_keymap) {
        fprintf(stderr, "clue: failed to compile xkb keymap\n");
        return;
    }

    wl.xkb_state = xkb_state_new(wl.xkb_keymap);
    if (!wl.xkb_state) {
        fprintf(stderr, "clue: failed to create xkb state\n");
    }
}

static void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                                  uint32_t serial, struct wl_surface *surface,
                                  struct wl_array *keys)
{
    (void)data; (void)keyboard; (void)serial; (void)keys;
    /* Update the window manager focused window */
    ClueWindow *win = find_window_by_surface(surface);
    if (win) {
        ClueWindowManager *wm = clue_wm_get();
        wm->focused = win;
    }
}

static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                                  uint32_t serial, struct wl_surface *surface)
{
    (void)data; (void)keyboard; (void)serial; (void)surface;
}

static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                                uint32_t serial, uint32_t time,
                                uint32_t key, uint32_t state)
{
    (void)data; (void)keyboard; (void)serial; (void)time;

    ClueWindowManager *wm = clue_wm_get();
    if (!wm->focused) return;

    /* Wayland keycodes are evdev codes; xkbcommon expects evdev + 8 */
    xkb_keycode_t xkb_key = key + 8;

    ClueEvent ev = {0};
    ev.type = CLUE_EVENT_KEY;
    ev.window = wm->focused;
    ev.key.keycode = (int)xkb_state_key_get_one_sym(wl.xkb_state, xkb_key);
    ev.key.pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
    ev.key.modifiers = 0;
    if (xkb_state_mod_name_is_active(wl.xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
        ev.key.modifiers |= CLUE_MOD_SHIFT;
    if (xkb_state_mod_name_is_active(wl.xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
        ev.key.modifiers |= CLUE_MOD_CTRL;
    if (xkb_state_mod_name_is_active(wl.xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE))
        ev.key.modifiers |= CLUE_MOD_ALT;
    if (xkb_state_mod_name_is_active(wl.xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE))
        ev.key.modifiers |= CLUE_MOD_SUPER;

    /* Get UTF-8 text for printable keys (skip for Ctrl combos) */
    if (wl.xkb_state && ev.key.pressed && !(ev.key.modifiers & CLUE_MOD_CTRL)) {
        xkb_state_key_get_utf8(wl.xkb_state, xkb_key,
                               ev.key.text, sizeof(ev.key.text));
    }

    push_event(&ev);
}

static void keyboard_handle_modifiers(void *data,
                                      struct wl_keyboard *keyboard,
                                      uint32_t serial,
                                      uint32_t mods_depressed,
                                      uint32_t mods_latched,
                                      uint32_t mods_locked,
                                      uint32_t group)
{
    (void)data; (void)keyboard; (void)serial;
    if (wl.xkb_state) {
        xkb_state_update_mask(wl.xkb_state,
                              mods_depressed, mods_latched, mods_locked,
                              0, 0, group);
    }
}

static void keyboard_handle_repeat_info(void *data,
                                        struct wl_keyboard *keyboard,
                                        int32_t rate, int32_t delay)
{
    (void)data; (void)keyboard; (void)rate; (void)delay;
    /* TODO: Implement key repeat using rate/delay if desired */
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap      = keyboard_handle_keymap,
    .enter       = keyboard_handle_enter,
    .leave       = keyboard_handle_leave,
    .key         = keyboard_handle_key,
    .modifiers   = keyboard_handle_modifiers,
    .repeat_info = keyboard_handle_repeat_info,
};

/* ------------------------------------------------------------------ */
/* Seat capabilities                                                   */
/* ------------------------------------------------------------------ */

static void seat_handle_capabilities(void *data, struct wl_seat *seat,
                                     uint32_t caps)
{
    (void)data;

    /* Pointer */
    bool have_pointer = caps & WL_SEAT_CAPABILITY_POINTER;
    if (have_pointer && !wl.pointer) {
        wl.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(wl.pointer, &pointer_listener, NULL);
    } else if (!have_pointer && wl.pointer) {
        wl_pointer_destroy(wl.pointer);
        wl.pointer = NULL;
    }

    /* Keyboard */
    bool have_keyboard = caps & WL_SEAT_CAPABILITY_KEYBOARD;
    if (have_keyboard && !wl.keyboard) {
        wl.keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(wl.keyboard, &keyboard_listener, NULL);
    } else if (!have_keyboard && wl.keyboard) {
        wl_keyboard_destroy(wl.keyboard);
        wl.keyboard = NULL;
    }
}

static void seat_handle_name(void *data, struct wl_seat *seat,
                             const char *name)
{
    (void)data; (void)seat; (void)name;
}

/* ------------------------------------------------------------------ */
/* xdg_toplevel listener                                               */
/* ------------------------------------------------------------------ */

static void xdg_toplevel_handle_configure(void *data,
                                          struct xdg_toplevel *toplevel,
                                          int32_t width, int32_t height,
                                          struct wl_array *states)
{
    (void)toplevel; (void)states;
    ClueWindow *win = data;
    if (!win) return;

    /* A zero size means the compositor has no preference */
    if (width <= 0 || height <= 0) return;

    if (width != win->w || height != win->h) {
        WaylandWindowData *wd = win->backend_data;

        win->w = width;
        win->h = height;
        win->dirty = true;

        if (wd && wd->egl_window) {
            wl_egl_window_resize(wd->egl_window, width, height, 0, 0);
        }

        ClueEvent ev = {0};
        ev.type = CLUE_EVENT_RESIZE;
        ev.window = win;
        ev.resize.w = width;
        ev.resize.h = height;
        push_event(&ev);
    }
}

static void xdg_toplevel_handle_close(void *data,
                                      struct xdg_toplevel *toplevel)
{
    (void)toplevel;
    ClueWindow *win = data;
    if (!win) return;

    WaylandWindowData *wd = win->backend_data;
    if (wd) wd->close_requested = true;

    ClueEvent ev = {0};
    ev.type = CLUE_EVENT_CLOSE;
    ev.window = win;
    push_event(&ev);
}

static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = xdg_toplevel_handle_configure,
    .close     = xdg_toplevel_handle_close,
};

/* ------------------------------------------------------------------ */
/* xdg_popup listener                                                  */
/* ------------------------------------------------------------------ */

static void xdg_popup_handle_configure(void *data,
                                       struct xdg_popup *popup,
                                       int32_t x, int32_t y,
                                       int32_t width, int32_t height)
{
    (void)popup;
    ClueWindow *win = data;
    if (!win) return;

    if (width > 0 && height > 0 && (width != win->w || height != win->h)) {
        WaylandWindowData *wd = win->backend_data;
        win->w = width;
        win->h = height;
        win->x = x;
        win->y = y;
        win->dirty = true;

        if (wd && wd->egl_window) {
            wl_egl_window_resize(wd->egl_window, width, height, 0, 0);
        }

        ClueEvent ev = {0};
        ev.type = CLUE_EVENT_RESIZE;
        ev.window = win;
        ev.resize.w = width;
        ev.resize.h = height;
        push_event(&ev);
    }
}

static void xdg_popup_handle_done(void *data, struct xdg_popup *popup)
{
    (void)popup;
    ClueWindow *win = data;
    if (!win) return;

    WaylandWindowData *wd = win->backend_data;
    if (wd) wd->close_requested = true;

    ClueEvent ev = {0};
    ev.type = CLUE_EVENT_CLOSE;
    ev.window = win;
    push_event(&ev);
}

static const struct xdg_popup_listener popup_listener = {
    .configure = xdg_popup_handle_configure,
    .popup_done = xdg_popup_handle_done,
};

/* ------------------------------------------------------------------ */
/* Per-window xdg_surface listener (passes ClueWindow* as user data)     */
/* ------------------------------------------------------------------ */

static void win_xdg_surface_handle_configure(void *data,
                                             struct xdg_surface *xdg_surface,
                                             uint32_t serial)
{
    ClueWindow *win = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    if (win) {
        WaylandWindowData *wd = win->backend_data;
        if (wd) wd->configured = true;
    }
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = win_xdg_surface_handle_configure,
};

/* ------------------------------------------------------------------ */
/* Registry listener                                                   */
/* ------------------------------------------------------------------ */

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version)
{
    (void)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        wl.compositor = wl_registry_bind(registry, name,
                                         &wl_compositor_interface,
                                         version < 4 ? version : 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        wl.xdg_wm_base = wl_registry_bind(registry, name,
                                           &xdg_wm_base_interface,
                                           version < 2 ? version : 2);
        xdg_wm_base_add_listener(wl.xdg_wm_base, &xdg_wm_base_listener, NULL);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        wl.shm = wl_registry_bind(registry, name,
                                   &wl_shm_interface,
                                   version < 1 ? version : 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        wl.seat = wl_registry_bind(registry, name,
                                   &wl_seat_interface,
                                   version < 5 ? version : 5);
        wl_seat_add_listener(wl.seat, &seat_listener, NULL);
    }
}

static void registry_handle_global_remove(void *data,
                                          struct wl_registry *registry,
                                          uint32_t name)
{
    (void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global        = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

/* ------------------------------------------------------------------ */
/* EGL initialisation                                                  */
/* ------------------------------------------------------------------ */

static int init_egl(void)
{
    wl.egl_display = eglGetDisplay((EGLNativeDisplayType)wl.display);
    if (wl.egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "clue: eglGetDisplay failed\n");
        return -1;
    }

    EGLint major, minor;
    if (!eglInitialize(wl.egl_display, &major, &minor)) {
        fprintf(stderr, "clue: eglInitialize failed\n");
        return -1;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        fprintf(stderr, "clue: eglBindAPI failed\n");
        return -1;
    }

    EGLint attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_SAMPLE_BUFFERS,  1,
        EGL_SAMPLES,         4,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(wl.egl_display, attribs, &wl.egl_config, 1,
                         &num_configs) || num_configs == 0) {
        fprintf(stderr, "clue: eglChooseConfig failed\n");
        return -1;
    }

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    wl.egl_context = eglCreateContext(wl.egl_display, wl.egl_config,
                                      EGL_NO_CONTEXT, ctx_attribs);
    if (wl.egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "clue: eglCreateContext failed\n");
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Backend interface                                                   */
/* ------------------------------------------------------------------ */

static int wayland_init(void)
{
    memset(&wl, 0, sizeof(wl));

    /* --- Wayland connection --- */
    wl.display = wl_display_connect(NULL);
    if (!wl.display) {
        fprintf(stderr, "clue: wl_display_connect failed\n");
        return -1;
    }

    wl.registry = wl_display_get_registry(wl.display);
    wl_registry_add_listener(wl.registry, &registry_listener, NULL);

    /* First roundtrip: binds compositor, xdg_wm_base, seat */
    wl_display_roundtrip(wl.display);

    if (!wl.compositor) {
        fprintf(stderr, "clue: compositor not found\n");
        return -1;
    }
    if (!wl.xdg_wm_base) {
        fprintf(stderr, "clue: xdg_wm_base not found\n");
        return -1;
    }

    /* Second roundtrip: processes seat capabilities, gets pointer/keyboard */
    wl_display_roundtrip(wl.display);

    /* Cursor theme */
    if (wl.shm) {
        wl.cursor_theme = wl_cursor_theme_load(NULL, 24, wl.shm);
        if (wl.cursor_theme) {
            wl.cursor_surface = wl_compositor_create_surface(wl.compositor);
        }
    }

    /* --- EGL --- */
    if (init_egl() != 0) {
        return -1;
    }

    /* --- xkbcommon --- */
    wl.xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!wl.xkb_ctx) {
        fprintf(stderr, "clue: xkb_context_new failed\n");
        return -1;
    }

    return 0;
}

static void wayland_shutdown(void)
{
    /* --- xkbcommon cleanup --- */
    if (wl.xkb_state)  xkb_state_unref(wl.xkb_state);
    if (wl.xkb_keymap) xkb_keymap_unref(wl.xkb_keymap);
    if (wl.xkb_ctx)    xkb_context_unref(wl.xkb_ctx);

    /* --- EGL cleanup --- */
    if (wl.egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(wl.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                        EGL_NO_CONTEXT);
        if (wl.egl_context != EGL_NO_CONTEXT) {
            eglDestroyContext(wl.egl_display, wl.egl_context);
        }
        eglTerminate(wl.egl_display);
    }

    /* --- Cursor cleanup --- */
    if (wl.cursor_surface) wl_surface_destroy(wl.cursor_surface);
    if (wl.cursor_theme)   wl_cursor_theme_destroy(wl.cursor_theme);

    /* --- Wayland cleanup --- */
    if (wl.shm)         wl_shm_destroy(wl.shm);
    if (wl.pointer)     wl_pointer_destroy(wl.pointer);
    if (wl.keyboard)    wl_keyboard_destroy(wl.keyboard);
    if (wl.seat)        wl_seat_destroy(wl.seat);
    if (wl.xdg_wm_base) xdg_wm_base_destroy(wl.xdg_wm_base);
    if (wl.compositor)  wl_compositor_destroy(wl.compositor);
    if (wl.registry)    wl_registry_destroy(wl.registry);
    if (wl.display)     wl_display_disconnect(wl.display);

    memset(&wl, 0, sizeof(wl));
}

static int wayland_create_window(struct ClueWindow *win)
{
    if (!win) return -1;

    WaylandWindowData *wd = calloc(1, sizeof(WaylandWindowData));
    if (!wd) return -1;

    /* --- wl_surface --- */
    wd->wl_surface = wl_compositor_create_surface(wl.compositor);
    if (!wd->wl_surface) {
        fprintf(stderr, "clue: wl_compositor_create_surface failed\n");
        free(wd);
        return -1;
    }

    /* --- xdg_surface --- */
    wd->xdg_surface = xdg_wm_base_get_xdg_surface(wl.xdg_wm_base,
                                                    wd->wl_surface);
    if (!wd->xdg_surface) {
        fprintf(stderr, "clue: xdg_wm_base_get_xdg_surface failed\n");
        wl_surface_destroy(wd->wl_surface);
        free(wd);
        return -1;
    }
    xdg_surface_add_listener(wd->xdg_surface, &xdg_surface_listener, win);

    /* Store backend_data early so listeners can find this window */
    win->backend_data = wd;

    /* --- Role: toplevel or popup --- */
    if (win->type == CLUE_WINDOW_POPUP && win->parent) {
        WaylandWindowData *parent_wd = win->parent->backend_data;

        struct xdg_positioner *positioner =
            xdg_wm_base_create_positioner(wl.xdg_wm_base);
        xdg_positioner_set_size(positioner, win->w, win->h);
        xdg_positioner_set_anchor_rect(positioner,
                                       win->x, win->y, 1, 1);

        wd->xdg_popup = xdg_surface_get_popup(wd->xdg_surface,
                                               parent_wd->xdg_surface,
                                               positioner);
        xdg_positioner_destroy(positioner);

        if (!wd->xdg_popup) {
            fprintf(stderr, "clue: xdg_surface_get_popup failed\n");
            xdg_surface_destroy(wd->xdg_surface);
            wl_surface_destroy(wd->wl_surface);
            free(wd);
            win->backend_data = NULL;
            return -1;
        }
        xdg_popup_add_listener(wd->xdg_popup, &popup_listener, win);
    } else {
        /* NORMAL or DIALOG */
        wd->xdg_toplevel = xdg_surface_get_toplevel(wd->xdg_surface);
        if (!wd->xdg_toplevel) {
            fprintf(stderr, "clue: xdg_surface_get_toplevel failed\n");
            xdg_surface_destroy(wd->xdg_surface);
            wl_surface_destroy(wd->wl_surface);
            free(wd);
            win->backend_data = NULL;
            return -1;
        }
        xdg_toplevel_add_listener(wd->xdg_toplevel, &toplevel_listener, win);

        if (win->title) {
            xdg_toplevel_set_title(wd->xdg_toplevel, win->title);
        }

        /* Set parent for dialog windows */
        if (win->type == CLUE_WINDOW_DIALOG && win->parent) {
            WaylandWindowData *parent_wd = win->parent->backend_data;
            if (parent_wd && parent_wd->xdg_toplevel) {
                xdg_toplevel_set_parent(wd->xdg_toplevel,
                                        parent_wd->xdg_toplevel);
            }
        }
    }

    /* --- EGL window surface --- */
    wd->egl_window = wl_egl_window_create(wd->wl_surface, win->w, win->h);
    if (!wd->egl_window) {
        fprintf(stderr, "clue: wl_egl_window_create failed\n");
        goto fail_role;
    }

    wd->egl_surface = eglCreateWindowSurface(wl.egl_display, wl.egl_config,
                                             (EGLNativeWindowType)wd->egl_window,
                                             NULL);
    if (wd->egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "clue: eglCreateWindowSurface failed\n");
        wl_egl_window_destroy(wd->egl_window);
        goto fail_role;
    }

    /* Commit and roundtrip to receive the initial configure event */
    wl_surface_commit(wd->wl_surface);
    wl_display_roundtrip(wl.display);

    return 0;

fail_role:
    if (wd->xdg_toplevel) xdg_toplevel_destroy(wd->xdg_toplevel);
    if (wd->xdg_popup)    xdg_popup_destroy(wd->xdg_popup);
    xdg_surface_destroy(wd->xdg_surface);
    wl_surface_destroy(wd->wl_surface);
    free(wd);
    win->backend_data = NULL;
    return -1;
}

static void wayland_destroy_window(struct ClueWindow *win)
{
    if (!win || !win->backend_data) return;
    WaylandWindowData *wd = win->backend_data;

    if (wd->egl_surface != EGL_NO_SURFACE) {
        eglDestroySurface(wl.egl_display, wd->egl_surface);
    }
    if (wd->egl_window) {
        wl_egl_window_destroy(wd->egl_window);
    }
    if (wd->xdg_toplevel) {
        xdg_toplevel_destroy(wd->xdg_toplevel);
    }
    if (wd->xdg_popup) {
        xdg_popup_destroy(wd->xdg_popup);
    }
    if (wd->xdg_surface) {
        xdg_surface_destroy(wd->xdg_surface);
    }
    if (wd->wl_surface) {
        wl_surface_destroy(wd->wl_surface);
    }

    free(wd);
    win->backend_data = NULL;
}

static void wayland_make_current(struct ClueWindow *win)
{
    if (!win || !win->backend_data) return;
    WaylandWindowData *wd = win->backend_data;
    eglMakeCurrent(wl.egl_display, wd->egl_surface,
                   wd->egl_surface, wl.egl_context);
}

static void wayland_swap_buffers(struct ClueWindow *win)
{
    if (!win || !win->backend_data) return;
    WaylandWindowData *wd = win->backend_data;

    eglMakeCurrent(wl.egl_display, wd->egl_surface,
                   wd->egl_surface, wl.egl_context);
    eglSwapBuffers(wl.egl_display, wd->egl_surface);
}

static int wayland_poll_events(ClueEvent *events, int max)
{
    if (!events || max <= 0) return 0;

    /* Dispatch any events the compositor has already sent us.
     * dispatch_pending is non-blocking -- it only processes data
     * already read into the client buffer. */
    wl_display_dispatch_pending(wl.display);
    wl_display_flush(wl.display);

    /* Drain the internal event queue into the caller's array */
    int count = 0;
    while (count < max && pop_event(&events[count])) {
        count++;
    }

    return count;
}

/* ------------------------------------------------------------------ */
/* Exported backend instance                                           */
/* ------------------------------------------------------------------ */

/*
 * TODO: Clipboard / data_device (wl_data_device_manager) -- not yet implemented
 * TODO: Drag and drop -- not yet implemented
 * TODO: IME / input method (zwp_text_input_v3) -- not yet implemented
 * TODO: HiDPI / fractional scaling -- implement via wp_fractional_scale_v1
 *       and wp_viewporter when needed
 */

static void wayland_set_cursor(ClueWindow *win, int shape)
{
    (void)win;
    if (!wl.cursor_theme || !wl.cursor_surface || !wl.pointer) return;

    const char *name;
    switch (shape) {
    case 1:  name = "pointer";       break; /* CLUE_CURSOR_POINTER */
    case 2:  name = "text";          break; /* CLUE_CURSOR_TEXT */
    case 3:  name = "col-resize";    break; /* CLUE_CURSOR_RESIZE_H */
    case 4:  name = "row-resize";    break; /* CLUE_CURSOR_RESIZE_V */
    case 5:  name = "move";          break; /* CLUE_CURSOR_MOVE */
    case 6:  name = "crosshair";     break; /* CLUE_CURSOR_CROSSHAIR */
    default: name = "default";       break;
    }

    struct wl_cursor *cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, name);
    if (!cursor && shape != 0) {
        /* Fallback names */
        switch (shape) {
        case 3: cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "sb_h_double_arrow"); break;
        case 4: cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "sb_v_double_arrow"); break;
        case 5: cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "grabbing"); break;
        default: cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "left_ptr"); break;
        }
    }
    if (!cursor) cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "left_ptr");
    if (!cursor) return;

    struct wl_cursor_image *img = cursor->images[0];
    struct wl_buffer *buf = wl_cursor_image_get_buffer(img);
    wl_surface_attach(wl.cursor_surface, buf, 0, 0);
    wl_surface_damage(wl.cursor_surface, 0, 0, img->width, img->height);
    wl_surface_commit(wl.cursor_surface);
    wl_pointer_set_cursor(wl.pointer, wl.pointer_serial,
                          wl.cursor_surface, img->hotspot_x, img->hotspot_y);
}

UIBackend wayland_backend = {
    .init           = wayland_init,
    .shutdown       = wayland_shutdown,
    .create_window  = wayland_create_window,
    .destroy_window = wayland_destroy_window,
    .make_current   = wayland_make_current,
    .swap_buffers   = wayland_swap_buffers,
    .poll_events    = wayland_poll_events,
    .set_cursor     = wayland_set_cursor,
    .native_display = NULL,
    .native_window  = NULL,
};
