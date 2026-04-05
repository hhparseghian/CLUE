#ifndef CLUE_OVERLAY_H
#define CLUE_OVERLAY_H

#include "clue_widget.h"
#include "button.h"
#include "signal.h"

/*
 * Embedded modal overlay -- renders inside the main window.
 * Shows a dimmed background with a centered panel containing
 * a title, content widget, and action buttons.
 * Works on headless/KMSDRM where multiple OS windows aren't available.
 *
 * Usage:
 *   ClueOverlay *ov = clue_overlay_new("Pick an item", 300, 400);
 *   clue_overlay_set_content(ov, my_listview);
 *   clue_overlay_add_button(ov, "OK", CLUE_OVERLAY_OK);
 *   clue_overlay_add_button(ov, "Cancel", CLUE_OVERLAY_CANCEL);
 *   clue_overlay_set_callback(ov, my_callback, my_data);
 *   clue_overlay_show(ov);   // becomes modal, drawn on top of everything
 *   // ... callback fires when a button is pressed or overlay dismissed
 */

#define CLUE_OVERLAY_MAX_BUTTONS 4

typedef enum {
    CLUE_OVERLAY_NONE   = -1,
    CLUE_OVERLAY_OK     = 0,
    CLUE_OVERLAY_CANCEL = 1,
} ClueOverlayResult;

typedef void (*ClueOverlayCallback)(ClueOverlayResult result, void *user_data);

typedef struct {
    ClueWidget           base;      /* MUST be first -- covers full window */
    char                *title;
    ClueWidget          *content;
    ClueButton          *buttons[CLUE_OVERLAY_MAX_BUTTONS];
    ClueOverlayResult    btn_results[CLUE_OVERLAY_MAX_BUTTONS];
    int                  button_count;
    int                  panel_w;   /* desired panel width */
    int                  panel_h;   /* desired panel height */
    ClueOverlayResult    result;
    ClueOverlayCallback  callback;
    void                *callback_data;
    bool                 visible;
} ClueOverlay;

/* Create an overlay with a title and panel size */
ClueOverlay *clue_overlay_new(const char *title, int panel_w, int panel_h);

/* Destroy the overlay */
void clue_overlay_destroy(ClueOverlay *ov);

/* Set body content widget */
void clue_overlay_set_content(ClueOverlay *ov, ClueWidget *content);

/* Add a button */
void clue_overlay_add_button(ClueOverlay *ov, const char *label,
                             ClueOverlayResult result);

/* Set callback for when overlay closes */
void clue_overlay_set_callback(ClueOverlay *ov, ClueOverlayCallback cb,
                               void *user_data);

/* Show the overlay (makes it modal, drawn on top of the app) */
void clue_overlay_show(ClueOverlay *ov);

/* Dismiss the overlay */
void clue_overlay_dismiss(ClueOverlay *ov, ClueOverlayResult result);

#endif /* CLUE_OVERLAY_H */
