#ifndef CLUE_TIMER_H
#define CLUE_TIMER_H

#include <stdbool.h>

/* Timer callback. Return true to keep a repeating timer alive, false to stop. */
typedef bool (*ClueTimerCallback)(void *data);

/* Create a one-shot timer that fires once after delay_ms.
 * Returns a timer ID (> 0), or 0 on failure. */
int clue_timer_once(int delay_ms, ClueTimerCallback cb, void *data);

/* Create a repeating timer that fires every interval_ms.
 * Returns a timer ID (> 0), or 0 on failure. */
int clue_timer_repeat(int interval_ms, ClueTimerCallback cb, void *data);

/* Cancel a timer by ID */
void clue_timer_cancel(int timer_id);

/* Cancel all timers */
void clue_timer_cancel_all(void);

/* Process pending timers. Called internally by the app loop each frame.
 * Returns number of timers that fired. */
int clue_timer_tick(void);

#endif /* CLUE_TIMER_H */
