#ifndef CLUE_TABS_H
#define CLUE_TABS_H

#include "clue_widget.h"

#define CLUE_TABS_MAX 16

/* Tab container -- tabbed view with switchable pages */
typedef struct {
    ClueWidget   base;                     /* MUST be first */
    char        *tab_labels[CLUE_TABS_MAX];
    ClueWidget  *tab_pages[CLUE_TABS_MAX]; /* content widget per tab */
    int          tab_count;
    int          active;                   /* active tab index */
    int          hovered;                  /* hovered tab index */
    int          tab_height;               /* height of tab bar */
    ClueColor      page_bg;                 /* page background color (notebook) */
} ClueTabs;

/* Create a new tab container */
ClueTabs *clue_tabs_new(void);

/* Add a tab with a label and content widget. Returns tab index. */
int clue_tabs_add(ClueTabs *tabs, const char *label, ClueWidget *content);

/* Remove a tab by index. Does NOT destroy the page widget. */
void clue_tabs_remove(ClueTabs *tabs, int index);

/* Get/set active tab */
int  clue_tabs_get_active(ClueTabs *tabs);
void clue_tabs_set_active(ClueTabs *tabs, int index);

/* Signals: "changed" -- emitted when active tab changes */

#endif /* CLUE_TABS_H */
