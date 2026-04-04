#ifndef CLUE_STATUSBAR_H
#define CLUE_STATUSBAR_H

#include "clue_widget.h"

#define CLUE_STATUSBAR_MAX_SECTIONS 8

/* Statusbar -- thin bar at the bottom with text sections */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    char       *sections[CLUE_STATUSBAR_MAX_SECTIONS];
    int         section_count;
    int         height;
} ClueStatusbar;

/* Create a new statusbar */
ClueStatusbar *clue_statusbar_new(void);

/* Set text of a section (creates section if index == section_count) */
void clue_statusbar_set_text(ClueStatusbar *sb, int index, const char *text);

/* Get text of a section */
const char *clue_statusbar_get_text(ClueStatusbar *sb, int index);

/* Get section count */
int clue_statusbar_section_count(ClueStatusbar *sb);

#endif /* CLUE_STATUSBAR_H */
