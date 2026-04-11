#ifndef CLUE_DND_H
#define CLUE_DND_H

#include <stdbool.h>
#include "clue_widget.h"

/*
 * Drag-and-drop system for CLUE.
 *
 * Signals emitted during drag operations:
 *   "drag-begin" — on the source widget when the drag starts
 *   "drag-end"   — on the source widget when the drag completes or is cancelled
 *   "drop"       — on the target widget when a widget is dropped on it
 *
 * Usage:
 *   clue_widget_set_draggable(CLUE_W(my_button), true);
 *   clue_widget_set_drop_target(CLUE_W(my_box), true);
 *   clue_signal_connect(my_box, "drop", on_drop, NULL);
 */

/* Drag event — accessible from signal handlers during a drag */
typedef struct {
    ClueWidget *source;         /* widget being dragged */
    ClueWidget *target;         /* drop target under cursor (NULL if none) */
    int         start_x, start_y;
    int         mouse_x, mouse_y;
    bool        accepted;       /* set to true in "drop" handler to accept */
} ClueDragEvent;

/* Mark a widget as draggable */
void clue_widget_set_draggable(ClueWidget *w, bool draggable);

/* Mark a widget as a drop target */
void clue_widget_set_drop_target(ClueWidget *w, bool target);

/* Get the current drag event (valid during drag signals) */
const ClueDragEvent *clue_drag_get_event(void);

/* Cancel an in-progress drag (returns widget to original position) */
void clue_drag_cancel(void);

/* Check if a drag is in progress */
bool clue_drag_active(void);

/* --- Internal: called by app.c event loop --- */

/* Process a raw event for drag-and-drop. Returns 1 if consumed. */
int clue_dnd_handle_event(ClueEvent *event);

/* Draw the drag ghost overlay (call after all other drawing) */
void clue_dnd_draw(void);

#endif /* CLUE_DND_H */
