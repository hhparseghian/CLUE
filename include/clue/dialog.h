#ifndef CLUE_DIALOG_H
#define CLUE_DIALOG_H

#include <stdbool.h>
#include "renderer.h"
#include "font.h"
#include "signal.h"

/* Forward declarations */
struct UIWindow;

#include "clue_widget.h"
#include "button.h"

#define CLUE_DIALOG_MAX_BUTTONS 4

/* Dialog flags */
typedef enum {
    CLUE_DIALOG_FLAG_MODAL     = 1 << 0,  /* blocks input to parent window */
    CLUE_DIALOG_FLAG_ON_TOP    = 1 << 1,  /* stays above the parent window */
} ClueDialogFlags;

/* Dialog result */
typedef enum {
    CLUE_DIALOG_NONE = -1,
    CLUE_DIALOG_OK = 0,
    CLUE_DIALOG_CANCEL,
    CLUE_DIALOG_YES,
    CLUE_DIALOG_NO,
} ClueDialogResult;

/* Dialog callback */
typedef void (*ClueDialogCallback)(ClueDialogResult result, void *user_data);

/* Dialog -- opens as a separate OS window */
typedef struct ClueDialog {
    char                   *title;
    ClueWidget             *content;
    ClueButton             *buttons[CLUE_DIALOG_MAX_BUTTONS];
    ClueDialogResult        btn_results[CLUE_DIALOG_MAX_BUTTONS];
    int                     button_count;
    int                     dialog_w;
    int                     dialog_h;
    ClueDialogResult        result;
    ClueDialogCallback      callback;
    void                   *callback_data;
    bool                    running;    /* nested event loop flag */
    int                     flags;     /* ClueDialogFlags bitmask */
    int                     pos_x;     /* window position (-1 = center on parent) */
    int                     pos_y;
} ClueDialog;

/* Create a dialog with a title and size */
ClueDialog *clue_dialog_new(const char *title, int w, int h);

/* Destroy a dialog */
void clue_dialog_destroy(ClueDialog *dlg);

/* Set body content widget */
void clue_dialog_set_content(ClueDialog *dlg, ClueWidget *content);

/* Add a button with a result code */
void clue_dialog_add_button(ClueDialog *dlg, const char *label,
                            ClueDialogResult result);

/* Set dialog flags (combine with |, e.g. CLUE_DIALOG_FLAG_MODAL | CLUE_DIALOG_FLAG_ON_TOP) */
void clue_dialog_set_flags(ClueDialog *dlg, int flags);

/* Set dialog position (-1, -1 = center on parent, which is the default) */
void clue_dialog_set_position(ClueDialog *dlg, int x, int y);

/* Set callback for when dialog closes */
void clue_dialog_set_callback(ClueDialog *dlg, ClueDialogCallback cb,
                              void *user_data);

/* Show the dialog as a modal window (blocks until closed).
 * Returns the result code of the button that was pressed. */
ClueDialogResult clue_dialog_run(ClueDialog *dlg);

/* Show non-blocking -- result delivered via callback */
void clue_dialog_show(ClueDialog *dlg);

/* Quick message box (blocking) */
ClueDialogResult clue_dialog_message(const char *title, const char *message);

/* Quick confirm box (blocking) */
ClueDialogResult clue_dialog_confirm(const char *title, const char *message);

/* Non-blocking variants with callback */
void clue_dialog_message_async(const char *title, const char *message,
                               ClueDialogCallback cb, void *user_data);
void clue_dialog_confirm_async(const char *title, const char *message,
                               ClueDialogCallback cb, void *user_data);

#endif /* CLUE_DIALOG_H */
