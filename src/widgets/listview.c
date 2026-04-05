#include <stdlib.h>
#include "clue/listview.h"
#include "clue/scrollbar.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/font.h"
#include "clue/theme.h"
#include <xkbcommon/xkbcommon-keysyms.h>

#define LV_PAD 6

static ClueFont *lv_font(ClueListView *lv)
{
    return lv->base.style.font ? lv->base.style.font : clue_app_default_font();
}

static int total_content_h(ClueListView *lv)
{
    return lv->item_count * lv->item_height;
}

static void clamp_scroll(ClueListView *lv)
{
    int max_y = total_content_h(lv) - lv->base.base.h;
    if (max_y < 0) max_y = 0;
    if (lv->scroll_y > max_y) lv->scroll_y = max_y;
    if (lv->scroll_y < 0) lv->scroll_y = 0;
}

static void listview_draw(ClueWidget *w)
{
    ClueListView *lv = (ClueListView *)w;
    ClueFont *font = lv_font(lv);
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    clue_fill_rounded_rect(x, y, bw, bh, th->corner_radius, th->list.bg);
    clue_draw_rounded_rect(x, y, bw, bh, th->corner_radius, 1.0f,
                           th->surface_border);

    if (!lv->item_cb || !font) return;

    clue_set_clip_rect(x, y, bw, bh);

    int first = lv->scroll_y / lv->item_height;
    int last = (lv->scroll_y + bh) / lv->item_height + 1;
    if (first < 0) first = 0;
    if (last > lv->item_count) last = lv->item_count;

    for (int i = first; i < last; i++) {
        int iy = y + i * lv->item_height - lv->scroll_y;

        if (i == lv->selected) {
            clue_fill_rect(x + 1, iy, bw - 2, lv->item_height, th->list.selected_bg);
        } else if (i == lv->hovered) {
            clue_fill_rect(x + 1, iy, bw - 2, lv->item_height, th->list.hover_bg);
        }

        if (i != lv->selected && i != lv->hovered && i % 2 == 1) {
            clue_fill_rect(x + 1, iy, bw - 2, lv->item_height, th->list.stripe);
        }

        int text_offset = LV_PAD;

        if (lv->icon_cb) {
            text_offset += lv->icon_cb(i, x + LV_PAD, iy, lv->item_height,
                                       lv->item_data);
        }

        const char *text = lv->item_cb(i, lv->item_data);
        if (text) {
            ClueColor fg = (i == lv->selected) ? th->list.selected_fg : th->list.fg;
            int ty = iy + (lv->item_height - clue_font_line_height(font)) / 2;
            clue_draw_text(x + text_offset, ty, text, font, fg);
        }
    }

    clue_reset_clip_rect();

    /* Scrollbar */
    clue_scrollbar_draw(&lv->sb, x, y, bw, bh,
                        total_content_h(lv), lv->scroll_y);
}

static void listview_layout(ClueWidget *w)
{
    ClueListView *lv = (ClueListView *)w;
    ClueFont *font = lv_font(lv);

    if (lv->item_height == 0 && font) {
        lv->item_height = clue_font_line_height(font) + LV_PAD * 2;
    }
    if (w->base.w == 0) w->base.w = 250;
    if (w->base.h == 0) w->base.h = 200;

    clamp_scroll(lv);
}

static int listview_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueListView *lv = (ClueListView *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Scrollbar drag takes priority */
    if (clue_scrollbar_handle_event(&lv->sb, x, y, bw, bh,
                                    total_content_h(lv), &lv->scroll_y,
                                    event, &w->base)) {
        clamp_scroll(lv);
        return 1;
    }

    switch (event->type) {
    case CLUE_EVENT_MOUSE_MOVE: {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            int idx = (my - y + lv->scroll_y) / lv->item_height;
            lv->hovered = (idx >= 0 && idx < lv->item_count) ? idx : -1;
            return 1;
        }
        lv->hovered = -1;
        return 0;
    }

    case CLUE_EVENT_MOUSE_BUTTON: {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            if (event->mouse_button.pressed && event->mouse_button.btn == 0) {
                clue_focus_widget(&w->base);
                int idx = (my - y + lv->scroll_y) / lv->item_height;
                if (idx >= 0 && idx < lv->item_count) {
                    lv->selected = idx;
                    clue_signal_emit(lv, "selected");
                }
            }
            return 1;
        }
        return 0;
    }

    case CLUE_EVENT_KEY: {
        if (!w->base.focused || !event->key.pressed) return 0;

        int key = event->key.keycode;

        if (key == XKB_KEY_Up) {
            if (lv->selected > 0) {
                lv->selected--;
                clue_signal_emit(lv, "selected");
            } else if (lv->selected < 0 && lv->item_count > 0) {
                lv->selected = 0;
                clue_signal_emit(lv, "selected");
            }
            int sel_y = lv->selected * lv->item_height;
            if (sel_y < lv->scroll_y)
                lv->scroll_y = sel_y;
            clamp_scroll(lv);
            return 1;
        }

        if (key == XKB_KEY_Down) {
            if (lv->selected < lv->item_count - 1) {
                lv->selected++;
                clue_signal_emit(lv, "selected");
            } else if (lv->selected < 0 && lv->item_count > 0) {
                lv->selected = 0;
                clue_signal_emit(lv, "selected");
            }
            int sel_bottom = (lv->selected + 1) * lv->item_height;
            if (sel_bottom > lv->scroll_y + bh)
                lv->scroll_y = sel_bottom - bh;
            clamp_scroll(lv);
            return 1;
        }

        if (key == XKB_KEY_Home) {
            lv->selected = 0;
            lv->scroll_y = 0;
            clue_signal_emit(lv, "selected");
            return 1;
        }

        if (key == XKB_KEY_End) {
            lv->selected = lv->item_count - 1;
            lv->scroll_y = lv->item_count * lv->item_height - bh;
            clamp_scroll(lv);
            clue_signal_emit(lv, "selected");
            return 1;
        }

        if (key == XKB_KEY_Page_Up) {
            int page = bh / lv->item_height;
            if (page < 1) page = 1;
            lv->selected -= page;
            if (lv->selected < 0) lv->selected = 0;
            int sel_y = lv->selected * lv->item_height;
            if (sel_y < lv->scroll_y) lv->scroll_y = sel_y;
            clamp_scroll(lv);
            clue_signal_emit(lv, "selected");
            return 1;
        }

        if (key == XKB_KEY_Page_Down) {
            int page = bh / lv->item_height;
            if (page < 1) page = 1;
            lv->selected += page;
            if (lv->selected >= lv->item_count) lv->selected = lv->item_count - 1;
            int sel_bottom = (lv->selected + 1) * lv->item_height;
            if (sel_bottom > lv->scroll_y + bh) lv->scroll_y = sel_bottom - bh;
            clamp_scroll(lv);
            clue_signal_emit(lv, "selected");
            return 1;
        }

        return 0;
    }

    case CLUE_EVENT_MOUSE_SCROLL: {
        int mx = event->mouse_scroll.x;
        int my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            lv->scroll_y -= (int)(event->mouse_scroll.dy * 30.0f);
            clamp_scroll(lv);
            return 1;
        }
        return 0;
    }

    default:
        return 0;
    }
}

static const ClueWidgetVTable listview_vtable = {
    .draw         = listview_draw,
    .layout       = listview_layout,
    .handle_event = listview_handle_event,
    .destroy      = NULL,
};

ClueListView *clue_listview_new(void)
{
    ClueListView *lv = calloc(1, sizeof(ClueListView));
    if (!lv) return NULL;

    clue_cwidget_init(&lv->base, &listview_vtable);
    lv->base.base.focusable = true;
    lv->selected = -1;
    lv->hovered = -1;

    return lv;
}

void clue_listview_set_data(ClueListView *lv, int count,
                            ClueListViewItemCallback cb, void *user_data)
{
    if (!lv) return;
    lv->item_count = count;
    lv->item_cb = cb;
    lv->item_data = user_data;
    lv->scroll_y = 0;
    lv->selected = -1;
}

int clue_listview_get_selected(ClueListView *lv)
{
    return lv ? lv->selected : -1;
}

void clue_listview_set_selected(ClueListView *lv, int index)
{
    if (!lv) return;
    if (index >= -1 && index < lv->item_count) {
        lv->selected = index;
    }
}
