#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/checkbox.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"
#include "clue/app.h"

#define BOX_SIZE 20
#define BOX_GAP  10

static ClueFont *cb_font(ClueCheckbox *cb)
{
    return cb->base.style.font ? cb->base.style.font : clue_app_default_font();
}

static void checkbox_draw(ClueWidget *w)
{
    ClueCheckbox *cb = (ClueCheckbox *)w;
    int x = w->base.x, y = w->base.y;
    ClueFont *font = cb_font(cb);
    int box_y = y + (w->base.h - BOX_SIZE) / 2;

    const ClueTheme *th = clue_theme_get();

    if (cb->checked) {
        /* Filled rounded box with checkmark */
        clue_fill_rounded_rect(x, box_y, BOX_SIZE, BOX_SIZE,
                               4.0f, th->checkbox.box_checked);
        /* Checkmark */
        int cx = x, cy = box_y;
        clue_draw_line(cx + 5, cy + 10, cx + 8, cy + 14, 2.5f, th->checkbox.checkmark);
        clue_draw_line(cx + 8, cy + 14, cx + 15, cy + 6, 2.5f, th->checkbox.checkmark);
    } else {
        /* Empty box with border */
        clue_fill_rounded_rect(x, box_y, BOX_SIZE, BOX_SIZE,
                               4.0f, th->input.bg);
        clue_draw_rounded_rect(x, box_y, BOX_SIZE, BOX_SIZE, 4.0f, 1.5f,
                               th->checkbox.box_border);
    }

    if (font && cb->label && cb->label[0]) {
        ClueColor fg = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->checkbox.fg;
        int text_y = y + (w->base.h - clue_font_line_height(font)) / 2;
        clue_draw_text(x + BOX_SIZE + BOX_GAP, text_y, cb->label, font, fg);
    }
}

static void checkbox_layout(ClueWidget *w)
{
    ClueCheckbox *cb = (ClueCheckbox *)w;
    ClueFont *font = cb_font(cb);
    if (!font) return;

    int tw = cb->label ? clue_font_text_width(font, cb->label) : 0;
    w->base.w = BOX_SIZE + BOX_GAP + tw;
    w->base.h = clue_font_line_height(font);
    if (w->base.h < BOX_SIZE) w->base.h = BOX_SIZE;
}

static int checkbox_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueCheckbox *cb = (ClueCheckbox *)w;

    if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed &&
        event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        if (mx >= w->base.x && mx < w->base.x + w->base.w &&
            my >= w->base.y && my < w->base.y + w->base.h) {
            cb->checked = !cb->checked;
            clue_focus_widget(NULL);
            clue_signal_emit(cb, "toggled");
            return 1;
        }
    }

    return 0;
}

static void checkbox_destroy(ClueWidget *w)
{
    ClueCheckbox *cb = (ClueCheckbox *)w;
    free(cb->label);
}

static const ClueWidgetVTable checkbox_vtable = {
    .draw         = checkbox_draw,
    .layout       = checkbox_layout,
    .handle_event = checkbox_handle_event,
    .destroy      = checkbox_destroy,
};

ClueCheckbox *clue_checkbox_new(const char *label)
{
    ClueCheckbox *cb = calloc(1, sizeof(ClueCheckbox));
    if (!cb) return NULL;

    clue_cwidget_init(&cb->base, &checkbox_vtable);
    cb->label = label ? strdup(label) : strdup("");
    cb->checked = false;

    checkbox_layout(&cb->base);
    return cb;
}

bool clue_checkbox_is_checked(ClueCheckbox *cb)
{
    return cb ? cb->checked : false;
}

void clue_checkbox_set_checked(ClueCheckbox *cb, bool checked)
{
    if (cb) cb->checked = checked;
}
