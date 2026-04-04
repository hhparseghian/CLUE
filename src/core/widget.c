#include <stdlib.h>
#include "clue/clue.h"

#define WIDGET_INITIAL_CAPACITY 4

void clue_widget_init(UIWidget *widget)
{
    if (!widget) return;
    widget->x                = 0;
    widget->y                = 0;
    widget->w                = 0;
    widget->h                = 0;
    widget->visible          = true;
    widget->focusable        = false;
    widget->focused          = false;
    widget->intercept_events = false;
    widget->skip_child_draw  = false;
    widget->parent           = NULL;
    widget->children         = NULL;
    widget->child_count      = 0;
    widget->child_capacity   = 0;
    widget->on_event         = NULL;
    widget->user_data        = NULL;
}

int clue_widget_add_child(UIWidget *parent, UIWidget *child)
{
    if (!parent || !child) return -1;

    if (parent->child_count >= parent->child_capacity) {
        int new_cap = parent->child_capacity == 0
            ? WIDGET_INITIAL_CAPACITY
            : parent->child_capacity * 2;
        UIWidget **new_list = realloc(parent->children, sizeof(UIWidget *) * new_cap);
        if (!new_list) return -1;
        parent->children       = new_list;
        parent->child_capacity = new_cap;
    }

    child->parent = parent;
    parent->children[parent->child_count++] = child;
    return 0;
}

int clue_widget_remove_child(UIWidget *parent, UIWidget *child)
{
    if (!parent || !child) return -1;

    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            for (int j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            child->parent = NULL;
            return 0;
        }
    }

    return -1;
}

int clue_widget_dispatch_event(UIWidget *widget, UIEvent *event)
{
    if (!widget || !widget->visible) return 0;

    /* If widget intercepts events, let it handle before children */
    if (widget->intercept_events && widget->on_event) {
        int r = widget->on_event(widget, event);
        if (r) return 1;
    }

    /* Try children (front-to-back, last child is on top) */
    for (int i = widget->child_count - 1; i >= 0; i--) {
        if (clue_widget_dispatch_event(widget->children[i], event)) {
            return 1;
        }
    }

    /* Then try this widget's own handler (if not already called) */
    if (!widget->intercept_events && widget->on_event) {
        return widget->on_event(widget, event);
    }

    return 0;
}
