#include <stdlib.h>
#include "clue/clue_widget.h"

/* Event trampoline: bridges UIWidget dispatch to ClueWidget vtable */
static int cwidget_event_handler(UIWidget *base, UIEvent *event)
{
    ClueWidget *w = (ClueWidget *)base;
    if (w->vtable && w->vtable->handle_event) {
        return w->vtable->handle_event(w, event);
    }
    return 0;
}

void clue_cwidget_init(ClueWidget *w, const ClueWidgetVTable *vt)
{
    if (!w) return;
    clue_widget_init(&w->base);
    w->vtable  = vt;
    w->type_id = CLUE_WIDGET_GENERIC;
    w->style   = CLUE_STYLE_DEFAULT;
    w->signals = NULL;
    w->base.on_event = cwidget_event_handler;
}

void clue_cwidget_destroy(ClueWidget *w)
{
    if (!w) return;

    /* Destroy children first (depth-first) */
    for (int i = 0; i < w->base.child_count; i++) {
        clue_cwidget_destroy((ClueWidget *)w->base.children[i]);
    }
    free(w->base.children);
    w->base.children = NULL;
    w->base.child_count = 0;
    w->base.child_capacity = 0;

    /* Disconnect signals */
    clue_signal_disconnect_all(w);

    /* Call type-specific destroy */
    if (w->vtable && w->vtable->destroy) {
        w->vtable->destroy(w);
    }

    free(w);
}

void clue_container_add(ClueWidget *parent, ClueWidget *child)
{
    if (!parent || !child) return;
    clue_widget_add_child(&parent->base, &child->base);
}

void clue_container_remove(ClueWidget *parent, ClueWidget *child)
{
    if (!parent || !child) return;
    clue_widget_remove_child(&parent->base, &child->base);
}

void clue_cwidget_draw_tree(ClueWidget *w)
{
    if (!w || !w->base.visible) return;

    if (w->vtable && w->vtable->draw) {
        w->vtable->draw(w);
    }

    /* Skip children if widget draws them itself (e.g. scroll container) */
    if (w->base.skip_child_draw) return;

    for (int i = 0; i < w->base.child_count; i++) {
        clue_cwidget_draw_tree((ClueWidget *)w->base.children[i]);
    }
}

void clue_cwidget_layout_tree(ClueWidget *w)
{
    if (!w) return;

    /* Layout children first (bottom-up) */
    for (int i = 0; i < w->base.child_count; i++) {
        clue_cwidget_layout_tree((ClueWidget *)w->base.children[i]);
    }

    if (w->vtable && w->vtable->layout) {
        w->vtable->layout(w);
    }
}
