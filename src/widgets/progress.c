#include <stdlib.h>
#include <stdio.h>
#include "clue/progress.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define BAR_H 10
#define LABEL_MARGIN 45

static void progress_draw(ClueWidget *w)
{
    ClueProgress *p = (ClueProgress *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w - LABEL_MARGIN;
    int bar_y = y;

    float v = p->value;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    /* Track */
    clue_fill_rounded_rect(x, bar_y, bw, BAR_H,
                           BAR_H / 2.0f, th->slider.track);

    /* Fill */
    int fill_w = (int)(v * bw);
    if (fill_w > BAR_H) {
        clue_fill_rounded_rect(x, bar_y, fill_w, BAR_H,
                               BAR_H / 2.0f, th->accent);
    } else if (fill_w > 0) {
        clue_fill_rounded_rect(x, bar_y, BAR_H, BAR_H,
                               BAR_H / 2.0f, th->accent);
    }

    /* Percentage text (drawn inside the reserved right margin) */
    ClueFont *font = clue_app_default_font();
    if (font) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", (int)(v * 100.0f));
        int tx = x + bw + 8;
        int ty = bar_y + (BAR_H - clue_font_line_height(font)) / 2;
        clue_draw_text(tx, ty, buf, font, th->fg_dim);
    }
}

static void progress_layout(ClueWidget *w)
{
    if (w->base.w == 0) w->base.w = 200 + LABEL_MARGIN;
    w->base.h = BAR_H;
}

static const ClueWidgetVTable progress_vtable = {
    .draw = progress_draw, .layout = progress_layout,
    .handle_event = NULL, .destroy = NULL,
};

ClueProgress *clue_progress_new(void)
{
    ClueProgress *p = calloc(1, sizeof(ClueProgress));
    if (!p) return NULL;
    clue_cwidget_init(&p->base, &progress_vtable);
    progress_layout(&p->base);
    return p;
}

void clue_progress_set_value(ClueProgress *p, float value)
{
    if (!p) return;
    p->value = value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}

float clue_progress_get_value(ClueProgress *p)
{
    return p ? p->value : 0.0f;
}

void clue_progress_set_indeterminate(ClueProgress *p, bool on)
{
    if (p) p->indeterminate = on;
}
