#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/label.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"

static ClueFont *label_font(ClueLabel *l)
{
    return l->base.style.font ? l->base.style.font : clue_app_default_font();
}

/* Measure width of a substring */
static int text_width_n(ClueFont *font, const char *text, int n)
{
    if (!font || !text || n <= 0) return 0;
    char buf[1024];
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    memcpy(buf, text, n);
    buf[n] = '\0';
    return clue_font_text_width(font, buf);
}

/* Draw text with word wrap. Returns number of lines drawn. */
static int draw_wrapped(ClueFont *font, const char *text, int x, int y,
                        int max_w, int lh, ClueColor color, bool draw)
{
    if (!text || !text[0]) return 0;

    int lines = 0;
    const char *p = text;

    while (*p) {
        /* Find end of line: either newline or word-wrap boundary */
        const char *line_start = p;
        const char *last_break = NULL;
        const char *end = p;

        while (*end && *end != '\n') {
            if (*end == ' ') last_break = end;
            int w = text_width_n(font, line_start, (int)(end - line_start + 1));
            if (max_w > 0 && w > max_w && last_break) {
                /* Wrap at last space */
                end = last_break;
                break;
            }
            end++;
        }

        /* Draw this line */
        int line_len = (int)(end - line_start);
        if (draw && line_len > 0) {
            char buf[1024];
            if (line_len >= (int)sizeof(buf)) line_len = (int)sizeof(buf) - 1;
            memcpy(buf, line_start, line_len);
            buf[line_len] = '\0';
            clue_draw_text(x, y + lines * lh, buf, font, color);
        }
        lines++;

        /* Advance past the line */
        p = end;
        if (*p == '\n') p++;
        else if (*p == ' ') p++;
        if (p == line_start && *p) { p++; } /* prevent infinite loop */
    }

    return lines;
}

static void label_draw(ClueWidget *w)
{
    ClueLabel *l = (ClueLabel *)w;
    if (!l->text || !l->text[0]) return;

    ClueStyle *s = &w->style;

    if (s->bg_color.a > 0.001f) {
        clue_fill_rounded_rect(w->base.x, w->base.y, w->base.w, w->base.h,
                               s->corner_radius, s->bg_color);
    }

    ClueFont *font = label_font(l);
    if (!font) return;

    int tx = w->base.x + s->padding_left;
    int ty = w->base.y + s->padding_top;

    if (l->wrap || strchr(l->text, '\n')) {
        int max_w = l->wrap ? w->base.w - s->padding_left - s->padding_right : 0;
        draw_wrapped(font, l->text, tx, ty, max_w,
                     clue_font_line_height(font), s->fg_color, true);
    } else {
        clue_draw_text(tx, ty, l->text, font, s->fg_color);
    }
}

static void label_layout(ClueWidget *w)
{
    ClueLabel *l = (ClueLabel *)w;
    ClueFont *font = label_font(l);
    if (!font || !l->text) return;

    ClueStyle *s = &w->style;
    int lh = clue_font_line_height(font);

    if (l->wrap || strchr(l->text, '\n')) {
        int max_w = l->wrap ? w->base.w - s->padding_left - s->padding_right : 0;
        int lines = draw_wrapped(font, l->text, 0, 0, max_w, lh,
                                 (ClueColor){0}, false);
        if (lines < 1) lines = 1;
        w->base.h = lines * lh + s->padding_top + s->padding_bottom;
        /* Width stays as set by user when wrapping */
        if (!l->wrap) {
            w->base.w = clue_font_text_width(font, l->text)
                        + s->padding_left + s->padding_right;
        }
    } else {
        w->base.w = clue_font_text_width(font, l->text)
                    + s->padding_left + s->padding_right;
        w->base.h = lh + s->padding_top + s->padding_bottom;
    }
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
    l->base.type_id = CLUE_WIDGET_LABEL;
    l->text = text ? strdup(text) : strdup("");
    l->wrap = false;

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

void clue_label_set_wrap(ClueLabel *label, bool wrap)
{
    if (label) {
        label->wrap = wrap;
        label_layout(&label->base);
    }
}
