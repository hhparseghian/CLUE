#ifndef CLUE_TOGGLE_H
#define CLUE_TOGGLE_H

#include "clue_widget.h"

/* Toggle / switch widget -- on/off pill switch */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    char       *label;
    bool        on;
    float       anim_pos;   /* 0.0 = off, 1.0 = on (for smooth slide) */
} ClueToggle;

/* Create a new toggle switch */
ClueToggle *clue_toggle_new(const char *label);

/* Get/set state */
bool clue_toggle_is_on(ClueToggle *t);
void clue_toggle_set_on(ClueToggle *t, bool on);

/* Signals: "toggled" -- emitted when state changes */

#endif /* CLUE_TOGGLE_H */
