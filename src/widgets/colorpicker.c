#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clue/colorpicker.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#define SWATCH_DEFAULT 20
#define SWATCH_GAP     2
#define SELECTED_H     24
#define PAD            4
#define HUE_BAR_W      20
#define HUE_BAR_GAP    6

/* --- HSV conversion --- */

static ClueColor hsv_to_rgb(float h, float s, float v)
{
    float c = v * s;
    float hp = fmodf(h / 60.0f, 6.0f);
    float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;

    if (hp < 1)      { r = c; g = x; }
    else if (hp < 2) { r = x; g = c; }
    else if (hp < 3) { g = c; b = x; }
    else if (hp < 4) { g = x; b = c; }
    else if (hp < 5) { r = x; b = c; }
    else             { r = c; b = x; }

    return (ClueColor){r + m, g + m, b + m, 1.0f};
}

static void rgb_to_hsv(ClueColor c, float *h, float *s, float *v)
{
    float mx = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
    float mn = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
    float d = mx - mn;
    *v = mx;
    *s = mx > 0.0001f ? d / mx : 0.0f;
    if (d < 0.0001f) { *h = 0.0f; return; }
    if (mx == c.r)      *h = 60.0f * fmodf((c.g - c.b) / d, 6.0f);
    else if (mx == c.g) *h = 60.0f * ((c.b - c.r) / d + 2.0f);
    else                *h = 60.0f * ((c.r - c.g) / d + 4.0f);
    if (*h < 0) *h += 360.0f;
}

/* --- Layout regions --- */

/* Returns Y offset of gradient area */
static int grad_y(ClueColorPicker *cp)
{
    return cp->base.base.y + SELECTED_H + PAD;
}

/* Returns Y offset of palette area */
static int palette_y(ClueColorPicker *cp)
{
    return grad_y(cp) + cp->grad_size + PAD;
}

/* --- Drawing --- */

static void colorpicker_draw(ClueWidget *w)
{
    ClueColorPicker *cp = (ClueColorPicker *)w;
    const ClueTheme *th = clue_theme_get();
    int x = w->base.x, y = w->base.y;
    int gs = cp->grad_size;

    /* Selected color swatch */
    clue_fill_rounded_rect(x, y, w->base.w, SELECTED_H, 4.0f, cp->color);
    clue_draw_rounded_rect(x, y, w->base.w, SELECTED_H, 4.0f, 1.0f, th->surface_border);

    /* SV gradient square - draw as rows of colored pixels */
    int gy = grad_y(cp);
    int step = 2;
    for (int row = 0; row < gs; row += step) {
        float v = 1.0f - (float)row / (float)gs;
        for (int col = 0; col < gs; col += step) {
            float s = (float)col / (float)gs;
            ClueColor c = hsv_to_rgb(cp->hue, s, v);
            int bw = (col + step <= gs) ? step : gs - col;
            int bh = (row + step <= gs) ? step : gs - row;
            clue_fill_rect(x + col, gy + row, bw, bh, c);
        }
    }
    clue_draw_rect(x, gy, gs, gs, 1.0f, th->surface_border);

    /* SV crosshair */
    int cx = x + (int)(cp->sat * gs);
    int cy = gy + (int)((1.0f - cp->val) * gs);
    clue_draw_circle(cx, cy, 5, 2.0f, (ClueColor){1, 1, 1, 1});
    clue_draw_circle(cx, cy, 4, 1.0f, (ClueColor){0, 0, 0, 1});

    /* Hue bar */
    int hx = x + gs + HUE_BAR_GAP;
    for (int row = 0; row < gs; row += step) {
        float h = (float)row / (float)gs * 360.0f;
        ClueColor c = hsv_to_rgb(h, 1.0f, 1.0f);
        int bh = (row + step <= gs) ? step : gs - row;
        clue_fill_rect(hx, gy + row, cp->hue_bar_w, bh, c);
    }
    clue_draw_rect(hx, gy, cp->hue_bar_w, gs, 1.0f, th->surface_border);

    /* Hue indicator */
    int hy = gy + (int)(cp->hue / 360.0f * gs);
    clue_fill_rect(hx - 2, hy - 1, cp->hue_bar_w + 4, 3, (ClueColor){1, 1, 1, 1});
    clue_draw_rect(hx - 2, hy - 1, cp->hue_bar_w + 4, 3, 1.0f, (ClueColor){0, 0, 0, 1});

    /* Palette grid */
    int py = palette_y(cp);
    int sz = cp->swatch_size;
    for (int i = 0; i < cp->palette_count; i++) {
        int col = i % cp->cols;
        int row = i / cp->cols;
        int sx = x + col * (sz + SWATCH_GAP);
        int sy = py + row * (sz + SWATCH_GAP);

        clue_fill_rect(sx, sy, sz, sz, cp->palette[i]);

        if (i == cp->hovered)
            clue_draw_rect(sx - 1, sy - 1, sz + 2, sz + 2, 2.0f, th->fg_bright);

        ClueColor c = cp->color, p = cp->palette[i];
        if (fabsf(c.r - p.r) < 0.01f && fabsf(c.g - p.g) < 0.01f && fabsf(c.b - p.b) < 0.01f)
            clue_draw_rect(sx, sy, sz, sz, 2.0f, th->fg_bright);
    }
}

static void colorpicker_layout(ClueWidget *w)
{
    ClueColorPicker *cp = (ClueColorPicker *)w;
    int sz = cp->swatch_size;
    int rows = (cp->palette_count + cp->cols - 1) / cp->cols;

    /* Total width matches the palette */
    int pal_w = cp->cols * (sz + SWATCH_GAP) - SWATCH_GAP;
    w->base.w = pal_w;

    /* SV square fills remaining width after hue bar */
    cp->grad_size = pal_w - HUE_BAR_GAP - cp->hue_bar_w;
    if (cp->grad_size < 60) cp->grad_size = 60;

    int gs = cp->grad_size;
    w->base.h = SELECTED_H + PAD + gs + PAD +
                rows * (sz + SWATCH_GAP) - SWATCH_GAP;
}

static void update_color_from_hsv(ClueColorPicker *cp)
{
    cp->color = hsv_to_rgb(cp->hue, cp->sat, cp->val);
}

static int colorpicker_handle_event(ClueWidget *w, ClueEvent *event)
{
    ClueColorPicker *cp = (ClueColorPicker *)w;
    int x = w->base.x;
    int gs = cp->grad_size;
    int gy = grad_y(cp);
    int hx = x + gs + HUE_BAR_GAP;
    int py = palette_y(cp);
    int sz = cp->swatch_size;

    if (event->type == CLUE_EVENT_MOUSE_MOVE) {
        int mx = event->mouse_move.x, my = event->mouse_move.y;

        /* Drag SV */
        if (cp->dragging_sv) {
            cp->sat = (float)(mx - x) / (float)gs;
            cp->val = 1.0f - (float)(my - gy) / (float)gs;
            if (cp->sat < 0) cp->sat = 0; if (cp->sat > 1) cp->sat = 1;
            if (cp->val < 0) cp->val = 0; if (cp->val > 1) cp->val = 1;
            update_color_from_hsv(cp);
            clue_signal_emit(cp, "changed");
            return 1;
        }

        /* Drag hue */
        if (cp->dragging_hue) {
            cp->hue = (float)(my - gy) / (float)gs * 360.0f;
            if (cp->hue < 0) cp->hue = 0; if (cp->hue > 360) cp->hue = 360;
            update_color_from_hsv(cp);
            clue_signal_emit(cp, "changed");
            return 1;
        }

        /* Palette hover */
        cp->hovered = -1;
        if (my >= py) {
            int row = (my - py) / (sz + SWATCH_GAP);
            int col = (mx - x) / (sz + SWATCH_GAP);
            if (col >= 0 && col < cp->cols) {
                int idx = row * cp->cols + col;
                if (idx >= 0 && idx < cp->palette_count)
                    cp->hovered = idx;
            }
        }
        return 0;
    }

    if (event->type == CLUE_EVENT_MOUSE_BUTTON && event->mouse_button.btn == 0) {
        int mx = event->mouse_button.x, my = event->mouse_button.y;

        if (event->mouse_button.pressed) {
            /* Click on SV square */
            if (mx >= x && mx < x + gs && my >= gy && my < gy + gs) {
                cp->dragging_sv = true;
                cp->sat = (float)(mx - x) / (float)gs;
                cp->val = 1.0f - (float)(my - gy) / (float)gs;
                if (cp->sat < 0) cp->sat = 0; if (cp->sat > 1) cp->sat = 1;
                if (cp->val < 0) cp->val = 0; if (cp->val > 1) cp->val = 1;
                update_color_from_hsv(cp);
                clue_capture_mouse(&w->base);
                clue_signal_emit(cp, "changed");
                return 1;
            }

            /* Click on hue bar */
            if (mx >= hx && mx < hx + cp->hue_bar_w && my >= gy && my < gy + gs) {
                cp->dragging_hue = true;
                cp->hue = (float)(my - gy) / (float)gs * 360.0f;
                if (cp->hue < 0) cp->hue = 0; if (cp->hue > 360) cp->hue = 360;
                update_color_from_hsv(cp);
                clue_capture_mouse(&w->base);
                clue_signal_emit(cp, "changed");
                return 1;
            }

            /* Click on palette */
            if (my >= py) {
                int row = (my - py) / (sz + SWATCH_GAP);
                int col = (mx - x) / (sz + SWATCH_GAP);
                if (col >= 0 && col < cp->cols) {
                    int idx = row * cp->cols + col;
                    if (idx >= 0 && idx < cp->palette_count) {
                        cp->color = cp->palette[idx];
                        rgb_to_hsv(cp->color, &cp->hue, &cp->sat, &cp->val);
                        clue_signal_emit(cp, "changed");
                        return 1;
                    }
                }
            }
        } else {
            if (cp->dragging_sv || cp->dragging_hue) {
                cp->dragging_sv = false;
                cp->dragging_hue = false;
                clue_release_mouse();
                return 1;
            }
        }
    }

    return 0;
}

static void colorpicker_destroy(ClueWidget *w)
{
    ClueColorPicker *cp = (ClueColorPicker *)w;
    free(cp->palette);
}

static const ClueWidgetVTable colorpicker_vtable = {
    .draw         = colorpicker_draw,
    .layout       = colorpicker_layout,
    .handle_event = colorpicker_handle_event,
    .destroy      = colorpicker_destroy,
};

/* Default palette: Tango Desktop Project colors.
 * 3 columns: light / mid / dark for each hue family.
 * https://tango-project.org/tango_icon_theme_guidelines */
#define C(r,g,b) {(r)/255.0f, (g)/255.0f, (b)/255.0f, 1.0f}
static const ClueColor default_palette[] = {
    /* Row 1: Light */
    C(255,255,255), C(85,87,83),    C(239,41,41),   C(252,175,62),
    C(252,233,79),  C(138,226,52),  C(114,159,207), C(173,127,168), C(233,185,110),
    /* Row 2: Mid */
    C(186,189,182), C(46,52,54),    C(204,0,0),     C(245,121,0),
    C(237,212,0),   C(115,210,22),  C(52,101,164),  C(117,80,123),  C(193,125,17),
    /* Row 3: Dark */
    C(136,138,133), C(0,0,0),       C(164,0,0),     C(206,92,0),
    C(196,160,0),   C(78,154,6),    C(32,74,135),   C(92,53,102),   C(143,89,2),
};
#undef C

ClueColorPicker *clue_colorpicker_new(void)
{
    ClueColorPicker *cp = calloc(1, sizeof(ClueColorPicker));
    if (!cp) return NULL;

    clue_cwidget_init(&cp->base, &colorpicker_vtable);
    cp->base.type_id = CLUE_WIDGET_GENERIC;
    cp->cols = 9;
    cp->swatch_size = SWATCH_DEFAULT;
    cp->grad_size = 0; /* computed by layout */
    cp->hue_bar_w = HUE_BAR_W;
    cp->hovered = -1;
    cp->hue = 0;
    cp->sat = 1.0f;
    cp->val = 1.0f;
    cp->color = hsv_to_rgb(0, 1.0f, 1.0f);

    int n = sizeof(default_palette) / sizeof(default_palette[0]);
    cp->palette = malloc(n * sizeof(ClueColor));
    if (cp->palette) {
        memcpy(cp->palette, default_palette, n * sizeof(ClueColor));
        cp->palette_count = n;
    }

    colorpicker_layout(&cp->base);
    return cp;
}

ClueColor clue_colorpicker_get_color(ClueColorPicker *cp)
{
    if (cp) return cp->color;
    return (ClueColor){0, 0, 0, 1};
}

void clue_colorpicker_set_color(ClueColorPicker *cp, ClueColor color)
{
    if (!cp) return;
    cp->color = color;
    rgb_to_hsv(color, &cp->hue, &cp->sat, &cp->val);
}

void clue_colorpicker_add_color(ClueColorPicker *cp, ClueColor color)
{
    if (!cp) return;
    ClueColor *new_pal = realloc(cp->palette, (cp->palette_count + 1) * sizeof(ClueColor));
    if (!new_pal) return;
    cp->palette = new_pal;
    cp->palette[cp->palette_count++] = color;
}
