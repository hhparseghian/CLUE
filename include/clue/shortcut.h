#ifndef CLUE_SHORTCUT_H
#define CLUE_SHORTCUT_H

#include "signal.h"

/* Register a global keyboard shortcut.
 * The shortcut string uses the format: "Ctrl+S", "Ctrl+Shift+Z", "F5", etc.
 * The callback fires when the key combo is pressed. */
void clue_shortcut_add(const char *shortcut, ClueSignalCallback callback,
                       void *user_data);

/* Remove all shortcuts with the given callback */
void clue_shortcut_remove(ClueSignalCallback callback);

/* Remove all registered shortcuts */
void clue_shortcut_clear(void);

/* Called internally by the app loop. Returns 1 if a shortcut fired. */
int clue_shortcut_dispatch(int keycode, int modifiers);

#endif /* CLUE_SHORTCUT_H */
