#ifndef CLUE_SPINBOX_H
#define CLUE_SPINBOX_H

#include "clue_widget.h"

/* Spinbox -- numeric input with increment/decrement buttons */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    double      value;
    double      min_val;
    double      max_val;
    double      step;
    int         decimals;   /* decimal places to display (0 = integer) */
    bool        btn_up_hover;
    bool        btn_dn_hover;
    bool        btn_up_pressed;
    bool        btn_dn_pressed;
    int         repeat_timer;   /* auto-repeat timer while button held */
    int         repeat_count;   /* ticks elapsed (accelerates after initial delay) */
} ClueSpinbox;

/* Create a new spinbox with range, step, and initial value */
ClueSpinbox *clue_spinbox_new(double min_val, double max_val,
                              double step, double value);

/* Get/set value */
double clue_spinbox_get_value(ClueSpinbox *s);
void   clue_spinbox_set_value(ClueSpinbox *s, double value);

/* Set decimal places (default 0 = integer) */
void clue_spinbox_set_decimals(ClueSpinbox *s, int decimals);

/* Signals: "changed" -- emitted when value changes */

#endif /* CLUE_SPINBOX_H */
