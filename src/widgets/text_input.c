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
#include "clue/clipboard.h"
#include <xkbcommon/xkbcommon-keysyms.h>

#include "clue/menu.h"
#include <time.h>
#include <ctype.h>

/* Forward declarations for context menu */
static bool has_selection(ClueTextInput *inp);
static void delete_selection(ClueTextInput *inp);
static void copy_selection(ClueTextInput *inp);

/* Context menu for text input */
static ClueTextInput *g_ctx_target = NULL;

static void ctx_cut(void *widget, void *data)
{
    ClueTextInput *inp = g_ctx_target;
    if (!inp || !has_selection(inp)) return;
    copy_selection(inp);
    delete_selection(inp);
    clue_signal_emit(inp, "changed");
}

static void ctx_copy(void *widget, void *data)
{
    if (g_ctx_target) copy_selection(g_ctx_target);
}

static void ctx_paste(void *widget, void *data)
{
    ClueTextInput *inp = g_ctx_target;
    if (!inp) return;
    char *clip = clue_clipboard_get();
    if (clip) {
        if (has_selection(inp)) delete_selection(inp);
        int len = (int)strlen(inp->text);
        int clen = (int)strlen(clip);
        if (len + clen < CLUE_TEXT_INPUT_MAX - 1) {
            memmove(&inp->text[inp->cursor + clen],
                    &inp->text[inp->cursor], len - inp->cursor + 1);
            memcpy(&inp->text[inp->cursor], clip, clen);
            inp->cursor += clen;
            clue_signal_emit(inp, "changed");
        }
        free(clip);
    }
}

static void ctx_select_all(void *widget, void *data)
{
    ClueTextInput *inp = g_ctx_target;
    if (!inp) return;
    inp->sel_start = 0;
    inp->sel_end = (int)strlen(inp->text);
    inp->cursor = inp->sel_end;
}

static ClueMenu *get_input_context_menu(void)
{
    static ClueMenu *menu = NULL;
    if (!menu) {
        menu = clue_menu_new();
        clue_menu_add_item(menu, "Cut", ctx_cut, NULL);
        clue_menu_add_item(menu, "Copy", ctx_copy, NULL);
        clue_menu_add_item(menu, "Paste", ctx_paste, NULL);
        clue_menu_add_separator(menu);
        clue_menu_add_item(menu, "Select All", ctx_select_all, NULL);
    }
    return menu;
}

#define INPUT_PAD_H 10
#define INPUT_PAD_V 8
#define CURSOR_BLINK_MS 530
#define DOUBLE_CLICK_MS 400

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int word_start(const char *text, int pos)
{
    int i = pos;
    while (i > 0 && !isspace((unsigned char)text[i - 1])) i--;
    return i;
}

static int word_end(const char *text, int len, int pos)
{
    int i = pos;
    while (i < len && !isspace((unsigned char)text[i])) i++;
    return i;
}

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

static ClueFont *input_font(ClueTextInput *inp)
{
    return inp->base.style.font ? inp->base.style.font : clue_app_default_font();
}

static int text_width_n(ClueFont *font, const char *text, int n)
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

static bool has_selection(ClueTextInput *inp)
{
    return inp->sel_start >= 0 && inp->sel_start != inp->sel_end;
}

static int sel_min(ClueTextInput *inp)
{
    return inp->sel_start < inp->sel_end ? inp->sel_start : inp->sel_end;
}

static int sel_max(ClueTextInput *inp)
{
    return inp->sel_start > inp->sel_end ? inp->sel_start : inp->sel_end;
}

static void clear_selection(ClueTextInput *inp)
{
    inp->sel_start = -1;
    inp->sel_end = -1;
}

static void delete_selection(ClueTextInput *inp)
{
    if (!has_selection(inp)) return;
    int s = sel_min(inp), e = sel_max(inp);
    int len = (int)strlen(inp->text);
    memmove(&inp->text[s], &inp->text[e], len - e + 1);
    inp->cursor = s;
    clear_selection(inp);
}

static void copy_selection(ClueTextInput *inp)
{
    if (!has_selection(inp)) return;
    int s = sel_min(inp), e = sel_max(inp);
    char buf[CLUE_TEXT_INPUT_MAX];
    int n = e - s;
    memcpy(buf, &inp->text[s], n);
    buf[n] = '\0';
    clue_clipboard_set(buf);
}

/* Place cursor at pixel position relative to widget */
static int cursor_from_x(ClueTextInput *inp, int mx)
{
    ClueFont *font = input_font(inp);
    if (!font) return 0;
    int rel_x = mx - inp->base.base.x - INPUT_PAD_H + inp->scroll_offset;
    int len = (int)strlen(inp->text);
    for (int i = 0; i <= len; i++) {
        if (text_width_n(font, inp->text, i) >= rel_x)
            return i;
    }
    return len;
}

static void text_input_draw(ClueWidget *w)
{
    ClueTextInput *inp = (ClueTextInput *)w;

    if (!w->base.focused && inp->blink_timer_id) {
        stop_blink(inp);
        clear_selection(inp);
    }

    ClueFont *font = input_font(inp);
    if (!font) return;

    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    const ClueTheme *th = clue_theme_get();
    float radius = w->style.corner_radius > 0.0f ? w->style.corner_radius : th->corner_radius;

    /* Subtle inset shadow */
    clue_fill_rounded_rect(x, y + 1, bw, bh, radius,
                           CLUE_RGBAF(0, 0, 0, 0.15f));

    /* Background */
    ClueColor bg = w->style.bg_color.a > 0.001f ? w->style.bg_color : th->input.bg;
    clue_fill_rounded_rect(x, y, bw, bh, radius, bg);

    /* Border -- thicker when focused */
    ClueColor border = w->base.focused ? th->input.focus_border : th->input.border;
    float border_w = w->base.focused ? 2.0f : 1.5f;
    clue_draw_rounded_rect(x, y, bw, bh, radius, border_w, border);

    /* Clip text to input area */
    clue_set_clip_rect(x + INPUT_PAD_H, y, bw - INPUT_PAD_H * 2, bh);

    int text_x = x + INPUT_PAD_H - inp->scroll_offset;
    int text_y = y + (bh - clue_font_line_height(font)) / 2;

    /* Selection highlight */
    if (has_selection(inp)) {
        int s = sel_min(inp), e = sel_max(inp);
        int sx = text_x + text_width_n(font, inp->text, s);
        int ex = text_x + text_width_n(font, inp->text, e);
        int sh = clue_font_line_height(font);
        clue_fill_rect(sx, y + INPUT_PAD_V, ex - sx, sh,
                       CLUE_RGBAF(th->accent.r, th->accent.g, th->accent.b, 0.35f));
    }

    if (inp->text[0]) {
        ClueColor fg = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->input.fg;
        clue_draw_text(text_x, text_y, inp->text, font, fg);
    } else if (inp->placeholder[0]) {
        clue_draw_text(text_x, text_y, inp->placeholder, font, th->input.placeholder);
    }

    /* Cursor (blinks) */
    if (w->base.focused && inp->cursor_visible && !has_selection(inp)) {
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
    ClueFont *font = input_font(inp);
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

static int text_input_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueTextInput *inp = (ClueTextInput *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    switch (event->type) {
    case CLUE_EVENT_MOUSE_MOVE: {
        int mx = event->mouse_move.x;
        int my = event->mouse_move.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            if (event->window)
                clue_window_set_cursor(event->window, CLUE_CURSOR_TEXT);
        }
        /* Drag selection */
        if (inp->mouse_selecting) {
            inp->cursor = cursor_from_x(inp, mx);
            inp->sel_end = inp->cursor;
            ensure_cursor_visible(inp);
            return 1;
        }
        return 0;
    }
    case CLUE_EVENT_MOUSE_BUTTON: {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;

        /* Right-click: context menu */
        if (event->mouse_button.pressed && event->mouse_button.btn == 1 && inside) {
            clue_focus_widget(&w->base);
            g_ctx_target = inp;
            clue_context_menu_show(get_input_context_menu(), mx, my);
            return 1;
        }

        if (event->mouse_button.pressed && event->mouse_button.btn == 0 && inside) {
            clue_focus_widget(&w->base);
            start_blink(inp);
            inp->cursor = cursor_from_x(inp, mx);

            /* Double-click: select word */
            long long now = now_ms();
            if (now - inp->last_click_ms < DOUBLE_CLICK_MS) {
                int len = (int)strlen(inp->text);
                inp->sel_start = word_start(inp->text, inp->cursor);
                inp->sel_end = word_end(inp->text, len, inp->cursor);
                inp->cursor = inp->sel_end;
                inp->last_click_ms = 0;
            } else {
                inp->sel_start = inp->cursor;
                inp->sel_end = inp->cursor;
                inp->last_click_ms = now;
            }

            inp->mouse_selecting = true;
            clue_capture_mouse(&w->base);
            return 1;
        }
        if (!event->mouse_button.pressed && event->mouse_button.btn == 0 && inp->mouse_selecting) {
            inp->mouse_selecting = false;
            clue_release_mouse();
            if (inp->sel_start == inp->sel_end)
                clear_selection(inp);
            return 1;
        }
        return 0;
    }

    case CLUE_EVENT_KEY: {
        if (!w->base.focused || !event->key.pressed) return 0;

        stop_blink(inp);
        start_blink(inp);

        int key = event->key.keycode;
        int mods = event->key.modifiers;
        int len = (int)strlen(inp->text);
        bool shift = mods & CLUE_MOD_SHIFT;
        bool ctrl = mods & CLUE_MOD_CTRL;

        /* Ctrl+A: select all */
        if (ctrl && (key == XKB_KEY_a || key == XKB_KEY_A)) {
            inp->sel_start = 0;
            inp->sel_end = len;
            inp->cursor = len;
            return 1;
        }

        /* Ctrl+C: copy */
        if (ctrl && (key == XKB_KEY_c || key == XKB_KEY_C)) {
            copy_selection(inp);
            return 1;
        }

        /* Ctrl+X: cut */
        if (ctrl && (key == XKB_KEY_x || key == XKB_KEY_X)) {
            copy_selection(inp);
            if (has_selection(inp)) {
                delete_selection(inp);
                ensure_cursor_visible(inp);
                clue_signal_emit(inp, "changed");
            }
            return 1;
        }

        /* Ctrl+V: paste */
        if (ctrl && (key == XKB_KEY_v || key == XKB_KEY_V)) {
            char *clip = clue_clipboard_get();
            if (clip) {
                if (has_selection(inp)) delete_selection(inp);
                len = (int)strlen(inp->text);
                int clen = (int)strlen(clip);
                if (len + clen < CLUE_TEXT_INPUT_MAX - 1) {
                    memmove(&inp->text[inp->cursor + clen],
                            &inp->text[inp->cursor], len - inp->cursor + 1);
                    memcpy(&inp->text[inp->cursor], clip, clen);
                    inp->cursor += clen;
                    ensure_cursor_visible(inp);
                    clue_signal_emit(inp, "changed");
                }
                free(clip);
            }
            return 1;
        }

        /* Cursor movement (with shift = extend selection) */
        if (key == XKB_KEY_Left) {
            if (shift) {
                if (inp->sel_start < 0) inp->sel_start = inp->cursor;
                if (inp->cursor > 0) inp->cursor--;
                inp->sel_end = inp->cursor;
            } else {
                if (has_selection(inp)) { inp->cursor = sel_min(inp); clear_selection(inp); }
                else if (inp->cursor > 0) inp->cursor--;
            }
            ensure_cursor_visible(inp);
            return 1;
        }
        if (key == XKB_KEY_Right) {
            if (shift) {
                if (inp->sel_start < 0) inp->sel_start = inp->cursor;
                if (inp->cursor < len) inp->cursor++;
                inp->sel_end = inp->cursor;
            } else {
                if (has_selection(inp)) { inp->cursor = sel_max(inp); clear_selection(inp); }
                else if (inp->cursor < len) inp->cursor++;
            }
            ensure_cursor_visible(inp);
            return 1;
        }
        if (key == XKB_KEY_Home) {
            if (shift) {
                if (inp->sel_start < 0) inp->sel_start = inp->cursor;
                inp->cursor = 0;
                inp->sel_end = 0;
            } else {
                clear_selection(inp);
                inp->cursor = 0;
            }
            ensure_cursor_visible(inp);
            return 1;
        }
        if (key == XKB_KEY_End) {
            if (shift) {
                if (inp->sel_start < 0) inp->sel_start = inp->cursor;
                inp->cursor = len;
                inp->sel_end = len;
            } else {
                clear_selection(inp);
                inp->cursor = len;
            }
            ensure_cursor_visible(inp);
            return 1;
        }

        /* Backspace */
        if (key == XKB_KEY_BackSpace) {
            if (has_selection(inp)) {
                delete_selection(inp);
            } else if (inp->cursor > 0) {
                memmove(&inp->text[inp->cursor - 1],
                        &inp->text[inp->cursor], len - inp->cursor + 1);
                inp->cursor--;
            }
            ensure_cursor_visible(inp);
            clue_signal_emit(inp, "changed");
            return 1;
        }

        /* Delete */
        if (key == XKB_KEY_Delete) {
            if (has_selection(inp)) {
                delete_selection(inp);
            } else if (inp->cursor < len) {
                memmove(&inp->text[inp->cursor],
                        &inp->text[inp->cursor + 1], len - inp->cursor);
            }
            clue_signal_emit(inp, "changed");
            return 1;
        }

        /* Enter */
        if (key == XKB_KEY_Return || key == XKB_KEY_KP_Enter) {
            clue_signal_emit(inp, "activate");
            return 1;
        }

        /* Printable text */
        if (event->key.text[0]) {
            if (has_selection(inp)) delete_selection(inp);
            len = (int)strlen(inp->text);
            int tlen = (int)strlen(event->key.text);
            if (len + tlen < CLUE_TEXT_INPUT_MAX - 1) {
                memmove(&inp->text[inp->cursor + tlen],
                        &inp->text[inp->cursor], len - inp->cursor + 1);
                memcpy(&inp->text[inp->cursor], event->key.text, tlen);
                inp->cursor += tlen;
                clear_selection(inp);
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
    ClueFont *font = input_font((ClueTextInput *)w);
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
    inp->sel_start = -1;
    inp->sel_end = -1;
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
    clear_selection(input);
}
