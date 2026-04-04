#ifndef CLUE_TEXT_EDITOR_H
#define CLUE_TEXT_EDITOR_H

#include "clue_widget.h"

#define CLUE_TEXT_EDITOR_MAX 8192
#define CLUE_UNDO_MAX 64

/* Undo snapshot */
typedef struct {
    char *text;
    int   text_len;
    int   cursor;
} ClueUndoEntry;

/* Multi-line text editor */
typedef struct {
    ClueWidget  base;       /* MUST be first */
    char       *text;       /* heap-allocated text buffer */
    int         text_cap;   /* capacity of text buffer */
    int         text_len;   /* current length */
    int         cursor;     /* cursor position (byte index) */
    int         sel_start;  /* selection anchor (-1 = no sel) */
    int         sel_end;    /* selection end */
    int         scroll_y;   /* vertical scroll offset in pixels */
    int         scroll_x;   /* horizontal scroll offset in pixels */
    bool        cursor_visible;
    int         blink_timer_id;
    bool        mouse_selecting;
    long long   last_click_ms;
    /* Undo/redo */
    ClueUndoEntry undo_stack[CLUE_UNDO_MAX];
    int         undo_count;
    int         undo_pos;    /* current position in undo stack */
    bool        word_wrap;
    bool        line_numbers;
} ClueTextEditor;

/* Create a new text editor */
ClueTextEditor *clue_text_editor_new(void);

/* Get/set text */
const char *clue_text_editor_get_text(ClueTextEditor *ed);
void clue_text_editor_set_text(ClueTextEditor *ed, const char *text);

/* Enable/disable word wrap (default: off) */
void clue_text_editor_set_word_wrap(ClueTextEditor *ed, bool wrap);

/* Enable/disable line numbers (default: off) */
void clue_text_editor_set_line_numbers(ClueTextEditor *ed, bool show);

/* Signals: "changed" -- emitted when text changes */

#endif /* CLUE_TEXT_EDITOR_H */
