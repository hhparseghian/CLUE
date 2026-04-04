#include <stdlib.h>
#include "clue/toolbar.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define TB_PAD 6

static void toolbar_draw(ClueWidget *w)
{
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    /* Background */
    clue_fill_rect(x, y, bw, bh, th->surface);
    /* Bottom border */
    clue_fill_rect(x, y + bh - 1, bw, 1, th->surface_border);
}

static void toolbar_layout(ClueWidget *w)
{
    ClueToolbar *tb = (ClueToolbar *)w;
    UIFont *font = clue_app_default_font();
    int max_h = font ? clue_font_line_height(font) + TB_PAD * 2 : 32;

    /* Layout children horizontally */
    int cx = w->base.x + TB_PAD;
    int cy = w->base.y + TB_PAD;
    for (int i = 0; i < w->base.child_count; i++) {
        ClueWidget *child = (ClueWidget *)w->base.children[i];
        child->base.x = cx;
        child->base.y = cy;
        cx += child->base.w + tb->spacing;
        if (child->base.h + TB_PAD * 2 > max_h)
            max_h = child->base.h + TB_PAD * 2;
    }

    w->base.h = max_h;
    /* Width set by parent (hexpand) */
}

static const ClueWidgetVTable toolbar_vtable = {
    .draw   = toolbar_draw,
    .layout = toolbar_layout,
};

ClueToolbar *clue_toolbar_new(int spacing)
{
    ClueToolbar *tb = calloc(1, sizeof(ClueToolbar));
    if (!tb) return NULL;

    clue_cwidget_init(&tb->base, &toolbar_vtable);
    tb->base.type_id = CLUE_WIDGET_TOOLBAR;
    tb->base.style.hexpand = true;
    tb->spacing = spacing > 0 ? spacing : 4;

    return tb;
}
