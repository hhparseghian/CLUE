#ifndef CLUE_TOOLBAR_H
#define CLUE_TOOLBAR_H

#include "clue_widget.h"

/* Toolbar -- horizontal bar of buttons/widgets with a background */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    int         spacing;
} ClueToolbar;

/* Create a new toolbar */
ClueToolbar *clue_toolbar_new(int spacing);

/* Add child widgets with clue_container_add() */

#endif /* CLUE_TOOLBAR_H */
