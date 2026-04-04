#ifndef CLUE_PROGRESS_H
#define CLUE_PROGRESS_H

#include "clue_widget.h"

/* Progress bar widget */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    float       value;      /* 0.0 - 1.0 */
    bool        indeterminate;
} ClueProgress;

ClueProgress *clue_progress_new(void);
void clue_progress_set_value(ClueProgress *p, float value);
float clue_progress_get_value(ClueProgress *p);
void clue_progress_set_indeterminate(ClueProgress *p, bool on);

#endif /* CLUE_PROGRESS_H */
