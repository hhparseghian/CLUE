#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/tabs.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/clue_widget.h"

#define TAB_PAD_H 16
#define TAB_PAD_V 8

static ClueFont *tabs_font(ClueTabs *t)
{
    return t->base.style.font ? t->base.style.font : clue_app_default_font();
}

static void tabs_draw(ClueWidget *w)
{
    ClueTabs *t = (ClueTabs *)w;
    ClueFont *font = tabs_font(t);
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y, bw = w->base.w;

    int content_y = y + t->tab_height;
    int content_h = w->base.h - t->tab_height;
    bool has_page_bg = t->page_bg.a > 0.001f;
    ClueColor page_bg = has_page_bg ? t->page_bg : th->bg;

    /* Measure total tab bar width */
    int tabs_width = 0;
    if (font) {
        for (int i = 0; i < t->tab_count; i++)
            tabs_width += clue_font_text_width(font, t->tab_labels[i]) + TAB_PAD_H * 2;
    }

    /* Page background */
    if (has_page_bg) {
        clue_fill_rect(x, y, bw, w->base.h, page_bg);
        /* Cover the tab bar area to the right of the tabs with app bg */
        if (tabs_width < bw) {
            clue_fill_rect(x + tabs_width, y, bw - tabs_width, t->tab_height, th->bg);
        }
    }

    /* Draw tab backgrounds */
    int tx = x;
    int active_x = 0, active_w = 0;
    for (int i = 0; i < t->tab_count; i++) {
        int tw = 0;
        if (font) tw = clue_font_text_width(font, t->tab_labels[i]) + TAB_PAD_H * 2;

        if (i == t->active) {
            active_x = tx;
            active_w = tw;
        } else {
            clue_fill_rect(tx, y, tw, t->tab_height, th->surface);
            if (i == t->hovered) {
                clue_fill_rect(tx, y, tw, t->tab_height, th->surface_hover);
            }
            ClueColor border = CLUE_RGB(0, 0, 0);
            clue_fill_rect(tx, y, tw, 1, border);
        }

        tx += tw;
    }

    /* Bottom border line across the full width */
    {
        ClueColor border = CLUE_RGB(0, 0, 0);
        clue_fill_rect(x, y + t->tab_height - 1, bw, 1, border);
    }

    /* Draw labels and separators */
    tx = x;
    for (int i = 0; i < t->tab_count; i++) {
        int tw = 0;
        if (font) tw = clue_font_text_width(font, t->tab_labels[i]) + TAB_PAD_H * 2;

        if (font) {
            ClueColor fg = (i == t->active) ? th->fg_bright : th->fg_dim;
            int text_y = y + (t->tab_height - clue_font_line_height(font)) / 2;
            clue_draw_text(tx + TAB_PAD_H, text_y, t->tab_labels[i], font, fg);
        }

        tx += tw;

        /* Separator: skip next to active tab */
        if (i != t->active && i + 1 != t->active) {
            ClueColor sep = CLUE_RGB(0, 0, 0);
            clue_fill_rect(tx, y, 1, t->tab_height, sep);
        }
    }

    /* Draw active page: force size to fill content area, layout, then draw */
    if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
        ClueWidget *page = t->tab_pages[t->active];
        int cy = y + t->tab_height;
        int ch = w->base.h - t->tab_height;
        page->base.x = x;
        page->base.y = cy;
        page->base.w = bw;
        page->base.h = ch;
        clue_cwidget_layout_tree(page);
        page->base.x = x;
        page->base.y = cy;
        page->base.w = bw;
        page->base.h = ch;
        clue_set_clip_rect(x, cy, bw, ch);
        clue_cwidget_draw_tree(page);
        clue_reset_clip_rect();
    }

    /* Redraw active tab AFTER page content so it covers the bottom border */
    if (active_w > 0) {
        clue_fill_rect(active_x, y, active_w, t->tab_height + 2, page_bg);
        clue_fill_rect(active_x, y, active_w, 3, th->accent);
        /* Re-draw the active tab label */
        if (font) {
            int text_y = y + (t->tab_height - clue_font_line_height(font)) / 2;
            clue_draw_text(active_x + TAB_PAD_H, text_y,
                           t->tab_labels[t->active], font, th->fg_bright);
        }
    }
}

static void tabs_layout(ClueWidget *w)
{
    ClueTabs *t = (ClueTabs *)w;
    ClueFont *font = tabs_font(t);

    t->tab_height = font ? clue_font_line_height(font) + TAB_PAD_V * 2 : 36;

    /* Layout the active page to fill below the tab bar */
    if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
        ClueWidget *page = t->tab_pages[t->active];
        int page_w = w->base.w;
        int page_h = w->base.h - t->tab_height;

        /* Set position and size before layout */
        page->base.x = w->base.x;
        page->base.y = w->base.y + t->tab_height;
        page->base.w = page_w;
        page->base.h = page_h;
        clue_cwidget_layout_tree(page);

        /* Set final page dimensions */
        page->base.x = w->base.x;
        page->base.y = w->base.y + t->tab_height;
        page->base.w = page_w;
        page->base.h = page_h;
    }
}

static int tabs_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueTabs *t = (ClueTabs *)w;
    ClueFont *font = tabs_font(t);
    int x = w->base.x, y = w->base.y;

    if (event->type == CLUE_EVENT_MOUSE_MOVE && font) {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        t->hovered = -1;

        if (my >= y && my < y + t->tab_height) {
            int tx = x;
            for (int i = 0; i < t->tab_count; i++) {
                int tw = clue_font_text_width(font, t->tab_labels[i]) + TAB_PAD_H * 2;
                if (mx >= tx && mx < tx + tw) {
                    t->hovered = i;
                    break;
                }
                tx += tw;
            }
        }
        /* Forward to active page so children see mouse moves */
        if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
            clue_widget_dispatch_event(
                &t->tab_pages[t->active]->base, event);
        }
        return 0;
    }

    /* Forward scroll events to active page */
    if (event->type == CLUE_EVENT_MOUSE_SCROLL) {
        if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
            return clue_widget_dispatch_event(
                &t->tab_pages[t->active]->base, event);
        }
        return 0;
    }

    if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed && event->mouse_button.btn == 0 && font) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;

        if (my >= y && my < y + t->tab_height) {
            int tx = x;
            for (int i = 0; i < t->tab_count; i++) {
                int tw = clue_font_text_width(font, t->tab_labels[i]) + TAB_PAD_H * 2;
                if (mx >= tx && mx < tx + tw) {
                    if (i != t->active) {
                        t->active = i;
                        clue_signal_emit(t, "changed");
                    }
                    return 1;
                }
                tx += tw;
            }
        }
    }

    /* Forward events to active page */
    if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
        return clue_widget_dispatch_event(
            &t->tab_pages[t->active]->base, event);
    }

    return 0;
}

static void tabs_destroy(ClueWidget *w)
{
    ClueTabs *t = (ClueTabs *)w;
    for (int i = 0; i < t->tab_count; i++) {
        free(t->tab_labels[i]);
        if (t->tab_pages[i]) {
            clue_cwidget_destroy(t->tab_pages[i]);
        }
    }
}

static const ClueWidgetVTable tabs_vtable = {
    .draw         = tabs_draw,
    .layout       = tabs_layout,
    .handle_event = tabs_handle_event,
    .destroy      = tabs_destroy,
};

ClueTabs *clue_tabs_new(void)
{
    ClueTabs *t = calloc(1, sizeof(ClueTabs));
    if (!t) return NULL;

    clue_cwidget_init(&t->base, &tabs_vtable);
    t->base.type_id = CLUE_WIDGET_TABS;
    t->base.base.skip_child_draw = true;
    t->base.base.intercept_events = true;
    t->active = 0;
    t->hovered = -1;

    return t;
}

int clue_tabs_add(ClueTabs *tabs, const char *label, ClueWidget *content)
{
    if (!tabs || tabs->tab_count >= CLUE_TABS_MAX) return -1;

    int idx = tabs->tab_count;
    tabs->tab_labels[idx] = strdup(label ? label : "");
    tabs->tab_pages[idx]  = content;
    tabs->tab_count++;

    return idx;
}

void clue_tabs_remove(ClueTabs *tabs, int index)
{
    if (!tabs || index < 0 || index >= tabs->tab_count) return;

    free(tabs->tab_labels[index]);

    for (int i = index; i < tabs->tab_count - 1; i++) {
        tabs->tab_labels[i] = tabs->tab_labels[i + 1];
        tabs->tab_pages[i]  = tabs->tab_pages[i + 1];
    }
    tabs->tab_count--;
    tabs->tab_labels[tabs->tab_count] = NULL;
    tabs->tab_pages[tabs->tab_count]  = NULL;

    if (tabs->active >= tabs->tab_count && tabs->tab_count > 0)
        tabs->active = tabs->tab_count - 1;
    else if (tabs->active > index)
        tabs->active--;
}

int clue_tabs_get_active(ClueTabs *tabs)
{
    return tabs ? tabs->active : -1;
}

void clue_tabs_set_active(ClueTabs *tabs, int index)
{
    if (!tabs || index < 0 || index >= tabs->tab_count) return;
    tabs->active = index;
}
