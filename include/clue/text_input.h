#ifndef CLUE_TEXT_INPUT_H
#define CLUE_TEXT_INPUT_H

#include "clue_widget.h"

#define CLUE_TEXT_INPUT_MAX 256

/* Text input widget -- editable single-line text field */
typedef struct {
    ClueWidget  base;                       /* MUST be first */
    char        text[CLUE_TEXT_INPUT_MAX];
    int         cursor;                     /* cursor position (char index) */
    int         sel_start;                  /* selection anchor (-1 = no sel) */
    int         sel_end;                    /* selection end (== cursor when selecting) */
    int         scroll_offset;              /* horizontal scroll in pixels */
    char        placeholder[128];
    bool        cursor_visible;             /* blink state */
    int         blink_timer_id;
    bool        mouse_selecting;            /* drag-selecting */
} ClueTextInput;

/* Create a new text input with optional placeholder text */
ClueTextInput *clue_text_input_new(const char *placeholder);

/* Get the current text */
const char *clue_text_input_get_text(ClueTextInput *input);

/* Set the text programmatically */
void clue_text_input_set_text(ClueTextInput *input, const char *text);

#endif /* CLUE_TEXT_INPUT_H */
