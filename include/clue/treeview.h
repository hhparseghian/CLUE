#ifndef CLUE_TREEVIEW_H
#define CLUE_TREEVIEW_H

#include "clue_widget.h"
#include "scrollbar.h"

#define CLUE_TREE_MAX_CHILDREN 64

/* Tree node */
typedef struct ClueTreeNode {
    char                   *label;
    struct ClueTreeNode    *children[CLUE_TREE_MAX_CHILDREN];
    int                     child_count;
    struct ClueTreeNode    *parent;
    bool                    expanded;
    int                     depth;
    void                   *user_data;
} ClueTreeNode;

/* Tree view widget */
typedef struct {
    ClueWidget      base;       /* MUST be first */
    ClueTreeNode   *root;       /* root node (not displayed) */
    ClueTreeNode   *selected;
    ClueTreeNode   *hovered;
    int             item_height;
    int             indent;     /* pixels per depth level */
    int             scroll_y;
    ClueScrollbar   sb;
} ClueTreeView;

/* Create a tree view */
ClueTreeView *clue_treeview_new(void);

/* Create a tree node */
ClueTreeNode *clue_tree_node_new(const char *label);

/* Add a child node. Returns the child. */
ClueTreeNode *clue_tree_node_add(ClueTreeNode *parent, ClueTreeNode *child);

/* Get/set selected node */
ClueTreeNode *clue_treeview_get_selected(ClueTreeView *tv);

/* Signals: "selected" -- emitted when selection changes */

#endif /* CLUE_TREEVIEW_H */
