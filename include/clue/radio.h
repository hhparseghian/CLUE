#ifndef CLUE_RADIO_H
#define CLUE_RADIO_H

#include "clue_widget.h"

/* Forward declarations */
typedef struct ClueRadioGroup ClueRadioGroup;
typedef struct ClueRadio      ClueRadio;

/* Radio group -- ensures only one radio button is selected at a time */
struct ClueRadioGroup {
    ClueRadio *buttons[32]; /* max buttons per group */
    int        count;
};

/* Radio button widget */
struct ClueRadio {
    ClueWidget      base;   /* MUST be first */
    char           *label;
    bool            selected;
    ClueRadioGroup *group;
};

/* Create a radio group */
ClueRadioGroup *clue_radio_group_new(void);

/* Destroy a radio group (does not destroy the radio widgets) */
void clue_radio_group_destroy(ClueRadioGroup *group);

/* Create a new radio button and add it to a group */
ClueRadio *clue_radio_new(const char *label, ClueRadioGroup *group);

/* Get/set selected state */
bool clue_radio_is_selected(ClueRadio *r);
void clue_radio_set_selected(ClueRadio *r);

/* Get the currently selected radio button in a group (NULL if none) */
ClueRadio *clue_radio_group_get_selected(ClueRadioGroup *group);

/* Get the index of the selected button (-1 if none) */
int clue_radio_group_get_selected_index(ClueRadioGroup *group);

/* Select a button by index */
void clue_radio_group_set_selected(ClueRadioGroup *group, int index);

/* Signals: "changed" -- emitted on the newly selected radio button */

#endif /* CLUE_RADIO_H */
