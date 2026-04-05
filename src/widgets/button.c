#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/button.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/app.h"

static UIFont *button_font(ClueButton *b)
{
    return b->base.style.font ? b->base.style.font : clue_app_default_font();
}

static void button_draw(ClueWidget *w)
{
    ClueButton *b = (ClueButton *)w;
    if (!b->label) return;

    const ClueTheme *th = clue_theme_get();

    UIColor bg;
    switch (b->state) {
    case CLUE_BUTTON_HOVER:   bg = th->button.bg_hover;   break;
    case CLUE_BUTTON_PRESSED: bg = th->button.bg_pressed;  break;
    default:                  bg = th->button.bg;          break;
    }

    /* Per-instance style override */
    if (w->style.bg_color.a > 0.001f) {
        bg = w->style.bg_color;
        if (b->state == CLUE_BUTTON_HOVER) {
            bg.r += 0.08f; bg.g += 0.08f; bg.b += 0.08f;
        } else if (b->state == CLUE_BUTTON_PRESSED) {
            bg.r -= 0.08f; bg.g -= 0.08f; bg.b -= 0.08f;
        }
    }

    float radius = w->style.corner_radius > 0.0f ? w->style.corner_radius : th->corner_radius;

    /* Subtle shadow behind button */
    if (b->state != CLUE_BUTTON_PRESSED) {
        clue_fill_rounded_rect(w->base.x + 1, w->base.y + 2,
                               w->base.w, w->base.h, radius,
                               UI_RGBAF(0, 0, 0, 0.2f));
    }

    /* Button face */
    int press_offset = (b->state == CLUE_BUTTON_PRESSED) ? 1 : 0;
    clue_fill_rounded_rect(w->base.x, w->base.y + press_offset,
                           w->base.w, w->base.h, radius, bg);

    UIFont *font = button_font(b);
    UIColor fg = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->button.fg;

    if (b->icon && font) {
        bool has_label = b->label && b->label[0];
        if (has_label) {
            /* Icon + label: icon centered above, label below */
            int thh = clue_font_line_height(font);
            int spacing = 6;
            int total_h = b->icon_h + spacing + thh;
            int top_y = w->base.y + (w->base.h - total_h) / 2 + press_offset;

            int ix = w->base.x + (w->base.w - b->icon_w) / 2;
            clue_draw_image(b->icon, ix, top_y, b->icon_w, b->icon_h);

            int tw = clue_font_text_width(font, b->label);
            int tx = w->base.x + (w->base.w - tw) / 2;
            int ty = top_y + b->icon_h + spacing;
            clue_draw_text(tx, ty, b->label, font, fg);
        } else {
            /* Icon only: centered */
            int ix = w->base.x + (w->base.w - b->icon_w) / 2;
            int iy = w->base.y + (w->base.h - b->icon_h) / 2 + press_offset;
            clue_draw_image(b->icon, ix, iy, b->icon_w, b->icon_h);
        }
    } else if (font) {
        /* Label only (original behaviour) */
        int tw = clue_font_text_width(font, b->label);
        int thh = clue_font_line_height(font);
        int tx = w->base.x + (w->base.w - tw) / 2;
        int ty = w->base.y + (w->base.h - thh) / 2 + press_offset;
        clue_draw_text(tx, ty, b->label, font, fg);
    }
}

static void button_layout(ClueWidget *w)
{
    ClueButton *b = (ClueButton *)w;
    UIFont *font = button_font(b);
    if (!font || !b->label) return;

    ClueStyle *s = &w->style;
    int pad_h = s->padding_left + s->padding_right;
    int pad_v = s->padding_top + s->padding_bottom;

    /* Use defaults if no padding set */
    if (pad_h == 0) pad_h = 32;
    if (pad_v == 0) pad_v = 16;

    int text_w = clue_font_text_width(font, b->label);
    int text_h = clue_font_line_height(font);

    if (b->icon) {
        bool has_label = b->label && b->label[0];
        int content_w = has_label ? (b->icon_w > text_w ? b->icon_w : text_w) : b->icon_w;
        int content_h = has_label ? b->icon_h + 6 + text_h : b->icon_h;
        w->base.w = content_w + pad_h;
        w->base.h = content_h + pad_v;
    } else {
        w->base.w = text_w + pad_h;
        w->base.h = text_h + pad_v;
    }
}

static int button_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueButton *b = (ClueButton *)w;
    int mx, my;

    switch (event->type) {
    case UI_EVENT_MOUSE_MOVE:
        mx = event->mouse_move.x;
        my = event->mouse_move.y;
        if (mx >= w->base.x && mx < w->base.x + w->base.w &&
            my >= w->base.y && my < w->base.y + w->base.h) {
            if (b->state != CLUE_BUTTON_PRESSED)
                b->state = CLUE_BUTTON_HOVER;
            return 1;
        }
        b->state = CLUE_BUTTON_NORMAL;
        return 0;

    case UI_EVENT_MOUSE_BUTTON:
        mx = event->mouse_button.x;
        my = event->mouse_button.y;
        if (mx >= w->base.x && mx < w->base.x + w->base.w &&
            my >= w->base.y && my < w->base.y + w->base.h) {
            if (event->mouse_button.pressed) {
                b->state = CLUE_BUTTON_PRESSED;
                clue_focus_widget(NULL);
            } else {
                if (b->state == CLUE_BUTTON_PRESSED) {
                    clue_signal_emit(b, "clicked");
                }
                b->state = CLUE_BUTTON_HOVER;
            }
            return 1;
        }
        b->state = CLUE_BUTTON_NORMAL;
        return 0;

    default:
        return 0;
    }
}

static void button_destroy(ClueWidget *w)
{
    ClueButton *b = (ClueButton *)w;
    free(b->label);
    b->label = NULL;
}

static const ClueWidgetVTable button_vtable = {
    .draw         = button_draw,
    .layout       = button_layout,
    .handle_event = button_handle_event,
    .destroy      = button_destroy,
};

ClueButton *clue_button_new(const char *label)
{
    ClueButton *b = calloc(1, sizeof(ClueButton));
    if (!b) return NULL;

    clue_cwidget_init(&b->base, &button_vtable);
    b->label = label ? strdup(label) : strdup("");
    b->state = CLUE_BUTTON_NORMAL;
    b->icon = 0;
    b->icon_w = 0;
    b->icon_h = 0;

    button_layout(&b->base);

    return b;
}

void clue_button_set_label(ClueButton *button, const char *label)
{
    if (!button) return;
    free(button->label);
    button->label = label ? strdup(label) : strdup("");
    button_layout(&button->base);
}

void clue_button_set_icon(ClueButton *button, UITexture icon, int w, int h)
{
    if (!button) return;
    button->icon = icon;
    button->icon_w = w;
    button->icon_h = h;
    button_layout(&button->base);
}
