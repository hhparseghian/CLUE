#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/menu.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

static void menubar_set_modal(ClueMenuBar *bar, bool on)
{
    ClueApp *app = clue_app_get();
    if (app) app->modal_widget = on ? (ClueWidget *)bar : NULL;
}

#define MENU_PAD_H    12
#define MENU_PAD_V    6
#define MENU_ITEM_H   28
#define MENU_SEP_H    9
#define MENUBAR_PAD_H 12
#define MENUBAR_PAD_V 6

/* ------------------------------------------------------------------ */
/* Popup Menu                                                          */
/* ------------------------------------------------------------------ */

static ClueFont *menu_font(void)
{
    return clue_app_default_font();
}

static int menu_item_height(ClueMenuItem *item)
{
    return item->type == CLUE_MENU_ITEM_SEPARATOR ? MENU_SEP_H : MENU_ITEM_H;
}

static int menu_total_height(ClueMenu *m)
{
    int h = MENU_PAD_V * 2;
    for (int i = 0; i < m->item_count; i++)
        h += menu_item_height(&m->items[i]);
    return h;
}

static int menu_width(ClueMenu *m)
{
    ClueFont *font = menu_font();
    if (!font) return 150;
    int max_w = 100;
    for (int i = 0; i < m->item_count; i++) {
        if (m->items[i].type == CLUE_MENU_ITEM_SEPARATOR) continue;
        int w = clue_font_text_width(font, m->items[i].label);
        if (m->items[i].shortcut)
            w += clue_font_text_width(font, m->items[i].shortcut) + 30;
        if (w > max_w) max_w = w;
    }
    return max_w + MENU_PAD_H * 2;
}

static void menu_draw(ClueWidget *w)
{
    ClueMenu *m = (ClueMenu *)w;
    if (!m->open) return;

    const ClueTheme *th = clue_theme_get();
    ClueFont *font = menu_font();
    int x = m->popup_x, y = m->popup_y;
    int mw = menu_width(m);
    int mh = menu_total_height(m);

    clue_fill_rounded_rect(x, y, mw, mh, th->corner_radius, th->surface);
    clue_draw_rounded_rect(x, y, mw, mh, th->corner_radius, 1.0f,
                           th->surface_border);

    int iy = y + MENU_PAD_V;
    for (int i = 0; i < m->item_count; i++) {
        ClueMenuItem *item = &m->items[i];
        int ih = menu_item_height(item);

        if (item->type == CLUE_MENU_ITEM_SEPARATOR) {
            clue_draw_line(x + MENU_PAD_H, iy + ih / 2,
                           x + mw - MENU_PAD_H, iy + ih / 2,
                           1.0f, th->surface_border);
        } else {
            if (i == m->hovered) {
                clue_fill_rect(x + 2, iy, mw - 4, ih, th->accent);
            }

            ClueColor fg = (i == m->hovered) ? th->fg_bright :
                         item->enabled ? th->fg : th->fg_dim;

            if (font) {
                int ty = iy + (ih - clue_font_line_height(font)) / 2;
                clue_draw_text(x + MENU_PAD_H, ty, item->label, font, fg);
                if (item->shortcut) {
                    int sw = clue_font_text_width(font, item->shortcut);
                    clue_draw_text(x + mw - MENU_PAD_H - sw, ty,
                                   item->shortcut, font, th->fg_dim);
                }
            }
        }

        iy += ih;
    }
}

static int menu_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueMenu *m = (ClueMenu *)w;
    if (!m->open) return 0;

    int x = m->popup_x, y = m->popup_y;
    int mw = menu_width(m);
    int mh = menu_total_height(m);

    if (event->type == CLUE_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        m->hovered = -1;

        if (mx >= x && mx < x + mw && my >= y && my < y + mh) {
            int iy = y + MENU_PAD_V;
            for (int i = 0; i < m->item_count; i++) {
                int ih = menu_item_height(&m->items[i]);
                if (my >= iy && my < iy + ih &&
                    m->items[i].type == CLUE_MENU_ITEM_NORMAL) {
                    m->hovered = i;
                    break;
                }
                iy += ih;
            }
            return 1;
        }
        return 0;
    }

    if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;

        if (mx >= x && mx < x + mw && my >= y && my < y + mh) {
            if (m->hovered >= 0 && m->items[m->hovered].enabled &&
                m->items[m->hovered].callback) {
                m->items[m->hovered].callback(m, m->items[m->hovered].user_data);
            }
            m->open = false;
            return 1;
        }

        m->open = false;
        return 0;
    }

    return m->open ? 1 : 0;
}

static void menu_destroy(ClueWidget *w)
{
    ClueMenu *m = (ClueMenu *)w;
    for (int i = 0; i < m->item_count; i++) {
        free(m->items[i].label);
        free(m->items[i].shortcut);
    }
}

static const ClueWidgetVTable menu_vtable = {
    .draw = menu_draw, .layout = NULL,
    .handle_event = menu_handle_event, .destroy = menu_destroy,
};

ClueMenu *clue_menu_new(void)
{
    ClueMenu *m = calloc(1, sizeof(ClueMenu));
    if (!m) return NULL;
    clue_cwidget_init(&m->base, &menu_vtable);
    m->base.base.intercept_events = true;
    m->hovered = -1;
    return m;
}

void clue_menu_add_item(ClueMenu *menu, const char *label,
                        ClueSignalCallback callback, void *user_data)
{
    clue_menu_add_item_shortcut(menu, label, NULL, callback, user_data);
}

void clue_menu_add_item_shortcut(ClueMenu *menu, const char *label,
                                 const char *shortcut,
                                 ClueSignalCallback callback, void *user_data)
{
    if (!menu || menu->item_count >= CLUE_MENU_MAX_ITEMS) return;
    ClueMenuItem *item = &menu->items[menu->item_count++];
    item->type = CLUE_MENU_ITEM_NORMAL;
    item->label = strdup(label ? label : "");
    item->shortcut = shortcut ? strdup(shortcut) : NULL;
    item->callback = callback;
    item->user_data = user_data;
    item->enabled = true;
}

void clue_menu_add_separator(ClueMenu *menu)
{
    if (!menu || menu->item_count >= CLUE_MENU_MAX_ITEMS) return;
    ClueMenuItem *item = &menu->items[menu->item_count++];
    item->type = CLUE_MENU_ITEM_SEPARATOR;
    item->enabled = false;
}

void clue_menu_popup(ClueMenu *menu, int x, int y)
{
    if (!menu) return;
    menu->popup_x = x;
    menu->popup_y = y;
    menu->open = true;
    menu->hovered = -1;
}

void clue_menu_close(ClueMenu *menu)
{
    if (menu) menu->open = false;
}

/* ------------------------------------------------------------------ */
/* Menu Bar                                                            */
/* ------------------------------------------------------------------ */

static void menubar_draw(ClueWidget *w)
{
    ClueMenuBar *bar = (ClueMenuBar *)w;
    const ClueTheme *th = clue_theme_get();
    ClueFont *font = menu_font();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    clue_fill_rect(x, y, bw, bh, th->surface);
    clue_draw_line(x, y + bh, x + bw, y + bh, 1.5f, th->surface_border);

    if (!font) return;

    int tx = x;
    for (int i = 0; i < bar->menu_count; i++) {
        int tw = clue_font_text_width(font, bar->labels[i]) + MENUBAR_PAD_H * 2;

        if (i == bar->active) {
            clue_fill_rect(tx, y, tw, bh, th->accent);
        } else if (i == bar->hovered) {
            clue_fill_rect(tx, y, tw, bh, th->surface_hover);
        }

        ClueColor fg = (i == bar->active) ? th->fg_bright : th->fg;
        int ty = y + (bh - clue_font_line_height(font)) / 2;
        clue_draw_text(tx + MENUBAR_PAD_H, ty, bar->labels[i], font, fg);

        tx += tw;
    }

    /* Menu dropdown is drawn in overlay pass -- see clue_menubar_draw_overlay */
}

static void menubar_layout(ClueWidget *w)
{
    ClueFont *font = menu_font();
    w->base.h = font ? clue_font_line_height(font) + MENUBAR_PAD_V * 2 : 30;
}

static int menubar_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueMenuBar *bar = (ClueMenuBar *)w;
    ClueFont *font = menu_font();
    int x = w->base.x, y = w->base.y, bh = w->base.h;

    /* Forward to active menu first */
    if (bar->active >= 0 && bar->menus[bar->active] &&
        bar->menus[bar->active]->open) {
        int r = menu_handle_event(&bar->menus[bar->active]->base, event);
        if (r) {
            if (!bar->menus[bar->active]->open) {
                bar->active = -1;
                menubar_set_modal(bar, false);
            }
            return 1;
        }
        /* Menu didn't consume -- it closed on outside click.
         * Let menubar handle the click (might be on another menu label). */
    }

    if (!font) return 0;

    if (event->type == CLUE_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        bar->hovered = -1;

        if (my >= y && my < y + bh) {
            int tx = x;
            for (int i = 0; i < bar->menu_count; i++) {
                int tw = clue_font_text_width(font, bar->labels[i]) + MENUBAR_PAD_H * 2;
                if (mx >= tx && mx < tx + tw) {
                    bar->hovered = i;
                    /* If another menu is open, switch to hovered */
                    if (bar->active >= 0 && bar->active != i) {
                        bar->menus[bar->active]->open = false;
                        bar->active = i;
                        clue_menu_popup(bar->menus[i], tx, y + bh);
                    }
                    break;
                }
                tx += tw;
            }
        }
        return 0;
    }

    if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;

        if (my >= y && my < y + bh) {
            int tx = x;
            for (int i = 0; i < bar->menu_count; i++) {
                int tw = clue_font_text_width(font, bar->labels[i]) + MENUBAR_PAD_H * 2;
                if (mx >= tx && mx < tx + tw) {
                    /* Toggle: if this menu is active (open or just closed), close it */
                    if (bar->active == i) {
                        if (bar->menus[i]->open)
                            bar->menus[i]->open = false;
                        bar->active = -1;
                        menubar_set_modal(bar, false);
                    } else {
                        if (bar->active >= 0)
                            bar->menus[bar->active]->open = false;
                        bar->active = i;
                        clue_menu_popup(bar->menus[i], tx, y + bh);
                        menubar_set_modal(bar, true);
                    }
                    return 1;
                }
                tx += tw;
            }
        }

        /* Click outside: close menu and consume the event */
        if (bar->active >= 0) {
            bar->menus[bar->active]->open = false;
            bar->active = -1;
            menubar_set_modal(bar, false);
            return 1;
        }
    }

    /* While a menu is open, consume all mouse events */
    if (bar->active >= 0 && bar->menus[bar->active] &&
        bar->menus[bar->active]->open &&
        (event->type == CLUE_EVENT_MOUSE_BUTTON ||
         event->type == CLUE_EVENT_MOUSE_MOVE ||
         event->type == CLUE_EVENT_MOUSE_SCROLL)) {
        return 1;
    }

    return 0;
}

static void menubar_destroy(ClueWidget *w)
{
    ClueMenuBar *bar = (ClueMenuBar *)w;
    for (int i = 0; i < bar->menu_count; i++) {
        free(bar->labels[i]);
        /* Menus are owned externally or by the bar -- destroy here */
        if (bar->menus[i]) clue_cwidget_destroy((ClueWidget *)bar->menus[i]);
    }
}

static const ClueWidgetVTable menubar_vtable = {
    .draw = menubar_draw, .layout = menubar_layout,
    .handle_event = menubar_handle_event, .destroy = menubar_destroy,
};

ClueMenuBar *clue_menubar_new(void)
{
    ClueMenuBar *bar = calloc(1, sizeof(ClueMenuBar));
    if (!bar) return NULL;
    clue_cwidget_init(&bar->base, &menubar_vtable);
    bar->base.type_id = CLUE_WIDGET_MENUBAR;
    bar->base.base.intercept_events = true;
    bar->base.base.skip_child_draw = true;
    bar->active = -1;
    bar->hovered = -1;
    return bar;
}

void clue_menubar_add(ClueMenuBar *bar, const char *label, ClueMenu *menu)
{
    if (!bar || bar->menu_count >= CLUE_MENU_MAX_ITEMS) return;
    int idx = bar->menu_count;
    bar->labels[idx] = strdup(label ? label : "");
    bar->menus[idx] = menu;
    bar->menu_count++;
}

void clue_menubar_draw_overlay(ClueMenuBar *bar)
{
    if (!bar || bar->active < 0 || bar->active >= bar->menu_count) return;
    if (!bar->menus[bar->active] || !bar->menus[bar->active]->open) return;
    menu_draw(&bar->menus[bar->active]->base);
}

/* --- Context menu (global popup) --- */

static ClueMenu *g_context_menu = NULL;

void clue_context_menu_show(ClueMenu *menu, int x, int y)
{
    if (g_context_menu && g_context_menu != menu)
        clue_menu_close(g_context_menu);
    clue_menu_popup(menu, x, y);
    g_context_menu = menu;
}

void clue_context_menu_close(void)
{
    if (g_context_menu) {
        clue_menu_close(g_context_menu);
        g_context_menu = NULL;
    }
}

void clue_context_menu_draw(void)
{
    if (g_context_menu && g_context_menu->open)
        menu_draw(&g_context_menu->base);
}

int clue_context_menu_dispatch(ClueEvent *event)
{
    if (!g_context_menu || !g_context_menu->open) return 0;
    int r = menu_handle_event(&g_context_menu->base, event);
    /* Menu closed itself after item click */
    if (!g_context_menu->open)
        g_context_menu = NULL;
    return r;
}

