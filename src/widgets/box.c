#include <stdlib.h>
#include "clue/box.h"
#include "clue/draw.h"

static void box_draw(ClueWidget *w)
{
    /* Draw background if set */
    if (w->style.bg_color.a > 0.001f) {
        clue_fill_rounded_rect(w->base.x, w->base.y, w->base.w, w->base.h,
                               w->style.corner_radius, w->style.bg_color);
    }
    /* Children are drawn by clue_cwidget_draw_tree after this returns */
}

static void box_layout(ClueWidget *w)
{
    ClueBox *box = (ClueBox *)w;
    ClueStyle *s = &w->style;

    int cx = w->base.x + s->padding_left;
    int cy = w->base.y + s->padding_top;
    int max_cross = 0;  /* max size in the cross axis */
    int total_main = 0; /* total size in the main axis */

    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        if (!child->base.visible) continue;

        /* Add spacing before all but the first child */
        if (i > 0) {
            if (box->orientation == CLUE_VERTICAL)
                cy += box->spacing;
            else
                cx += box->spacing;
            total_main += box->spacing;
        }

        /* Apply child margin */
        int child_x = cx + child->style.margin_left;
        int child_y = cy + child->style.margin_top;
        child->base.x = child_x;
        child->base.y = child_y;

        int child_w = child->base.w + child->style.margin_left + child->style.margin_right;
        int child_h = child->base.h + child->style.margin_top + child->style.margin_bottom;

        if (box->orientation == CLUE_VERTICAL) {
            cy += child_h;
            total_main += child_h;
            if (child_w > max_cross) max_cross = child_w;
        } else {
            cx += child_w;
            total_main += child_w;
            if (child_h > max_cross) max_cross = child_h;
        }
    }

    /* Size the box to contain all children */
    int pad_h = s->padding_left + s->padding_right;
    int pad_v = s->padding_top + s->padding_bottom;

    if (box->orientation == CLUE_VERTICAL) {
        w->base.w = max_cross + pad_h;
        w->base.h = total_main + pad_v;
    } else {
        w->base.w = total_main + pad_h;
        w->base.h = max_cross + pad_v;
    }
}

static const ClueWidgetVTable box_vtable = {
    .draw         = box_draw,
    .layout       = box_layout,
    .handle_event = NULL,
    .destroy      = NULL,
};

ClueBox *clue_box_new(ClueBoxOrientation orientation, int spacing)
{
    ClueBox *b = calloc(1, sizeof(ClueBox));
    if (!b) return NULL;

    clue_cwidget_init(&b->base, &box_vtable);
    b->orientation = orientation;
    b->spacing     = spacing;

    return b;
}
