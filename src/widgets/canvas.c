#include <stdlib.h>
#include <GLES2/gl2.h>
#include "clue/canvas.h"
#include "clue/draw.h"
#include "clue/theme.h"
#include "clue/app.h"
#include "clue/window.h"

/* GL state we save/restore so user 3D calls don't break CLUE rendering */
typedef struct {
    GLint viewport[4];
    GLint program;
    GLint blend_enabled;
    GLint blend_src, blend_dst;
    GLint depth_test;
    GLint cull_face;
    GLint scissor_test;
    GLint scissor_box[4];
} GLSavedState;

static void gl_save(GLSavedState *s)
{
    glGetIntegerv(GL_VIEWPORT, s->viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &s->program);
    s->blend_enabled = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &s->blend_src);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &s->blend_dst);
    s->depth_test = glIsEnabled(GL_DEPTH_TEST);
    s->cull_face = glIsEnabled(GL_CULL_FACE);
    s->scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_SCISSOR_BOX, s->scissor_box);
}

static void gl_restore(const GLSavedState *s)
{
    glViewport(s->viewport[0], s->viewport[1], s->viewport[2], s->viewport[3]);
    glUseProgram(s->program);
    if (s->blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    glBlendFunc(s->blend_src, s->blend_dst);
    if (s->depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (s->cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (s->scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glScissor(s->scissor_box[0], s->scissor_box[1], s->scissor_box[2], s->scissor_box[3]);
}

static void canvas_draw(ClueWidget *w)
{
    ClueCanvas *c = (ClueCanvas *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    if (c->draw_background) {
        ClueColor bg = w->style.bg_color.a > 0.001f ? w->style.bg_color : th->surface;
        clue_fill_rect(x, y, bw, bh, bg);
        clue_draw_rect(x, y, bw, bh, 1.0f, th->surface_border);
    }

    /* User draw callback */
    if (c->draw_cb) {
        GLSavedState saved;
        gl_save(&saved);

        clue_set_clip_rect(x, y, bw, bh);
        c->draw_cb(x, y, bw, bh, c->draw_data);
        clue_reset_clip_rect();

        gl_restore(&saved);
    }
}

static void canvas_layout(ClueWidget *w)
{
    if (w->base.w == 0) w->base.w = 200;
    if (w->base.h == 0) w->base.h = 150;
}

static void fire_event(ClueCanvas *c, ClueCanvasEvent *ev)
{
    if (c->event_cb)
        c->event_cb(ev, c->event_data);
}

static int canvas_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueCanvas *c = (ClueCanvas *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    if (event->type == CLUE_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x, my = event->mouse_move.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;
        if (inside && event->window)
            clue_window_set_cursor(event->window, CLUE_CURSOR_CROSSHAIR);

        if (c->painting || inside) {
            ClueCanvasEvent ev = {0};
            ev.type = CLUE_CANVAS_MOTION;
            ev.x = mx - x;
            ev.y = my - y;
            ev.dx = (c->last_mx >= 0) ? mx - c->last_mx : 0;
            ev.dy = (c->last_my >= 0) ? my - c->last_my : 0;
            c->last_mx = mx;
            c->last_my = my;
            fire_event(c, &ev);
            if (c->painting) return 1;
        }
        return 0;
    }

    if (event->type == CLUE_EVENT_MOUSE_BUTTON) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;

        if (event->mouse_button.pressed && inside) {
            if (c->focusable_canvas)
                clue_focus_widget(&w->base);

            /* Only capture mouse for left button (painting/dragging) */
            if (event->mouse_button.btn == 0) {
                c->painting = true;
                c->last_mx = mx;
                c->last_my = my;
                clue_capture_mouse(&w->base);
            }

            ClueCanvasEvent ev = {0};
            ev.type = CLUE_CANVAS_PRESS;
            ev.x = mx - x;
            ev.y = my - y;
            ev.button = event->mouse_button.btn;
            fire_event(c, &ev);
            return 1;
        }
        if (!event->mouse_button.pressed && c->painting && event->mouse_button.btn == 0) {
            c->painting = false;
            clue_release_mouse();

            ClueCanvasEvent ev = {0};
            ev.type = CLUE_CANVAS_RELEASE;
            ev.x = mx - x;
            ev.y = my - y;
            ev.button = event->mouse_button.btn;
            fire_event(c, &ev);
            c->last_mx = -1;
            c->last_my = -1;
            return 1;
        }
    }

    if (event->type == CLUE_EVENT_MOUSE_SCROLL) {
        int mx = event->mouse_scroll.x, my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            ClueCanvasEvent ev = {0};
            ev.type = CLUE_CANVAS_SCROLL;
            ev.x = mx - x;
            ev.y = my - y;
            ev.scroll_x = event->mouse_scroll.dx;
            ev.scroll_y = event->mouse_scroll.dy;
            fire_event(c, &ev);
            return 1;
        }
    }

    if (event->type == CLUE_EVENT_KEY && c->focusable_canvas && w->base.focused) {
        ClueCanvasEvent ev = {0};
        ev.type = CLUE_CANVAS_KEY;
        ev.keycode = event->key.keycode;
        ev.modifiers = event->key.modifiers;
        ev.pressed = event->key.pressed;
        fire_event(c, &ev);
        return 1;
    }

    return 0;
}

static const ClueWidgetVTable canvas_vtable = {
    .draw         = canvas_draw,
    .layout       = canvas_layout,
    .handle_event = canvas_handle_event,
};

ClueCanvas *clue_canvas_new(int w, int h)
{
    ClueCanvas *c = calloc(1, sizeof(ClueCanvas));
    if (!c) return NULL;

    clue_cwidget_init(&c->base, &canvas_vtable);
    c->base.type_id = CLUE_WIDGET_CANVAS;
    c->base.base.w = w;
    c->base.base.h = h;
    c->last_mx = -1;
    c->last_my = -1;

    return c;
}

void clue_canvas_set_draw(ClueCanvas *c, ClueCanvasDrawCb cb, void *user_data)
{
    if (!c) return;
    c->draw_cb   = cb;
    c->draw_data = user_data;
}

void clue_canvas_set_event(ClueCanvas *c, ClueCanvasEventCb cb, void *user_data)
{
    if (!c) return;
    c->event_cb   = cb;
    c->event_data = user_data;
}

void clue_canvas_set_focusable(ClueCanvas *c, bool focusable)
{
    if (!c) return;
    c->focusable_canvas = focusable;
    c->base.base.focusable = focusable;
}

