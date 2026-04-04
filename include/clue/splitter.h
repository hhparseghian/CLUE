#ifndef CLUE_SPLITTER_H
#define CLUE_SPLITTER_H

#include "clue_widget.h"
#include "box.h"

/* Splitter -- two-pane resizable container with a draggable divider */
typedef struct {
    ClueWidget          base;       /* MUST be first */
    ClueBoxOrientation  orientation;
    float               ratio;      /* 0.0-1.0, position of divider */
    int                 divider_px; /* divider thickness in pixels */
    bool                dragging;
} ClueSplitter;

/* Create a splitter (CLUE_HORIZONTAL = side-by-side, CLUE_VERTICAL = top/bottom) */
ClueSplitter *clue_splitter_new(ClueBoxOrientation orientation);

/* Set the split ratio (0.0-1.0) */
void clue_splitter_set_ratio(ClueSplitter *s, float ratio);

/* Get the split ratio */
float clue_splitter_get_ratio(ClueSplitter *s);

/* Add exactly two children with clue_container_add().
 * The first child is the "start" pane, the second is the "end" pane. */

/* Signals: "changed" -- emitted when ratio changes */

#endif /* CLUE_SPLITTER_H */
