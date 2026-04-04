#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "clue/signal.h"
#include "clue/clue_widget.h"

void clue_signal_connect(void *widget, const char *signal_name,
                         ClueSignalCallback callback, void *user_data)
{
    if (!widget || !signal_name || !callback) return;

    ClueWidget *w = (ClueWidget *)widget;

    ClueSignalSlot *slot = malloc(sizeof(ClueSignalSlot));
    if (!slot) return;

    slot->name      = strdup(signal_name);
    slot->callback  = callback;
    slot->user_data = user_data;
    slot->next      = w->signals;
    w->signals      = slot;
}

void clue_signal_emit(void *widget, const char *signal_name)
{
    if (!widget || !signal_name) return;

    ClueWidget *w = (ClueWidget *)widget;
    ClueSignalSlot *slot = w->signals;

    while (slot) {
        if (strcmp(slot->name, signal_name) == 0) {
            slot->callback(widget, slot->user_data);
        }
        slot = slot->next;
    }
}

void clue_signal_disconnect_all(void *widget)
{
    if (!widget) return;

    ClueWidget *w = (ClueWidget *)widget;
    ClueSignalSlot *slot = w->signals;

    while (slot) {
        ClueSignalSlot *next = slot->next;
        free(slot->name);
        free(slot);
        slot = next;
    }

    w->signals = NULL;
}
