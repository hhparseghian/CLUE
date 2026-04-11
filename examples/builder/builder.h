#ifndef BUILDER_H
#define BUILDER_H

#include <stdbool.h>
#include "clue/clue.h"

#define MAX_NODES 256

typedef struct {
    int         id;
    const char *type_name;     /* "Button", "Label", etc. */
    char        var_name[64];  /* generated variable name */
    char        label[128];    /* label/text/placeholder */
    int         w, h;
    bool        hexpand, vexpand;
    int         parent_id;     /* id of parent node, -1 = root canvas */
    ClueWidget *widget;        /* live widget in canvas */
} BuilderNode;

typedef struct {
    BuilderNode     nodes[MAX_NODES];
    int             count;
    int             selected;       /* index of selected node, -1 = none */
    int             next_id;        /* auto-increment for var names */
    ClueBox        *canvas_root;    /* root box in canvas */
    ClueTextEditor *code_view;      /* code output */
} BuilderState;

extern BuilderState g_state;

/* Palette */
ClueScroll *builder_palette_create(void);

/* Canvas */
ClueScroll *builder_canvas_create(void);
void        builder_canvas_add_widget(const char *type_name);
void        builder_canvas_delete_selected(void);
void        builder_canvas_select(int index);
void        builder_canvas_handle_click(int mx, int my, int btn);
void        builder_canvas_draw_selection(void);
ClueMenu   *builder_canvas_get_context_menu(void);

/* Properties */
ClueScroll *builder_properties_create(void);
void        builder_properties_refresh(void);

/* Tree view — widget hierarchy */
ClueTreeView *builder_tree_create(void);
void          builder_tree_refresh(void);

/* Code generation */
void        builder_codegen_update(void);

#endif /* BUILDER_H */
