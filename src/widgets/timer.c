#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "clue/timer.h"

#define MAX_TIMERS 64

typedef struct {
    int                id;
    bool               active;
    bool               repeating;
    int                interval_ms;
    long long          fire_at_ms;   /* absolute time to fire */
    ClueTimerCallback  callback;
    void              *data;
} Timer;

static Timer g_timers[MAX_TIMERS];
static int   g_next_id = 1;

/* Get current time in milliseconds (monotonic clock) */
static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static Timer *find_free_slot(void)
{
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!g_timers[i].active) return &g_timers[i];
    }
    return NULL;
}

static int create_timer(int ms, bool repeating, ClueTimerCallback cb, void *data)
{
    if (!cb || ms <= 0) return 0;

    Timer *t = find_free_slot();
    if (!t) return 0;

    t->id          = g_next_id++;
    t->active      = true;
    t->repeating   = repeating;
    t->interval_ms = ms;
    t->fire_at_ms  = now_ms() + ms;
    t->callback    = cb;
    t->data        = data;

    return t->id;
}

int clue_timer_once(int delay_ms, ClueTimerCallback cb, void *data)
{
    return create_timer(delay_ms, false, cb, data);
}

int clue_timer_repeat(int interval_ms, ClueTimerCallback cb, void *data)
{
    return create_timer(interval_ms, true, cb, data);
}

void clue_timer_cancel(int timer_id)
{
    if (timer_id <= 0) return;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (g_timers[i].active && g_timers[i].id == timer_id) {
            g_timers[i].active = false;
            return;
        }
    }
}

void clue_timer_cancel_all(void)
{
    for (int i = 0; i < MAX_TIMERS; i++) {
        g_timers[i].active = false;
    }
}

int clue_timer_tick(void)
{
    long long current = now_ms();
    int fired = 0;

    for (int i = 0; i < MAX_TIMERS; i++) {
        Timer *t = &g_timers[i];
        if (!t->active) continue;
        if (current < t->fire_at_ms) continue;

        /* Fire the callback */
        bool keep = t->callback(t->data);
        fired++;

        if (t->repeating && keep) {
            /* Schedule next firing relative to when it should have fired,
             * not current time, to avoid drift */
            t->fire_at_ms += t->interval_ms;
            /* If we fell behind, catch up to now */
            if (t->fire_at_ms < current) {
                t->fire_at_ms = current + t->interval_ms;
            }
        } else {
            t->active = false;
        }
    }

    return fired;
}
