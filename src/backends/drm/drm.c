/*
 * CLUE -- DRM/KMS backend
 *
 * Implements the UIBackend interface for bare-metal Linux using
 * DRM/KMS + GBM + EGL for display, libinput for input, and
 * xkbcommon for keyboard translation. No compositor required.
 */

/* Need _GNU_SOURCE for O_CLOEXEC and other Linux-specific APIs */
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <libinput.h>
#include <libudev.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include "drm.h"
#include "clue/window.h"

/* ------------------------------------------------------------------ */
/* Internal types                                                      */
/* ------------------------------------------------------------------ */

#define EVENT_QUEUE_SIZE 64
#define MAX_DRM_WINDOWS  32

/* Per-window DRM state.
 *
 * On bare-metal DRM there is one physical screen. Each logical window
 * renders into its own offscreen FBO, then all windows are composited
 * onto the main GBM scanout surface during swap_buffers.
 *
 * TODO: An alternative approach is EGL pbuffer surfaces per window.
 *       The FBO/texture approach used here is generally more efficient
 *       because it avoids EGL context switching and keeps all GL work
 *       in one context.
 */
typedef struct {
    int      x, y;
    bool     dirty;
    GLuint   fbo;
    GLuint   color_tex;
} DRMWindowData;

/* Global DRM state (file-scope singleton) */
typedef struct {
    /* DRM */
    int                    drm_fd;
    drmModeConnector      *connector;
    drmModeCrtc           *saved_crtc;
    drmModeModeInfo        mode;
    uint32_t               crtc_id;
    uint32_t               connector_id;

    /* GBM */
    struct gbm_device     *gbm_device;
    struct gbm_surface    *gbm_surface;
    struct gbm_bo         *prev_bo;
    uint32_t               prev_fb;

    /* EGL */
    EGLDisplay             egl_display;
    EGLContext             egl_context;
    EGLConfig              egl_config;
    EGLSurface             egl_surface;

    /* Screen dimensions (from selected mode) */
    int                    screen_w;
    int                    screen_h;

    /* libinput */
    struct libinput       *libinput;
    struct udev           *udev;

    /* xkbcommon */
    struct xkb_context    *xkb_ctx;
    struct xkb_keymap     *xkb_keymap;
    struct xkb_state      *xkb_state;

    /* Page flip state */
    bool                   flip_pending;
    bool                   first_frame;

    /* Pointer tracking (software cursor) */
    int                    pointer_x;
    int                    pointer_y;

    /* Event queue: written by input callbacks, drained by poll_events */
    UIEvent                event_queue[EVENT_QUEUE_SIZE];
    int                    event_head;
    int                    event_tail;

    /* Window list for software compositing */
    UIWindow              *windows[MAX_DRM_WINDOWS];
    int                    window_count;

    /* Blit shader (compositing) */
    GLuint                 blit_prog;
    GLint                  blit_loc_pos;
    GLint                  blit_loc_uv;
    GLint                  blit_loc_tex;
} DRMState;

static DRMState drm = {0};

/* ------------------------------------------------------------------ */
/* Event queue helpers                                                 */
/* ------------------------------------------------------------------ */

static void push_event(const UIEvent *ev)
{
    int next = (drm.event_head + 1) % EVENT_QUEUE_SIZE;
    if (next == drm.event_tail) return; /* full -- drop */
    drm.event_queue[drm.event_head] = *ev;
    drm.event_head = next;
}

static bool pop_event(UIEvent *out)
{
    if (drm.event_tail == drm.event_head) return false;
    *out = drm.event_queue[drm.event_tail];
    drm.event_tail = (drm.event_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

/* ------------------------------------------------------------------ */
/* Window hit testing                                                  */
/* ------------------------------------------------------------------ */

/* Find the topmost window containing (x, y). Last in list = topmost. */
static UIWindow *find_window_at(int x, int y)
{
    for (int i = drm.window_count - 1; i >= 0; i--) {
        UIWindow *w = drm.windows[i];
        if (w->visible &&
            x >= w->x && x < w->x + w->w &&
            y >= w->y && y < w->y + w->h)
            return w;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* libinput interface callbacks                                        */
/* ------------------------------------------------------------------ */

static int open_restricted(const char *path, int flags, void *user_data)
{
    (void)user_data;
    int fd = open(path, flags);
    if (fd < 0) {
        fprintf(stderr, "clue-drm: open_restricted %s: %s\n",
                path, strerror(errno));
    }
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
    (void)user_data;
    close(fd);
}

static const struct libinput_interface libinput_iface = {
    .open_restricted  = open_restricted,
    .close_restricted = close_restricted,
};

/* ------------------------------------------------------------------ */
/* libinput event processing                                           */
/* ------------------------------------------------------------------ */

static void process_libinput_event(struct libinput_event *ev)
{
    enum libinput_event_type type = libinput_event_get_type(ev);
    UIWindowManager *wm = clue_wm_get();

    switch (type) {

    /* --- Pointer relative motion --- */
    case LIBINPUT_EVENT_POINTER_MOTION: {
        struct libinput_event_pointer *p = libinput_event_get_pointer_event(ev);
        drm.pointer_x += (int)libinput_event_pointer_get_dx(p);
        drm.pointer_y += (int)libinput_event_pointer_get_dy(p);

        /* Clamp to screen bounds */
        if (drm.pointer_x < 0) drm.pointer_x = 0;
        if (drm.pointer_y < 0) drm.pointer_y = 0;
        if (drm.pointer_x >= drm.screen_w) drm.pointer_x = drm.screen_w - 1;
        if (drm.pointer_y >= drm.screen_h) drm.pointer_y = drm.screen_h - 1;

        UIWindow *win = find_window_at(drm.pointer_x, drm.pointer_y);
        if (win) {
            UIEvent uev = {0};
            uev.type = UI_EVENT_MOUSE_MOVE;
            uev.window = win;
            uev.mouse_move.x = drm.pointer_x - win->x;
            uev.mouse_move.y = drm.pointer_y - win->y;
            push_event(&uev);
        }
        break;
    }

    /* --- Pointer absolute motion (touchscreens, tablets) --- */
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE: {
        struct libinput_event_pointer *p = libinput_event_get_pointer_event(ev);
        drm.pointer_x = (int)libinput_event_pointer_get_absolute_x_transformed(
            p, drm.screen_w);
        drm.pointer_y = (int)libinput_event_pointer_get_absolute_y_transformed(
            p, drm.screen_h);

        UIWindow *win = find_window_at(drm.pointer_x, drm.pointer_y);
        if (win) {
            UIEvent uev = {0};
            uev.type = UI_EVENT_MOUSE_MOVE;
            uev.window = win;
            uev.mouse_move.x = drm.pointer_x - win->x;
            uev.mouse_move.y = drm.pointer_y - win->y;
            push_event(&uev);
        }
        break;
    }

    /* --- Pointer button --- */
    case LIBINPUT_EVENT_POINTER_BUTTON: {
        struct libinput_event_pointer *p = libinput_event_get_pointer_event(ev);
        uint32_t button = libinput_event_pointer_get_button(p);
        enum libinput_button_state state = libinput_event_pointer_get_button_state(p);

        int btn;
        switch (button) {
        case BTN_LEFT:   btn = 0; break;
        case BTN_RIGHT:  btn = 1; break;
        case BTN_MIDDLE: btn = 2; break;
        default:         btn = (int)button; break;
        }

        UIWindow *win = find_window_at(drm.pointer_x, drm.pointer_y);

        /* Update focus if clicking a different window */
        if (win && win != wm->focused &&
            state == LIBINPUT_BUTTON_STATE_PRESSED) {
            wm->focused = win;
        }

        if (win) {
            UIEvent uev = {0};
            uev.type = UI_EVENT_MOUSE_BUTTON;
            uev.window = win;
            uev.mouse_button.x = drm.pointer_x - win->x;
            uev.mouse_button.y = drm.pointer_y - win->y;
            uev.mouse_button.btn = btn;
            uev.mouse_button.pressed =
                (state == LIBINPUT_BUTTON_STATE_PRESSED);
            push_event(&uev);
        }
        break;
    }

    /* --- Keyboard key --- */
    case LIBINPUT_EVENT_KEYBOARD_KEY: {
        struct libinput_event_keyboard *k = libinput_event_get_keyboard_event(ev);
        uint32_t key = libinput_event_keyboard_get_key(k);
        enum libinput_key_state state = libinput_event_keyboard_get_key_state(k);

        /* evdev keycode + 8 = xkb keycode */
        xkb_keycode_t xkb_key = key + 8;

        bool pressed = (state == LIBINPUT_KEY_STATE_PRESSED);

        UIEvent uev = {0};
        uev.type = UI_EVENT_KEY;
        uev.window = wm->focused;
        uev.key.keycode = (int)xkb_state_key_get_one_sym(drm.xkb_state, xkb_key);
        uev.key.pressed = pressed;

        if (pressed && drm.xkb_state) {
            xkb_state_key_get_utf8(drm.xkb_state, xkb_key,
                                   uev.key.text, sizeof(uev.key.text));
        }

        push_event(&uev);

        /* Update xkb modifier state */
        xkb_state_update_key(drm.xkb_state, xkb_key,
                             pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
        break;
    }

    /* --- Touch events: treat as mouse for now --- */
    case LIBINPUT_EVENT_TOUCH_DOWN:
    case LIBINPUT_EVENT_TOUCH_MOTION: {
        struct libinput_event_touch *t = libinput_event_get_touch_event(ev);
        drm.pointer_x = (int)libinput_event_touch_get_x_transformed(
            t, drm.screen_w);
        drm.pointer_y = (int)libinput_event_touch_get_y_transformed(
            t, drm.screen_h);

        UIWindow *win = find_window_at(drm.pointer_x, drm.pointer_y);
        if (win) {
            UIEvent uev = {0};
            uev.type = UI_EVENT_MOUSE_MOVE;
            uev.window = win;
            uev.mouse_move.x = drm.pointer_x - win->x;
            uev.mouse_move.y = drm.pointer_y - win->y;
            push_event(&uev);

            /* Treat touch-down as left button press */
            if (type == LIBINPUT_EVENT_TOUCH_DOWN) {
                UIEvent btn_ev = {0};
                btn_ev.type = UI_EVENT_MOUSE_BUTTON;
                btn_ev.window = win;
                btn_ev.mouse_button.x = drm.pointer_x - win->x;
                btn_ev.mouse_button.y = drm.pointer_y - win->y;
                btn_ev.mouse_button.btn = 0;
                btn_ev.mouse_button.pressed = 1;
                push_event(&btn_ev);

                if (win != wm->focused) wm->focused = win;
            }
        }
        /* TODO: Proper multi-touch handling via ABS_MT_ slots */
        break;
    }

    case LIBINPUT_EVENT_TOUCH_UP: {
        UIWindow *win = find_window_at(drm.pointer_x, drm.pointer_y);
        if (win) {
            UIEvent uev = {0};
            uev.type = UI_EVENT_MOUSE_BUTTON;
            uev.window = win;
            uev.mouse_button.x = drm.pointer_x - win->x;
            uev.mouse_button.y = drm.pointer_y - win->y;
            uev.mouse_button.btn = 0;
            uev.mouse_button.pressed = 0;
            push_event(&uev);
        }
        break;
    }

    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Page flip handler                                                   */
/* ------------------------------------------------------------------ */

static void page_flip_handler(int fd, unsigned int sequence,
                              unsigned int tv_sec, unsigned int tv_usec,
                              void *user_data)
{
    (void)fd; (void)sequence; (void)tv_sec; (void)tv_usec; (void)user_data;
    drm.flip_pending = false;
}

/* ------------------------------------------------------------------ */
/* GBM buffer object -> DRM framebuffer helper                         */
/* ------------------------------------------------------------------ */

/* Stored in gbm_bo user data so we can cache the fb id */
typedef struct {
    uint32_t fb_id;
    int      drm_fd;
} DRMFBData;

static void destroy_fb_callback(struct gbm_bo *bo, void *data)
{
    (void)bo;
    DRMFBData *fb = data;
    if (fb) {
        drmModeRmFB(fb->drm_fd, fb->fb_id);
        free(fb);
    }
}

static uint32_t drm_fb_get_from_bo(struct gbm_bo *bo)
{
    /* Return cached fb id if we already created one for this bo */
    DRMFBData *existing = gbm_bo_get_user_data(bo);
    if (existing) return existing->fb_id;

    uint32_t w      = gbm_bo_get_width(bo);
    uint32_t h      = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    uint32_t fb_id;
    int ret = drmModeAddFB(drm.drm_fd, w, h, 24, 32, stride, handle, &fb_id);
    if (ret) {
        fprintf(stderr, "clue-drm: drmModeAddFB failed: %s\n", strerror(errno));
        return 0;
    }

    DRMFBData *fb_data = malloc(sizeof(DRMFBData));
    if (!fb_data) {
        drmModeRmFB(drm.drm_fd, fb_id);
        return 0;
    }
    fb_data->fb_id  = fb_id;
    fb_data->drm_fd = drm.drm_fd;
    gbm_bo_set_user_data(bo, fb_data, destroy_fb_callback);

    return fb_id;
}

/* ------------------------------------------------------------------ */
/* Blit shader (compositing windows onto main framebuffer)             */
/* ------------------------------------------------------------------ */

static const char *blit_vert_src =
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "    v_uv = a_uv;\n"
    "}\n";

static const char *blit_frag_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(u_tex, v_uv);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[256];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "clue-drm: shader compile: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static void init_blit_shader(void)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, blit_vert_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, blit_frag_src);
    if (!vs || !fs) return;

    drm.blit_prog = glCreateProgram();
    glAttachShader(drm.blit_prog, vs);
    glAttachShader(drm.blit_prog, fs);
    glLinkProgram(drm.blit_prog);

    GLint ok;
    glGetProgramiv(drm.blit_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[256];
        glGetProgramInfoLog(drm.blit_prog, sizeof(log), NULL, log);
        fprintf(stderr, "clue-drm: shader link: %s\n", log);
        glDeleteProgram(drm.blit_prog);
        drm.blit_prog = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (drm.blit_prog) {
        drm.blit_loc_pos = glGetAttribLocation(drm.blit_prog, "a_pos");
        drm.blit_loc_uv  = glGetAttribLocation(drm.blit_prog, "a_uv");
        drm.blit_loc_tex  = glGetUniformLocation(drm.blit_prog, "u_tex");
    }
}

/* Blit a texture at pixel rect (x,y,w,h) onto the screen.
 * sw, sh = screen dimensions for NDC conversion. */
static void blit_texture(GLuint tex, int x, int y, int w, int h,
                         int sw, int sh)
{
    if (!drm.blit_prog) return;

    /* Convert pixel coords to NDC [-1, 1] */
    float x0 = (float)x / (float)sw * 2.0f - 1.0f;
    float y0 = 1.0f - (float)(y + h) / (float)sh * 2.0f;
    float x1 = (float)(x + w) / (float)sw * 2.0f - 1.0f;
    float y1 = 1.0f - (float)y / (float)sh * 2.0f;

    /* Two triangles forming a quad */
    GLfloat verts[] = {
        x0, y0,  0.0f, 1.0f,
        x1, y0,  1.0f, 1.0f,
        x0, y1,  0.0f, 0.0f,
        x1, y0,  1.0f, 1.0f,
        x1, y1,  1.0f, 0.0f,
        x0, y1,  0.0f, 0.0f,
    };

    glUseProgram(drm.blit_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(drm.blit_loc_tex, 0);

    glVertexAttribPointer(drm.blit_loc_pos, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts);
    glEnableVertexAttribArray(drm.blit_loc_pos);

    glVertexAttribPointer(drm.blit_loc_uv, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), verts + 2);
    glEnableVertexAttribArray(drm.blit_loc_uv);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(drm.blit_loc_pos);
    glDisableVertexAttribArray(drm.blit_loc_uv);
}

/* ------------------------------------------------------------------ */
/* DRM device open helper                                              */
/* ------------------------------------------------------------------ */

static int open_drm_device(void)
{
    static const char *cards[] = {
        "/dev/dri/card0",
        "/dev/dri/card1",
        NULL,
    };

    for (int i = 0; cards[i]; i++) {
        int fd = open(cards[i], O_RDWR | O_CLOEXEC);
        if (fd < 0) continue;

        /* Verify this device has KMS capability */
        drmModeRes *res = drmModeGetResources(fd);
        if (res) {
            drmModeFreeResources(res);
            return fd;
        }
        close(fd);
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* DRM connector / CRTC discovery                                      */
/* ------------------------------------------------------------------ */

static int find_connector_and_crtc(void)
{
    drmModeRes *res = drmModeGetResources(drm.drm_fd);
    if (!res) {
        fprintf(stderr, "clue-drm: drmModeGetResources failed\n");
        return -1;
    }

    /* Find first connected connector */
    drmModeConnector *conn = NULL;
    for (int i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(drm.drm_fd, res->connectors[i]);
        if (!conn) continue;
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            break;
        }
        drmModeFreeConnector(conn);
        conn = NULL;
    }

    if (!conn) {
        fprintf(stderr, "clue-drm: no connected display found\n");
        drmModeFreeResources(res);
        return -1;
    }

    drm.connector    = conn;
    drm.connector_id = conn->connector_id;
    drm.mode         = conn->modes[0]; /* preferred / native mode */
    drm.screen_w     = (int)drm.mode.hdisplay;
    drm.screen_h     = (int)drm.mode.vdisplay;

    /* Find a CRTC for this connector */
    drmModeEncoder *enc = NULL;

    /* Try the currently attached encoder first */
    if (conn->encoder_id) {
        enc = drmModeGetEncoder(drm.drm_fd, conn->encoder_id);
        if (enc) {
            drm.crtc_id = enc->crtc_id;
            drmModeFreeEncoder(enc);
            goto found;
        }
    }

    /* Walk all encoders the connector supports */
    for (int i = 0; i < conn->count_encoders; i++) {
        enc = drmModeGetEncoder(drm.drm_fd, conn->encoders[i]);
        if (!enc) continue;

        for (int j = 0; j < res->count_crtcs; j++) {
            if (enc->possible_crtcs & (1u << j)) {
                drm.crtc_id = res->crtcs[j];
                drmModeFreeEncoder(enc);
                goto found;
            }
        }
        drmModeFreeEncoder(enc);
    }

    fprintf(stderr, "clue-drm: no suitable CRTC found\n");
    drmModeFreeConnector(conn);
    drm.connector = NULL;
    drmModeFreeResources(res);
    return -1;

found:
    /* Save current CRTC so we can restore it on shutdown */
    drm.saved_crtc = drmModeGetCrtc(drm.drm_fd, drm.crtc_id);
    drmModeFreeResources(res);
    return 0;
}

/* ------------------------------------------------------------------ */
/* EGL initialisation                                                  */
/* ------------------------------------------------------------------ */

static int init_egl(void)
{
    drm.egl_display = eglGetDisplay((EGLNativeDisplayType)drm.gbm_device);
    if (drm.egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "clue-drm: eglGetDisplay failed\n");
        return -1;
    }

    EGLint major, minor;
    if (!eglInitialize(drm.egl_display, &major, &minor)) {
        fprintf(stderr, "clue-drm: eglInitialize failed\n");
        return -1;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        fprintf(stderr, "clue-drm: eglBindAPI failed\n");
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
    if (!eglChooseConfig(drm.egl_display, attribs, &drm.egl_config, 1,
                         &num_configs) || num_configs == 0) {
        fprintf(stderr, "clue-drm: eglChooseConfig failed\n");
        return -1;
    }

    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    drm.egl_context = eglCreateContext(drm.egl_display, drm.egl_config,
                                       EGL_NO_CONTEXT, ctx_attribs);
    if (drm.egl_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "clue-drm: eglCreateContext failed\n");
        return -1;
    }

    drm.egl_surface = eglCreateWindowSurface(drm.egl_display, drm.egl_config,
                                             (EGLNativeWindowType)drm.gbm_surface,
                                             NULL);
    if (drm.egl_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "clue-drm: eglCreateWindowSurface failed\n");
        return -1;
    }

    if (!eglMakeCurrent(drm.egl_display, drm.egl_surface,
                        drm.egl_surface, drm.egl_context)) {
        fprintf(stderr, "clue-drm: eglMakeCurrent failed\n");
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Backend interface                                                   */
/* ------------------------------------------------------------------ */

static int drm_init(void)
{
    memset(&drm, 0, sizeof(drm));
    drm.drm_fd      = -1;
    drm.first_frame  = true;

    /* --- Open DRM device --- */
    drm.drm_fd = open_drm_device();
    if (drm.drm_fd < 0) {
        fprintf(stderr, "clue-drm: failed to open DRM device\n");
        return -1;
    }

    /* --- Find connector and CRTC --- */
    if (find_connector_and_crtc() != 0) {
        close(drm.drm_fd);
        return -1;
    }

    fprintf(stderr, "clue-drm: display %dx%d @ %dHz\n",
            drm.screen_w, drm.screen_h, drm.mode.vrefresh);

    /* --- GBM setup --- */
    drm.gbm_device = gbm_create_device(drm.drm_fd);
    if (!drm.gbm_device) {
        fprintf(stderr, "clue-drm: gbm_create_device failed\n");
        goto fail;
    }

    drm.gbm_surface = gbm_surface_create(drm.gbm_device,
                                          drm.screen_w, drm.screen_h,
                                          GBM_FORMAT_XRGB8888,
                                          GBM_BO_USE_SCANOUT |
                                          GBM_BO_USE_RENDERING);
    if (!drm.gbm_surface) {
        fprintf(stderr, "clue-drm: gbm_surface_create failed\n");
        goto fail;
    }

    /* --- EGL setup --- */
    if (init_egl() != 0) {
        goto fail;
    }

    /* --- Blit shader for compositing --- */
    init_blit_shader();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* --- libinput setup --- */
    drm.udev = udev_new();
    if (!drm.udev) {
        fprintf(stderr, "clue-drm: udev_new failed\n");
        goto fail;
    }

    drm.libinput = libinput_udev_create_context(&libinput_iface,
                                                NULL, drm.udev);
    if (!drm.libinput) {
        fprintf(stderr, "clue-drm: libinput_udev_create_context failed\n");
        goto fail;
    }

    if (libinput_udev_assign_seat(drm.libinput, "seat0") != 0) {
        fprintf(stderr, "clue-drm: libinput_udev_assign_seat failed\n");
        goto fail;
    }

    /* --- xkbcommon setup --- */
    drm.xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!drm.xkb_ctx) {
        fprintf(stderr, "clue-drm: xkb_context_new failed\n");
        goto fail;
    }

    /* Use default keymap (system layout from XKB_DEFAULT_* env vars) */
    drm.xkb_keymap = xkb_keymap_new_from_names(drm.xkb_ctx, NULL,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!drm.xkb_keymap) {
        fprintf(stderr, "clue-drm: xkb_keymap_new_from_names failed\n");
        goto fail;
    }

    drm.xkb_state = xkb_state_new(drm.xkb_keymap);
    if (!drm.xkb_state) {
        fprintf(stderr, "clue-drm: xkb_state_new failed\n");
        goto fail;
    }

    /* Centre the pointer */
    drm.pointer_x = drm.screen_w / 2;
    drm.pointer_y = drm.screen_h / 2;

    return 0;

fail:
    drm_backend.shutdown();
    return -1;
}

static void drm_shutdown(void)
{
    /* --- Destroy window FBOs --- */
    for (int i = 0; i < drm.window_count; i++) {
        UIWindow *win = drm.windows[i];
        if (win && win->backend_data) {
            DRMWindowData *wd = win->backend_data;
            if (wd->fbo)       glDeleteFramebuffers(1, &wd->fbo);
            if (wd->color_tex) glDeleteTextures(1, &wd->color_tex);
            free(wd);
            win->backend_data = NULL;
        }
    }
    drm.window_count = 0;

    /* --- Blit shader --- */
    if (drm.blit_prog) {
        glDeleteProgram(drm.blit_prog);
        drm.blit_prog = 0;
    }

    /* --- xkbcommon --- */
    if (drm.xkb_state)  xkb_state_unref(drm.xkb_state);
    if (drm.xkb_keymap) xkb_keymap_unref(drm.xkb_keymap);
    if (drm.xkb_ctx)    xkb_context_unref(drm.xkb_ctx);

    /* --- libinput --- */
    if (drm.libinput) libinput_unref(drm.libinput);
    if (drm.udev)     udev_unref(drm.udev);

    /* --- EGL --- */
    if (drm.egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(drm.egl_display, EGL_NO_SURFACE,
                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (drm.egl_surface != EGL_NO_SURFACE)
            eglDestroySurface(drm.egl_display, drm.egl_surface);
        if (drm.egl_context != EGL_NO_CONTEXT)
            eglDestroyContext(drm.egl_display, drm.egl_context);
        eglTerminate(drm.egl_display);
    }

    /* --- GBM --- */
    if (drm.prev_bo) {
        gbm_surface_release_buffer(drm.gbm_surface, drm.prev_bo);
    }
    if (drm.gbm_surface) gbm_surface_destroy(drm.gbm_surface);
    if (drm.gbm_device)  gbm_device_destroy(drm.gbm_device);

    /* --- DRM: restore saved CRTC --- */
    if (drm.saved_crtc) {
        drmModeSetCrtc(drm.drm_fd,
                       drm.saved_crtc->crtc_id,
                       drm.saved_crtc->buffer_id,
                       drm.saved_crtc->x, drm.saved_crtc->y,
                       &drm.connector_id, 1,
                       &drm.saved_crtc->mode);
        drmModeFreeCrtc(drm.saved_crtc);
    }

    if (drm.connector) drmModeFreeConnector(drm.connector);
    if (drm.drm_fd >= 0) close(drm.drm_fd);

    memset(&drm, 0, sizeof(drm));
    drm.drm_fd = -1;
}

static int drm_create_window(struct UIWindow *win)
{
    if (!win) return -1;
    if (drm.window_count >= MAX_DRM_WINDOWS) {
        fprintf(stderr, "clue-drm: max windows reached (%d)\n",
                MAX_DRM_WINDOWS);
        return -1;
    }

    DRMWindowData *wd = calloc(1, sizeof(DRMWindowData));
    if (!wd) return -1;

    wd->x     = win->x;
    wd->y     = win->y;
    wd->dirty = true;

    /* --- Create offscreen FBO for this window --- */
    eglMakeCurrent(drm.egl_display, drm.egl_surface,
                   drm.egl_surface, drm.egl_context);

    glGenTextures(1, &wd->color_tex);
    glBindTexture(GL_TEXTURE_2D, wd->color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win->w, win->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &wd->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, wd->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, wd->color_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "clue-drm: FBO incomplete (0x%x)\n", status);
        glDeleteFramebuffers(1, &wd->fbo);
        glDeleteTextures(1, &wd->color_tex);
        free(wd);
        return -1;
    }

    /* Unbind back to default */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    win->backend_data = wd;
    drm.windows[drm.window_count++] = win;

    return 0;
}

static void drm_destroy_window(struct UIWindow *win)
{
    if (!win || !win->backend_data) return;
    DRMWindowData *wd = win->backend_data;

    /* Remove from window list */
    for (int i = 0; i < drm.window_count; i++) {
        if (drm.windows[i] == win) {
            memmove(&drm.windows[i], &drm.windows[i + 1],
                    sizeof(UIWindow *) * (drm.window_count - i - 1));
            drm.window_count--;
            break;
        }
    }

    eglMakeCurrent(drm.egl_display, drm.egl_surface,
                   drm.egl_surface, drm.egl_context);

    if (wd->fbo)       glDeleteFramebuffers(1, &wd->fbo);
    if (wd->color_tex) glDeleteTextures(1, &wd->color_tex);

    free(wd);
    win->backend_data = NULL;
}

static void drm_make_current(struct UIWindow *win)
{
    (void)win;
    eglMakeCurrent(drm.egl_display, drm.egl_surface,
                   drm.egl_surface, drm.egl_context);
}

static void drm_swap_buffers(struct UIWindow *win)
{
    (void)win;

    eglMakeCurrent(drm.egl_display, drm.egl_surface,
                   drm.egl_surface, drm.egl_context);

    /* --- Composite all windows onto main surface --- */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, drm.screen_w, drm.screen_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < drm.window_count; i++) {
        UIWindow *w = drm.windows[i];
        if (!w->visible) continue;
        DRMWindowData *wd = w->backend_data;
        if (!wd) continue;

        blit_texture(wd->color_tex, w->x, w->y, w->w, w->h,
                     drm.screen_w, drm.screen_h);
    }

    /* --- EGL swap into GBM front buffer --- */
    eglSwapBuffers(drm.egl_display, drm.egl_surface);

    /* --- DRM page flip --- */
    struct gbm_bo *bo = gbm_surface_lock_front_buffer(drm.gbm_surface);
    if (!bo) {
        fprintf(stderr, "clue-drm: gbm_surface_lock_front_buffer failed\n");
        return;
    }

    uint32_t fb = drm_fb_get_from_bo(bo);
    if (!fb) {
        gbm_surface_release_buffer(drm.gbm_surface, bo);
        return;
    }

    if (drm.first_frame) {
        /* First frame: set the mode directly */
        int ret = drmModeSetCrtc(drm.drm_fd, drm.crtc_id, fb, 0, 0,
                                 &drm.connector_id, 1, &drm.mode);
        if (ret) {
            fprintf(stderr, "clue-drm: drmModeSetCrtc failed: %s\n",
                    strerror(errno));
        }
        drm.first_frame = false;
    } else {
        /* Subsequent frames: async page flip */
        drm.flip_pending = true;
        int ret = drmModePageFlip(drm.drm_fd, drm.crtc_id, fb,
                                  DRM_MODE_PAGE_FLIP_EVENT, NULL);
        if (ret) {
            fprintf(stderr, "clue-drm: drmModePageFlip failed: %s\n",
                    strerror(errno));
            drm.flip_pending = false;
        }

        /* Wait for page flip completion */
        while (drm.flip_pending) {
            struct pollfd pfd = {
                .fd     = drm.drm_fd,
                .events = POLLIN,
            };
            int pr = poll(&pfd, 1, 100); /* 100ms timeout */
            if (pr > 0 && (pfd.revents & POLLIN)) {
                drmEventContext evctx = {0};
                evctx.version = 2;
                evctx.page_flip_handler = page_flip_handler;
                drmHandleEvent(drm.drm_fd, &evctx);
            } else if (pr == 0) {
                /* Timeout -- break to avoid hanging */
                drm.flip_pending = false;
            }
        }
    }

    /* Release previous buffer */
    if (drm.prev_bo) {
        gbm_surface_release_buffer(drm.gbm_surface, drm.prev_bo);
    }
    drm.prev_bo = bo;
    drm.prev_fb = fb;
}

static int drm_poll_events(UIEvent *events, int max)
{
    if (!events || max <= 0) return 0;

    /* Poll both libinput and DRM fds, non-blocking */
    struct pollfd fds[2];
    int nfds = 0;

    if (drm.libinput) {
        fds[nfds].fd      = libinput_get_fd(drm.libinput);
        fds[nfds].events  = POLLIN;
        fds[nfds].revents = 0;
        nfds++;
    }

    fds[nfds].fd      = drm.drm_fd;
    fds[nfds].events  = POLLIN;
    fds[nfds].revents = 0;
    int drm_idx = nfds;
    nfds++;

    poll(fds, nfds, 0); /* non-blocking */

    /* --- Process libinput events --- */
    if (drm.libinput && (fds[0].revents & POLLIN)) {
        libinput_dispatch(drm.libinput);
        struct libinput_event *ev;
        while ((ev = libinput_get_event(drm.libinput)) != NULL) {
            process_libinput_event(ev);
            libinput_event_destroy(ev);
        }
    }

    /* --- Process DRM events (page flip completion) --- */
    if (fds[drm_idx].revents & POLLIN) {
        drmEventContext evctx = {0};
        evctx.version = 2;
        evctx.page_flip_handler = page_flip_handler;
        drmHandleEvent(drm.drm_fd, &evctx);
    }

    /* --- Drain event queue into caller's array --- */
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
 * TODO: Hardware cursor via drmModeSetCursor2 -- currently software only
 * TODO: Atomic modesetting (drmModeAtomicCommit) vs legacy drmModePageFlip
 * TODO: HDR metadata
 * TODO: Multiple displays / connectors
 * TODO: DPMS / screen blanking
 * TODO: Multi-touch proper handling (ABS_MT_ slots)
 */

UIBackend drm_backend = {
    .init           = drm_init,
    .shutdown       = drm_shutdown,
    .create_window  = drm_create_window,
    .destroy_window = drm_destroy_window,
    .make_current   = drm_make_current,
    .swap_buffers   = drm_swap_buffers,
    .poll_events    = drm_poll_events,
    .native_display = NULL,
    .native_window  = NULL,
};
