#ifndef CLUE_DROPDOWN_H
#define CLUE_DROPDOWN_H

#include "clue_widget.h"

#define CLUE_DROPDOWN_MAX_ITEMS 32

/* Dropdown / select widget */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    char       *items[CLUE_DROPDOWN_MAX_ITEMS];
    int         item_count;
    int         selected;   /* -1 = none */
    int         hovered;    /* -1 = none */
    bool        open;       /* dropdown list visible */
    char       *placeholder;
} ClueDropdown;

/* Create a new dropdown */
ClueDropdown *clue_dropdown_new(const char *placeholder);

/* Add an item to the dropdown */
void clue_dropdown_add_item(ClueDropdown *dd, const char *text);

/* Get selected index (-1 if none) */
int clue_dropdown_get_selected(ClueDropdown *dd);

/* Get selected item text (NULL if none) */
const char *clue_dropdown_get_selected_text(ClueDropdown *dd);

/* Set selected index */
void clue_dropdown_set_selected(ClueDropdown *dd, int index);

/* Draw the popup list overlay (called after all widgets) */
void clue_dropdown_draw_overlay(ClueDropdown *dd);

/* Signals: "changed" -- emitted when selection changes */

#endif /* CLUE_DROPDOWN_H */
