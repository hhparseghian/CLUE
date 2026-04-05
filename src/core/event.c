#include "clue/clue.h"

/* Dispatch an event to the appropriate window/widget tree.
 * Returns 1 if the event was consumed, 0 otherwise. */
int clue_event_dispatch(ClueEvent *event)
{
    if (!event || event->type == CLUE_EVENT_NONE) return 0;

    ClueWindow *win = event->window;
    if (!win) return 0;

    /* Handle close events at the window manager level */
    if (event->type == CLUE_EVENT_CLOSE) {
        return 1;
    }

    /* Handle resize events by updating window dimensions */
    if (event->type == CLUE_EVENT_RESIZE) {
        win->w     = event->resize.w;
        win->h     = event->resize.h;
        win->dirty = true;
        return 1;
    }

    /* Deliver to the widget tree if present */
    if (win->widget_root) {
        return clue_widget_dispatch_event((UIWidget *)win->widget_root, event);
    }

    return 0;
}
