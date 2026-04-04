#ifndef CLUE_TABLE_H
#define CLUE_TABLE_H

#include "clue_widget.h"

#define CLUE_TABLE_MAX_COLS 16

/* Callback to provide cell text at (row, col) */
typedef const char *(*ClueTableCellCallback)(int row, int col, void *user_data);

/* Table widget -- multi-column data grid with headers */
typedef struct {
    ClueWidget              base;       /* MUST be first */
    char                   *headers[CLUE_TABLE_MAX_COLS];
    int                     col_widths[CLUE_TABLE_MAX_COLS];
    int                     col_count;
    int                     row_count;
    ClueTableCellCallback   cell_cb;
    void                   *cell_data;
    int                     row_height;
    int                     header_height;
    int                     selected_row;   /* -1 = none */
    int                     hovered_row;
    int                     scroll_y;
} ClueTable;

/* Create a table */
ClueTable *clue_table_new(void);

/* Add a column with header text and width */
void clue_table_add_column(ClueTable *t, const char *header, int width);

/* Set the data source */
void clue_table_set_data(ClueTable *t, int rows,
                         ClueTableCellCallback cb, void *user_data);

/* Get/set selected row */
int  clue_table_get_selected(ClueTable *t);
void clue_table_set_selected(ClueTable *t, int row);

/* Signals: "selected" */

#endif /* CLUE_TABLE_H */
