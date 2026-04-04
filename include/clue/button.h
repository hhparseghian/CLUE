#ifndef CLUE_BUTTON_H
#define CLUE_BUTTON_H

#include "clue_widget.h"

/* Button visual state */
typedef enum {
    CLUE_BUTTON_NORMAL = 0,
    CLUE_BUTTON_HOVER,
    CLUE_BUTTON_PRESSED,
} ClueButtonState;

/* Button widget -- clickable, emits "clicked" signal */
typedef struct {
    ClueWidget       base;   /* MUST be first */
    char            *label;
    ClueButtonState  state;
} ClueButton;

/* Create a new button with the given label text */
ClueButton *clue_button_new(const char *label);

/* Update the button label */
void clue_button_set_label(ClueButton *button, const char *label);

#endif /* CLUE_BUTTON_H */
