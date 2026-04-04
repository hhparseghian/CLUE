#ifndef CLUE_SEPARATOR_H
#define CLUE_SEPARATOR_H

#include "clue_widget.h"
#include "box.h"

/* Separator widget -- draws a thin line, horizontal or vertical */
typedef struct {
    ClueWidget          base;        /* MUST be first */
    ClueBoxOrientation  orientation;
} ClueSeparator;

/* Create a new separator (CLUE_HORIZONTAL or CLUE_VERTICAL) */
ClueSeparator *clue_separator_new(ClueBoxOrientation orientation);

#endif /* CLUE_SEPARATOR_H */
