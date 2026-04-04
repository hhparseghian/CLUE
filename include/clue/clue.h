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

#endif /* CLUE_H */
