#ifndef CLUE_GAUGE_H
#define CLUE_GAUGE_H

#include "clue_widget.h"
#include "font.h"

/*
 * Circular analog gauge widget.
 *
 *   - Coloured arc (green inside the normal range, red outside)
 *   - Auto-computed major/minor ticks with 1-2-5 nice-number rounding
 *   - Needle with shadow and hub cap
 *   - Value + unit and optional label drawn below the dial
 *   - Sweep defaults to 130 deg .. 410 deg (8 o'clock to 4 o'clock)
 */

#define CLUE_GAUGE_MAX_TICKS   32
#define CLUE_GAUGE_ANGLE_MIN   130.0
#define CLUE_GAUGE_ANGLE_MAX   410.0

typedef struct {
    ClueWidget  base;           /* MUST be first */

    double      v_min;
    double      v_max;
    double      norm_min;       /* green zone start */
    double      norm_max;       /* green zone end   */

    double      angle_min;      /* sweep start in degrees */
    double      angle_max;      /* sweep end   in degrees */

    double      value;

    char        unit[16];
    char        label[32];
    bool        show_numbers;

    /* Optional fonts (NULL = app default). Widget does NOT own them. */
    ClueFont   *label_font;     /* tick labels */
    ClueFont   *value_font;     /* value readout + name below dial */

    /* tick controls */
    int         target_divisions;     /* desired major divisions (default 5) */
    int         minor_subdivisions;   /* minor ticks per major  (default 5; 0 disables) */
    double      manual_step;          /* if > 0, overrides nice-number step */
    bool        manual_ticks;         /* if true, major_ticks[] is user-set */

    /* internal tick cache */
    double      major_step;
    int         num_major_ticks;
    double      major_ticks[CLUE_GAUGE_MAX_TICKS];
} ClueGauge;

/* Create a gauge with [vmin, vmax] range, [norm_min, norm_max] green zone,
 * and an optional unit string (e.g. "km/h"). unit may be NULL. */
ClueGauge *clue_gauge_new(double vmin, double vmax,
                          double norm_min, double norm_max,
                          const char *unit);

void   clue_gauge_set_value(ClueGauge *g, double value);
double clue_gauge_get_value(ClueGauge *g);

void clue_gauge_set_range(ClueGauge *g, double vmin, double vmax);
void clue_gauge_set_normal_range(ClueGauge *g, double norm_min, double norm_max);

void clue_gauge_set_label(ClueGauge *g, const char *label);
void clue_gauge_set_unit(ClueGauge *g, const char *unit);

void clue_gauge_set_angles(ClueGauge *g, double angle_min_deg, double angle_max_deg);
void clue_gauge_set_show_numbers(ClueGauge *g, bool on);

/* Target number of major divisions to aim for when auto-computing ticks.
 * The 1-2-5 nice-number rounding may produce a few more or fewer. Default 5. */
void clue_gauge_set_target_divisions(ClueGauge *g, int n);

/* Minor ticks drawn between each major. 0 disables minors. Default 5. */
void clue_gauge_set_minor_subdivisions(ClueGauge *g, int n);

/* Force a specific spacing between major ticks. Pass 0 to return to auto. */
void clue_gauge_set_major_step(ClueGauge *g, double step);

/* Set explicit major tick values (e.g. {0, 50, 100, 150, 200, 220}).
 * Overrides target_divisions and major_step. Pass NULL/0 to revert to auto.
 * Values are copied; max CLUE_GAUGE_MAX_TICKS. */
void clue_gauge_set_major_ticks(ClueGauge *g, const double *values, int count);

/* Font for tick labels. NULL to fall back to the app's default font.
 * The widget does not take ownership of the font. */
void clue_gauge_set_label_font(ClueGauge *g, ClueFont *font);

/* Font for value readout + name label below the dial. NULL = app default. */
void clue_gauge_set_value_font(ClueGauge *g, ClueFont *font);

#endif /* CLUE_GAUGE_H */
