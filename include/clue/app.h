#ifndef CLUE_APP_H
#define CLUE_APP_H

#include <stdbool.h>
#include "renderer.h"
#include "font.h"
#include "clue_widget.h"

/* Forward declaration */
struct ClueWindow;

/* Application context */
typedef struct ClueApp {
    struct ClueWindow *window;
    ClueRenderer      *renderer;
    ClueFont          *default_font;
    ClueWidget      *root;
    UIWidget        *focused_widget;  /* currently focused widget */
    UIWidget        *captured_widget; /* receives all mouse events while set */
    ClueWidget      *modal_widget;   /* when set, only this widget receives events */
    bool             running;
} ClueApp;

/* Create a new application with a window */
ClueApp *clue_app_new(const char *title, int w, int h);

/* Set the root widget */
void clue_app_set_root(ClueApp *app, ClueWidget *root);

/* Run the main loop (blocks until quit) */
void clue_app_run(ClueApp *app);

/* Signal the app to quit */
void clue_app_quit(ClueApp *app);

/* Destroy the app and all its resources */
void clue_app_destroy(ClueApp *app);

/* Get the global app instance (set by clue_app_new) */
ClueApp *clue_app_get(void);

/* Get the app's default font */
ClueFont *clue_app_default_font(void);

/* Set focus to a widget (clears focus on the previous one) */
void clue_focus_widget(UIWidget *widget);

/* Capture mouse events -- all mouse events go to this widget until released */
void clue_capture_mouse(UIWidget *widget);
void clue_release_mouse(void);

#endif /* CLUE_APP_H */
