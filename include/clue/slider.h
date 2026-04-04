#ifndef CLUE_SLIDER_H
#define CLUE_SLIDER_H

#include "clue_widget.h"

/* Slider widget -- horizontal range selector */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    float       value;      /* 0.0 - 1.0 */
    float       min_val;
    float       max_val;
    bool        dragging;
} ClueSlider;

/* Create a new slider with range [min, max], initial value */
ClueSlider *clue_slider_new(float min_val, float max_val, float value);

/* Get/set value */
float clue_slider_get_value(ClueSlider *slider);
void  clue_slider_set_value(ClueSlider *slider, float value);

/* Signals: "changed" -- emitted when value changes */

#endif /* CLUE_SLIDER_H */
