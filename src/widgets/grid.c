#include <stdlib.h>
#include "clue/grid.h"
#include "clue/draw.h"

static void grid_draw(ClueWidget *w)
{
    if (w->style.bg_color.a > 0.001f) {
        clue_fill_rounded_rect(w->base.x, w->base.y, w->base.w, w->base.h,
                               w->style.corner_radius, w->style.bg_color);
    }
}

static void grid_layout(ClueWidget *w)
{
    ClueGrid *g = (ClueGrid *)w;
    ClueStyle *s = &w->style;
    int n = w->base.child_count;
    if (n == 0) return;

    int cols = g->cols;
    if (cols < 1) cols = 1;
    int rows = (n + cols - 1) / cols;

    int avail_w = w->base.w - s->padding_left - s->padding_right
                  - g->col_spacing * (cols - 1);

    /* Calculate column widths */
    int cw[CLUE_GRID_MAX_COLS];
    int fixed_total = 0, auto_count = 0;
    for (int c = 0; c < cols; c++) {
        if (g->col_widths[c] > 0) {
            cw[c] = g->col_widths[c];
            fixed_total += cw[c];
        } else {
            cw[c] = 0;
            auto_count++;
        }
    }
    int auto_w = auto_count > 0 ? (avail_w - fixed_total) / auto_count : 0;
    if (auto_w < 0) auto_w = 0;
    for (int c = 0; c < cols; c++) {
        if (cw[c] == 0) cw[c] = auto_w;
    }

    /* Calculate row heights from children */
    int rh[256];
    for (int r = 0; r < rows && r < 256; r++) rh[r] = 0;

    for (int i = 0; i < n; i++) {
        int r = i / cols;
        if (r >= 256) break;
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        if (child->base.h > rh[r]) rh[r] = child->base.h;
    }

    /* Position children */
    int cy = w->base.y + s->padding_top;
    for (int r = 0; r < rows; r++) {
        if (r >= 256) break;
        int cx = w->base.x + s->padding_left;
        for (int c = 0; c < cols; c++) {
            int idx = r * cols + c;
            if (idx >= n) break;

            ClueWidget *child = (ClueWidget *)w->base.children[idx];
            child->base.x = cx + child->style.margin_left;
            child->base.y = cy + child->style.margin_top;

            cx += cw[c] + g->col_spacing;
        }
        cy += rh[r] + g->row_spacing;
    }

    /* Update grid total size */
    w->base.h = cy - w->base.y - g->row_spacing + s->padding_bottom;
}

static const ClueWidgetVTable grid_vtable = {
    .draw         = grid_draw,
    .layout       = grid_layout,
    .handle_event = NULL,
    .destroy      = NULL,
};

ClueGrid *clue_grid_new(int cols, int row_spacing, int col_spacing)
{
    ClueGrid *g = calloc(1, sizeof(ClueGrid));
    if (!g) return NULL;

    clue_cwidget_init(&g->base, &grid_vtable);
    g->cols = cols > 0 ? cols : 1;
    if (g->cols > CLUE_GRID_MAX_COLS) g->cols = CLUE_GRID_MAX_COLS;
    g->row_spacing = row_spacing;
    g->col_spacing = col_spacing;

    return g;
}

void clue_grid_set_col_width(ClueGrid *grid, int col, int width)
{
    if (!grid || col < 0 || col >= CLUE_GRID_MAX_COLS) return;
    grid->col_widths[col] = width;
}
