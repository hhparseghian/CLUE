#define CLUE_IMPL
#include <stdlib.h>
#include <string.h>
#include "clue/dnd.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/draw.h"
#include "clue/theme.h"
#include "clue/signal.h"
#include "clue/clue_widget.h"
#include "clue/scroll.h"
#include "clue/tabs.h"

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */

#define DRAG_THRESHOLD  5   /* pixels before drag starts */
#define GHOST_ALPHA     0.5f

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

typedef enum {
    DND_IDLE,
    DND_PENDING,    /* mouse down on draggable, waiting for threshold */
    DND_DRAGGING,   /* actively dragging */
} DndPhase;

typedef struct {
    DndPhase     phase;
    ClueDragEvent event;

    /* Original widget position and parent (for cancel) */
    UIWidget    *orig_parent;
    int          orig_x, orig_y;
    int          orig_child_index;

    /* Dummy widget to receive captured mouse events */
    UIWidget     capture_target;
} DndState;

static DndState g_dnd = {0};

/* ------------------------------------------------------------------ */
/* Hit testing                                                         */
/* ------------------------------------------------------------------ */

/* Find the deepest widget at (x, y) that has the given flag set.
 * Searches children in reverse order (topmost first).
 * ox/oy accumulate scroll offsets from parent scroll containers. */
static ClueWidget *hit_test_impl(ClueWidget *w, int x, int y,
                                 int ox, int oy, bool want_draggable)
{
    if (!w || !w->base.visible) return NULL;

    int wx = w->base.x + ox, wy = w->base.y + oy;
    int ww = w->base.w, wh = w->base.h;

    if (x < wx || x >= wx + ww || y < wy || y >= wy + wh)
        return NULL;

    /* If this is a scroll container, adjust offset for children */
    int cox = ox, coy = oy;
    if (w->type_id == CLUE_WIDGET_SCROLL) {
        ClueScroll *s = (ClueScroll *)w;
        cox -= s->scroll_x;
        coy -= s->scroll_y;
    }

    /* Tabs store active page separately from children */
    if (w->type_id == CLUE_WIDGET_TABS) {
        ClueTabs *tabs = (ClueTabs *)w;
        int active = clue_tabs_get_active(tabs);
        if (active >= 0 && active < tabs->tab_count && tabs->tab_pages[active]) {
            ClueWidget *found = hit_test_impl(tabs->tab_pages[active],
                                              x, y, cox, coy, want_draggable);
            if (found) return found;
        }
    }

    /* Check children first (deepest match wins) */
    for (int i = w->base.child_count - 1; i >= 0; i--) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        ClueWidget *found = hit_test_impl(child, x, y, cox, coy, want_draggable);
        if (found) return found;
    }

    /* Check this widget */
    if (want_draggable && w->base.draggable) return w;
    if (!want_draggable && w->base.drop_target) return w;

    return NULL;
}

static ClueWidget *hit_test_flag(ClueWidget *w, int x, int y, bool want_draggable)
{
    return hit_test_impl(w, x, y, 0, 0, want_draggable);
}

static int child_index(UIWidget *parent, UIWidget *child)
{
    if (!parent) return -1;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) return i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Capture event handler                                               */
/* ------------------------------------------------------------------ */

static int dnd_capture_handler(UIWidget *widget, ClueEvent *event)
{
    (void)widget;
    /* Forward to our main handler */
    return clue_dnd_handle_event(event);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void clue_widget_set_draggable(ClueWidget *w, bool draggable)
{
    if (w) w->base.draggable = draggable;
}

void clue_widget_set_drop_target(ClueWidget *w, bool target)
{
    if (w) w->base.drop_target = target;
}

const ClueDragEvent *clue_drag_get_event(void)
{
    return &g_dnd.event;
}

bool clue_drag_active(void)
{
    return g_dnd.phase == DND_DRAGGING;
}

void clue_drag_cancel(void)
{
    if (g_dnd.phase != DND_DRAGGING) {
        g_dnd.phase = DND_IDLE;
        return;
    }

    ClueWidget *src = g_dnd.event.source;

    /* Restore widget to original parent and position */
    if (src && src->base.parent) {
        clue_widget_remove_child(src->base.parent, &src->base);
    }
    if (src && g_dnd.orig_parent) {
        clue_widget_add_child(g_dnd.orig_parent, &src->base);
    }

    /* Restore original position */
    if (src) {
        src->base.x = g_dnd.orig_x;
        src->base.y = g_dnd.orig_y;
    }

    clue_release_mouse();
    clue_signal_emit(src, "drag-end");
    g_dnd.phase = DND_IDLE;
}

/* ------------------------------------------------------------------ */
/* Event processing                                                    */
/* ------------------------------------------------------------------ */

int clue_dnd_handle_event(ClueEvent *event)
{
    ClueApp *app = clue_app_get();
    if (!app || !app->root) return 0;

    switch (g_dnd.phase) {

    case DND_IDLE: {
        /* Look for mouse press on a draggable widget */
        if (event->type != CLUE_EVENT_MOUSE_BUTTON ||
            !event->mouse_button.pressed ||
            event->mouse_button.btn != 0)
            return 0;

        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;

        ClueWidget *hit = hit_test_flag(app->root, mx, my, true);
        if (!hit) return 0;
        /* Start pending drag */
        g_dnd.phase = DND_PENDING;
        g_dnd.event.source = hit;
        g_dnd.event.target = NULL;
        g_dnd.event.start_x = mx;
        g_dnd.event.start_y = my;
        g_dnd.event.mouse_x = mx;
        g_dnd.event.mouse_y = my;
        g_dnd.event.accepted = false;

        g_dnd.orig_parent = hit->base.parent;
        g_dnd.orig_x = hit->base.x;
        g_dnd.orig_y = hit->base.y;
        g_dnd.orig_child_index = child_index(hit->base.parent, &hit->base);

        return 0; /* don't consume yet — let normal click handling work */
    }

    case DND_PENDING: {
        /* Check for threshold */
        if (event->type == CLUE_EVENT_MOUSE_MOVE) {
            int mx = event->mouse_move.x;
            int my = event->mouse_move.y;
            int dx = mx - g_dnd.event.start_x;
            int dy = my - g_dnd.event.start_y;

            if (dx * dx + dy * dy >= DRAG_THRESHOLD * DRAG_THRESHOLD) {
                /* Threshold crossed — start actual drag */
                g_dnd.phase = DND_DRAGGING;
                g_dnd.event.mouse_x = mx;
                g_dnd.event.mouse_y = my;

                /* Set up capture */
                clue_widget_init(&g_dnd.capture_target);
                g_dnd.capture_target.on_event = dnd_capture_handler;
                clue_capture_mouse(&g_dnd.capture_target);

                clue_signal_emit(g_dnd.event.source, "drag-begin");
                clue_window_set_cursor(app->window, CLUE_CURSOR_MOVE);
                return 1;
            }
            return 0;
        }

        /* Mouse released before threshold — cancel pending */
        if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
            !event->mouse_button.pressed) {
            g_dnd.phase = DND_IDLE;
            return 0;
        }

        return 0;
    }

    case DND_DRAGGING: {
        if (event->type == CLUE_EVENT_MOUSE_MOVE) {
            int mx = event->mouse_move.x;
            int my = event->mouse_move.y;
            g_dnd.event.mouse_x = mx;
            g_dnd.event.mouse_y = my;

            /* Hit-test for drop targets */
            g_dnd.event.target = hit_test_flag(app->root, mx, my, false);
            return 1;
        }

        if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
            !event->mouse_button.pressed &&
            event->mouse_button.btn == 0) {

            ClueWidget *src = g_dnd.event.source;
            ClueWidget *tgt = g_dnd.event.target;

            clue_release_mouse();
            clue_window_set_cursor(app->window, CLUE_CURSOR_DEFAULT);

            if (tgt) {
                /* Emit "drop" on target — handler sets accepted */
                g_dnd.event.accepted = false;
                clue_signal_emit(tgt, "drop");

                if (g_dnd.event.accepted) {
                    /* Reparent: remove from old parent, add to new */
                    if (src->base.parent)
                        clue_widget_remove_child(src->base.parent, &src->base);
                    clue_widget_add_child(&tgt->base, &src->base);
                }
            }

            if (!g_dnd.event.accepted) {
                /* Return to original position */
                src->base.x = g_dnd.orig_x;
                src->base.y = g_dnd.orig_y;
            }

            clue_signal_emit(src, "drag-end");
            g_dnd.phase = DND_IDLE;
            return 1;
        }

        /* Escape to cancel */
        if (event->type == CLUE_EVENT_KEY &&
            event->key.pressed &&
            event->key.keycode == 0xff1b /* XKB_KEY_Escape */) {
            clue_drag_cancel();
            clue_window_set_cursor(app->window, CLUE_CURSOR_DEFAULT);
            return 1;
        }

        return 1; /* consume everything while dragging */
    }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Draw ghost                                                          */
/* ------------------------------------------------------------------ */

void clue_dnd_draw(void)
{
    if (g_dnd.phase != DND_DRAGGING) return;

    ClueWidget *src = g_dnd.event.source;
    if (!src) return;

    const ClueTheme *th = clue_theme_get();
    int mx = g_dnd.event.mouse_x;
    int my = g_dnd.event.mouse_y;

    /* Draw ghost at cursor position, offset by half widget size */
    int gw = src->base.w;
    int gh = src->base.h;
    int gx = mx - gw / 2;
    int gy = my - gh / 2;

    /* Ghost background */
    ClueColor ghost_bg = th->accent;
    ghost_bg.a = GHOST_ALPHA;
    clue_fill_rounded_rect(gx, gy, gw, gh, th->corner_radius, ghost_bg);

    /* Ghost border */
    ClueColor ghost_border = th->accent;
    ghost_border.a = 0.8f;
    clue_draw_rounded_rect(gx, gy, gw, gh, th->corner_radius, 2.0f, ghost_border);

    /* Draw drop target highlight */
    ClueWidget *tgt = g_dnd.event.target;
    if (tgt) {
        ClueColor hl = th->accent;
        hl.a = 0.25f;
        clue_fill_rounded_rect(tgt->base.x, tgt->base.y,
                               tgt->base.w, tgt->base.h,
                               th->corner_radius, hl);
        clue_draw_rounded_rect(tgt->base.x, tgt->base.y,
                               tgt->base.w, tgt->base.h,
                               th->corner_radius, 2.0f, th->accent);
    }
}
