#ifndef CLUE_OSK_H
#define CLUE_OSK_H

#include <stdbool.h>
#include "clue_widget.h"

/* On-screen keyboard types */
typedef enum {
    CLUE_OSK_QWERTY,
    CLUE_OSK_NUMPAD,
} ClueOskType;

/* Show the on-screen keyboard overlay (docked at bottom of window).
 * Input is injected into the currently focused widget. */
void clue_osk_show(ClueOskType type);

/* Dismiss the on-screen keyboard */
void clue_osk_hide(void);

/* Check if the OSK is currently visible */
bool clue_osk_visible(void);

/* Enable/disable auto-show: OSK appears when a text input gets focus,
 * hides when focus moves to a non-text widget.
 * type selects which keyboard layout to auto-show (QWERTY or NUMPAD). */
void clue_osk_set_auto(bool enabled, ClueOskType type);

/* Set a context label displayed above the preview (e.g. "Editing Min").
 * Pass NULL or "" to clear. */
void clue_osk_set_label(const char *label);

/* Called internally by the focus system -- do not call directly */
void clue_osk_on_focus_changed(struct UIWidget *widget);

#endif /* CLUE_OSK_H */