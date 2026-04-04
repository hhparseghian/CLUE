#ifndef CLUE_SIGNAL_H
#define CLUE_SIGNAL_H

/* Signal callback: receives the widget that emitted and user data */
typedef void (*ClueSignalCallback)(void *widget, void *user_data);

/* Opaque slot node (internal linked list) */
typedef struct ClueSignalSlot {
    char                   *name;
    ClueSignalCallback      callback;
    void                   *user_data;
    struct ClueSignalSlot  *next;
} ClueSignalSlot;

/* Connect a callback to a named signal on a widget */
void clue_signal_connect(void *widget, const char *signal_name,
                         ClueSignalCallback callback, void *user_data);

/* Emit a signal by name -- calls all connected callbacks */
void clue_signal_emit(void *widget, const char *signal_name);

/* Disconnect and free all signals for a widget */
void clue_signal_disconnect_all(void *widget);

#endif /* CLUE_SIGNAL_H */
