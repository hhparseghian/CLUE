#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/label.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"

static UIFont *label_font(ClueLabel *l)
{
    return l->base.style.font ? l->base.style.font : clue_app_default_font();
}

static void label_draw(ClueWidget *w)
{
    ClueLabel *l = (ClueLabel *)w;
    if (!l->text) return;

    ClueStyle *s = &w->style;

    /* Draw background if not transparent */
    if (s->bg_color.a > 0.001f) {
        clue_fill_rounded_rect(w->base.x, w->base.y, w->base.w, w->base.h,
                               s->corner_radius, s->bg_color);
    }

    UIFont *font = label_font(l);
    if (font) {
        clue_draw_text(w->base.x + s->padding_left,
                       w->base.y + s->padding_top,
                       l->text, font, s->fg_color);
    }
}

static void label_layout(ClueWidget *w)
{
    ClueLabel *l = (ClueLabel *)w;
    UIFont *font = label_font(l);
    if (!font || !l->text) return;

    ClueStyle *s = &w->style;
    w->base.w = clue_font_text_width(font, l->text)
                + s->padding_left + s->padding_right;
    w->base.h = clue_font_line_height(font)
                + s->padding_top + s->padding_bottom;
}

static void label_destroy(ClueWidget *w)
{
    ClueLabel *l = (ClueLabel *)w;
    free(l->text);
    l->text = NULL;
}

static const ClueWidgetVTable label_vtable = {
    .draw         = label_draw,
    .layout       = label_layout,
    .handle_event = NULL,
    .destroy      = label_destroy,
};

ClueLabel *clue_label_new(const char *text)
{
    ClueLabel *l = calloc(1, sizeof(ClueLabel));
    if (!l) return NULL;

    clue_cwidget_init(&l->base, &label_vtable);
    l->text = text ? strdup(text) : strdup("");

    /* Initial size from font */
    label_layout(&l->base);

    return l;
}

void clue_label_set_text(ClueLabel *label, const char *text)
{
    if (!label) return;
    free(label->text);
    label->text = text ? strdup(text) : strdup("");
    label_layout(&label->base);
}

const char *clue_label_get_text(ClueLabel *label)
{
    return label ? label->text : NULL;
}
