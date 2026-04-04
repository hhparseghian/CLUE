#include <stdlib.h>
#include "clue/separator.h"
#include "clue/draw.h"
#include "clue/theme.h"

#define SEP_THICKNESS 1
#define SEP_MIN_SIZE  8

static void separator_draw(ClueWidget *w)
{
    ClueSeparator *s = (ClueSeparator *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;

    if (s->orientation == CLUE_HORIZONTAL) {
        int cy = y + w->base.h / 2;
        clue_fill_rect_solid(x, cy, w->base.w, SEP_THICKNESS, th->surface_border);
    } else {
        int cx = x + w->base.w / 2;
        clue_fill_rect_solid(cx, y, SEP_THICKNESS, w->base.h, th->surface_border);
    }
}

static void separator_layout(ClueWidget *w)
{
    ClueSeparator *s = (ClueSeparator *)w;

    if (s->orientation == CLUE_HORIZONTAL) {
        w->base.w = 0; /* reset so parent can set correct width */
        w->base.h = SEP_MIN_SIZE;
        w->style.hexpand = true;
    } else {
        w->base.w = SEP_MIN_SIZE;
        w->base.h = 0;
        w->style.vexpand = true;
    }
}

static const ClueWidgetVTable separator_vtable = {
    .draw         = separator_draw,
    .layout       = separator_layout,
    .handle_event = NULL,
    .destroy      = NULL,
};

ClueSeparator *clue_separator_new(ClueBoxOrientation orientation)
{
    ClueSeparator *s = calloc(1, sizeof(ClueSeparator));
    if (!s) return NULL;

    clue_cwidget_init(&s->base, &separator_vtable);
    s->orientation = orientation;

    separator_layout(&s->base);
    return s;
}
