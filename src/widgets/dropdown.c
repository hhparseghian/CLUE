#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/dropdown.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define DD_PAD_H 12
#define DD_PAD_V 8
#define DD_ARROW_W 28
#define DD_ITEM_PAD 6

static UIFont *dd_font(ClueDropdown *dd)
{
    return dd->base.style.font ? dd->base.style.font : clue_app_default_font();
}

static int item_height(ClueDropdown *dd)
{
    UIFont *font = dd_font(dd);
    return font ? clue_font_line_height(font) + DD_ITEM_PAD * 2 : 28;
}

static void dropdown_draw(ClueWidget *w)
{
    ClueDropdown *dd = (ClueDropdown *)w;
    UIFont *font = dd_font(dd);
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    const ClueTheme *th = clue_theme_get();
    float cr = th->corner_radius;

    /* Subtle shadow */
    clue_fill_rounded_rect(x + 1, y + 2, bw, bh, cr,
                           UI_RGBAF(0, 0, 0, 0.1f));

    /* Background */
    UIColor bg = w->style.bg_color.a > 0.001f ? w->style.bg_color : th->dropdown.bg;
    clue_fill_rounded_rect(x, y, bw, bh, cr, bg);
    clue_draw_rounded_rect(x, y, bw, bh, cr, 1.5f, th->dropdown.border);

    /* Separator line before arrow */
    int sep_x = x + bw - DD_ARROW_W;
    clue_draw_line(sep_x, y + 6, sep_x, y + bh - 6, 1.0f, th->dropdown.border);

    if (font) {
        const char *display = NULL;
        UIColor text_color;

        if (dd->selected >= 0 && dd->selected < dd->item_count) {
            display = dd->items[dd->selected];
            text_color = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->dropdown.fg;
        } else if (dd->placeholder) {
            display = dd->placeholder;
            text_color = th->input.placeholder;
        }

        if (display) {
            int text_y = y + (bh - clue_font_line_height(font)) / 2;
            /* Clip text so it doesn't overlap the arrow */
            clue_set_clip_rect(x, y, bw - DD_ARROW_W, bh);
            clue_draw_text(x + DD_PAD_H, text_y, display, font, text_color);
            clue_reset_clip_rect();
        }
    }

    /* Down arrow (chevron) */
    int ax = x + bw - DD_ARROW_W / 2;
    int ay = y + bh / 2;
    UIColor arrow_c = dd->open ? th->accent : th->dropdown.arrow;
    clue_draw_line(ax - 5, ay - 3, ax, ay + 3, 2.0f, arrow_c);
    clue_draw_line(ax, ay + 3, ax + 5, ay - 3, 2.0f, arrow_c);
}

/* Draw the popup list (called after all widgets, so it's on top) */
static int dd_max_vis(ClueDropdown *dd)
{
    return dd->max_visible > 0 ? dd->max_visible : 6;
}

static int dd_visible_count(ClueDropdown *dd)
{
    int mv = dd_max_vis(dd);
    return dd->item_count < mv ? dd->item_count : mv;
}

static int dd_list_h(ClueDropdown *dd)
{
    return item_height(dd) * dd_visible_count(dd) + 4;
}

void clue_dropdown_draw_overlay(ClueDropdown *dd)
{
    if (!dd || !dd->open || dd->item_count == 0) return;
    UIFont *font = dd_font(dd);
    if (!font) return;

    const ClueTheme *th = clue_theme_get();
    float cr = th->corner_radius;
    int x = dd->base.base.x;
    int y = dd->base.base.y + dd->base.base.h + 4;
    int bw = dd->base.base.w;
    int ih = item_height(dd);
    int vis = dd_visible_count(dd);
    int list_h = dd_list_h(dd);

    /* Drop shadow */
    clue_fill_rounded_rect(x + 2, y + 3, bw, list_h, cr,
                           UI_RGBAF(0, 0, 0, 0.25f));

    /* Background */
    clue_fill_rounded_rect(x, y, bw, list_h, cr, th->dropdown.list_bg);
    clue_draw_rounded_rect(x, y, bw, list_h, cr, 1.5f, th->dropdown.border);

    clue_set_clip_rect(x, y, bw, list_h);

    int first = dd->scroll_y / ih;
    int last = first + vis + 1;
    if (last > dd->item_count) last = dd->item_count;

    for (int i = first; i < last; i++) {
        int iy = y + 2 + i * ih - dd->scroll_y;

        if (i == dd->hovered) {
            clue_fill_rect(x + 2, iy, bw - 4, ih, th->dropdown.hover_bg);
        }

        if (i == dd->selected) {
            clue_fill_rounded_rect(x + 3, iy + 3, 3, ih - 6,
                                   1.5f, th->accent);
        }

        int tx = x + DD_PAD_H + (i == dd->selected ? 6 : 0);
        UIColor item_fg = (i == dd->hovered) ? th->fg_bright : th->dropdown.fg;
        if (i == dd->selected) item_fg = th->accent;
        clue_draw_text(tx, iy + DD_ITEM_PAD,
                       dd->items[i], font, item_fg);
    }

    clue_reset_clip_rect();

    /* Scrollbar */
    int total = ih * dd->item_count;
    if (total > list_h - 4) {
        float ratio = (float)(list_h - 4) / (float)total;
        int bar_h = (int)(ratio * (list_h - 4));
        if (bar_h < 16) bar_h = 16;
        int max_scroll = total - (list_h - 4);
        float pos = max_scroll > 0 ? (float)dd->scroll_y / (float)max_scroll : 0;
        int bar_y = y + 2 + (int)(pos * (list_h - 4 - bar_h));
        clue_fill_rounded_rect(x + bw - 8, bar_y, 4, bar_h, 2.0f,
                               UI_RGBA(150, 150, 160, 120));
    }
}

static void dropdown_layout(ClueWidget *w)
{
    ClueDropdown *dd = (ClueDropdown *)w;
    UIFont *font = dd_font(dd);
    if (!font) return;

    if (w->base.w == 0) w->base.w = 180;
    w->base.h = clue_font_line_height(font) + DD_PAD_V * 2;
}

static int dropdown_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueDropdown *dd = (ClueDropdown *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Scroll the popup list */
    if (event->type == UI_EVENT_MOUSE_SCROLL && dd->open) {
        int ih = item_height(dd);
        int total = ih * dd->item_count;
        int vis_h = dd_list_h(dd) - 4;
        dd->scroll_y -= (int)(event->mouse_scroll.dy * 30.0f);
        int max_scroll = total - vis_h;
        if (max_scroll < 0) max_scroll = 0;
        if (dd->scroll_y > max_scroll) dd->scroll_y = max_scroll;
        if (dd->scroll_y < 0) dd->scroll_y = 0;
        return 1;
    }

    /* Track hover over list items */
    if (event->type == UI_EVENT_MOUSE_MOVE && dd->open && dd->item_count > 0) {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        int ih = item_height(dd);
        int list_y = y + bh + 4;
        int list_h = dd_list_h(dd);

        if (mx >= x && mx < x + bw && my >= list_y && my < list_y + list_h) {
            dd->hovered = (my - list_y - 2 + dd->scroll_y) / ih;
            if (dd->hovered >= dd->item_count) dd->hovered = -1;
        } else {
            dd->hovered = -1;
        }
        return 1;
    }

    if (event->type != UI_EVENT_MOUSE_BUTTON || !event->mouse_button.pressed)
        return 0;
    if (event->mouse_button.btn != 0) return 0;

    int mx = event->mouse_button.x;
    int my = event->mouse_button.y;

    /* Click on main button: toggle dropdown */
    if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
        dd->open = !dd->open;
        dd->scroll_y = 0;
        if (dd->open) {
            clue_capture_mouse(&w->base);
        } else {
            clue_release_mouse();
        }
        return 1;
    }

    /* When open, all clicks are captured here */
    if (dd->open) {
        if (dd->item_count > 0) {
            int ih = item_height(dd);
            int list_y = y + bh + 4;
            int list_h = dd_list_h(dd);

            if (mx >= x && mx < x + bw && my >= list_y && my < list_y + list_h) {
                int idx = (my - list_y - 2 + dd->scroll_y) / ih;
                if (idx >= 0 && idx < dd->item_count) {
                    dd->selected = idx;
                    clue_signal_emit(dd, "changed");
                }
            }
        }

        dd->open = false;
        dd->hovered = -1;
        clue_release_mouse();
        return 1;
    }

    return 0;
}

static void dropdown_destroy(ClueWidget *w)
{
    ClueDropdown *dd = (ClueDropdown *)w;
    for (int i = 0; i < dd->item_count; i++) {
        free(dd->items[i]);
    }
    free(dd->placeholder);
}

static const ClueWidgetVTable dropdown_vtable = {
    .draw         = dropdown_draw,
    .layout       = dropdown_layout,
    .handle_event = dropdown_handle_event,
    .destroy      = dropdown_destroy,
};

ClueDropdown *clue_dropdown_new(const char *placeholder)
{
    ClueDropdown *dd = calloc(1, sizeof(ClueDropdown));
    if (!dd) return NULL;

    clue_cwidget_init(&dd->base, &dropdown_vtable);
    dd->base.type_id = CLUE_WIDGET_DROPDOWN;
    dd->selected = -1;
    dd->hovered = -1;
    dd->placeholder = placeholder ? strdup(placeholder) : NULL;

    dropdown_layout(&dd->base);
    return dd;
}

void clue_dropdown_add_item(ClueDropdown *dd, const char *text)
{
    if (!dd || dd->item_count >= CLUE_DROPDOWN_MAX_ITEMS) return;
    dd->items[dd->item_count++] = strdup(text);
}

int clue_dropdown_get_selected(ClueDropdown *dd)
{
    return dd ? dd->selected : -1;
}

const char *clue_dropdown_get_selected_text(ClueDropdown *dd)
{
    if (!dd || dd->selected < 0 || dd->selected >= dd->item_count)
        return NULL;
    return dd->items[dd->selected];
}

void clue_dropdown_set_selected(ClueDropdown *dd, int index)
{
    if (!dd) return;
    if (index >= -1 && index < dd->item_count) {
        dd->selected = index;
    }
}
