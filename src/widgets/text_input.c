#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/text_input.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/window.h"
#include "clue/theme.h"
#include "clue/font.h"
#include "clue/timer.h"
#include <xkbcommon/xkbcommon-keysyms.h>

#define INPUT_PAD_H 10
#define INPUT_PAD_V 8
#define CURSOR_BLINK_MS 530

static bool blink_callback(void *data)
{
    ClueTextInput *inp = (ClueTextInput *)data;
    inp->cursor_visible = !inp->cursor_visible;
    return true;
}

static void start_blink(ClueTextInput *inp)
{
    inp->cursor_visible = true;
    if (inp->blink_timer_id == 0) {
        inp->blink_timer_id = clue_timer_repeat(CURSOR_BLINK_MS, blink_callback, inp);
    }
}

static void stop_blink(ClueTextInput *inp)
{
    if (inp->blink_timer_id) {
        clue_timer_cancel(inp->blink_timer_id);
        inp->blink_timer_id = 0;
    }
    inp->cursor_visible = false;
}

static UIFont *input_font(ClueTextInput *inp)
{
    return inp->base.style.font ? inp->base.style.font : clue_app_default_font();
}

static int text_width_n(UIFont *font, const char *text, int n)
{
    if (!font || !text || n <= 0) return 0;
    char buf[CLUE_TEXT_INPUT_MAX];
    int len = (int)strlen(text);
    if (n > len) n = len;
    if (n >= CLUE_TEXT_INPUT_MAX) n = CLUE_TEXT_INPUT_MAX - 1;
    memcpy(buf, text, n);
    buf[n] = '\0';
    return clue_font_text_width(font, buf);
}

static void text_input_draw(ClueWidget *w)
{
    ClueTextInput *inp = (ClueTextInput *)w;

    if (!w->base.focused && inp->blink_timer_id) {
        stop_blink(inp);
    }

    UIFont *font = input_font(inp);
    if (!font) return;

    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    const ClueTheme *th = clue_theme_get();
    float radius = w->style.corner_radius > 0.0f ? w->style.corner_radius : th->corner_radius;

    /* Subtle inset shadow */
    clue_fill_rounded_rect(x, y + 1, bw, bh, radius,
                           UI_RGBAF(0, 0, 0, 0.15f));

    /* Background */
    UIColor bg = w->style.bg_color.a > 0.001f ? w->style.bg_color : th->input.bg;
    clue_fill_rounded_rect(x, y, bw, bh, radius, bg);

    /* Border -- thicker when focused */
    UIColor border = w->base.focused ? th->input.focus_border : th->input.border;
    float border_w = w->base.focused ? 2.0f : 1.5f;
    clue_draw_rounded_rect(x, y, bw, bh, radius, border_w, border);

    /* Clip text to input area */
    clue_set_clip_rect(x + INPUT_PAD_H, y, bw - INPUT_PAD_H * 2, bh);

    int text_x = x + INPUT_PAD_H - inp->scroll_offset;
    int text_y = y + (bh - clue_font_line_height(font)) / 2;

    if (inp->text[0]) {
        UIColor fg = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->input.fg;
        clue_draw_text(text_x, text_y, inp->text, font, fg);
    } else if (inp->placeholder[0]) {
        /* Show placeholder even when focused if text is empty */
        clue_draw_text(text_x, text_y, inp->placeholder, font, th->input.placeholder);
    }

    /* Cursor (blinks) */
    if (w->base.focused && inp->cursor_visible) {
        int cursor_x = text_x + text_width_n(font, inp->text, inp->cursor);
        int cursor_y = y + INPUT_PAD_V;
        int cursor_h = clue_font_line_height(font);
        clue_fill_rect(cursor_x, cursor_y, 2, cursor_h, th->input.cursor);
    }

    clue_reset_clip_rect();
}

/* Keep cursor visible by adjusting scroll_offset */
static void ensure_cursor_visible(ClueTextInput *inp)
{
    UIFont *font = input_font(inp);
    if (!font) return;

    int cursor_px = text_width_n(font, inp->text, inp->cursor);
    int visible_w = inp->base.base.w - INPUT_PAD_H * 2;

    if (cursor_px - inp->scroll_offset > visible_w) {
        inp->scroll_offset = cursor_px - visible_w + 10;
    }
    if (cursor_px - inp->scroll_offset < 0) {
        inp->scroll_offset = cursor_px - 10;
        if (inp->scroll_offset < 0) inp->scroll_offset = 0;
    }
}

static int text_input_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueTextInput *inp = (ClueTextInput *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    switch (event->type) {
    case UI_EVENT_MOUSE_BUTTON: {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;

        if (event->mouse_button.pressed && inside) {
            clue_focus_widget(&w->base);
            start_blink(inp);

            /* Place cursor at click position */
            UIFont *font = input_font(inp);
            if (font) {
                int rel_x = mx - x - INPUT_PAD_H + inp->scroll_offset;
                int len = (int)strlen(inp->text);
                inp->cursor = len;
                for (int i = 0; i <= len; i++) {
                    if (text_width_n(font, inp->text, i) >= rel_x) {
                        inp->cursor = i;
                        break;
                    }
                }
            }
            return 1;
        }
        /* Don't steal focus here -- let the clicked widget claim it */
        return 0;
    }

    case UI_EVENT_KEY: {
        if (!w->base.focused || !event->key.pressed) return 0;

        /* Reset blink so cursor stays visible while typing */
        stop_blink(inp);
        start_blink(inp);

        int key = event->key.keycode;
        int len = (int)strlen(inp->text);

        /* Cursor movement */
        if (key == XKB_KEY_Left)  { if (inp->cursor > 0) inp->cursor--; ensure_cursor_visible(inp); return 1; }
        if (key == XKB_KEY_Right) { if (inp->cursor < len) inp->cursor++; ensure_cursor_visible(inp); return 1; }
        if (key == XKB_KEY_Home)  { inp->cursor = 0; ensure_cursor_visible(inp); return 1; }
        if (key == XKB_KEY_End)   { inp->cursor = len; ensure_cursor_visible(inp); return 1; }

        /* Backspace */
        if (key == XKB_KEY_BackSpace) {
            if (inp->cursor > 0) {
                memmove(&inp->text[inp->cursor - 1],
                        &inp->text[inp->cursor], len - inp->cursor + 1);
                inp->cursor--;
                ensure_cursor_visible(inp);
                clue_signal_emit(inp, "changed");
            }
            return 1;
        }

        /* Delete */
        if (key == XKB_KEY_Delete) {
            if (inp->cursor < len) {
                memmove(&inp->text[inp->cursor],
                        &inp->text[inp->cursor + 1], len - inp->cursor);
                clue_signal_emit(inp, "changed");
            }
            return 1;
        }

        /* Enter */
        if (key == XKB_KEY_Return || key == XKB_KEY_KP_Enter) {
            clue_signal_emit(inp, "activate");
            return 1;
        }

        /* Printable text */
        if (event->key.text[0] && len < CLUE_TEXT_INPUT_MAX - 2) {
            int tlen = (int)strlen(event->key.text);
            if (len + tlen < CLUE_TEXT_INPUT_MAX - 1) {
                memmove(&inp->text[inp->cursor + tlen],
                        &inp->text[inp->cursor], len - inp->cursor + 1);
                memcpy(&inp->text[inp->cursor], event->key.text, tlen);
                inp->cursor += tlen;
                ensure_cursor_visible(inp);
                clue_signal_emit(inp, "changed");
            }
            return 1;
        }

        return 0;
    }

    default:
        return 0;
    }
}

static void text_input_layout(ClueWidget *w)
{
    UIFont *font = input_font((ClueTextInput *)w);
    if (!font) return;
    if (w->base.w == 0) w->base.w = 200;
    w->base.h = clue_font_line_height(font) + INPUT_PAD_V * 2;
}

static void text_input_destroy(ClueWidget *w)
{
    ClueTextInput *inp = (ClueTextInput *)w;
    stop_blink(inp);
}

static const ClueWidgetVTable text_input_vtable = {
    .draw         = text_input_draw,
    .layout       = text_input_layout,
    .handle_event = text_input_handle_event,
    .destroy      = text_input_destroy,
};

ClueTextInput *clue_text_input_new(const char *placeholder)
{
    ClueTextInput *inp = calloc(1, sizeof(ClueTextInput));
    if (!inp) return NULL;

    clue_cwidget_init(&inp->base, &text_input_vtable);
    inp->base.type_id = CLUE_WIDGET_TEXT_INPUT;
    inp->base.base.focusable = true;
    if (placeholder) {
        strncpy(inp->placeholder, placeholder, sizeof(inp->placeholder) - 1);
    }

    text_input_layout(&inp->base);
    return inp;
}

const char *clue_text_input_get_text(ClueTextInput *input)
{
    return input ? input->text : "";
}

void clue_text_input_set_text(ClueTextInput *input, const char *text)
{
    if (!input) return;
    if (text) {
        strncpy(input->text, text, CLUE_TEXT_INPUT_MAX - 1);
        input->text[CLUE_TEXT_INPUT_MAX - 1] = '\0';
    } else {
        input->text[0] = '\0';
    }
    input->cursor = (int)strlen(input->text);
    input->scroll_offset = 0;
}
