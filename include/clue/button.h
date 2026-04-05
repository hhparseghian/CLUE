#ifndef CLUE_BUTTON_H
#define CLUE_BUTTON_H

#include "clue_widget.h"
#include "renderer.h"

/* Button visual state */
typedef enum {
    CLUE_BUTTON_NORMAL = 0,
    CLUE_BUTTON_HOVER,
    CLUE_BUTTON_PRESSED,
} ClueButtonState;

/* Button widget -- clickable, emits "clicked" signal.
 * Optionally displays an icon texture above the label. */
typedef struct {
    ClueWidget       base;   /* MUST be first */
    char            *label;
    ClueButtonState  state;
    UITexture        icon;       /* optional icon texture (0 = none) */
    int              icon_w;     /* display width for the icon */
    int              icon_h;     /* display height for the icon */
} ClueButton;

/* Create a new button with the given label text */
ClueButton *clue_button_new(const char *label);

/* Update the button label */
void clue_button_set_label(ClueButton *button, const char *label);

/* Set an icon texture to display above the label.
 * Pass 0 to remove the icon. w/h set the display size. */
void clue_button_set_icon(ClueButton *button, UITexture icon, int w, int h);

#endif /* CLUE_BUTTON_H */
