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

static UIFont *tabs_font(ClueTabs *t)
{
    return t->base.style.font ? t->base.style.font : clue_app_default_font();
}

static void tabs_draw(ClueWidget *w)
{
    ClueTabs *t = (ClueTabs *)w;
    UIFont *font = tabs_font(t);
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y, bw = w->base.w;

    /* Tab bar background */
    clue_fill_rect(x, y, bw, t->tab_height, th->surface);

    /* Draw tab buttons */
    int tx = x;
    for (int i = 0; i < t->tab_count; i++) {
        int tw = 0;
        if (font) tw = clue_font_text_width(font, t->tab_labels[i]) + TAB_PAD_H * 2;

        /* Active tab highlight */
        if (i == t->active) {
            clue_fill_rect(tx, y, tw, t->tab_height, th->bg);
            clue_fill_rect(tx, y + t->tab_height - 3, tw, 3, th->accent);
        } else if (i == t->hovered) {
            clue_fill_rect(tx, y, tw, t->tab_height, th->surface_hover);
        }

        /* Tab label */
        if (font) {
            UIColor fg = (i == t->active) ? th->fg_bright : th->fg_dim;
            int text_y = y + (t->tab_height - clue_font_line_height(font)) / 2;
            clue_draw_text(tx + TAB_PAD_H, text_y, t->tab_labels[i], font, fg);
        }

        tx += tw;
    }

    /* Bottom border */
    clue_draw_line(x, y + t->tab_height, x + bw, y + t->tab_height,
                   1.0f, th->surface_border);

    /* Draw active page content */
    if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
        clue_cwidget_draw_tree(t->tab_pages[t->active]);
    }
}

static void tabs_layout(ClueWidget *w)
{
    ClueTabs *t = (ClueTabs *)w;
    UIFont *font = tabs_font(t);

    t->tab_height = font ? clue_font_line_height(font) + TAB_PAD_V * 2 : 36;

    /* Layout the active page to fill below the tab bar */
    if (t->active >= 0 && t->active < t->tab_count && t->tab_pages[t->active]) {
        ClueWidget *page = t->tab_pages[t->active];
        page->base.x = w->base.x;
        page->base.y = w->base.y + t->tab_height;
        page->base.w = w->base.w;
        page->base.h = w->base.h - t->tab_height;
        clue_cwidget_layout_tree(page);
    }
}

static int tabs_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueTabs *t = (ClueTabs *)w;
    UIFont *font = tabs_font(t);
    int x = w->base.x, y = w->base.y;

    if (event->type == UI_EVENT_MOUSE_MOVE && font) {
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
        /* Don't consume -- let children also get the event */
        return 0;
    }

    if (event->type == UI_EVENT_MOUSE_BUTTON &&
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

int clue_tabs_get_active(ClueTabs *tabs)
{
    return tabs ? tabs->active : -1;
}

void clue_tabs_set_active(ClueTabs *tabs, int index)
{
    if (!tabs || index < 0 || index >= tabs->tab_count) return;
    tabs->active = index;
}
