#ifndef CLUE_CHECKBOX_H
#define CLUE_CHECKBOX_H

#include "clue_widget.h"

/* Checkbox widget -- toggleable on/off */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    char       *label;
    bool        checked;
} ClueCheckbox;

/* Create a new checkbox */
ClueCheckbox *clue_checkbox_new(const char *label);

/* Get/set checked state */
bool clue_checkbox_is_checked(ClueCheckbox *cb);
void clue_checkbox_set_checked(ClueCheckbox *cb, bool checked);

/* Signals: "toggled" -- emitted when state changes */

#endif /* CLUE_CHECKBOX_H */
