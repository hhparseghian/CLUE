#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clue/gauge.h"
#include "clue/draw.h"
#include "clue/app.h"
#include "clue/font.h"
#include "clue/theme.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Palette — matches smart-dash look. Kept local so the gauge is self-contained. */
#define GAUGE_COLOR_GREEN     CLUE_RGB(0, 210, 0)
#define GAUGE_COLOR_RED       CLUE_RGB(220, 50, 50)
#define GAUGE_COLOR_NEEDLE    CLUE_RGB(255, 50, 50)

/* ---- nice-number rounding (1-2-5 scheme) ---- */

static double nice_num(double x, bool do_round)
{
    if (x <= 0.0 || !isfinite(x)) return 1.0;

    double exp_val = floor(log10(x));
    double f = x / pow(10.0, exp_val);
    double nf;

    if (do_round) {
        if (f < 1.5)      nf = 1.0;
        else if (f < 3.0) nf = 2.0;
        else if (f < 7.0) nf = 5.0;
        else              nf = 10.0;
    } else {
        if (f <= 1.0)      nf = 1.0;
        else if (f <= 2.0) nf = 2.0;
        else if (f <= 5.0) nf = 5.0;
        else               nf = 10.0;
    }

    return nf * pow(10.0, exp_val);
}

static void compute_ticks(ClueGauge *g)
{
    /* User-supplied ticks override everything; just keep a sane major_step
     * for minor-tick subdivision (use the smallest gap in the user list). */
    if (g->manual_ticks) {
        double smallest = g->v_max - g->v_min;
        for (int i = 1; i < g->num_major_ticks; i++) {
            double gap = g->major_ticks[i] - g->major_ticks[i - 1];
            if (gap > 0 && gap < smallest) smallest = gap;
        }
        g->major_step = smallest > 0 ? smallest : 1.0;
        return;
    }

    double span = g->v_max - g->v_min;
    if (span < 1e-12) span = 1.0;

    if (g->manual_step > 0.0) {
        g->major_step = g->manual_step;
    } else {
        int divs = g->target_divisions > 0 ? g->target_divisions : 5;
        double approx_step = span / (double)divs;
        g->major_step = nice_num(approx_step, true);
    }
    if (g->major_step <= 1e-12) g->major_step = 1.0;

    double start = ceil(g->v_min / g->major_step) * g->major_step;
    double end   = floor(g->v_max / g->major_step) * g->major_step;

    g->num_major_ticks = 0;
    for (double v = start;
         v <= end + 1e-9 && g->num_major_ticks < CLUE_GAUGE_MAX_TICKS;
         v += g->major_step) {
        double t = (fabs(v) < 1e-9) ? 0.0 : v;
        g->major_ticks[g->num_major_ticks++] = t;
    }
}

/* ---- angle math ---- */

static double deg_to_rad(double deg) { return deg * M_PI / 180.0; }

static double value_to_angle(const ClueGauge *g, double val)
{
    double span = g->v_max - g->v_min;
    if (span <= 1e-12) return deg_to_rad(g->angle_min);

    double frac = (val - g->v_min) / span;
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;

    return deg_to_rad(g->angle_min + frac * (g->angle_max - g->angle_min));
}

static void format_tick(double v, char *buf, int bufsize)
{
    if (fabs(v) >= 1000.0 && fabs(v - round(v)) < 1e-6) {
        int iv = (int)round(v);
        if (iv % 1000 == 0)
            snprintf(buf, bufsize, "%dk", iv / 1000);
        else
            snprintf(buf, bufsize, "%d", iv);
    } else if (fabs(v - round(v)) < 1e-6) {
        snprintf(buf, bufsize, "%d", (int)round(v));
    } else if (fabs(v) < 100.0) {
        snprintf(buf, bufsize, "%.1f", v);
    } else {
        snprintf(buf, bufsize, "%d", (int)round(v));
    }
}

/* ---- widget vtable ---- */

static void gauge_draw(ClueWidget *w)
{
    ClueGauge *g = (ClueGauge *)w;
    int x = w->base.x, y = w->base.y;
    int width = w->base.w, height = w->base.h;

    const ClueTheme *th = clue_theme_get();
    ClueFont *default_font = clue_app_default_font();
    ClueFont *label_font = g->label_font ? g->label_font : default_font;
    ClueFont *value_font = g->value_font ? g->value_font : default_font;

    int value_font_h = value_font ? clue_font_line_height(value_font) : 14;

    /* Reserve space below dial for value + name label */
    int text_area = value_font_h * 2 + 8;
    int gauge_h = height - text_area;
    if (gauge_h < 40) gauge_h = height;

    int avail = (width < gauge_h ? width : gauge_h);
    int radius = (avail - 20) / 2;
    if (radius < 20) return;

    int cx = x + width / 2;
    int cy = y + 6 + radius;

    double span_deg = g->angle_max - g->angle_min;
    double vspan = g->v_max - g->v_min;
    if (vspan < 1e-12) vspan = 1.0;

    /* ---- bezel ---- */
    clue_fill_circle(cx, cy, radius,     CLUE_RGB(50, 55, 65));
    clue_fill_circle(cx, cy, radius - 2, CLUE_RGB(35, 38, 48));
    clue_fill_circle(cx, cy, radius - 4, CLUE_RGB(22, 25, 35));
    clue_draw_circle(cx, cy, radius,     2.0f, CLUE_RGB(90, 95, 110));
    clue_draw_circle(cx, cy, radius - 5, 1.0f, CLUE_RGB(55, 60, 70));

    /* ---- coloured arc ---- */
    int arc_r = (int)(radius * 0.90);
    float arc_thick = (radius > 60) ? 4.0f : 3.0f;

    double norm_min_frac = (g->norm_min - g->v_min) / vspan;
    double norm_max_frac = (g->norm_max - g->v_min) / vspan;
    if (norm_min_frac < 0.0) norm_min_frac = 0.0;
    if (norm_max_frac > 1.0) norm_max_frac = 1.0;

    double green_start_deg = g->angle_min + norm_min_frac * span_deg;
    double green_end_deg   = g->angle_min + norm_max_frac * span_deg;

    clue_draw_arc(cx, cy, arc_r,
                  (float)deg_to_rad(g->angle_min),
                  (float)deg_to_rad(g->angle_max),
                  arc_thick, GAUGE_COLOR_RED);

    if (norm_max_frac > norm_min_frac) {
        clue_draw_arc(cx, cy, arc_r,
                      (float)deg_to_rad(green_start_deg),
                      (float)deg_to_rad(green_end_deg),
                      arc_thick, GAUGE_COLOR_GREEN);
    }

    /* ---- ticks + labels ---- */
    int tick_outer       = (int)(radius * 0.84);
    int tick_inner_major = (int)(radius * 0.68);
    int tick_inner_minor = (int)(radius * 0.76);

    #define DRAW_TICK(val, is_major) do { \
        double _ang = value_to_angle(g, val); \
        double _ca = cos(_ang), _sa = sin(_ang); \
        int _inner = (is_major) ? tick_inner_major : tick_inner_minor; \
        float _thick = (is_major) ? 2.0f : 1.0f; \
        ClueColor _col = (is_major) ? CLUE_RGB(200, 205, 215) : CLUE_RGB(90, 95, 105); \
        clue_draw_line( \
            cx + (int)(_inner * _ca), cy + (int)(_inner * _sa), \
            cx + (int)(tick_outer * _ca), cy + (int)(tick_outer * _sa), \
            _thick, _col); \
    } while (0)

    /* Major ticks + labels (no forced endpoint ticks — the arc itself caps the dial) */
    for (int i = 0; i < g->num_major_ticks; i++) {
        double v = g->major_ticks[i];
        DRAW_TICK(v, true);

        if (g->show_numbers && label_font) {
            char buf[16];
            format_tick(v, buf, sizeof(buf));
            double ang = value_to_angle(g, v);
            double ca = cos(ang), sa = sin(ang);
            int tw = clue_font_text_width(label_font, buf);
            int th_px = clue_font_line_height(label_font);
            /* Radial extent of the label's bbox from its center at this angle,
             * so the outer edge of the text lands just inside the tick. */
            int radial_half = (int)(fabs(tw * 0.5 * ca) + fabs(th_px * 0.5 * sa));
            int lr = tick_inner_major - radial_half - 3;
            if (lr < 0) lr = 0;
            clue_draw_text(cx + (int)(lr * ca) - tw / 2,
                           cy + (int)(lr * sa) - th_px / 2,
                           buf, label_font, CLUE_RGB(180, 185, 195));
        }
    }

    /* Minor ticks */
    double minor_step = g->minor_subdivisions > 0
                        ? g->major_step / (double)g->minor_subdivisions
                        : 0.0;
    if (minor_step > 0) {
        double mv = ceil(g->v_min / minor_step) * minor_step;
        for (; mv <= g->v_max + 1e-9; mv += minor_step) {
            bool skip = false;
            if (fabs(mv - g->v_min) < minor_step * 0.3) skip = true;
            if (fabs(mv - g->v_max) < minor_step * 0.3) skip = true;
            for (int j = 0; j < g->num_major_ticks && !skip; j++) {
                if (fabs(mv - g->major_ticks[j]) < minor_step * 0.3)
                    skip = true;
            }
            if (skip) continue;
            DRAW_TICK(mv, false);
        }
    }
    #undef DRAW_TICK

    /* ---- needle ---- */
    double needle_ang = value_to_angle(g, g->value);
    double nca = cos(needle_ang), nsa = sin(needle_ang);
    int tip_len  = (int)(radius * 0.72);
    int tail_len = (int)(radius * 0.15);

    int tip_x  = cx + (int)(tip_len * nca);
    int tip_y  = cy + (int)(tip_len * nsa);
    int tail_x = cx - (int)(tail_len * nca);
    int tail_y = cy - (int)(tail_len * nsa);

    clue_draw_line(cx + 1, cy + 2, tip_x + 1, tip_y + 2,
                   2.0f, CLUE_RGBA(0, 0, 0, 100));
    clue_draw_line(tail_x, tail_y, tip_x, tip_y,
                   2.0f, GAUGE_COLOR_NEEDLE);

    /* Hub */
    clue_fill_circle(cx, cy, (int)(radius * 0.06) + 1, CLUE_RGB(180, 40, 40));
    clue_fill_circle(cx, cy, (int)(radius * 0.03) + 1, CLUE_RGB(240, 60, 60));

    /* ---- value + unit below dial ---- */
    int text_y = cy + radius + 6;
    if (value_font) {
        char vbuf[64];
        if (g->unit[0])
            snprintf(vbuf, sizeof(vbuf), "%.1f %s", g->value, g->unit);
        else
            snprintf(vbuf, sizeof(vbuf), "%.1f", g->value);
        int tw = clue_font_text_width(value_font, vbuf);
        clue_draw_text(cx - tw / 2, text_y, vbuf, value_font, th->fg);
        text_y += value_font_h + 2;
    }

    if (g->label[0] && value_font) {
        int tw = clue_font_text_width(value_font, g->label);
        clue_draw_text(cx - tw / 2, text_y, g->label, value_font, th->fg_dim);
    }
}

static void gauge_layout(ClueWidget *w)
{
    if (w->base.w == 0) w->base.w = 160;
    if (w->base.h == 0) w->base.h = 190;
}

static const ClueWidgetVTable gauge_vtable = {
    .draw         = gauge_draw,
    .layout       = gauge_layout,
    .handle_event = NULL,
    .destroy      = NULL,
};

/* ---- public API ---- */

ClueGauge *clue_gauge_new(double vmin, double vmax,
                          double norm_min, double norm_max,
                          const char *unit)
{
    ClueGauge *g = calloc(1, sizeof(ClueGauge));
    if (!g) return NULL;

    clue_cwidget_init(&g->base, &gauge_vtable);

    g->v_min = vmin;
    g->v_max = vmax;
    g->norm_min = norm_min;
    g->norm_max = norm_max;
    g->angle_min = CLUE_GAUGE_ANGLE_MIN;
    g->angle_max = CLUE_GAUGE_ANGLE_MAX;
    g->show_numbers = true;
    g->target_divisions = 5;
    g->minor_subdivisions = 5;
    g->manual_step = 0.0;
    g->value = vmin;

    if (unit)
        snprintf(g->unit, sizeof(g->unit), "%s", unit);

    compute_ticks(g);
    gauge_layout(&g->base);
    return g;
}

void clue_gauge_set_value(ClueGauge *g, double value)
{
    if (!g) return;
    g->value = value;
}

double clue_gauge_get_value(ClueGauge *g)
{
    return g ? g->value : 0.0;
}

void clue_gauge_set_range(ClueGauge *g, double vmin, double vmax)
{
    if (!g) return;
    g->v_min = vmin;
    g->v_max = vmax;
    compute_ticks(g);
}

void clue_gauge_set_normal_range(ClueGauge *g, double norm_min, double norm_max)
{
    if (!g) return;
    g->norm_min = norm_min;
    g->norm_max = norm_max;
}

void clue_gauge_set_label(ClueGauge *g, const char *label)
{
    if (!g) return;
    if (label) snprintf(g->label, sizeof(g->label), "%s", label);
    else       g->label[0] = '\0';
}

void clue_gauge_set_unit(ClueGauge *g, const char *unit)
{
    if (!g) return;
    if (unit) snprintf(g->unit, sizeof(g->unit), "%s", unit);
    else      g->unit[0] = '\0';
}

void clue_gauge_set_angles(ClueGauge *g, double angle_min_deg, double angle_max_deg)
{
    if (!g) return;
    g->angle_min = angle_min_deg;
    g->angle_max = angle_max_deg;
}

void clue_gauge_set_show_numbers(ClueGauge *g, bool on)
{
    if (!g) return;
    g->show_numbers = on;
}

void clue_gauge_set_target_divisions(ClueGauge *g, int n)
{
    if (!g) return;
    g->target_divisions = n < 1 ? 1 : n;
    compute_ticks(g);
}

void clue_gauge_set_minor_subdivisions(ClueGauge *g, int n)
{
    if (!g) return;
    g->minor_subdivisions = n < 0 ? 0 : n;
}

void clue_gauge_set_major_step(ClueGauge *g, double step)
{
    if (!g) return;
    g->manual_step = step > 0.0 ? step : 0.0;
    compute_ticks(g);
}

void clue_gauge_set_major_ticks(ClueGauge *g, const double *values, int count)
{
    if (!g) return;
    if (!values || count <= 0) {
        g->manual_ticks = false;
        compute_ticks(g);
        return;
    }
    int n = count < CLUE_GAUGE_MAX_TICKS ? count : CLUE_GAUGE_MAX_TICKS;
    for (int i = 0; i < n; i++) g->major_ticks[i] = values[i];
    g->num_major_ticks = n;
    g->manual_ticks = true;
    compute_ticks(g);
}

void clue_gauge_set_label_font(ClueGauge *g, ClueFont *font)
{
    if (!g) return;
    g->label_font = font;
}

void clue_gauge_set_value_font(ClueGauge *g, ClueFont *font)
{
    if (!g) return;
    g->value_font = font;
}
