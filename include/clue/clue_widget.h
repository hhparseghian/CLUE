#ifndef CLUE_CLUE_WIDGET_H
#define CLUE_CLUE_WIDGET_H

#include "widget.h"
#include "style.h"
#include "signal.h"
#include "event.h"

/* Widget type identifiers */
typedef enum {
    CLUE_WIDGET_GENERIC = 0,
    CLUE_WIDGET_LABEL,
    CLUE_WIDGET_BUTTON,
    CLUE_WIDGET_BOX,
    CLUE_WIDGET_TEXT_INPUT,
    CLUE_WIDGET_CHECKBOX,
    CLUE_WIDGET_RADIO,
    CLUE_WIDGET_SLIDER,
    CLUE_WIDGET_DROPDOWN,
    CLUE_WIDGET_LISTVIEW,
    CLUE_WIDGET_SCROLL,
    CLUE_WIDGET_IMAGE,
    CLUE_WIDGET_GRID,
    CLUE_WIDGET_TABS,
    CLUE_WIDGET_DIALOG,
    CLUE_WIDGET_PROGRESS,
    CLUE_WIDGET_MENU,
    CLUE_WIDGET_MENUBAR,
    CLUE_WIDGET_TREEVIEW,
    CLUE_WIDGET_TABLE,
    CLUE_WIDGET_TOGGLE,
    CLUE_WIDGET_SPINBOX,
    CLUE_WIDGET_STATUSBAR,
    CLUE_WIDGET_TOOLBAR,
    CLUE_WIDGET_CANVAS,
    CLUE_WIDGET_SPLITTER,
    CLUE_WIDGET_TEXT_EDITOR,
} ClueWidgetType;

/* Forward declaration */
struct ClueWidget;

/* Virtual function table -- each widget type provides its own */
typedef struct {
    void (*draw)(struct ClueWidget *widget);
    void (*layout)(struct ClueWidget *widget);
    int  (*handle_event)(struct ClueWidget *widget, UIEvent *event);
    void (*destroy)(struct ClueWidget *widget);
} ClueWidgetVTable;

/* High-level widget base. Embeds UIWidget as first member for C inheritance. */
typedef struct ClueWidget {
    UIWidget                base;       /* MUST be first */
    const ClueWidgetVTable *vtable;
    ClueWidgetType          type_id;
    ClueStyle               style;
    ClueSignalSlot         *signals;
} ClueWidget;

/* Initialise a ClueWidget with defaults and a vtable */
void clue_cwidget_init(ClueWidget *w, const ClueWidgetVTable *vt);

/* Recursively destroy a widget and all its children */
void clue_cwidget_destroy(ClueWidget *w);

/* Add a child widget to a container */
void clue_container_add(ClueWidget *parent, ClueWidget *child);

/* Remove a child widget from a container */
void clue_container_remove(ClueWidget *parent, ClueWidget *child);

/* Recursively draw a widget tree */
void clue_cwidget_draw_tree(ClueWidget *w);

/* Recursively layout a widget tree */
void clue_cwidget_layout_tree(ClueWidget *w);

#endif /* CLUE_CLUE_WIDGET_H */
