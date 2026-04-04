#include "demo.h"
#include "clue/menu.h"
#include <stdlib.h>

#define PAINT_MAX 4096
static float g_brush_size = 3.0f;

typedef struct {
    int x, y;
    UIColor color;
    float size;
} PaintPoint;

static PaintPoint g_paint_points[PAINT_MAX];
static int g_paint_count = 0;
static int g_paint_last_x = -1, g_paint_last_y = -1;
static UIColor g_paint_color = {0.86f, 0.86f, 1.0f, 1.0f};
static ClueCanvas *g_paint_canvas = NULL;

/* Forward declarations */
static ClueMenu *get_canvas_context_menu(void);

/* Stamp circles along a line using Bresenham */
static void stamp_line(int x0, int y0, int x1, int y1, int r, UIColor col)
{
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        clue_fill_circle(x0, y0, r, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

static void canvas_draw_cb(int x, int y, int w, int h, void *data)
{
    for (int i = 0; i < g_paint_count; i++) {
        if (g_paint_points[i].x < 0) continue;
        int r = (int)(g_paint_points[i].size * 0.5f + 0.5f);
        if (r < 1) r = 1;
        if (i > 0 && g_paint_points[i - 1].x >= 0) {
            stamp_line(x + g_paint_points[i - 1].x,
                       y + g_paint_points[i - 1].y,
                       x + g_paint_points[i].x,
                       y + g_paint_points[i].y,
                       r, g_paint_points[i].color);
        } else {
            clue_fill_circle(x + g_paint_points[i].x,
                             y + g_paint_points[i].y,
                             r, g_paint_points[i].color);
        }
    }
    if (g_paint_count == 0) {
        clue_draw_text_default(x + 10, y + 10, "Draw here with the mouse!",
                               UI_RGB(100, 100, 120));
    }
}

static void canvas_event_cb(const ClueCanvasEvent *ev, void *data)
{
    /* Right-click: show context menu */
    if (ev->type == CLUE_CANVAS_PRESS && ev->button == 1) {
        int wx = ev->x + g_paint_canvas->base.base.x;
        int wy = ev->y + g_paint_canvas->base.base.y;
        clue_context_menu_show(get_canvas_context_menu(), wx, wy);
        return;
    }

    if (ev->type == CLUE_CANVAS_PRESS) {
        if (g_paint_count > 0 && g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){-1, -1, {0, 0, 0, 0}, 0};
            g_paint_count++;
        }
        if (g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){ev->x, ev->y, g_paint_color, g_brush_size};
            g_paint_count++;
        }
        g_paint_last_x = ev->x;
        g_paint_last_y = ev->y;
    } else if (ev->type == CLUE_CANVAS_MOTION && g_paint_last_x >= 0) {
        if (g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){ev->x, ev->y, g_paint_color, g_brush_size};
            g_paint_count++;
        }
        g_paint_last_x = ev->x;
        g_paint_last_y = ev->y;
    } else if (ev->type == CLUE_CANVAS_RELEASE) {
        g_paint_last_x = -1;
        g_paint_last_y = -1;
    }
}

static void ctx_clear(void *w, void *d)
{
    g_paint_count = 0;
    clue_label_set_text(g_status, "Canvas cleared");
}

static void ctx_brush_small(void *w, void *d) { g_brush_size = 2.0f; }
static void ctx_brush_medium(void *w, void *d) { g_brush_size = 4.0f; }
static void ctx_brush_large(void *w, void *d) { g_brush_size = 8.0f; }

static ClueMenu *get_canvas_context_menu(void)
{
    static ClueMenu *menu = NULL;
    if (!menu) {
        menu = clue_menu_new();
        clue_menu_add_item(menu, "Clear", ctx_clear, NULL);
        clue_menu_add_separator(menu);
        clue_menu_add_item(menu, "Small Brush", ctx_brush_small, NULL);
        clue_menu_add_item(menu, "Medium Brush", ctx_brush_medium, NULL);
        clue_menu_add_item(menu, "Large Brush", ctx_brush_large, NULL);
    }
    return menu;
}

static void on_canvas_clear(ClueButton *button, void *data)
{
    g_paint_count = 0;
    clue_label_set_text(g_status, "Canvas cleared");
}

static void on_color_changed(ClueColorPicker *picker, void *data)
{
    g_paint_color = clue_colorpicker_get_color(picker);
}

ClueBox *build_canvas_page(void)
{
    ClueBox *page = clue_box_new(CLUE_VERTICAL, 8);
    clue_style_set_padding(&page->base.style, 12);
    page->base.style.corner_radius = 0;
    page->base.style.hexpand = true;
    page->base.style.vexpand = true;

    ClueBox *top = clue_box_new(CLUE_HORIZONTAL, 8);
    ClueLabel *lbl = clue_label_new("Paint:");
    lbl->base.style.fg_color = UI_RGB(180, 180, 190);
    ClueButton *clear_btn = clue_button_new("Clear");
    clue_signal_connect(clear_btn, "clicked", on_canvas_clear, NULL);
    clue_container_add(top, lbl);
    clue_container_add(top, clear_btn);

    ClueBox *body = clue_box_new(CLUE_HORIZONTAL, 8);
    body->base.style.hexpand = true;
    body->base.style.vexpand = true;

    ClueCanvas *canvas = clue_canvas_new(500, 300);
    canvas->base.style.hexpand = true;
    canvas->base.style.vexpand = true;
    clue_canvas_set_draw(canvas, canvas_draw_cb, NULL);
    clue_canvas_set_event(canvas, canvas_event_cb, NULL);
    g_paint_canvas = canvas;

    ClueColorPicker *cp = clue_colorpicker_new();
    clue_colorpicker_set_color(cp, g_paint_color);
    clue_signal_connect(cp, "changed", on_color_changed, NULL);

    clue_container_add(body, canvas);
    clue_container_add(body, cp);

    clue_container_add(page, top);
    clue_container_add(page, body);

    return page;
}
