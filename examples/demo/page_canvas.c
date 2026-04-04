#include "demo.h"

#define PAINT_MAX 4096

typedef struct {
    int x, y;
    UIColor color;
} PaintPoint;

static PaintPoint g_paint_points[PAINT_MAX];
static int g_paint_count = 0;
static int g_paint_last_x = -1, g_paint_last_y = -1;
static UIColor g_paint_color = {0.86f, 0.86f, 1.0f, 1.0f};

static void canvas_draw_cb(int x, int y, int w, int h, void *data)
{
    (void)data; (void)w; (void)h;
    for (int i = 1; i < g_paint_count; i++) {
        if (g_paint_points[i].x < 0 || g_paint_points[i - 1].x < 0)
            continue;
        clue_draw_line(x + g_paint_points[i - 1].x,
                       y + g_paint_points[i - 1].y,
                       x + g_paint_points[i].x,
                       y + g_paint_points[i].y,
                       3.0f, g_paint_points[i].color);
    }
    if (g_paint_count == 0) {
        clue_draw_text_default(x + 10, y + 10, "Draw here with the mouse!",
                               UI_RGB(100, 100, 120));
    }
}

static void canvas_event_cb(const ClueCanvasEvent *ev, void *data)
{
    (void)data;
    if (ev->type == CLUE_CANVAS_PRESS) {
        if (g_paint_count > 0 && g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){-1, -1, {0}};
            g_paint_count++;
        }
        if (g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){ev->x, ev->y, g_paint_color};
            g_paint_count++;
        }
        g_paint_last_x = ev->x;
        g_paint_last_y = ev->y;
    } else if (ev->type == CLUE_CANVAS_MOTION && g_paint_last_x >= 0) {
        if (g_paint_count < PAINT_MAX) {
            g_paint_points[g_paint_count] = (PaintPoint){ev->x, ev->y, g_paint_color};
            g_paint_count++;
        }
        g_paint_last_x = ev->x;
        g_paint_last_y = ev->y;
    } else if (ev->type == CLUE_CANVAS_RELEASE) {
        g_paint_last_x = -1;
        g_paint_last_y = -1;
    }
}

static void on_canvas_clear(void *widget, void *data)
{
    (void)widget; (void)data;
    g_paint_count = 0;
    clue_label_set_text(g_status, "Canvas cleared");
}

static void on_color_changed(void *widget, void *data)
{
    (void)data;
    ClueColorPicker *cp = widget;
    g_paint_color = clue_colorpicker_get_color(cp);
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

    ClueColorPicker *cp = clue_colorpicker_new();
    clue_colorpicker_set_color(cp, g_paint_color);
    clue_signal_connect(cp, "changed", on_color_changed, NULL);

    clue_container_add(body, canvas);
    clue_container_add(body, cp);

    clue_container_add(page, top);
    clue_container_add(page, body);

    return page;
}
