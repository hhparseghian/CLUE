#ifndef CLUE_H
#define CLUE_H

/*
 * CLUE -- Cross-platform Lightweight UI Engine
 *
 * Main public header. Include this to get the full API.
 */

#include "event.h"
#include "backend.h"
#include "renderer.h"
#include "font.h"
#include "style.h"
#include "signal.h"
#include "widget.h"
#include "clue_widget.h"
#include "window.h"
#include "draw.h"
#include "label.h"
#include "button.h"
#include "box.h"
#include "text_input.h"
#include "scroll.h"
#include "image.h"
#include "checkbox.h"
#include "radio.h"
#include "separator.h"
#include "slider.h"
#include "dropdown.h"
#include "theme.h"
#include "grid.h"
#include "tabs.h"
#include "dialog.h"
#include "listview.h"
#include "progress.h"
#include "tooltip.h"
#include "menu.h"
#include "treeview.h"
#include "table.h"
#include "timer.h"
#include "file_dialog.h"
#include "toggle.h"
#include "spinbox.h"
#include "statusbar.h"
#include "toolbar.h"
#include "canvas.h"
#include "splitter.h"
#include "text_editor.h"
#include "clipboard.h"
#include "app.h"

/* Initialise CLUE: selects a backend and sets up the window manager.
 * Returns 0 on success, -1 on failure. */
int clue_init(void);

/* Shut down CLUE: destroys all windows and releases backend resources */
void clue_shutdown(void);

/* Poll for pending events. Fills the events array up to max entries.
 * Returns the number of events written. */
int clue_poll_events(UIEvent *events, int max);

/* Get the active renderer */
UIRenderer *clue_get_renderer(void);

/* ------------------------------------------------------------------ */
/* Type-safe auto-cast macro                                           */
/*                                                                     */
/* CLUE_W(x) converts any known widget pointer to ClueWidget * at      */
/* compile time.  Passing an unrecognised type is a compile error.      */
/* ------------------------------------------------------------------ */

#define CLUE_W(x) _Generic((x),                  \
    ClueWidget    *: (ClueWidget *)(x),           \
    ClueBox       *: (ClueWidget *)(x),           \
    ClueLabel     *: (ClueWidget *)(x),           \
    ClueButton    *: (ClueWidget *)(x),           \
    ClueTextInput *: (ClueWidget *)(x),           \
    ClueCheckbox  *: (ClueWidget *)(x),           \
    ClueRadio     *: (ClueWidget *)(x),           \
    ClueSlider    *: (ClueWidget *)(x),           \
    ClueDropdown  *: (ClueWidget *)(x),           \
    ClueListView  *: (ClueWidget *)(x),           \
    ClueTreeView  *: (ClueWidget *)(x),           \
    ClueTable     *: (ClueWidget *)(x),           \
    ClueProgress  *: (ClueWidget *)(x),           \
    ClueScroll    *: (ClueWidget *)(x),           \
    ClueImage     *: (ClueWidget *)(x),           \
    ClueGrid      *: (ClueWidget *)(x),           \
    ClueTabs      *: (ClueWidget *)(x),           \
    ClueSeparator *: (ClueWidget *)(x),           \
    ClueMenu      *: (ClueWidget *)(x),           \
    ClueMenuBar   *: (ClueWidget *)(x),           \
    ClueToggle    *: (ClueWidget *)(x),           \
    ClueSpinbox   *: (ClueWidget *)(x),           \
    ClueStatusbar *: (ClueWidget *)(x),           \
    ClueToolbar   *: (ClueWidget *)(x),           \
    ClueCanvas    *: (ClueWidget *)(x),           \
    ClueSplitter  *: (ClueWidget *)(x),           \
    ClueTextEditor *: (ClueWidget *)(x)           \
)

/* Wrapper macros -- shadow the real functions so call sites need no casts.
 * Internal implementation files define CLUE_IMPL before including this
 * header to suppress the macros and use the real function signatures. */
#ifndef CLUE_IMPL
#define clue_container_add(p, c)       (clue_container_add)(CLUE_W(p), CLUE_W(c))
#define clue_container_remove(p, c)    (clue_container_remove)(CLUE_W(p), CLUE_W(c))
#define clue_app_set_root(app, r)      (clue_app_set_root)(app, CLUE_W(r))
#define clue_tabs_add(t, l, c)         (clue_tabs_add)(t, l, CLUE_W(c))
#define clue_dialog_set_content(d, c)  (clue_dialog_set_content)(d, CLUE_W(c))
#define clue_tooltip_set(w, t)         (clue_tooltip_set)(CLUE_W(w), t)
#define clue_tooltip_get(w)            (clue_tooltip_get)(CLUE_W(w))
#define clue_cwidget_destroy(w)        (clue_cwidget_destroy)(CLUE_W(w))
#define clue_cwidget_draw_tree(w)      (clue_cwidget_draw_tree)(CLUE_W(w))
#define clue_cwidget_layout_tree(w)    (clue_cwidget_layout_tree)(CLUE_W(w))
#endif

#endif /* CLUE_H */
