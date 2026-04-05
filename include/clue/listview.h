#ifndef CLUE_LISTVIEW_H
#define CLUE_LISTVIEW_H

#include "clue_widget.h"
#include "scrollbar.h"

/* Callback to provide item text at a given index */
typedef const char *(*ClueListViewItemCallback)(int index, void *user_data);

/* Optional callback to draw a custom icon/prefix per item.
 * Called with the item index, x/y position, item height, and user data.
 * Returns the horizontal offset to indent the text by. */
typedef int (*ClueListViewIconCallback)(int index, int x, int y, int h, void *user_data);

/* Virtual-scrolling list view for large data sets */
typedef struct {
    ClueWidget                  base;       /* MUST be first */
    ClueListViewItemCallback    item_cb;    /* provides item text */
    ClueListViewIconCallback    icon_cb;    /* optional: draws icon per item */
    void                       *item_data;  /* user data for callback */
    int                         item_count;
    int                         item_height;/* height per item in px */
    int                         scroll_y;
    int                         selected;   /* -1 = none */
    int                         hovered;    /* -1 = none */
    ClueScrollbar               sb;         /* shared scrollbar state */
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
