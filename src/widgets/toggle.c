#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/toggle.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/timer.h"

#define TRACK_W  40
#define TRACK_H  22
#define THUMB_R  8
#define THUMB_PAD 3
#define LABEL_GAP 10
#define ANIM_STEP 0.15f

static ClueFont *toggle_font(ClueToggle *t)
{
    return t->base.style.font ? t->base.style.font : clue_app_default_font();
}

static bool anim_tick(void *data)
{
    ClueToggle *t = data;
    float target = t->on ? 1.0f : 0.0f;
    if (t->anim_pos < target) {
        t->anim_pos += ANIM_STEP;
        if (t->anim_pos >= target) { t->anim_pos = target; return false; }
    } else {
        t->anim_pos -= ANIM_STEP;
        if (t->anim_pos <= target) { t->anim_pos = target; return false; }
    }
    return true;
}

static void toggle_draw(ClueWidget *w)
{
    ClueToggle *t = (ClueToggle *)w;
    const ClueTheme *th = clue_theme_get();
    ClueFont *font = toggle_font(t);

    int x = w->base.x, y = w->base.y;
    int track_y = y + (w->base.h - TRACK_H) / 2;

    /* Track */
    float r = TRACK_H / 2.0f;
    ClueColor track_off = th->input.border;
    ClueColor track_on  = th->checkbox.box_checked;
    float p = t->anim_pos;
    ClueColor track = {
        track_off.r + (track_on.r - track_off.r) * p,
        track_off.g + (track_on.g - track_off.g) * p,
        track_off.b + (track_on.b - track_off.b) * p,
        1.0f
    };
    clue_fill_rounded_rect(x, track_y, TRACK_W, TRACK_H, r, track);

    /* Thumb */
    int thumb_min_x = x + THUMB_PAD + THUMB_R;
    int thumb_max_x = x + TRACK_W - THUMB_PAD - THUMB_R;
    int thumb_cx = thumb_min_x + (int)((thumb_max_x - thumb_min_x) * p);
    int thumb_cy = track_y + TRACK_H / 2;
    clue_fill_circle(thumb_cx, thumb_cy, THUMB_R, th->slider.thumb);

    /* Label */
    if (font && t->label && t->label[0]) {
        ClueColor fg = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->checkbox.fg;
        int text_y = y + (w->base.h - clue_font_line_height(font)) / 2;
        clue_draw_text(x + TRACK_W + LABEL_GAP, text_y, t->label, font, fg);
    }
}

static void toggle_layout(ClueWidget *w)
{
    ClueToggle *t = (ClueToggle *)w;
    ClueFont *font = toggle_font(t);
    if (!font) return;

    int tw = t->label ? clue_font_text_width(font, t->label) : 0;
    w->base.w = TRACK_W + LABEL_GAP + tw;
    w->base.h = clue_font_line_height(font);
    if (w->base.h < TRACK_H) w->base.h = TRACK_H;
}

static int toggle_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueToggle *t = (ClueToggle *)w;

    if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed &&
        event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        if (mx >= w->base.x && mx < w->base.x + w->base.w &&
            my >= w->base.y && my < w->base.y + w->base.h) {
            t->on = !t->on;
            clue_timer_repeat(16, anim_tick, t);
            clue_signal_emit(t, "toggled");
            return 1;
        }
    }
    return 0;
}

static void toggle_destroy(ClueWidget *w)
{
    ClueToggle *t = (ClueToggle *)w;
    free(t->label);
}

static const ClueWidgetVTable toggle_vtable = {
    .draw         = toggle_draw,
    .layout       = toggle_layout,
    .handle_event = toggle_handle_event,
    .destroy      = toggle_destroy,
};

ClueToggle *clue_toggle_new(const char *label)
{
    ClueToggle *t = calloc(1, sizeof(ClueToggle));
    if (!t) return NULL;

    clue_cwidget_init(&t->base, &toggle_vtable);
    t->base.type_id = CLUE_WIDGET_TOGGLE;
    t->label = label ? strdup(label) : strdup("");
    t->on = false;
    t->anim_pos = 0.0f;

    toggle_layout(&t->base);
    return t;
}

bool clue_toggle_is_on(ClueToggle *t)
{
    return t ? t->on : false;
}

void clue_toggle_set_on(ClueToggle *t, bool on)
{
    if (t) {
        t->on = on;
        t->anim_pos = on ? 1.0f : 0.0f;
    }
}
