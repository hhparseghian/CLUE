#ifndef CLUE_TOOLTIP_H
#define CLUE_TOOLTIP_H

#include "clue_widget.h"

/* Attach a tooltip to any widget.
 * The tooltip text is drawn by the app after all widgets. */
void clue_tooltip_set(ClueWidget *widget, const char *text);

/* Get tooltip text for a widget (NULL if none) */
const char *clue_tooltip_get(ClueWidget *widget);

/* Called internally by the app loop to draw the active tooltip */
void clue_tooltip_draw(int mouse_x, int mouse_y);

/* Called internally to update hover state */
void clue_tooltip_update(ClueWidget *root, int mouse_x, int mouse_y);

#endif /* CLUE_TOOLTIP_H */
