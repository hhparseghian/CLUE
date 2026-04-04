#ifndef CLUE_MENU_H
#define CLUE_MENU_H

#include "clue_widget.h"

#define CLUE_MENU_MAX_ITEMS 24

/* Menu item types */
typedef enum {
    CLUE_MENU_ITEM_NORMAL,
    CLUE_MENU_ITEM_SEPARATOR,
} ClueMenuItemType;

typedef struct {
    ClueMenuItemType  type;
    char             *label;
    char             *shortcut;      /* display text like "Ctrl+S" */
    ClueSignalCallback callback;
    void             *user_data;
    bool              enabled;
} ClueMenuItem;

/* Popup / context menu */
typedef struct {
    ClueWidget    base;              /* MUST be first */
    ClueMenuItem  items[CLUE_MENU_MAX_ITEMS];
    int           item_count;
    int           hovered;
    bool          open;
    int           popup_x, popup_y;  /* screen position when open */
} ClueMenu;

/* Menu bar -- horizontal strip of menus */
typedef struct {
    ClueWidget    base;              /* MUST be first */
    char         *labels[CLUE_MENU_MAX_ITEMS];
    ClueMenu     *menus[CLUE_MENU_MAX_ITEMS];
    int           menu_count;
    int           active;            /* open menu index, -1 = none */
    int           hovered;
} ClueMenuBar;

/* Create a popup/context menu */
ClueMenu *clue_menu_new(void);

/* Add items to a menu */
void clue_menu_add_item(ClueMenu *menu, const char *label,
                        ClueSignalCallback callback, void *user_data);
void clue_menu_add_item_shortcut(ClueMenu *menu, const char *label,
                                 const char *shortcut,
                                 ClueSignalCallback callback, void *user_data);
void clue_menu_add_separator(ClueMenu *menu);

/* Show a menu at screen position */
void clue_menu_popup(ClueMenu *menu, int x, int y);
void clue_menu_close(ClueMenu *menu);

/* Create a menu bar */
ClueMenuBar *clue_menubar_new(void);

/* Add a menu to the menu bar */
void clue_menubar_add(ClueMenuBar *bar, const char *label, ClueMenu *menu);

/* Draw the active menu dropdown overlay (called after all widgets) */
void clue_menubar_draw_overlay(ClueMenuBar *bar);

#endif /* CLUE_MENU_H */
