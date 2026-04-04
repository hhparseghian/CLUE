#ifndef CLUE_BOX_H
#define CLUE_BOX_H

#include "clue_widget.h"

/* Box orientation */
typedef enum {
    CLUE_VERTICAL,
    CLUE_HORIZONTAL,
} ClueBoxOrientation;

/* Box container -- lays out children vertically or horizontally */
typedef struct {
    ClueWidget          base;   /* MUST be first */
    ClueBoxOrientation  orientation;
    int                 spacing;
} ClueBox;

/* Create a new box container */
ClueBox *clue_box_new(ClueBoxOrientation orientation, int spacing);

#endif /* CLUE_BOX_H */
