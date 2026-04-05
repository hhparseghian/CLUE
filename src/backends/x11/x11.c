/*
 * CLUE -- X11 backend
 *
 * Implements the UIBackend interface for X11 using Xlib + EGL + xkbcommon.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <xkbcommon/xkbcommon.h>

#include "x11.h"
#include "clue/window.h"

/* ------------------------------------------------------------------ */
/* Internal types                                                      */
/* ------------------------------------------------------------------ */

#define EVENT_QUEUE_SIZE 64

/* Per-window X11 state, stored in win->backend_data */
typedef struct {
    Window             xwin;
    EGLSurface         egl_surface;
    bool               close_requested;
} X11WindowData;

/* Global X11 state */
typedef struct {
    Display           *display;
    int                screen;
    Window             root;
    Atom               wm_delete_window;
    Atom               net_wm_name;
    Atom               utf8_string;

    /* EGL */
    EGLDisplay         egl_display;
    EGLContext         egl_context;
    EGLConfig          egl_config;

    /* xkbcommon */
    struct xkb_context *xkb_ctx;
    struct xkb_keymap  *xkb_keymap;
    struct xkb_state   *xkb_state;

    /* Event queue */
    ClueEvent            event_queue[EVENT_QUEUE_SIZE];
    int                event_head;
    int                event_tail;
} X11State;

static X11State x11 = {0};

/* ------------------------------------------------------------------ */
/* Event queue helpers                                                 */
/* ------------------------------------------------------------------ */

static void push_event(const ClueEvent *ev)
{
    int next = (x11.event_head + 1) % EVENT_QUEUE_SIZE;
    if (next == x11.event_tail) return;
    x11.event_queue[x11.event_head] = *ev;
    x11.event_head = next;
}

static bool pop_event(ClueEvent *out)
{
    if (x11.event_tail == x11.event_head) return false;
    *out = x11.event_queue[x11.event_tail];
    x11.event_tail = (x11.event_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

/* ------------------------------------------------------------------ */
/* Window lookup                                                       */
/* ------------------------------------------------------------------ */

static ClueWindow *find_window_by_xwin(Window xwin)
{
    ClueWindowManager *wm = clue_wm_get();
    for (int i = 0; i < wm->count; i++) {
        X11WindowData *d = wm->windows[i]->backend_data;
        if (d && d->xwin == xwin) {
            return wm->windows[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* X11 button mapping                                                  */
/* ------------------------------------------------------------------ */

static int x11_button_to_clue(unsigned int button)
{
    switch (button) {
    case Button1: return 0; /* left */
    case Button3: return 1; /* right */
    case Button2: return 2; /* middle */
    default:      return (int)button;
    }
}

/* ------------------------------------------------------------------ */
/* EGL initialisation                                                  */
/* ------------------------------------------------------------------ */

static int init_egl(void)
{
    x11.egl_display = eglGetDisplay((EGLNativeDisplayType)x11.display);
    if (x11.egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "clue-x11: eglGetDisplay failed\n");
        return -1;
    }

    EGLint major, minor;
    if (!eglInitialize(x11.egl_display, &major, &minor)) {
        fprintf(stderr, "clue-x11: eglInitialize failed\n");
        return -1;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        fprintf(stderr, "clue-x11: eglBindAPI failed\n");
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
    if (!eglChooseConfig(x11.egl_display, attribs, &x11.egl_config, 1,
                         &num_configs) || num_configs == 0) {
        fprintf(stderr, "clue-x11: eglChooseConfig failed\n");
        return -1;
    }

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    x11.egl_context = eglCreateContext(x11.egl_display, x11.egl_config,
                                       EGL_NO_CONTEXT, ctx_attribs);
    if (x11.egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "clue-x11: eglCreateContext failed\n");
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* xkbcommon setup                                                     */
/* ------------------------------------------------------------------ */

static int init_xkb(void)
{
    x11.xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!x11.xkb_ctx) {
        fprintf(stderr, "clue-x11: xkb_context_new failed\n");
        return -1;
    }

    /* Use default keymap (system layout from XKB_DEFAULT_* env vars) */
    x11.xkb_keymap = xkb_keymap_new_from_names(x11.xkb_ctx, NULL,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!x11.xkb_keymap) {
        fprintf(stderr, "clue-x11: failed to get keymap\n");
        return -1;
    }

    x11.xkb_state = xkb_state_new(x11.xkb_keymap);
    if (!x11.xkb_state) {
        fprintf(stderr, "clue-x11: xkb_state_new failed\n");
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Backend interface                                                   */
/* ------------------------------------------------------------------ */

static int x11_init(void)
{
    memset(&x11, 0, sizeof(x11));

    /* --- X11 connection --- */
    x11.display = XOpenDisplay(NULL);
    if (!x11.display) {
        fprintf(stderr, "clue-x11: XOpenDisplay failed\n");
        return -1;
    }

    x11.screen = DefaultScreen(x11.display);
    x11.root   = RootWindow(x11.display, x11.screen);

    /* Atoms for window close and UTF-8 title */
    x11.wm_delete_window = XInternAtom(x11.display, "WM_DELETE_WINDOW", False);
    x11.net_wm_name      = XInternAtom(x11.display, "_NET_WM_NAME", False);
    x11.utf8_string       = XInternAtom(x11.display, "UTF8_STRING", False);

    /* --- EGL --- */
    if (init_egl() != 0) {
        XCloseDisplay(x11.display);
        return -1;
    }

    /* --- xkbcommon --- */
    if (init_xkb() != 0) {
        eglTerminate(x11.egl_display);
        XCloseDisplay(x11.display);
        return -1;
    }

    return 0;
}

static void x11_shutdown(void)
{
    /* --- xkbcommon --- */
    if (x11.xkb_state)  xkb_state_unref(x11.xkb_state);
    if (x11.xkb_keymap) xkb_keymap_unref(x11.xkb_keymap);
    if (x11.xkb_ctx)    xkb_context_unref(x11.xkb_ctx);

    /* --- EGL --- */
    if (x11.egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(x11.egl_display, EGL_NO_SURFACE,
                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (x11.egl_context != EGL_NO_CONTEXT)
            eglDestroyContext(x11.egl_display, x11.egl_context);
        eglTerminate(x11.egl_display);
    }

    /* --- X11 --- */
    if (x11.display) XCloseDisplay(x11.display);

    memset(&x11, 0, sizeof(x11));
}

static int x11_create_window(struct ClueWindow *win)
{
    if (!win) return -1;

    X11WindowData *wd = calloc(1, sizeof(X11WindowData));
    if (!wd) return -1;

    /* --- Create X window --- */
    XSetWindowAttributes attrs;
    attrs.event_mask = ExposureMask | StructureNotifyMask |
                       KeyPressMask | KeyReleaseMask |
                       ButtonPressMask | ButtonReleaseMask |
                       PointerMotionMask | EnterWindowMask |
                       LeaveWindowMask;

    wd->xwin = XCreateWindow(
        x11.display, x11.root,
        win->x, win->y, win->w, win->h,
        0,                          /* border width */
        CopyFromParent,             /* depth */
        InputOutput,                /* class */
        CopyFromParent,             /* visual */
        CWEventMask,
        &attrs);

    if (!wd->xwin) {
        fprintf(stderr, "clue-x11: XCreateWindow failed\n");
        free(wd);
        return -1;
    }

    /* Set window title */
    if (win->title) {
        XChangeProperty(x11.display, wd->xwin,
                        x11.net_wm_name, x11.utf8_string, 8,
                        PropModeReplace,
                        (const unsigned char *)win->title,
                        (int)strlen(win->title));
        XStoreName(x11.display, wd->xwin, win->title);
    }

    /* Register for WM_DELETE_WINDOW */
    XSetWMProtocols(x11.display, wd->xwin, &x11.wm_delete_window, 1);

    /* Set parent hint for dialogs */
    if (win->type == CLUE_WINDOW_DIALOG && win->parent) {
        X11WindowData *parent_wd = win->parent->backend_data;
        if (parent_wd) {
            XSetTransientForHint(x11.display, wd->xwin, parent_wd->xwin);
        }
    }

    /* --- EGL surface --- */
    wd->egl_surface = eglCreateWindowSurface(x11.egl_display, x11.egl_config,
                                             (EGLNativeWindowType)wd->xwin,
                                             NULL);
    if (wd->egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "clue-x11: eglCreateWindowSurface failed\n");
        XDestroyWindow(x11.display, wd->xwin);
        free(wd);
        return -1;
    }

    /* Map (show) the window */
    XMapWindow(x11.display, wd->xwin);
    XFlush(x11.display);

    /* Make this window's EGL surface current so GL calls work immediately */
    eglMakeCurrent(x11.egl_display, wd->egl_surface,
                   wd->egl_surface, x11.egl_context);

    win->backend_data = wd;
    return 0;
}

static void x11_destroy_window(struct ClueWindow *win)
{
    if (!win || !win->backend_data) return;
    X11WindowData *wd = win->backend_data;

    if (wd->egl_surface != EGL_NO_SURFACE) {
        eglDestroySurface(x11.egl_display, wd->egl_surface);
    }
    if (wd->xwin) {
        XDestroyWindow(x11.display, wd->xwin);
    }

    free(wd);
    win->backend_data = NULL;
}

static void x11_set_window_type(struct ClueWindow *win, int type)
{
    (void)type;
    if (!win || !win->backend_data) return;
    X11WindowData *wd = win->backend_data;
    if (win->type == CLUE_WINDOW_DIALOG && win->parent) {
        X11WindowData *parent_wd = win->parent->backend_data;
        if (parent_wd)
            XSetTransientForHint(x11.display, wd->xwin, parent_wd->xwin);
    }
    XFlush(x11.display);
}

static void x11_set_window_parent(struct ClueWindow *win, struct ClueWindow *parent)
{
    (void)parent;
    if (!win || !win->backend_data) return;
    X11WindowData *wd = win->backend_data;
    if (win->type == CLUE_WINDOW_DIALOG && win->parent) {
        X11WindowData *parent_wd = win->parent->backend_data;
        if (parent_wd) {
            XSetTransientForHint(x11.display, wd->xwin, parent_wd->xwin);
            /* Center dialog on parent's actual screen position */
            Window child;
            int px, py;
            XTranslateCoordinates(x11.display, parent_wd->xwin, x11.root,
                                  0, 0, &px, &py, &child);
            int cx = px + (win->parent->w - win->w) / 2;
            int cy = py + (win->parent->h - win->h) / 2;
            XMoveWindow(x11.display, wd->xwin, cx, cy);
        }
    }
    XFlush(x11.display);
}

static void x11_set_window_position(struct ClueWindow *win, int px, int py)
{
    if (!win || !win->backend_data) return;
    X11WindowData *wd = win->backend_data;
    XMoveWindow(x11.display, wd->xwin, px, py);
    XFlush(x11.display);
}

static void x11_make_current(struct ClueWindow *win)
{
    if (!win || !win->backend_data) return;
    X11WindowData *wd = win->backend_data;
    eglMakeCurrent(x11.egl_display, wd->egl_surface,
                   wd->egl_surface, x11.egl_context);
}

static void x11_swap_buffers(struct ClueWindow *win)
{
    if (!win || !win->backend_data) return;
    X11WindowData *wd = win->backend_data;

    eglMakeCurrent(x11.egl_display, wd->egl_surface,
                   wd->egl_surface, x11.egl_context);
    eglSwapBuffers(x11.egl_display, wd->egl_surface);
}

static void process_x_event(XEvent *xev)
{
    ClueEvent ev = {0};

    switch (xev->type) {

    case MotionNotify: {
        ClueWindow *win = find_window_by_xwin(xev->xmotion.window);
        if (!win) break;
        ev.type = CLUE_EVENT_MOUSE_MOVE;
        ev.window = win;
        ev.mouse_move.x = xev->xmotion.x;
        ev.mouse_move.y = xev->xmotion.y;
        push_event(&ev);
        break;
    }

    case ButtonPress:
    case ButtonRelease: {
        ClueWindow *win = find_window_by_xwin(xev->xbutton.window);
        if (!win) break;

        /* Scroll wheel (buttons 4-7) */
        if (xev->xbutton.button >= Button4 && xev->xbutton.button <= 7 &&
            xev->type == ButtonPress) {
            ev.type = CLUE_EVENT_MOUSE_SCROLL;
            ev.window = win;
            ev.mouse_scroll.x = xev->xbutton.x;
            ev.mouse_scroll.y = xev->xbutton.y;
            ev.mouse_scroll.dx = 0.0f;
            ev.mouse_scroll.dy = (xev->xbutton.button == Button4) ? 1.0f :
                                 (xev->xbutton.button == Button5) ? -1.0f : 0.0f;
            if (xev->xbutton.button == 6) ev.mouse_scroll.dx = -1.0f;
            if (xev->xbutton.button == 7) ev.mouse_scroll.dx =  1.0f;
            push_event(&ev);
            break;
        }

        ev.type = CLUE_EVENT_MOUSE_BUTTON;
        ev.window = win;
        ev.mouse_button.x = xev->xbutton.x;
        ev.mouse_button.y = xev->xbutton.y;
        ev.mouse_button.btn = x11_button_to_clue(xev->xbutton.button);
        ev.mouse_button.pressed = (xev->type == ButtonPress);
        push_event(&ev);
        break;
    }

    case KeyPress:
    case KeyRelease: {
        ClueWindow *win = find_window_by_xwin(xev->xkey.window);
        if (!win) break;

        /* X11 keycode = xkb keycode (no +8 needed, X already uses XKB codes) */
        xkb_keycode_t keycode = (xkb_keycode_t)xev->xkey.keycode;
        bool pressed = (xev->type == KeyPress);

        ev.type = CLUE_EVENT_KEY;
        ev.window = win;
        ev.key.keycode = (int)xkb_state_key_get_one_sym(x11.xkb_state, keycode);
        ev.key.pressed = pressed;
        ev.key.modifiers = 0;
        if (xkb_state_mod_name_is_active(x11.xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
            ev.key.modifiers |= CLUE_MOD_SHIFT;
        if (xkb_state_mod_name_is_active(x11.xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
            ev.key.modifiers |= CLUE_MOD_CTRL;
        if (xkb_state_mod_name_is_active(x11.xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE))
            ev.key.modifiers |= CLUE_MOD_ALT;
        if (xkb_state_mod_name_is_active(x11.xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE))
            ev.key.modifiers |= CLUE_MOD_SUPER;

        if (pressed && x11.xkb_state) {
            /* Don't generate text for Ctrl combos */
            if (!(ev.key.modifiers & CLUE_MOD_CTRL))
                xkb_state_key_get_utf8(x11.xkb_state, keycode,
                                       ev.key.text, sizeof(ev.key.text));
        }

        push_event(&ev);

        xkb_state_update_key(x11.xkb_state, keycode,
                             pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
        break;
    }

    case ConfigureNotify: {
        ClueWindow *win = find_window_by_xwin(xev->xconfigure.window);
        if (!win) break;

        int new_w = xev->xconfigure.width;
        int new_h = xev->xconfigure.height;

        if (new_w != win->w || new_h != win->h) {
            win->w = new_w;
            win->h = new_h;
            win->dirty = true;

            ev.type = CLUE_EVENT_RESIZE;
            ev.window = win;
            ev.resize.w = new_w;
            ev.resize.h = new_h;
            push_event(&ev);
        }
        break;
    }

    case ClientMessage: {
        if ((Atom)xev->xclient.data.l[0] == x11.wm_delete_window) {
            ClueWindow *win = find_window_by_xwin(xev->xclient.window);
            if (!win) break;

            X11WindowData *wd = win->backend_data;
            if (wd) wd->close_requested = true;

            ev.type = CLUE_EVENT_CLOSE;
            ev.window = win;
            push_event(&ev);
        }
        break;
    }

    default:
        break;
    }
}

static int x11_poll_events(ClueEvent *events, int max)
{
    if (!events || max <= 0) return 0;

    /* Process all pending X events (non-blocking) */
    while (XPending(x11.display)) {
        XEvent xev;
        XNextEvent(x11.display, &xev);
        process_x_event(&xev);
    }

    /* Drain event queue into caller's array */
    int count = 0;
    while (count < max && pop_event(&events[count])) {
        count++;
    }

    return count;
}

/* ------------------------------------------------------------------ */
/* Cursor                                                              */
/* ------------------------------------------------------------------ */

#include <X11/cursorfont.h>

static void x11_set_cursor(ClueWindow *win, int shape)
{
    if (!win || !x11.display) return;
    X11WindowData *wd = win->backend_data;
    if (!wd) return;

    unsigned int xfont;
    switch (shape) {
    case 1:  xfont = XC_hand2;            break; /* CLUE_CURSOR_POINTER */
    case 2:  xfont = XC_xterm;            break; /* CLUE_CURSOR_TEXT */
    case 3:  xfont = XC_sb_h_double_arrow; break; /* CLUE_CURSOR_RESIZE_H */
    case 4:  xfont = XC_sb_v_double_arrow; break; /* CLUE_CURSOR_RESIZE_V */
    case 5:  xfont = XC_fleur;            break; /* CLUE_CURSOR_MOVE */
    case 6:  xfont = XC_crosshair;        break; /* CLUE_CURSOR_CROSSHAIR */
    default: xfont = 0; break;
    }

    if (xfont) {
        Cursor c = XCreateFontCursor(x11.display, xfont);
        XDefineCursor(x11.display, wd->xwin, c);
        XFreeCursor(x11.display, c);
    } else {
        XUndefineCursor(x11.display, wd->xwin);
    }
    XFlush(x11.display);
}

/* ------------------------------------------------------------------ */
/* Exported backend instance                                           */
/* ------------------------------------------------------------------ */

UIBackend x11_backend = {
    .init           = x11_init,
    .shutdown       = x11_shutdown,
    .create_window    = x11_create_window,
    .destroy_window   = x11_destroy_window,
    .set_window_type    = x11_set_window_type,
    .set_window_parent  = x11_set_window_parent,
    .set_window_position = x11_set_window_position,
    .make_current       = x11_make_current,
    .swap_buffers   = x11_swap_buffers,
    .poll_events    = x11_poll_events,
    .set_cursor     = x11_set_cursor,
    .native_display = NULL,
    .native_window  = NULL,
};
