#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/statusbar.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define SB_PAD_H 10
#define SB_PAD_V 4
#define SB_SEP_GAP 16

static ClueFont *sb_font(ClueStatusbar *sb)
{
    return sb->base.style.font ? sb->base.style.font : clue_app_default_font();
}

static void statusbar_draw(ClueWidget *w)
{
    ClueStatusbar *sb = (ClueStatusbar *)w;
    const ClueTheme *th = clue_theme_get();
    ClueFont *font = sb_font(sb);
    if (!font) return;

    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    clue_fill_rect(x, y, bw, bh, th->surface);
    /* Top border */
    clue_fill_rect(x, y, bw, 1, th->surface_border);

    /* Sections */
    int tx = x + SB_PAD_H;
    int ty = y + (bh - clue_font_line_height(font)) / 2;
    for (int i = 0; i < sb->section_count; i++) {
        if (!sb->sections[i]) continue;
        if (i > 0) {
            /* Separator */
            clue_fill_rect(tx, y + 4, 1, bh - 8, th->surface_border);
            tx += SB_SEP_GAP;
        }
        clue_draw_text(tx, ty, sb->sections[i], font, th->fg_dim);
        tx += clue_font_text_width(font, sb->sections[i]) + SB_SEP_GAP;
    }
}

static void statusbar_layout(ClueWidget *w)
{
    ClueStatusbar *sb = (ClueStatusbar *)w;
    ClueFont *font = sb_font(sb);
    if (!font) return;
    w->base.h = clue_font_line_height(font) + SB_PAD_V * 2;
    /* Width is set by parent (hexpand) */
}

static void statusbar_destroy(ClueWidget *w)
{
    ClueStatusbar *sb = (ClueStatusbar *)w;
    for (int i = 0; i < sb->section_count; i++)
        free(sb->sections[i]);
}

static const ClueWidgetVTable statusbar_vtable = {
    .draw    = statusbar_draw,
    .layout  = statusbar_layout,
    .destroy = statusbar_destroy,
};

ClueStatusbar *clue_statusbar_new(void)
{
    ClueStatusbar *sb = calloc(1, sizeof(ClueStatusbar));
    if (!sb) return NULL;

    clue_cwidget_init(&sb->base, &statusbar_vtable);
    sb->base.type_id = CLUE_WIDGET_STATUSBAR;
    sb->base.style.hexpand = true;

    statusbar_layout(&sb->base);
    return sb;
}

void clue_statusbar_set_text(ClueStatusbar *sb, int index, const char *text)
{
    if (!sb || index < 0 || index > sb->section_count ||
        index >= CLUE_STATUSBAR_MAX_SECTIONS) return;
    if (index == sb->section_count) sb->section_count++;
    free(sb->sections[index]);
    sb->sections[index] = text ? strdup(text) : strdup("");
}

const char *clue_statusbar_get_text(ClueStatusbar *sb, int index)
{
    if (!sb || index < 0 || index >= sb->section_count) return NULL;
    return sb->sections[index];
}

int clue_statusbar_section_count(ClueStatusbar *sb)
{
    return sb ? sb->section_count : 0;
}
