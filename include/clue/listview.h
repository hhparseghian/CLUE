#ifndef CLUE_LISTVIEW_H
#define CLUE_LISTVIEW_H

#include "clue_widget.h"

/* Callback to provide item text at a given index */
typedef const char *(*ClueListViewItemCallback)(int index, void *user_data);

/* Virtual-scrolling list view for large data sets */
typedef struct {
    ClueWidget                  base;       /* MUST be first */
    ClueListViewItemCallback    item_cb;    /* provides item text */
    void                       *item_data;  /* user data for callback */
    int                         item_count;
    int                         item_height;/* height per item in px */
    int                         scroll_y;
    int                         selected;   /* -1 = none */
    int                         hovered;    /* -1 = none */
} ClueListView;

/* Create a list view */
ClueListView *clue_listview_new(void);

/* Set the data source */
void clue_listview_set_data(ClueListView *lv, int count,
                            ClueListViewItemCallback cb, void *user_data);

/* Get/set selection */
int  clue_listview_get_selected(ClueListView *lv);
void clue_listview_set_selected(ClueListView *lv, int index);

/* Signals: "selected" -- emitted when selection changes */

#endif /* CLUE_LISTVIEW_H */
