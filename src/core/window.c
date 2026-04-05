#include <stdlib.h>
#include <string.h>
#include "clue/clue.h"

/* Initial capacity for the window list */
#define WM_INITIAL_CAPACITY 8

/* Global window manager singleton */
static ClueWindowManager g_wm = {0};

/* Get the global window manager singleton */
ClueWindowManager *clue_wm_get(void)
{
    return &g_wm;
}

/* Create a new window with the given dimensions and title.
 * Returns NULL on failure. */
ClueWindow *clue_window_create(int w, int h, const char *title)
{
    ClueWindowManager *wm = &g_wm;

    /* Grow the window list if needed */
    if (wm->count >= wm->capacity) {
        int new_cap = wm->capacity == 0 ? WM_INITIAL_CAPACITY : wm->capacity * 2;
        ClueWindow **new_list = realloc(wm->windows, sizeof(ClueWindow *) * new_cap);
        if (!new_list) return NULL;
        wm->windows  = new_list;
        wm->capacity = new_cap;
    }

    ClueWindow *win = calloc(1, sizeof(ClueWindow));
    if (!win) return NULL;

    win->w       = w;
    win->h       = h;
    win->title   = title;
    win->visible = true;
    win->dirty   = true;
    win->type    = CLUE_WINDOW_NORMAL;

    /* Ask the backend to create the native window */
    if (wm->backend && wm->backend->create_window) {
        if (wm->backend->create_window(win) != 0) {
            free(win);
            return NULL;
        }
    }

    wm->windows[wm->count++] = win;

    /* First window gets focus by default */
    if (!wm->focused) {
        wm->focused = win;
    }

    return win;
}

/* Destroy a window and free its resources */
void clue_window_destroy(ClueWindow *win)
{
    if (!win) return;

    ClueWindowManager *wm = &g_wm;

    /* Let the backend clean up native resources */
    if (wm->backend && wm->backend->destroy_window) {
        wm->backend->destroy_window(win);
    }

    /* Remove from the window list */
    for (int i = 0; i < wm->count; i++) {
        if (wm->windows[i] == win) {
            memmove(&wm->windows[i], &wm->windows[i + 1],
                    sizeof(ClueWindow *) * (wm->count - i - 1));
            wm->count--;
            break;
        }
    }

    /* Update focus if needed */
    if (wm->focused == win) {
        wm->focused = wm->count > 0 ? wm->windows[0] : NULL;
    }

    free(win);
}

/* Set the window type (NORMAL, DIALOG, POPUP) */
void clue_window_set_type(ClueWindow *win, ClueWindowType type)
{
    if (!win) return;
    win->type = type;
    /* Re-apply backend hints if window already created */
    ClueWindowManager *wm = &g_wm;
    if (wm->backend && wm->backend->set_window_type)
        wm->backend->set_window_type(win, type);
}

/* Set the parent window (required for DIALOG and POPUP types) */
void clue_window_set_parent(ClueWindow *win, ClueWindow *parent)
{
    if (!win) return;
    win->parent = parent;
    ClueWindowManager *wm = &g_wm;
    if (wm->backend && wm->backend->set_window_parent)
        wm->backend->set_window_parent(win, parent);
}

void clue_window_set_position(ClueWindow *win, int x, int y)
{
    if (!win) return;
    win->x = x;
    win->y = y;
    ClueWindowManager *wm = &g_wm;
    if (wm->backend && wm->backend->set_window_position)
        wm->backend->set_window_position(win, x, y);
}

/* Show or hide a window */
void clue_window_set_visible(ClueWindow *win, bool visible)
{
    if (win) {
        win->visible = visible;
        win->dirty   = true;
    }
}

/* Mark a window as needing a redraw */
void clue_window_mark_dirty(ClueWindow *win)
{
    if (win) win->dirty = true;
}

/* Make a window's GL context current for rendering */
void clue_window_make_current(ClueWindow *win)
{
    ClueWindowManager *wm = &g_wm;
    if (win && wm->backend && wm->backend->make_current) {
        wm->backend->make_current(win);
    }
}

/* Swap the front/back buffers for a window */
void clue_window_swap_buffers(ClueWindow *win)
{
    ClueWindowManager *wm = &g_wm;
    if (win && wm->backend && wm->backend->swap_buffers) {
        wm->backend->swap_buffers(win);
        win->dirty = false;
    }
}

/* Set the mouse cursor shape */
void clue_window_set_cursor(ClueWindow *win, ClueCursorShape cursor)
{
    ClueWindowManager *wm = &g_wm;
    if (win && wm->backend && wm->backend->set_cursor)
        wm->backend->set_cursor(win, (int)cursor);
}

/* Initialise CLUE: selects a backend and sets up the window manager.
 * Returns 0 on success, -1 on failure. */
int clue_init(void)
{
    memset(&g_wm, 0, sizeof(g_wm));

    UIBackend *backend = clue_select_backend();
    if (!backend) return -1;

    g_wm.backend = backend;

    if (backend->init && backend->init() != 0) {
        return -1;
    }

    return 0;
}

/* Shut down CLUE: destroys all windows and releases backend resources */
void clue_shutdown(void)
{
    /* Destroy all windows in reverse order */
    while (g_wm.count > 0) {
        clue_window_destroy(g_wm.windows[g_wm.count - 1]);
    }

    free(g_wm.windows);

    if (g_wm.backend && g_wm.backend->shutdown) {
        g_wm.backend->shutdown();
    }

    memset(&g_wm, 0, sizeof(g_wm));
}

/* Poll for pending events. Fills the events array up to max entries.
 * Returns the number of events written. */
int clue_poll_events(ClueEvent *events, int max)
{
    if (!g_wm.backend || !g_wm.backend->poll_events) return 0;
    return g_wm.backend->poll_events(events, max);
}
