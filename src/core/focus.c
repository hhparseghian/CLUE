#include <stddef.h>
#include "clue/clue.h"

/* Set keyboard focus to a specific window */
void clue_focus_window(ClueWindow *win)
{
    ClueWindowManager *wm = clue_wm_get();
    if (wm && win && win->visible) {
        wm->focused = win;
    }
}

/* Get the currently focused window, or NULL if none */
ClueWindow *clue_focus_get_window(void)
{
    ClueWindowManager *wm = clue_wm_get();
    return wm ? wm->focused : NULL;
}

/* Cycle focus to the next window in the window list.
 * Wraps around to the first window after the last. */
void clue_focus_next_window(void)
{
    ClueWindowManager *wm = clue_wm_get();
    if (!wm || wm->count == 0) return;

    int idx = 0;
    for (int i = 0; i < wm->count; i++) {
        if (wm->windows[i] == wm->focused) {
            idx = (i + 1) % wm->count;
            break;
        }
    }

    wm->focused = wm->windows[idx];
}
