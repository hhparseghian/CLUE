#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "clue/text_editor.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/timer.h"
#include <xkbcommon/xkbcommon-keysyms.h>

#define PAD       8
#define GUTTER_W  40
#define CURSOR_BLINK_MS 530

static UIFont *ed_font(ClueTextEditor *ed)
{
    return ed->base.style.font ? ed->base.style.font : clue_app_default_font();
}

static bool blink_cb(void *data)
{
    ClueTextEditor *ed = data;
    ed->cursor_visible = !ed->cursor_visible;
    return true;
}

static void start_blink(ClueTextEditor *ed)
{
    ed->cursor_visible = true;
    if (ed->blink_timer_id == 0)
        ed->blink_timer_id = clue_timer_repeat(CURSOR_BLINK_MS, blink_cb, ed);
}

static void stop_blink(ClueTextEditor *ed)
{
    if (ed->blink_timer_id) {
        clue_timer_cancel(ed->blink_timer_id);
        ed->blink_timer_id = 0;
    }
    ed->cursor_visible = false;
}

/* Find the start of line containing byte position `pos` */
static int line_start(const char *text, int pos)
{
    int i = pos;
    while (i > 0 && text[i - 1] != '\n') i--;
    return i;
}

/* Find the end of line containing byte position `pos` (not including \n) */
static int line_end(const char *text, int len, int pos)
{
    int i = pos;
    while (i < len && text[i] != '\n') i++;
    return i;
}

/* Count lines up to byte position */
static int count_lines(const char *text, int pos)
{
    int n = 0;
    for (int i = 0; i < pos; i++)
        if (text[i] == '\n') n++;
    return n;
}

/* Get byte offset of line N start */
static int line_n_start(const char *text, int len, int line)
{
    int n = 0, i = 0;
    while (i < len && n < line) {
        if (text[i] == '\n') n++;
        i++;
    }
    return i;
}

static int text_width_range(UIFont *font, const char *text, int from, int to)
{
    if (!font || from >= to) return 0;
    char buf[512];
    int n = to - from;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    memcpy(buf, text + from, n);
    buf[n] = '\0';
    return clue_font_text_width(font, buf);
}

static void ensure_grow(ClueTextEditor *ed, int need)
{
    if (ed->text_len + need + 1 <= ed->text_cap) return;
    int new_cap = ed->text_cap * 2;
    if (new_cap < ed->text_len + need + 1) new_cap = ed->text_len + need + 256;
    char *new_buf = realloc(ed->text, new_cap);
    if (!new_buf) return;
    ed->text = new_buf;
    ed->text_cap = new_cap;
}

static void ensure_cursor_visible(ClueTextEditor *ed)
{
    UIFont *font = ed_font(ed);
    if (!font) return;
    int lh = clue_font_line_height(font);
    int gutter = ed->line_numbers ? GUTTER_W : 0;
    int bh = ed->base.base.h;
    int bw = ed->base.base.w;

    /* Vertical */
    int cur_line = count_lines(ed->text, ed->cursor);
    int cur_y = cur_line * lh;
    if (cur_y - ed->scroll_y < 0)
        ed->scroll_y = cur_y;
    if (cur_y + lh - ed->scroll_y > bh - PAD * 2)
        ed->scroll_y = cur_y + lh - (bh - PAD * 2);
    if (ed->scroll_y < 0) ed->scroll_y = 0;

    /* Horizontal */
    int ls = line_start(ed->text, ed->cursor);
    int cpx = text_width_range(font, ed->text, ls, ed->cursor);
    int vis_w = bw - PAD * 2 - gutter;
    if (cpx - ed->scroll_x > vis_w)
        ed->scroll_x = cpx - vis_w + 10;
    if (cpx - ed->scroll_x < 0) {
        ed->scroll_x = cpx - 10;
        if (ed->scroll_x < 0) ed->scroll_x = 0;
    }
}

static void text_editor_draw(ClueWidget *w)
{
    ClueTextEditor *ed = (ClueTextEditor *)w;
    const ClueTheme *th = clue_theme_get();
    UIFont *font = ed_font(ed);
    if (!font) return;

    if (!w->base.focused && ed->blink_timer_id)
        stop_blink(ed);

    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;
    int lh = clue_font_line_height(font);
    float radius = th->corner_radius;
    int gutter = ed->line_numbers ? GUTTER_W : 0;

    /* Background */
    clue_fill_rounded_rect(x, y, bw, bh, radius, th->input.bg);
    UIColor border = w->base.focused ? th->input.focus_border : th->input.border;
    clue_draw_rounded_rect(x, y, bw, bh, radius, 1.5f, border);

    /* Gutter */
    if (ed->line_numbers) {
        clue_fill_rect(x + 1, y + 1, gutter - 1, bh - 2, th->surface);
    }

    /* Clip to text area */
    clue_set_clip_rect(x + gutter + PAD, y + PAD, bw - gutter - PAD * 2, bh - PAD * 2);

    /* Draw lines */
    int total_lines = count_lines(ed->text, ed->text_len) + 1;
    int first_vis = ed->scroll_y / lh;
    int last_vis = (ed->scroll_y + bh) / lh + 1;
    if (last_vis > total_lines) last_vis = total_lines;

    int cursor_line = count_lines(ed->text, ed->cursor);
    int cursor_ls = line_start(ed->text, ed->cursor);

    for (int ln = first_vis; ln < last_vis; ln++) {
        int ls = line_n_start(ed->text, ed->text_len, ln);
        int le = line_end(ed->text, ed->text_len, ls);
        int ty = y + PAD + ln * lh - ed->scroll_y;
        int tx = x + gutter + PAD - ed->scroll_x;

        /* Line text */
        if (le > ls) {
            char buf[512];
            int n = le - ls;
            if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
            memcpy(buf, ed->text + ls, n);
            buf[n] = '\0';
            clue_draw_text(tx, ty, buf, font, th->input.fg);
        }
    }

    /* Cursor */
    if (w->base.focused && ed->cursor_visible) {
        int cy = y + PAD + cursor_line * lh - ed->scroll_y;
        int cx = x + gutter + PAD + text_width_range(font, ed->text, cursor_ls, ed->cursor) - ed->scroll_x;
        clue_fill_rect(cx, cy, 2, lh, th->input.cursor);
    }

    clue_reset_clip_rect();

    /* Line numbers (drawn outside clip) */
    if (ed->line_numbers) {
        clue_set_clip_rect(x + 1, y + PAD, gutter - 2, bh - PAD * 2);
        for (int ln = first_vis; ln < last_vis; ln++) {
            char num[16];
            snprintf(num, sizeof(num), "%d", ln + 1);
            int nw = clue_font_text_width(font, num);
            int ty = y + PAD + ln * lh - ed->scroll_y;
            clue_draw_text(x + gutter - nw - 4, ty, num, font, th->fg_dim);
        }
        clue_reset_clip_rect();
    }
}

static void text_editor_layout(ClueWidget *w)
{
    if (w->base.w == 0) w->base.w = 400;
    if (w->base.h == 0) w->base.h = 250;
}

static int text_editor_handle_event(ClueWidget *w, UIEvent *event)
{
    ClueTextEditor *ed = (ClueTextEditor *)w;
    int x = w->base.x, y = w->base.y;
    int bw = w->base.w, bh = w->base.h;

    if (event->type == UI_EVENT_MOUSE_BUTTON && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;
        bool inside = mx >= x && mx < x + bw && my >= y && my < y + bh;
        if (event->mouse_button.pressed && inside) {
            clue_focus_widget(&w->base);
            start_blink(ed);

            /* Place cursor at click */
            UIFont *font = ed_font(ed);
            if (font) {
                int lh = clue_font_line_height(font);
                int gutter = ed->line_numbers ? GUTTER_W : 0;
                int click_line = (my - y - PAD + ed->scroll_y) / lh;
                int total = count_lines(ed->text, ed->text_len) + 1;
                if (click_line < 0) click_line = 0;
                if (click_line >= total) click_line = total - 1;

                int ls = line_n_start(ed->text, ed->text_len, click_line);
                int le = line_end(ed->text, ed->text_len, ls);
                int rel_x = mx - x - gutter - PAD + ed->scroll_x;

                ed->cursor = le;
                for (int i = ls; i <= le; i++) {
                    if (text_width_range(font, ed->text, ls, i) >= rel_x) {
                        ed->cursor = i;
                        break;
                    }
                }
            }
            return 1;
        }
        return 0;
    }

    if (event->type == UI_EVENT_MOUSE_SCROLL) {
        int mx = event->mouse_scroll.x, my = event->mouse_scroll.y;
        if (mx >= x && mx < x + bw && my >= y && my < y + bh) {
            UIFont *font = ed_font(ed);
            int lh = font ? clue_font_line_height(font) : 16;
            ed->scroll_y -= (int)(event->mouse_scroll.dy * lh * 3);
            if (ed->scroll_y < 0) ed->scroll_y = 0;
            return 1;
        }
    }

    if (event->type == UI_EVENT_KEY && w->base.focused && event->key.pressed) {
        stop_blink(ed);
        start_blink(ed);

        int key = event->key.keycode;

        /* Cursor movement */
        if (key == XKB_KEY_Left) {
            if (ed->cursor > 0) ed->cursor--;
            ensure_cursor_visible(ed);
            return 1;
        }
        if (key == XKB_KEY_Right) {
            if (ed->cursor < ed->text_len) ed->cursor++;
            ensure_cursor_visible(ed);
            return 1;
        }
        if (key == XKB_KEY_Up) {
            int ls = line_start(ed->text, ed->cursor);
            if (ls > 0) {
                int col = ed->cursor - ls;
                int prev_ls = line_start(ed->text, ls - 1);
                int prev_le = ls - 1;
                int prev_len = prev_le - prev_ls;
                ed->cursor = prev_ls + (col < prev_len ? col : prev_len);
            }
            ensure_cursor_visible(ed);
            return 1;
        }
        if (key == XKB_KEY_Down) {
            int le = line_end(ed->text, ed->text_len, ed->cursor);
            if (le < ed->text_len) {
                int ls = line_start(ed->text, ed->cursor);
                int col = ed->cursor - ls;
                int next_ls = le + 1;
                int next_le = line_end(ed->text, ed->text_len, next_ls);
                int next_len = next_le - next_ls;
                ed->cursor = next_ls + (col < next_len ? col : next_len);
            }
            ensure_cursor_visible(ed);
            return 1;
        }
        if (key == XKB_KEY_Home) {
            ed->cursor = line_start(ed->text, ed->cursor);
            ensure_cursor_visible(ed);
            return 1;
        }
        if (key == XKB_KEY_End) {
            ed->cursor = line_end(ed->text, ed->text_len, ed->cursor);
            ensure_cursor_visible(ed);
            return 1;
        }

        /* Backspace */
        if (key == XKB_KEY_BackSpace) {
            if (ed->cursor > 0) {
                memmove(&ed->text[ed->cursor - 1], &ed->text[ed->cursor],
                        ed->text_len - ed->cursor + 1);
                ed->cursor--;
                ed->text_len--;
                ensure_cursor_visible(ed);
                clue_signal_emit(ed, "changed");
            }
            return 1;
        }

        /* Delete */
        if (key == XKB_KEY_Delete) {
            if (ed->cursor < ed->text_len) {
                memmove(&ed->text[ed->cursor], &ed->text[ed->cursor + 1],
                        ed->text_len - ed->cursor);
                ed->text_len--;
                clue_signal_emit(ed, "changed");
            }
            return 1;
        }

        /* Enter */
        if (key == XKB_KEY_Return || key == XKB_KEY_KP_Enter) {
            ensure_grow(ed, 1);
            memmove(&ed->text[ed->cursor + 1], &ed->text[ed->cursor],
                    ed->text_len - ed->cursor + 1);
            ed->text[ed->cursor] = '\n';
            ed->cursor++;
            ed->text_len++;
            ensure_cursor_visible(ed);
            clue_signal_emit(ed, "changed");
            return 1;
        }

        /* Tab */
        if (key == XKB_KEY_Tab) {
            int spaces = 4;
            ensure_grow(ed, spaces);
            memmove(&ed->text[ed->cursor + spaces], &ed->text[ed->cursor],
                    ed->text_len - ed->cursor + 1);
            memset(&ed->text[ed->cursor], ' ', spaces);
            ed->cursor += spaces;
            ed->text_len += spaces;
            ensure_cursor_visible(ed);
            clue_signal_emit(ed, "changed");
            return 1;
        }

        /* Printable text */
        if (event->key.text[0]) {
            int tlen = (int)strlen(event->key.text);
            ensure_grow(ed, tlen);
            memmove(&ed->text[ed->cursor + tlen], &ed->text[ed->cursor],
                    ed->text_len - ed->cursor + 1);
            memcpy(&ed->text[ed->cursor], event->key.text, tlen);
            ed->cursor += tlen;
            ed->text_len += tlen;
            ensure_cursor_visible(ed);
            clue_signal_emit(ed, "changed");
            return 1;
        }
        return 0;
    }

    return 0;
}

static void text_editor_destroy(ClueWidget *w)
{
    ClueTextEditor *ed = (ClueTextEditor *)w;
    stop_blink(ed);
    free(ed->text);
}

static const ClueWidgetVTable text_editor_vtable = {
    .draw         = text_editor_draw,
    .layout       = text_editor_layout,
    .handle_event = text_editor_handle_event,
    .destroy      = text_editor_destroy,
};

ClueTextEditor *clue_text_editor_new(void)
{
    ClueTextEditor *ed = calloc(1, sizeof(ClueTextEditor));
    if (!ed) return NULL;

    clue_cwidget_init(&ed->base, &text_editor_vtable);
    ed->base.type_id = CLUE_WIDGET_TEXT_EDITOR;
    ed->base.base.focusable = true;

    ed->text_cap = 1024;
    ed->text = calloc(1, ed->text_cap);
    ed->text_len = 0;
    ed->word_wrap = false;
    ed->line_numbers = false;

    text_editor_layout(&ed->base);
    return ed;
}

const char *clue_text_editor_get_text(ClueTextEditor *ed)
{
    return ed ? ed->text : "";
}

void clue_text_editor_set_text(ClueTextEditor *ed, const char *text)
{
    if (!ed) return;
    int len = text ? (int)strlen(text) : 0;
    if (len + 1 > ed->text_cap) {
        char *new_buf = realloc(ed->text, len + 256);
        if (!new_buf) return;
        ed->text = new_buf;
        ed->text_cap = len + 256;
    }
    if (text) memcpy(ed->text, text, len);
    ed->text[len] = '\0';
    ed->text_len = len;
    ed->cursor = 0;
    ed->scroll_x = 0;
    ed->scroll_y = 0;
}

void clue_text_editor_set_word_wrap(ClueTextEditor *ed, bool wrap)
{
    if (ed) ed->word_wrap = wrap;
}

void clue_text_editor_set_line_numbers(ClueTextEditor *ed, bool show)
{
    if (ed) ed->line_numbers = show;
}
