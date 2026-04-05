#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/table.h"
#include "clue/scrollbar.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define TABLE_PAD 6
#define SCROLLBAR_W 6

static UIFont *table_font(ClueTable *t)
{
    return t->base.style.font ? t->base.style.font : clue_app_default_font();
}

static void clamp_scroll(ClueTable *t)
{
    int total = t->row_count * t->row_height;
    int vis = t->base.base.h - t->header_height;
    int max_y = total - vis;
    if (max_y < 0) max_y = 0;
    if (t->scroll_y > max_y) t->scroll_y = max_y;
    if (t->scroll_y < 0) t->scroll_y = 0;
}

static void table_draw(ClueWidget *w)
{
    ClueTable *t = (ClueTable *)w;
    const ClueTheme *th = clue_theme_get();
    UIFont *font = table_font(t);
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    clue_fill_rounded_rect(x, y, bw, bh, th->corner_radius, th->list.bg);
    clue_draw_rounded_rect(x, y, bw, bh, th->corner_radius, 1.0f,
                           th->surface_border);

    if (!font) return;

    /* Header */
    clue_fill_rect(x + 1, y + 1, bw - 2, t->header_height, th->surface);
    clue_draw_line(x, y + t->header_height, x + bw, y + t->header_height,
                   1.0f, th->surface_border);

    int cx = x;
    for (int c = 0; c < t->col_count; c++) {
        if (t->headers[c]) {
            int ty = y + (t->header_height - clue_font_line_height(font)) / 2;
            clue_set_clip_rect(cx, y, t->col_widths[c], t->header_height);
            clue_draw_text(cx + TABLE_PAD, ty, t->headers[c], font, th->fg_bright);
            clue_reset_clip_rect();
        }
        cx += t->col_widths[c];
        /* Column separator */
        if (c < t->col_count - 1)
            clue_draw_line(cx, y, cx, y + bh, 1.5f, th->surface_border);
    }

    if (!t->cell_cb) return;

    /* Rows */
    int body_y = y + t->header_height;
    int body_h = bh - t->header_height;
    clue_set_clip_rect(x, body_y, bw, body_h);

    int first = t->scroll_y / t->row_height;
    int last = (t->scroll_y + body_h) / t->row_height + 1;
    if (first < 0) first = 0;
    if (last > t->row_count) last = t->row_count;

    for (int r = first; r < last; r++) {
        int ry = body_y + r * t->row_height - t->scroll_y;

        if (r == t->selected_row) {
            clue_fill_rect(x + 1, ry, bw - 2, t->row_height, th->list.selected_bg);
        } else if (r == t->hovered_row) {
            clue_fill_rect(x + 1, ry, bw - 2, t->row_height, th->list.hover_bg);
        } else if (r % 2 == 1) {
            clue_fill_rect(x + 1, ry, bw - 2, t->row_height, th->list.stripe);
        }

        cx = x;
        for (int c = 0; c < t->col_count; c++) {
            const char *text = t->cell_cb(r, c, t->cell_data);
            if (text) {
                UIColor fg = (r == t->selected_row) ? th->list.selected_fg : th->list.fg;
                int ty = ry + (t->row_height - clue_font_line_height(font)) / 2;
                clue_set_clip_rect(cx, body_y, t->col_widths[c], body_h);
                clue_draw_text(cx + TABLE_PAD, ty, text, font, fg);
                clue_reset_clip_rect();
            }
            cx += t->col_widths[c];
        }
    }

    clue_reset_clip_rect();

    /* Scrollbar (in body area, below header) */
    {
        int total = t->row_count * t->row_height;
        clue_scrollbar_draw(&t->sb, x, body_y, bw, body_h, total, t->scroll_y);
    }
}

static void table_layout(ClueWidget *w)
{
    ClueTable *t = (ClueTable *)w;
    UIFont *font = table_font(t);
    if (font) {
        if (t->row_height == 0)
            t->row_height = clue_font_line_height(font) + TABLE_PAD * 2;
        if (t->header_height == 0)
            t->header_height = clue_font_line_height(font) + TABLE_PAD * 2 + 2;
    }
    if (w->base.w == 0) {
        int total = 0;
        for (int c = 0; c < t->col_count; c++) total += t->col_widths[c];
        w->base.w = total;
    }
    if (w->base.h == 0) w->base.h = 200;
    clamp_scroll(t);
}

static int table_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueTable *t = (ClueTable *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;
    int body_y = y + t->header_height;
    int body_h = bh - t->header_height;

    /* Scrollbar drag */
    {
        int total = t->row_count * t->row_height;
        if (clue_scrollbar_handle_event(&t->sb, x, body_y, bw, body_h,
                                        total, &t->scroll_y,
                                        event, &w->base)) {
            return 1;
        }
    }

    switch (event->type) {
    case UI_EVENT_MOUSE_MOVE: {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        t->hovered_row = -1;
        if (mx >= x && mx < x + bw && my >= body_y && my < y + bh) {
            int idx = (my - body_y + t->scroll_y) / t->row_height;
            if (idx >= 0 && idx < t->row_count)
                t->hovered_row = idx;
            return 1;
        }
        return 0;
    }

    case UI_EVENT_MOUSE_BUTTON: {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        if (mx >= x && mx < x + bw && my >= body_y && my < y + bh) {
            if (event->mouse_button.pressed && event->mouse_button.btn == 0) {
                clue_focus_widget(&w->base);
                int idx = (my - body_y + t->scroll_y) / t->row_height;
                if (idx >= 0 && idx < t->row_count) {
                    t->selected_row = idx;
                    clue_signal_emit(t, "selected");
                }
            }
            return 1;
        }
        return 0;
    }

    case UI_EVENT_MOUSE_SCROLL: {
        int mx = event->mouse_scroll.x;
        int my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            t->scroll_y -= (int)(event->mouse_scroll.dy * 30.0f);
            clamp_scroll(t);
            return 1;
        }
        return 0;
    }

    default: return 0;
    }
}

static void table_destroy(ClueWidget *w)
{
    ClueTable *t = (ClueTable *)w;
    for (int c = 0; c < t->col_count; c++)
        free(t->headers[c]);
}

static const ClueWidgetVTable table_vtable = {
    .draw = table_draw, .layout = table_layout,
    .handle_event = table_handle_event, .destroy = table_destroy,
};

ClueTable *clue_table_new(void)
{
    ClueTable *t = calloc(1, sizeof(ClueTable));
    if (!t) return NULL;
    clue_cwidget_init(&t->base, &table_vtable);
    t->base.base.focusable = true;
    t->selected_row = -1;
    t->hovered_row = -1;
    return t;
}

void clue_table_add_column(ClueTable *t, const char *header, int width)
{
    if (!t || t->col_count >= CLUE_TABLE_MAX_COLS) return;
    int idx = t->col_count;
    t->headers[idx] = header ? strdup(header) : NULL;
    t->col_widths[idx] = width;
    t->col_count++;
}

void clue_table_set_data(ClueTable *t, int rows,
                         ClueTableCellCallback cb, void *user_data)
{
    if (!t) return;
    t->row_count = rows;
    t->cell_cb = cb;
    t->cell_data = user_data;
    t->scroll_y = 0;
    t->selected_row = -1;
}

int clue_table_get_selected(ClueTable *t)
{
    return t ? t->selected_row : -1;
}

void clue_table_set_selected(ClueTable *t, int row)
{
    if (!t) return;
    if (row >= -1 && row < t->row_count) t->selected_row = row;
}
