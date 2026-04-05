#ifndef DEMO_H
#define DEMO_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "clue/clue.h"

/* Shared globals */
extern ClueLabel *g_status;

/* Shared data */
extern const char *fruits[];
extern const char *fruit_item(int index, void *user_data);

/* Page builders */
ClueScroll *build_widgets_page(ClueApp *app);
ClueBox *build_list_page(void);
ClueBox *build_grid_page(void);
ClueBox *build_tree_page(void);
ClueBox *build_table_page(void);
ClueBox *build_editor_page(void);
ClueBox *build_canvas_page(void);
ClueBox *build_3d_page(void);
ClueBox *build_splitter_page(void);

#endif /* DEMO_H */
