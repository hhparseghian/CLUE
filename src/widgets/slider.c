#include <stdlib.h>
#include "clue/slider.h"
#include "clue/draw.h"
#include "clue/theme.h"
#include "clue/app.h"

#define TRACK_H   4
#define THUMB_R   8
#define SLIDER_H  (THUMB_R * 2 + 4)

static float normalize(ClueSlider *s)
{
    if (s->max_val <= s->min_val) return 0.0f;
    float n = (s->value - s->min_val) / (s->max_val - s->min_val);
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n;
}

static void slider_draw(ClueWidget *w)
{
    ClueSlider *s = (ClueSlider *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w;
    int cy = y + w->base.h / 2;
    float n = normalize(s);

    int track_x = x + THUMB_R;
    int track_w = bw - THUMB_R * 2;

    const ClueTheme *th = clue_theme_get();

    /* Track background */
    clue_fill_rounded_rect(track_x, cy - TRACK_H / 2,
                           track_w, TRACK_H,
                           TRACK_H / 2.0f, th->slider.track);

    /* Track fill */
    int fill_w = (int)(n * track_w);
    if (fill_w > 0) {
        clue_fill_rounded_rect(track_x, cy - TRACK_H / 2,
                               fill_w, TRACK_H,
                               TRACK_H / 2.0f, th->slider.fill);
    }

    /* Thumb shadow */
    int thumb_x = track_x + fill_w;
    clue_fill_circle(thumb_x + 1, cy + 2, THUMB_R,
                     UI_RGBAF(0, 0, 0, 0.25f));

    /* Thumb */
    UIColor thumb_color = s->dragging ? th->slider.thumb_active : th->slider.thumb;
    clue_fill_circle(thumb_x, cy, THUMB_R, thumb_color);

    /* Thumb border */
    clue_draw_circle(thumb_x, cy, THUMB_R, 1.5f,
                     UI_RGBAF(0, 0, 0, 0.15f));
}

static void slider_layout(ClueWidget *w)
{
    if (w->base.w == 0) w->base.w = 200;
    w->base.h = SLIDER_H;
}

static void update_value_from_x(ClueSlider *s, int mx)
{
    int x = s->base.base.x + THUMB_R;
    int w = s->base.base.w - THUMB_R * 2;
    float n = (float)(mx - x) / (float)w;
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    float new_val = s->min_val + n * (s->max_val - s->min_val);
    if (new_val != s->value) {
        s->value = new_val;
        clue_signal_emit(s, "changed");
    }
}

static int slider_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueSlider *s = (ClueSlider *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    switch (event->type) {
    case UI_EVENT_MOUSE_BUTTON: {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;

        if (event->mouse_button.pressed && event->mouse_button.btn == 0 && inside) {
            s->dragging = true;
            clue_capture_mouse(&w->base);
            update_value_from_x(s, mx);
            return 1;
        }
        if (!event->mouse_button.pressed && event->mouse_button.btn == 0) {
            if (s->dragging) {
                s->dragging = false;
                clue_release_mouse();
            }
            return inside ? 1 : 0;
        }
        return 0;
    }

    case UI_EVENT_MOUSE_MOVE:
        if (s->dragging) {
            update_value_from_x(s, event->mouse_move.x);
            return 1;
        }
        return 0;

    default:
        return 0;
    }
}

static const ClueWidgetVTable slider_vtable = {
    .draw         = slider_draw,
    .layout       = slider_layout,
    .handle_event = slider_handle_event,
    .destroy      = NULL,
};

ClueSlider *clue_slider_new(float min_val, float max_val, float value)
{
    ClueSlider *s = calloc(1, sizeof(ClueSlider));
    if (!s) return NULL;

    clue_cwidget_init(&s->base, &slider_vtable);
    s->min_val = min_val;
    s->max_val = max_val;
    s->value = value;

    slider_layout(&s->base);
    return s;
}

float clue_slider_get_value(ClueSlider *slider)
{
    return slider ? slider->value : 0.0f;
}

void clue_slider_set_value(ClueSlider *slider, float value)
{
    if (!slider) return;
    slider->value = value;
    if (slider->value < slider->min_val) slider->value = slider->min_val;
    if (slider->value > slider->max_val) slider->value = slider->max_val;
}
