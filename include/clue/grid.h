#ifndef CLUE_GRID_H
#define CLUE_GRID_H

#include "clue_widget.h"

#define CLUE_GRID_MAX_COLS 16

/* Grid container -- lays out children in rows and columns */
typedef struct {
    ClueWidget  base;               /* MUST be first */
    int         cols;               /* number of columns */
    int         row_spacing;
    int         col_spacing;
    int         col_widths[CLUE_GRID_MAX_COLS]; /* 0 = auto */
} ClueGrid;

/* Create a grid with the given number of columns */
ClueGrid *clue_grid_new(int cols, int row_spacing, int col_spacing);

/* Set a fixed width for a column (0 = auto-distribute) */
void clue_grid_set_col_width(ClueGrid *grid, int col, int width);

#endif /* CLUE_GRID_H */
