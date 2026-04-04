#ifndef CLUE_LABEL_H
#define CLUE_LABEL_H

#include "clue_widget.h"

/* Label widget -- displays static text, supports word wrap and newlines */
typedef struct {
    ClueWidget  base;   /* MUST be first */
    char       *text;
    bool        wrap;   /* enable word wrapping */
} ClueLabel;

/* Create a new label */
ClueLabel *clue_label_new(const char *text);

/* Update label text */
void clue_label_set_text(ClueLabel *label, const char *text);

/* Get label text */
const char *clue_label_get_text(ClueLabel *label);

/* Enable/disable word wrapping (requires a fixed width to be set) */
void clue_label_set_wrap(ClueLabel *label, bool wrap);

#endif /* CLUE_LABEL_H */
