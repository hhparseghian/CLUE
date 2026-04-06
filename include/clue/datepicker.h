#ifndef CLUE_DATEPICKER_H
#define CLUE_DATEPICKER_H

#include <stdbool.h>

/* Date/time result */
typedef struct {
    int  year, month, day;  /* month 1-12, day 1-31 */
    int  hour, minute;      /* 24h format */
    bool has_time;          /* true if time was also picked */
} ClueDateTimeResult;

/* Callback fired when the picker closes.
 * ok is true if the user confirmed, false if cancelled. */
typedef void (*ClueDateTimeCallback)(const ClueDateTimeResult *result,
                                      bool ok, void *user_data);

/* Show a date picker overlay (calendar grid). */
void clue_datepicker_show(ClueDateTimeCallback callback, void *user_data);

/* Show a date + time picker overlay (calendar grid + hour/minute). */
void clue_datetimepicker_show(ClueDateTimeCallback callback, void *user_data);

/* Show a time-only picker overlay (hour/minute). */
void clue_timepicker_show(ClueDateTimeCallback callback, void *user_data);

/* Set the initial date (call before show). */
void clue_datepicker_set_date(int year, int month, int day);

/* Set the initial time (call before show). */
void clue_datepicker_set_time(int hour, int minute);

#endif /* CLUE_DATEPICKER_H */
