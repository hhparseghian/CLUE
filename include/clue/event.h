#ifndef CLUE_EVENT_H
#define CLUE_EVENT_H

#include <stdint.h>

/* Forward declarations */
struct UIWindow;

/* Event types supported by CLUE */
typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_MOUSE_MOVE,
    UI_EVENT_MOUSE_BUTTON,
    UI_EVENT_MOUSE_SCROLL,
    UI_EVENT_KEY,
    UI_EVENT_RESIZE,
    UI_EVENT_CLOSE,
} UIEventType;

/* Unified event structure delivered to the application */
typedef struct {
    UIEventType     type;
    struct UIWindow *window;
    union {
        struct { int x, y; }                mouse_move;
        struct { int x, y, btn, pressed; }  mouse_button;
        struct { int x, y; float dx, dy; }  mouse_scroll;
        struct { int keycode, pressed;
                 char text[8]; }            key;
        struct { int w, h; }                resize;
    };
} UIEvent;

/* Dispatch an event to the appropriate window/widget tree.
 * Returns 1 if the event was consumed, 0 otherwise. */
int clue_event_dispatch(UIEvent *event);

#endif /* CLUE_EVENT_H */
