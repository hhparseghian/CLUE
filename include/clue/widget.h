#ifndef CLUE_WIDGET_H
#define CLUE_WIDGET_H

#include <stdbool.h>
#include "event.h"

/* Forward declarations */
struct UIWidget;

/* Widget event handler callback */
typedef int (*UIWidgetEventHandler)(struct UIWidget *widget, UIEvent *event);

/* Base widget -- all UI elements derive from this */
typedef struct UIWidget {
    int                 x, y, w, h;
    bool                visible;
    bool                focusable;
    bool                focused;
    bool                intercept_events;  /* handle events before children */
    bool                skip_child_draw;   /* container draws children itself */
    struct UIWidget    *parent;
    struct UIWidget   **children;
    int                 child_count;
    int                 child_capacity;
    UIWidgetEventHandler on_event;
    void               *user_data;
} UIWidget;

/* Initialise a widget with default values */
void clue_widget_init(UIWidget *widget);

/* Add a child widget. Returns 0 on success, -1 on failure. */
int clue_widget_add_child(UIWidget *parent, UIWidget *child);

/* Remove a child widget. Returns 0 on success, -1 if not found. */
int clue_widget_remove_child(UIWidget *parent, UIWidget *child);

/* Deliver an event to a widget and its children (depth-first).
 * Returns 1 if the event was consumed, 0 otherwise. */
int clue_widget_dispatch_event(UIWidget *widget, UIEvent *event);

#endif /* CLUE_WIDGET_H */
