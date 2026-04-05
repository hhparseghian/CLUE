#ifndef CLUE_SCROLL_H
#define CLUE_SCROLL_H

#include "clue_widget.h"
#include "scrollbar.h"

/* Scroll container -- clips and scrolls its content */
typedef struct {
    ClueWidget    base;           /* MUST be first */
    int           scroll_x;
    int           scroll_y;
    int           content_w;      /* total content width */
    int           content_h;      /* total content height */
    int           scroll_speed;   /* pixels per scroll step */
    ClueScrollbar sb;             /* scrollbar drag state */
} ClueScroll;

/* Create a new scroll container */
ClueScroll *clue_scroll_new(void);

/* Scroll to a specific position */
void clue_scroll_to(ClueScroll *scroll, int x, int y);

#endif /* CLUE_SCROLL_H */
