#define _POSIX_C_SOURCE 200809L
#define CLUE_IMPL
#include <stdlib.h>
#include <string.h>
#include "clue/overlay.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/clue_widget.h"

#define TITLE_H   36
#define BTN_H     36
#define BTN_PAD   10
#define PANEL_PAD 12

/* Number of frames after show during which mouse-button events are
 * swallowed. Protects the overlay from being dismissed by a stray click
 * delivered in the same input batch as the click that opened it
 * (touchpad taps, synthesized clicks from accessibility tools, etc.). */
#define SETTLE_FRAMES 6

/* Cached panel rect from the last draw — used by handle_event to reject
 * mouse events outside the visible panel without routing them to widgets
 * that might still be at uninitialised positions. */
typedef struct {
    int x, y, w, h;
    int frames_since_show;  /* incremented each draw, reset on show */
} OverlayInternal;

/* ---- draw ---- */

/* Per-overlay state lives in a parallel structure keyed by overlay pointer.
 * We can't extend ClueOverlay's public struct without breaking ABI, so this
 * little map holds the runtime-only fields. Capacity is small because
 * overlays are usually 1-2 at a time. */
#define INTERNAL_CAP 16
static struct { ClueOverlay *ov; OverlayInternal data; } g_internal[INTERNAL_CAP];

static OverlayInternal *internal_for(ClueOverlay *ov)
{
    for (int i = 0; i < INTERNAL_CAP; i++)
        if (g_internal[i].ov == ov) return &g_internal[i].data;
    for (int i = 0; i < INTERNAL_CAP; i++) {
        if (g_internal[i].ov == NULL) {
            g_internal[i].ov = ov;
            g_internal[i].data = (OverlayInternal){0};
            return &g_internal[i].data;
        }
    }
    return NULL;
}

static void internal_drop(ClueOverlay *ov)
{
    for (int i = 0; i < INTERNAL_CAP; i++) {
        if (g_internal[i].ov == ov) {
            g_internal[i].ov = NULL;
            return;
        }
    }
}

static void overlay_draw(ClueWidget *w)
{
    ClueOverlay *ov = (ClueOverlay *)w;
    if (!ov->visible) return;

    ClueApp *app = clue_app_get();
    if (!app) return;

    OverlayInternal *oi = internal_for(ov);
    if (oi) oi->frames_since_show++;

    const ClueTheme *th = clue_theme_get();
    ClueFont *font = clue_app_default_font();

    int win_w = app->window->w;
    int win_h = app->window->h;

    /* Dim background */
    clue_fill_rect(0, 0, win_w, win_h, CLUE_RGBA(0, 0, 0, 160));

    /* Panel centered */
    int pw = ov->panel_w;
    int ph = ov->panel_h;
    int px = (win_w - pw) / 2;
    int py = (win_h - ph) / 2;

    /* Stash panel rect for handle_event bounds checking. */
    if (oi) { oi->x = px; oi->y = py; oi->w = pw; oi->h = ph; }

    clue_fill_rounded_rect(px, py, pw, ph, 8.0f, th->surface);
    clue_draw_rounded_rect(px, py, pw, ph, 8.0f, 1.0f, th->surface_border);

    /* Title bar */
    if (ov->title && font) {
        int tw = clue_font_text_width(font, ov->title);
        int th_h = clue_font_line_height(font);
        clue_draw_text(px + (pw - tw) / 2, py + (TITLE_H - th_h) / 2,
                       ov->title, font, th->fg);
        /* Separator line */
        clue_draw_line(px + PANEL_PAD, py + TITLE_H,
                       px + pw - PANEL_PAD, py + TITLE_H,
                       1.0f, th->surface_border);
    }

    /* Content */
    if (ov->content) {
        int cx = px + PANEL_PAD;
        int cy = py + TITLE_H + 4;
        int cw = pw - PANEL_PAD * 2;
        int ch = ph - TITLE_H - BTN_H - BTN_PAD * 2 - 4;

        ov->content->base.x = cx;
        ov->content->base.y = cy;
        ov->content->base.w = cw;
        ov->content->base.h = ch;
        if (ov->content->style.hexpand)
            ov->content->base.w = cw;
        if (ov->content->style.vexpand)
            ov->content->base.h = ch;

        clue_cwidget_layout_tree(ov->content);
        clue_cwidget_draw_tree(ov->content);
    }

    /* Buttons at bottom, right-aligned */
    int btn_y = py + ph - BTN_PAD - BTN_H;
    int btn_x = px + pw - BTN_PAD;
    for (int i = ov->button_count - 1; i >= 0; i--) {
        ClueWidget *bw = (ClueWidget *)ov->buttons[i];
        clue_cwidget_layout_tree(bw);
        btn_x -= bw->base.w;
        bw->base.x = btn_x;
        bw->base.y = btn_y;
        clue_cwidget_draw_tree(bw);
        btn_x -= BTN_PAD;
    }
}

/* ---- event handling ---- */

static int overlay_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueOverlay *ov = (ClueOverlay *)w;
    if (!ov->visible) return 0;

    OverlayInternal *oi = internal_for(ov);

    /* Mouse-button events get extra scrutiny:
     *   1. During the first SETTLE_FRAMES draws, ignore them entirely so a
     *      synthesised click delivered in the same input batch as the click
     *      that opened the overlay can't dismiss it instantly.
     *   2. After settling, only route them if they land inside the panel.
     *      A click on the dim background is consumed but not propagated. */
    if (event->type == CLUE_EVENT_MOUSE_BUTTON && oi) {
        if (oi->frames_since_show < SETTLE_FRAMES) {
            return 1;  /* still settling — swallow */
        }
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        bool inside_panel = (mx >= oi->x && mx < oi->x + oi->w &&
                             my >= oi->y && my < oi->y + oi->h);
        if (!inside_panel) {
            return 1;  /* click outside panel: consume silently */
        }
    }

    /* Route events to buttons */
    for (int i = 0; i < ov->button_count; i++) {
        if (clue_widget_dispatch_event(&ov->buttons[i]->base.base, event))
            return 1;
    }

    /* Route events to content */
    if (ov->content) {
        if (clue_widget_dispatch_event(&ov->content->base, event))
            return 1;
    }

    /* Consume all events (modal) */
    return 1;
}

/* ---- layout (fills entire window) ---- */

static void overlay_layout(ClueWidget *w)
{
    ClueApp *app = clue_app_get();
    if (app) {
        w->base.x = 0;
        w->base.y = 0;
        w->base.w = app->window->w;
        w->base.h = app->window->h;
    }
}

/* ---- destroy ---- */

static void overlay_destroy_impl(ClueWidget *w)
{
    ClueOverlay *ov = (ClueOverlay *)w;
    free(ov->title);
    if (ov->content) clue_cwidget_destroy(ov->content);
    for (int i = 0; i < ov->button_count; i++)
        clue_cwidget_destroy((ClueWidget *)ov->buttons[i]);
}

static const ClueWidgetVTable overlay_vtable = {
    .draw         = overlay_draw,
    .layout       = overlay_layout,
    .handle_event = overlay_handle_event,
    .destroy      = overlay_destroy_impl,
};

/* ---- button click handler ---- */

static void overlay_btn_clicked(void *widget, void *user_data)
{
    ClueOverlay *ov = (ClueOverlay *)user_data;

    for (int i = 0; i < ov->button_count; i++) {
        if ((void *)ov->buttons[i] == widget) {
            clue_overlay_dismiss(ov, ov->btn_results[i]);
            return;
        }
    }
}

/* ---- public API ---- */

ClueOverlay *clue_overlay_new(const char *title, int panel_w, int panel_h)
{
    ClueOverlay *ov = calloc(1, sizeof(ClueOverlay));
    if (!ov) return NULL;

    clue_cwidget_init(&ov->base, &overlay_vtable);
    ov->title   = title ? strdup(title) : NULL;
    ov->panel_w = panel_w;
    ov->panel_h = panel_h;
    ov->result  = CLUE_OVERLAY_NONE;
    ov->visible = false;

    return ov;
}

void clue_overlay_destroy(ClueOverlay *ov)
{
    if (!ov) return;
    if (ov->visible) clue_overlay_dismiss(ov, CLUE_OVERLAY_CANCEL);
    /* clue_cwidget_destroy frees the widget itself at the end, so no extra
     * free(ov) here. The vtable destroy (overlay_destroy_impl) handles the
     * overlay's owned children — content widget, buttons, title. */
    clue_cwidget_destroy((ClueWidget *)ov);
}

void clue_overlay_set_content(ClueOverlay *ov, ClueWidget *content)
{
    if (ov) ov->content = content;
}

void clue_overlay_add_button(ClueOverlay *ov, const char *label,
                             ClueOverlayResult result)
{
    if (!ov || ov->button_count >= CLUE_OVERLAY_MAX_BUTTONS) return;

    ClueButton *btn = clue_button_new(label);
    if (!btn) return;

    int idx = ov->button_count;
    ov->buttons[idx] = btn;
    ov->btn_results[idx] = result;
    ov->button_count++;

    clue_signal_connect(btn, "clicked", overlay_btn_clicked, ov);
}

void clue_overlay_set_callback(ClueOverlay *ov, ClueOverlayCallback cb,
                               void *user_data)
{
    if (!ov) return;
    ov->callback = cb;
    ov->callback_data = user_data;
}

void clue_overlay_show(ClueOverlay *ov)
{
    if (!ov) return;

    ClueApp *app = clue_app_get();
    if (!app) return;

    ov->visible = true;
    ov->result = CLUE_OVERLAY_NONE;

    /* Start the settle period fresh — the next SETTLE_FRAMES draws will
     * reject mouse-button events to protect against the click that opened
     * the overlay also landing on a panel button on the same frame. */
    OverlayInternal *oi = internal_for(ov);
    if (oi) *oi = (OverlayInternal){0};

    /* Set as modal so only overlay receives events */
    app->modal_widget = (ClueWidget *)ov;
}

void clue_overlay_dismiss(ClueOverlay *ov, ClueOverlayResult result)
{
    if (!ov || !ov->visible) return;

    ov->visible = false;
    ov->result = result;
    internal_drop(ov);

    /* Clear modal */
    ClueApp *app = clue_app_get();
    if (app && app->modal_widget == (ClueWidget *)ov)
        app->modal_widget = NULL;

    if (ov->callback)
        ov->callback(result, ov->callback_data);
}
