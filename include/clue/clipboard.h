#ifndef CLUE_CLIPBOARD_H
#define CLUE_CLIPBOARD_H

/* Set the clipboard text (copies the string) */
void clue_clipboard_set(const char *text);

/* Get the clipboard text (caller must free the returned string).
 * Returns NULL if clipboard is empty or unavailable. */
char *clue_clipboard_get(void);

#endif /* CLUE_CLIPBOARD_H */
