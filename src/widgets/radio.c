#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/radio.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define CIRCLE_RADIUS 10
#define CIRCLE_GAP    10
#define INNER_RADIUS   5

static ClueFont *radio_font(ClueRadio *r)
{
    return r->base.style.font ? r->base.style.font : clue_app_default_font();
}

static void radio_draw(ClueWidget *w)
{
    ClueRadio *r = (ClueRadio *)w;
    int x = w->base.x, y = w->base.y;
    ClueFont *font = radio_font(r);
    int cy = y + w->base.h / 2;
    int cx = x + CIRCLE_RADIUS;

    const ClueTheme *th = clue_theme_get();

    if (r->selected) {
        /* Filled outer circle */
        clue_fill_circle(cx, cy, CIRCLE_RADIUS, th->checkbox.box_checked);
        /* White inner dot */
        clue_fill_circle(cx, cy, INNER_RADIUS, th->checkbox.checkmark);
    } else {
        /* Draw ring: filled border circle, then filled bg circle on top */
        clue_fill_circle(cx, cy, CIRCLE_RADIUS, th->checkbox.box_border);
        clue_fill_circle(cx, cy, CIRCLE_RADIUS - 2, th->bg);
    }

    if (font && r->label && r->label[0]) {
        ClueColor fg = w->style.fg_color.a > 0.001f ? w->style.fg_color : th->checkbox.fg;
        int text_y = y + (w->base.h - clue_font_line_height(font)) / 2;
        clue_draw_text(x + CIRCLE_RADIUS * 2 + CIRCLE_GAP, text_y,
                       r->label, font, fg);
    }
}

static void radio_layout(ClueWidget *w)
{
    ClueRadio *r = (ClueRadio *)w;
    ClueFont *font = radio_font(r);
    if (!font) return;

    int tw = r->label ? clue_font_text_width(font, r->label) : 0;
    w->base.w = CIRCLE_RADIUS * 2 + CIRCLE_GAP + tw;
    w->base.h = clue_font_line_height(font);
    if (w->base.h < CIRCLE_RADIUS * 2) w->base.h = CIRCLE_RADIUS * 2;
}

static void radio_select(ClueRadio *r)
{
    if (!r || r->selected) return;

    /* Deselect all others in the group */
    if (r->group) {
        for (int i = 0; i < r->group->count; i++) {
            r->group->buttons[i]->selected = false;
        }
    }

    r->selected = true;
    clue_signal_emit(r, "changed");
}

static int radio_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueRadio *r = (ClueRadio *)w;

    if (event->type == CLUE_EVENT_MOUSE_BUTTON &&
        event->mouse_button.pressed &&
        event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x;
        int my = event->mouse_button.y;
        if (mx >= w->base.x && mx < w->base.x + w->base.w &&
            my >= w->base.y && my < w->base.y + w->base.h) {
            radio_select(r);
            clue_focus_widget(NULL);
            return 1;
        }
    }

    return 0;
}

static void radio_destroy(ClueWidget *w)
{
    ClueRadio *r = (ClueRadio *)w;

    /* Remove from group */
    if (r->group) {
        for (int i = 0; i < r->group->count; i++) {
            if (r->group->buttons[i] == r) {
                r->group->buttons[i] = r->group->buttons[r->group->count - 1];
                r->group->count--;
                break;
            }
        }
    }

    free(r->label);
}

static const ClueWidgetVTable radio_vtable = {
    .draw         = radio_draw,
    .layout       = radio_layout,
    .handle_event = radio_handle_event,
    .destroy      = radio_destroy,
};

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

ClueRadioGroup *clue_radio_group_new(void)
{
    ClueRadioGroup *g = calloc(1, sizeof(ClueRadioGroup));
    return g;
}

void clue_radio_group_destroy(ClueRadioGroup *group)
{
    free(group);
}

ClueRadio *clue_radio_new(const char *label, ClueRadioGroup *group)
{
    ClueRadio *r = calloc(1, sizeof(ClueRadio));
    if (!r) return NULL;

    clue_cwidget_init(&r->base, &radio_vtable);
    r->label    = label ? strdup(label) : strdup("");
    r->selected = false;
    r->group    = group;

    if (group && group->count < 32) {
        group->buttons[group->count++] = r;
    }

    radio_layout(&r->base);
    return r;
}

bool clue_radio_is_selected(ClueRadio *r)
{
    return r ? r->selected : false;
}

void clue_radio_set_selected(ClueRadio *r)
{
    if (r) radio_select(r);
}

ClueRadio *clue_radio_group_get_selected(ClueRadioGroup *group)
{
    if (!group) return NULL;
    for (int i = 0; i < group->count; i++) {
        if (group->buttons[i]->selected) return group->buttons[i];
    }
    return NULL;
}

int clue_radio_group_get_selected_index(ClueRadioGroup *group)
{
    if (!group) return -1;
    for (int i = 0; i < group->count; i++) {
        if (group->buttons[i]->selected) return i;
    }
    return -1;
}

void clue_radio_group_set_selected(ClueRadioGroup *group, int index)
{
    if (!group || index < 0 || index >= group->count) return;
    radio_select(group->buttons[index]);
}
