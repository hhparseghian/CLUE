#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/tooltip.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/tabs.h"
#include "clue/clue_widget.h"

#define TIP_PAD_H 10
#define TIP_PAD_V 6

/* We store tooltip text in the widget's user_data field */

void clue_tooltip_set(ClueWidget *widget, const char *text)
{
    if (!widget) return;
    free(widget->base.user_data);
    widget->base.user_data = text ? strdup(text) : NULL;
}

const char *clue_tooltip_get(ClueWidget *widget)
{
    return widget ? (const char *)widget->base.user_data : NULL;
}

/* Global hover tracking */
static ClueWidget *g_hovered = NULL;

static ClueWidget *find_hovered(ClueWidget *w, int mx, int my)
{
    if (!w || !w->base.visible) return NULL;

    /* Traverse into active tab page (stored separately from children) */
    if (w->type_id == CLUE_WIDGET_TABS) {
        ClueTabs *tabs = (ClueTabs *)w;
        int active = clue_tabs_get_active(tabs);
        if (active >= 0 && active < tabs->tab_count && tabs->tab_pages[active]) {
            ClueWidget *hit = find_hovered(tabs->tab_pages[active], mx, my);
            if (hit) return hit;
        }
    }

    /* Check children back-to-front */
    for (int i = w->base.child_count - 1; i >= 0; i--) {
        ClueWidget *hit = find_hovered((ClueWidget *)w->base.children[i], mx, my);
        if (hit) return hit;
    }

    /* Check this widget */
    if (mx >= w->base.x && mx < w->base.x + w->base.w &&
        my >= w->base.y && my < w->base.y + w->base.h &&
        w->base.user_data) {
        return w;
    }

    return NULL;
}

void clue_tooltip_update(ClueWidget *root, int mouse_x, int mouse_y)
{
    g_hovered = find_hovered(root, mouse_x, mouse_y);
}

void clue_tooltip_draw(int mouse_x, int mouse_y)
{
    (void)mouse_x;
    (void)mouse_y;
    if (!g_hovered || !g_hovered->base.user_data) return;

    const char *text = (const char *)g_hovered->base.user_data;
    const ClueTheme *th = clue_theme_get();
    UIFont *font = clue_app_default_font();
    if (!font) return;

    int tw = clue_font_text_width(font, text);
    int fh = clue_font_line_height(font);
    int w = tw + TIP_PAD_H * 2;
    int h = fh + TIP_PAD_V * 2;

    /* Anchor tooltip below the widget, not the cursor */
    int x = g_hovered->base.x;
    int y = g_hovered->base.y + g_hovered->base.h + 4;

    /* Keep on screen */
    ClueApp *app = clue_app_get();
    if (app) {
        if (x + w > app->window->w) x = app->window->w - w;
        if (x < 0) x = 0;
        if (y + h > app->window->h) y = g_hovered->base.y - h - 4;
    }

    /* Drop shadow */
    clue_fill_rounded_rect(x + 2, y + 3, w, h, 5.0f,
                           UI_RGBAF(0, 0, 0, 0.3f));

    /* Background + border */
    clue_fill_rounded_rect(x, y, w, h, 5.0f, th->surface);
    clue_draw_rounded_rect(x, y, w, h, 5.0f, 1.5f, th->surface_border);
    clue_draw_text(x + TIP_PAD_H, y + TIP_PAD_V, text, font, th->fg);
}
