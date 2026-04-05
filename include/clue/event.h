#ifndef CLUE_EVENT_H
#define CLUE_EVENT_H

#include <stdint.h>

/* Forward declarations */
struct ClueWindow;

/* Event types supported by CLUE */
typedef enum {
    CLUE_EVENT_NONE = 0,
    CLUE_EVENT_MOUSE_MOVE,
    CLUE_EVENT_MOUSE_BUTTON,
    CLUE_EVENT_MOUSE_SCROLL,
    CLUE_EVENT_KEY,
    CLUE_EVENT_RESIZE,
    CLUE_EVENT_CLOSE,
} ClueEventType;

/* Modifier key flags */
typedef enum {
    CLUE_MOD_SHIFT = 1 << 0,
    CLUE_MOD_CTRL  = 1 << 1,
    CLUE_MOD_ALT   = 1 << 2,
    CLUE_MOD_SUPER = 1 << 3,
} UIModifiers;

/* Unified event structure delivered to the application */
typedef struct {
    ClueEventType     type;
    struct ClueWindow *window;
    union {
        struct { int x, y; }                mouse_move;
        struct { int x, y, btn, pressed; }  mouse_button;
        struct { int x, y; float dx, dy; }  mouse_scroll;
        struct { int keycode, pressed;
                 int modifiers;
                 char text[8]; }            key;
        struct { int w, h; }                resize;
    };
} ClueEvent;

/* Dispatch an event to the appropriate window/widget tree.
 * Returns 1 if the event was consumed, 0 otherwise. */
int clue_event_dispatch(ClueEvent *event);

#endif /* CLUE_EVENT_H */
