#define _POSIX_C_SOURCE 200809L
#include "stdlib_calendar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── calendar Module ──
 * Calendar/date operations: day-of-week, days-in-month, leap year checks,
 * week numbers, and formatted calendar output.
 * Pure computation — no allocations beyond result strings/arrays.
 * Works on all platforms including 32-bit single-core.
 */

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ── Zeller's congruence for day of week ──
 * Returns 0=Sunday, 1=Monday, ..., 6=Saturday
 */
static int day_of_week(int year, int month, int day) {
    if (month < 3) { month += 12; year--; }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    /* Zeller returns 0=Saturday, adjust to 0=Sunday */
    return (h + 6) % 7;
}

/* ── Days in each month (non-leap) ── */
static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int month_days(int year, int month) {
    if (month < 1 || month > 12) return 0;
    if (month == 2 && is_leap(year)) return 29;
    return days_in_month[month];
}

/* ── calendar.day_of_week(year, month, day) → int (0=Sun..6=Sat) ── */
Value native_calendar_day_of_week(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_INT || args[1].type != VAL_INT || args[2].type != VAL_INT)
        return make_null();
    int y = (int)args[0].int_val;
    int m = (int)args[1].int_val;
    int d = (int)args[2].int_val;
    if (m < 1 || m > 12 || d < 1 || d > month_days(y, m)) return make_null();
    return make_int(day_of_week(y, m, d));
}

/* ── calendar.days_in_month(year, month) → int ── */
Value native_calendar_days_in_month(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT)
        return make_null();
    return make_int(month_days((int)args[0].int_val, (int)args[1].int_val));
}

/* ── calendar.is_leap_year(year) → bool ── */
Value native_calendar_is_leap_year(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    return make_bool(is_leap((int)args[0].int_val));
}

/* ── calendar.day_of_year(year, month, day) → int (1-366) ── */
Value native_calendar_day_of_year(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_INT || args[1].type != VAL_INT || args[2].type != VAL_INT)
        return make_null();
    int y = (int)args[0].int_val;
    int m = (int)args[1].int_val;
    int d = (int)args[2].int_val;
    int doy = d;
    for (int i = 1; i < m; i++) doy += month_days(y, i);
    return make_int(doy);
}

/* ── calendar.week_number(year, month, day) → int (ISO 8601) ── */
Value native_calendar_week_number(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_INT || args[1].type != VAL_INT || args[2].type != VAL_INT)
        return make_null();
    int y = (int)args[0].int_val;
    int m = (int)args[1].int_val;
    int d = (int)args[2].int_val;

    /* Calculate day of year */
    int doy = d;
    for (int i = 1; i < m; i++) doy += month_days(y, i);

    /* Get Jan 1 day of week (Monday=0 for ISO) */
    int jan1 = (day_of_week(y, 1, 1) + 6) % 7; /* convert Sun-based to Mon-based */

    /* ISO week number */
    int week = (doy + jan1 - 1) / 7;
    /* Adjust for year boundary */
    if (jan1 > 4) { /* Jan 1 is in last week of previous year */
        if (week == 0) {
            /* Count from last week of previous year */
            int prev_doy = is_leap(y - 1) ? 366 : 365;
            int prev_jan1 = (day_of_week(y - 1, 1, 1) + 6) % 7;
            week = (prev_doy + prev_jan1 - 1) / 7;
        }
    }
    if (week == 0) week = 1;
    /* Week 53 check */
    if (week >= 53) {
        int days_left = (is_leap(y) ? 366 : 365) - doy;
        int dec31_dow = (day_of_week(y, 12, 31) + 6) % 7;
        if (dec31_dow < 3 && days_left <= (3 - dec31_dow - 1)) {
            week = 1;
        }
    }
    return make_int(week);
}

/* ── calendar.month_name(month) → string ── */
Value native_calendar_month_name(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    static const char* names[] = {
        "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    int m = (int)args[0].int_val;
    if (m < 1 || m > 12) return make_null();
    return make_string(names[m]);
}

/* ── calendar.day_name(day_index) → string (0=Sun..6=Sat) ── */
Value native_calendar_day_name(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    static const char* names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    int d = (int)args[0].int_val;
    if (d < 0 || d > 6) return make_null();
    return make_string(names[d]);
}

/* ── calendar.month_days_array(year, month) → array of ints (1..N) ── */
Value native_calendar_month_days_array(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT)
        return make_null();
    int y = (int)args[0].int_val;
    int m = (int)args[1].int_val;
    int days = month_days(y, m);

    Array* arr = array_create((size_t)days);
    for (int i = 1; i <= days; i++) {
        array_push(arr, make_int(i));
    }
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── calendar.month_calendar(year, month) → array of weeks (each week = array of 7 ints, 0=empty) ── */
Value native_calendar_month_calendar(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_INT || args[1].type != VAL_INT)
        return make_null();
    int y = (int)args[0].int_val;
    int m = (int)args[1].int_val;
    if (m < 1 || m > 12) return make_null();

    int days = month_days(y, m);
    int first_dow = day_of_week(y, m, 1); /* 0=Sun */

    /* Build array of weeks */
    Array* weeks = array_create(6);
    int day = 1;
    for (int week = 0; week < 6 && day <= days; week++) {
        Array* w = array_create(7);
        for (int d = 0; d < 7; d++) {
            if (week == 0 && d < first_dow) {
                array_push(w, make_int(0));
            } else if (day > days) {
                array_push(w, make_int(0));
            } else {
                array_push(w, make_int(day));
                day++;
            }
        }
        Value wv; wv.type = VAL_ARRAY; wv.array_val = w;
        array_push(weeks, wv);
    }
    Value v; v.type = VAL_ARRAY; v.array_val = weeks; return v;
}

/* ── calendar.today() → array [year, month, day] ── */
Value native_calendar_today(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    Array* arr = array_create(3);
    array_push(arr, make_int(1900 + tm->tm_year));
    array_push(arr, make_int(1 + tm->tm_mon));
    array_push(arr, make_int(tm->tm_mday));
    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── calendar.days_in_year(year) → int ── */
Value native_calendar_days_in_year(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    return make_int(is_leap((int)args[0].int_val) ? 366 : 365);
}

void stdlib_calendar_register(void) {
    register_native("calendar.day_of_week", native_calendar_day_of_week);
    register_native("calendar.days_in_month", native_calendar_days_in_month);
    register_native("calendar.is_leap_year", native_calendar_is_leap_year);
    register_native("calendar.day_of_year", native_calendar_day_of_year);
    register_native("calendar.week_number", native_calendar_week_number);
    register_native("calendar.month_name", native_calendar_month_name);
    register_native("calendar.day_name", native_calendar_day_name);
    register_native("calendar.month_days_array", native_calendar_month_days_array);
    register_native("calendar.month_calendar", native_calendar_month_calendar);
    register_native("calendar.today", native_calendar_today);
    register_native("calendar.days_in_year", native_calendar_days_in_year);
}
