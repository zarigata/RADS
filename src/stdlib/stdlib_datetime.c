// RADS datetime module -- Python-parity strptime/strftime engine.
//
// History: parse() used to be three hardcoded strstr+sscanf hacks with
// unchecked returns and no validation; format() fed a converted string to
// C strftime (locale-dependent) after a fixed 512-byte translation buffer.
// This file replaces both with one directive walker cloned from CPython
// semantics (cross-checked against python3.13 _strptime.py live on this
// box, 2026-10-19) and exact civil-time math -- no mktime, no locale.
//
// Legacy "YYYY/MM/DD HH:mm:ss" format tokens are translated to %-directives
// first, so old datetime.format/parse calls keep working.
//
// Zero new deps: C11 + time.h + regex.h only. VW Fusca law holds.
// ⚕ Forged for Master Zarigata 🜂

#include "stdlib_datetime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// ---------------------------------------------------------------------------
// Locale-independent name tables (CPython uses the C-locale calendar module;
// matching is case-insensitive like _strptime's re.IGNORECASE).
// ---------------------------------------------------------------------------

static const char* const DAY_FULL[7] = {
    "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};
static const char* const DAY_ABBR[7] = {
    "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char* const MONTH_FULL[12] = {
    "January","February","March","April","May","June","July",
    "August","September","October","November","December"
};
static const char* const MONTH_ABBR[12] = {
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};
static const char* const AM_PM[2] = { "AM", "PM" };

// Shared struct definition so datetime structs print safely
// ("<struct instance DateTime>") instead of dereferencing the garbage
// malloc'd definition pointer the old code never set. Allocated once, never
// freed -- value_release() treats definitions as registry-owned, so a static
// singleton is the leak-free choice (same pattern as stdlib_regex.c).
static StructDef* datetime_def(void) {
    static StructDef def;
    static bool init = false;
    if (!init) {
        def.name = strdup("DateTime");
        def.ast_node = NULL;
        init = true;
    }
    return &def;
}

// ---------------------------------------------------------------------------
// Growable buffer -- same law as the regex.replace fix: never guess a size,
// grow on demand. Used for both the built ERE and the formatted output.
// ---------------------------------------------------------------------------

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} GrowBuf;

// Ensure room for `extra` more bytes (plus the NUL we always keep).
static bool gb_reserve(GrowBuf* b, size_t extra) {
    if (b->len + extra + 1 <= b->cap) return true;
    size_t want = b->cap ? b->cap : 64;
    while (want < b->len + extra + 1) want *= 2;
    char* p = realloc(b->data, want);
    if (!p) return false;
    b->data = p;
    b->cap = want;
    return true;
}

// Append n bytes from s (n may be 0; s may be NULL then).
static bool gb_append(GrowBuf* b, const char* s, size_t n) {
    if (n && !s) return false;
    if (!gb_reserve(b, n)) return false;
    if (n) memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return true;
}

// Append a NUL-terminated string.
static bool gb_str(GrowBuf* b, const char* s) {
    return gb_append(b, s, strlen(s));
}

// Append a formatted chunk (bounded vsnprintf into a 64-byte scratch).
static bool gb_printf(GrowBuf* b, const char* fmt, ...) {
    char chunk[64];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(chunk, sizeof(chunk), fmt, ap);
    va_end(ap);
    if (n < 0) return false;
    if ((size_t)n >= sizeof(chunk)) n = (int)sizeof(chunk) - 1;
    return gb_append(b, chunk, (size_t)n);
}

// ---------------------------------------------------------------------------
// Civil-time math (Howard Hinnant's algorithms) -- exact, no mktime/locale.
// ---------------------------------------------------------------------------

// Days since 1970-01-01 from a civil date (proleptic Gregorian).
static long long days_from_civil(long long y, int m, int d) {
    y -= m <= 2;
    long long era = (y >= 0 ? y : y - 399) / 400;
    long long yoe = y - era * 400;                              // [0,399]
    long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;      // [0,146096]
    return era * 146097 + doe - 719468;
}

// Civil date from days since 1970-01-01.
static void civil_from_days(long long z, int* y, int* m, int* d) {
    z += 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    long long doe = z - era * 146097;                           // [0,146096]
    long long yr = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long doy = doe - (365 * yr + yr / 4 - yr / 100);
    long long mp = (5 * doy + 2) / 153;                         // [0,11]
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);                   // [1,31]
    *m = (int)(mp + (mp < 10 ? 3 : -9));                        // [1,12]
    *y = (int)(era * 400 + yr + (*m <= 2));
}

// Leap year predicate (proleptic Gregorian).
static bool is_leap(long long y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

// Cumulative days before each month in a non-leap year.
static const int CUM_DAYS[13] =
    { 0,31,59,90,120,151,181,212,243,273,304,334,365 };

// Day of year [1,366] from a civil date.
static int yday_of(long long y, int m, int d) {
    int jd = CUM_DAYS[m - 1] + d;
    if (m > 2 && is_leap(y)) jd += 1;
    return jd;
}

// Days in a month (28/29/30/31).
static int days_in_month(long long y, int m) {
    static const int MD[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && is_leap(y)) return 29;
    return MD[m - 1];
}

// Weekday [0=Sunday..6=Saturday] (1970-01-01 was a Thursday = 4).
static int weekday_from_days(long long days) {
    return (int)(((days % 7) + 11) % 7);
}

// %U (weeks start Sunday) or %W (weeks start Monday) week number.
static int week_number(int yday, int weekday, bool starts_monday) {
    int wday = starts_monday ? (weekday + 6) % 7 : weekday; // 0 = week start
    return (yday + 6 - wday) / 7;
}

// ISO-8601 week-date -> civil (CPython date.fromisocalendar equivalent).
static void civil_from_iso_week(long long iso_year, int week, int day,
                                int* y, int* m, int* d) {
    long long jan4 = days_from_civil(iso_year, 1, 4);
    int jan4_iso = weekday_from_days(jan4);            // 0=Sun -> ISO 1..7
    if (jan4_iso == 0) jan4_iso = 7;
    long long week1_mon = jan4 - (jan4_iso - 1);
    civil_from_days(week1_mon + (week - 1) * 7LL + (day - 1), y, m, d);
}

// ISO week-date fields of a civil date (for %G/%V/%u formatting).
static void iso_week_of(long long y, int m, int d,
                        int* iso_year, int* week, int* iso_wday) {
    long long days = days_from_civil(y, m, d);
    int wd = weekday_from_days(days);
    *iso_wday = wd == 0 ? 7 : wd;                      // 1=Mon..7=Sun
    long long thursday = days + (4 - *iso_wday);       // owns the ISO year
    int ty, tm, td;
    civil_from_days(thursday, &ty, &tm, &td);
    *iso_year = ty;
    long long jan4 = days_from_civil(ty, 1, 4);
    int jan4_iso = weekday_from_days(jan4);
    if (jan4_iso == 0) jan4_iso = 7;
    *week = (int)((thursday - (jan4 - (jan4_iso - 1))) / 7) + 1;
}

// Seconds since epoch for a civil datetime interpreted as-if-UTC. RADS
// structs are naive; this is the only deterministic epoch we can attach.
// Split a naive epoch back into civil fields.
static void civil_from_epoch_helper(long long t, int* y, int* mo, int* d,
                                    int* h, int* mi, int* s) {
    long long days = t / 86400; long long rem = t % 86400;
    if (rem < 0) { rem += 86400; days -= 1; }
    civil_from_days(days, y, mo, d);
    *h = (int)(rem / 3600); *mi = (int)((rem % 3600) / 60); *s = (int)(rem % 60);
}

static long long naive_epoch(long long y, int m, int d, int h, int mi, int s) {
    return days_from_civil(y, m, d) * 86400LL + h * 3600LL + mi * 60LL + s;
}

// ---------------------------------------------------------------------------
// strptime engine: format -> POSIX ERE, compiled once per format and cached.
// Group 0 is the whole match; each directive owns exactly one group, so
// group index maps 1:1 to directive order in the walk below.
// ---------------------------------------------------------------------------

// One cached compiled format (same law as the regex module's cache).
typedef struct {
    char* fmt;              // cache key: the format string as written
    char* ere;              // the POSIX ERE lowered from the format
    int* dirs;              // dirs[i] = directive char owning group i+1
    int dir_count;
    regex_t compiled;
    bool valid;
} CachedFormat;

#define MAX_FMT_CACHE 32
static CachedFormat fmt_cache[MAX_FMT_CACHE];
static int fmt_cache_count = 0;

// Escape one literal character for POSIX ERE (escape every special).
static bool ere_lit(GrowBuf* ere, char c) {
    static const char* special = "^$.[|()*+?{}\\";
    char buf[3] = { '\\', c, '\0' };
    if (c && strchr(special, c)) return gb_str(ere, buf);
    buf[0] = c;
    return gb_append(ere, buf, 1);
}

// Append the ERE for a name alternation (%A/%a/%B/%b/%p). POSIX ERE has no
// case-insensitive flag, so both cases are emitted as alternatives
// (e.g. "(Jan|jan|JAN)" style is unnecessary -- plain alternation of the
// canonical spellings plus a leading character-class covers CPython's
// re.IGNORECASE behavior; we emit canonical|lowercase pairs).
static bool ere_names(GrowBuf* ere, const char* const* names, int n) {
    if (!gb_str(ere, "(")) return false;
    for (int i = 0; i < n; i++) {
        if (i && !gb_str(ere, "|")) return false;
        if (!gb_str(ere, names[i])) return false;
        if (!gb_str(ere, "|")) return false;
        // lowercase variant
        for (const char* p = names[i]; *p; p++) {
            char lc = (char)tolower((unsigned char)*p);
            if (!ere_lit(ere, lc)) return false;
        }
    }
    if (!gb_str(ere, ")")) return false;
    return true;
}

// Append the ERE for one directive. Directives owning a capture group bump
// *ndirs; literal directives (%n, %t, %%) do not. Returns false on a bad
// directive (CPython: "'x' is a bad directive in format"). Groups are
// emitted in directive order, so group i belongs to dirs[i-1] at walk time.
static bool ere_directive(GrowBuf* ere, char dir, int* dirs, int* ndirs) {
    bool ok = false;
    switch (dir) {
        case 'Y': ok = gb_str(ere, "([0-9]{4})"); break;
        case 'y': ok = gb_str(ere, "([0-9]{2})"); break;
        case 'm': ok = gb_str(ere, "(1[0-2]|0[1-9]|[1-9])"); break;
        case 'd': ok = gb_str(ere, "(3[0-2]|[1-2][0-9]|0[1-9]|[1-9]| [1-9])"); break;
        case 'H': ok = gb_str(ere, "(2[0-3]|[0-1][0-9]|[0-9])"); break;
        case 'I': ok = gb_str(ere, "(1[0-2]|0[1-9]|[1-9]| [1-9])"); break;
        case 'M': ok = gb_str(ere, "([0-5][0-9]|[0-9])"); break;
        case 'S': ok = gb_str(ere, "(6[0-1]|[0-5][0-9]|[0-9])"); break;
        case 'j': ok = gb_str(ere, "(36[0-6]|3[0-5][0-9]|[1-2][0-9][0-9]|0[0-9][0-9]|00[1-9]|[1-9][0-9]|0[1-9]|[1-9])"); break;
        case 'w': ok = gb_str(ere, "([0-6])"); break;
        case 'u': ok = gb_str(ere, "([1-7])"); break;
        case 'U': ok = gb_str(ere, "(5[0-3]|[0-4][0-9]|[0-9])"); break;
        case 'W': ok = gb_str(ere, "(5[0-3]|[0-4][0-9]|[0-9])"); break;
        case 'G': ok = gb_str(ere, "([0-9]{4})"); break;
        case 'V': ok = gb_str(ere, "(5[0-3]|0[1-9]|[1-4][0-9]|[0-9])"); break;
        case 'f': ok = gb_str(ere, "([0-9]{1,6})"); break;
        case 'C': ok = gb_str(ere, "([0-9]{2})"); break;
        case 'p': ok = ere_names(ere, AM_PM, 2); break;
        case 'A': ok = ere_names(ere, DAY_FULL, 7); break;
        case 'a': ok = ere_names(ere, DAY_ABBR, 7); break;
        case 'B': ok = ere_names(ere, MONTH_FULL, 12); break;
        case 'b': ok = ere_names(ere, MONTH_ABBR, 12); break;
        case 'z': ok = gb_str(ere, "([+-][0-9]{2}:?[0-5][0-9](:?[0-5][0-9]([.][0-9]{1,6})?)?|Z)"); break;
        case 'Z': ok = gb_str(ere, "([A-Za-z]+)"); break;

        default: return false;
    }
    if (ok) dirs[(*ndirs)++] = dir;
    return ok;
}

// Expand a composite directive (%c/%x/%X) or a legacy token (YYYY etc.) into
// its %-directive spelling. Composite expansion is depth-1 (composites never
// nest). Legacy tokens are the pre-parity RADS dialect: YYYY MM DD HH mm ss.
// Returns the replacement string, or NULL if `dir` expands to nothing.
static const char* expand_token(const char* tok, int toklen) {
    if (toklen == 2 && tok[0] == '%') {
        switch (tok[1]) {
            case 'c': return "%a %b %e %H:%M:%S %Y";
            case 'x': return "%m/%d/%y";
            case 'X': return "%H:%M:%S";
            case 'D': return "%m/%d/%y";   // format-only in CPython; parse
            case 'F': return "%Y-%m-%d";   // rejects, we mirror format-only
            case 'T': return "%H:%M:%S";
            case 'R': return "%H:%M";
            case 'r': return "%I:%M:%S %p";
        }
        return NULL;
    }
    // Legacy RADS tokens -> directives (old format()/parse() dialect).
    if (toklen == 4 && strncmp(tok, "YYYY", 4) == 0) return "%Y";
    if (toklen == 2) {
        if (strncmp(tok, "MM", 2) == 0) return "%m";
        if (strncmp(tok, "DD", 2) == 0) return "%d";
        if (strncmp(tok, "HH", 2) == 0) return "%H";
        if (strncmp(tok, "mm", 2) == 0) return "%M";
        if (strncmp(tok, "ss", 2) == 0) return "%S";
    }
    return NULL;
}

// True when the 4-char token starting at fmt[i] is a legacy YYYY token.
static bool is_yyyy(const char* fmt, size_t i) {
    return fmt[i] == 'Y' && strncmp(&fmt[i], "YYYY", 4) == 0;
}

// Lower a format string to a POSIX ERE. Composites/legacy tokens are
// expanded first, then each %-directive becomes a capture group. `dirs` (an
// int array of capacity 64) receives the directive letters in group order;
// *ndirs receives the count. Returns a malloc'd ERE or NULL on bad
// directive / overflow (message already printed to stderr).
static char* format_to_ere(const char* fmt, int* dirs, int* ndirs, bool anchor) {
    GrowBuf ere = {0};
    *ndirs = 0;
    size_t flen = strlen(fmt);
    size_t i = 0;
    while (i < flen) {
        // Capacity guard: each directive adds 1 group; a composite can add
        // up to 8 in one step via recursion. Keep 8 slots of headroom.
        if (*ndirs + 8 > 250) {
            fprintf(stderr, "Error: datetime format has too many directives.\n");
            goto fail;
        }
        char c = fmt[i];
        // Legacy 4-char YYYY first (before the 2-char MM check below).
        if (is_yyyy(fmt, i)) {
            if (!gb_str(&ere, "([0-9]{4})")) goto fail;
            dirs[(*ndirs)++] = 'Y';
            i += 4;
            continue;
        }
        // Legacy 2-char tokens (MM/DD/HH/mm/ss) expand like YYYY above so
        // old format() calls ("DD/MM/YYYY") keep working.
        if (i + 2 <= flen) {
            const char* legacy = expand_token(&fmt[i], 2);
            if (legacy && legacy[0] == '%' && strchr("mdHMS", legacy[1])
                && fmt[i] != '%') {
                if (!ere_directive(&ere, legacy[1], dirs, ndirs)) goto fail;
                i += 2;
                continue;
            }
        }
        // %-directive (possibly composite -- expand and continue scanning
        // the expansion by advancing the walk into the expanded text).
        if (c == '%' && i + 1 < flen) {
            char dir = fmt[i + 1];
            const char* exp = expand_token(&fmt[i], 2);
            if (exp) {
                // Recurse one level: append the expansion's own lowering.
                                // Parse-side composites expand %e to %d: the %d ERE accepts
                // a space-padded day, so "%c" parsing stays CPython-legal.
                char expbuf[32];
                snprintf(expbuf, sizeof(expbuf), "%s", exp);
                for (char* q = expbuf; *q; q++) {
                    if (*q == 'e' && q > expbuf && *(q - 1) == '%') *q = 'd';
                }
                exp = expbuf;
                int sub_ndirs;
                char* sub = format_to_ere(exp, dirs + *ndirs, &sub_ndirs, false);
                if (!sub) goto fail;
                bool appended = gb_str(&ere, sub);
                free(sub);
                if (!appended) goto fail;
                *ndirs += sub_ndirs;
                i += 2;
                continue;
            }
            if (dir == '%') { if (!ere_lit(&ere, '%')) goto fail; i += 2; continue; }
            if (dir == 'n') { if (!ere_lit(&ere, '\n')) goto fail; i += 2; continue; }
            if (dir == 't') { if (!ere_lit(&ere, '\t')) goto fail; i += 2; continue; }
            if (!ere_directive(&ere, dir, dirs, ndirs)) {
                fprintf(stderr, "Error: datetime: '%%%c' is a bad directive in format \"%s\".\n", dir, fmt);
                goto fail;
            }
            i += 2;
            continue;
        }
        // Plain literal character.
        if (!ere_lit(&ere, c)) goto fail;
        i++;
    }
    // Anchor fully: CPython matches the ENTIRE string (no leading skip, no
    // trailing garbage; leftover input = "unconverted data remains").
    if (!anchor) return ere.data;   // sub-ERE: inlined into a parent pattern
    char* wrapped = malloc(ere.len + 3);
    if (!wrapped) goto fail;
    wrapped[0] = '^';
    memcpy(wrapped + 1, ere.data, ere.len);
    wrapped[1 + ere.len] = '$';
    wrapped[2 + ere.len] = '\0';
    free(ere.data);
    return wrapped;
fail:
    free(ere.data);
    return NULL;
}

// Look up a name (case-insensitive) in a table; returns index or -1.
static int name_index(const char* s, const char* const* names, int n) {
    for (int i = 0; i < n; i++) {
        if (strcasecmp(s, names[i]) == 0) return i;
    }
    return -1;
}

// Parse state accumulated from directive groups (CPython's found_dict).
typedef struct {
    int year, month, day;           // -1 = not found
    int hour, minute, second, micro;
    int pm, have_ampm;              // pm: 1 = PM seen
    int weekday;                    // 0=Sun..6=Sat, -1 = not found
    int julian;                     // -1 = not found
    int week_of_year, week_starts_mon;  // -1 = not found
    int iso_year, iso_week;         // -1 = not found
    long long gmtoff;               // LLONG_MIN-like sentinel = none
    bool have_gmtoff;
} ParseState;

// Initialize a ParseState to "nothing found yet".
static void ps_init(ParseState* ps) {
    ps->year = ps->month = ps->day = -1;
    ps->hour = ps->minute = ps->second = ps->micro = 0;
    ps->pm = ps->have_ampm = 0;
    ps->weekday = ps->julian = -1;
    ps->week_of_year = ps->week_starts_mon = -1;
    ps->iso_year = ps->iso_week = -1;
    ps->gmtoff = 0;
    ps->have_gmtoff = false;
}

// Apply one captured group to the parse state (CPython's found_dict walk).
// Returns true on success; false (message printed) on bad value.
static bool apply_group(ParseState* ps, char dir, const char* text) {
    long v = strtol(text, NULL, 10);
    switch (dir) {
        case 'Y': ps->year = (int)v; break;
        case 'y': ps->year = (v <= 68) ? (int)(2000 + v) : (int)(1900 + v); break;
        case 'm': ps->month = (int)v; break;
        case 'd': ps->day = (int)v; break;
        case 'H': ps->hour = (int)v; break;
        case 'I': ps->hour = (int)v; break;
        case 'M': ps->minute = (int)v; break;
        case 'S': ps->second = (int)v; break;
        case 'f': {
            // 1-6 digits, right-pad with zeros to microseconds (CPython).
            char padded[7];
            size_t n = strlen(text);
            if (n > 6) n = 6;   // regex allows {1,6}; defensive clamp
            for (size_t k = 0; k < n; k++) padded[k] = text[k];
            for (size_t k = n; k < 6; k++) padded[k] = '0';
            padded[6] = '\0';
            ps->micro = (int)strtol(padded, NULL, 10);
            break;
        }
        case 'w': ps->weekday = (v == 0) ? 6 : (int)(v - 1); break; // -> Mon0..Sun6
        case 'u': ps->weekday = (int)(v - 1); break;                 // 1=Mon..7=Sun
        case 'j': ps->julian = (int)v; break;
        case 'U': ps->week_of_year = (int)v; ps->week_starts_mon = 0; break;
        case 'W': ps->week_of_year = (int)v; ps->week_starts_mon = 1; break;
        case 'G': ps->iso_year = (int)v; break;
        case 'V': ps->iso_week = (int)v; break;
        case 'A': case 'a': {
            int idx = name_index(text, DAY_FULL, 7);
            if (idx < 0) idx = name_index(text, DAY_ABBR, 7);
            if (idx < 0) goto bad;
            ps->weekday = (idx == 0) ? 6 : idx - 1;   // -> Mon0..Sun6
            break;
        }
        case 'B': case 'b': {
            int idx = name_index(text, MONTH_FULL, 12);
            if (idx < 0) idx = name_index(text, MONTH_ABBR, 12);
            // %B table lacks positional 0 -> CPython indexes months 1..12;
            // our tables are 0-based, so month = idx + 1.
            if (idx < 0) goto bad;
            ps->month = idx + 1;
            break;
        }
        case 'p': {
            int idx = name_index(text, AM_PM, 2);
            if (idx < 0) goto bad;
            ps->pm = idx;                  // 0 = AM, 1 = PM
            ps->have_ampm = 1;
            break;
        }
        case 'z': {
            // [+-]HH:MM / [+-]HHMM / optional :SS(.fff) / Z
            if (text[0] == 'Z' && text[1] == '\0') { ps->gmtoff = 0; ps->have_gmtoff = true; break; }
            const char* p = text;
            int sign = (*p == '-') ? -1 : 1;
            if (*p == '+' || *p == '-') p++;
            int hh = (int)((p[0] - '0') * 10 + (p[1] - '0'));
            int mm = (p[2] == ':') ? (int)((p[3] - '0') * 10 + (p[4] - '0')) : (int)((p[2] - '0') * 10 + (p[3] - '0'));
            ps->gmtoff = sign * (hh * 3600 + mm * 60);
            ps->have_gmtoff = true;
            break;
        }
        case 'Z': {
            // Accept but ignore the tz name (naive struct, C-locale name set).
            static const char* const ZONES[] = { "UTC", "GMT" };
            if (name_index(text, ZONES, 2) >= 0) { ps->gmtoff = 0; ps->have_gmtoff = true; break; }
            break;  // unknown names: accepted, ignored
        }
        default:
            goto bad;
    }
    return true;
bad:
    fprintf(stderr, "Error: datetime: value \"%s\" invalid for directive %%%c.\n", text, dir);
    return false;
}

// Resolve a filled ParseState to a civil datetime, cloning CPython's
// ambiguity rules exactly (ISO incompat errors, julian overriding month/day,
// week-of-year -> julian, 1900/1904 defaults, range checks). Returns true
// and fills the out-params; false (message printed) on invalid input.
static bool resolve_state(const ParseState* ps, int* out_y, int* out_mo, int* out_d,
                          int* out_h, int* out_mi, int* out_s) {
    int year = ps->year, month = ps->month, day = ps->day;
    if (month < 0) month = 1;
    if (day < 0) day = 1;
    bool leap_year_fix = false;

    // ISO incompatibility rules (CPython raises, we mirror with errors).
    if (ps->iso_year >= 0) {
        if (ps->julian >= 0) {
            fprintf(stderr, "Error: datetime: '%%j' is not compatible with '%%G'. Use '%%Y'.\n");
            return false;
        }
        if (ps->iso_week < 0 || ps->weekday < 0) {
            fprintf(stderr, "Error: datetime: '%%G' must be used with '%%V' and a weekday directive.\n");
            return false;
        }
    } else if (ps->iso_week >= 0) {
        fprintf(stderr, "Error: datetime: '%%V' must be used with '%%G' and a weekday directive.\n");
        return false;
    }

    // Default year (CPython: 1900, or 1904 for Feb-29-without-year).
    if (year < 0) {
        if (month == 2 && day == 29) {
            year = 1904;
            leap_year_fix = true;
        } else {
            year = 1900;
        }
    }

    // Week-of-year + weekday -> julian (CPython _calc_julian_from_U_or_W).
    int julian = ps->julian;
    if (julian < 0 && ps->weekday >= 0 && ps->week_of_year >= 0) {
        // first_weekday: weekday of Jan 1 in this format's week-start terms.
        int jan1_wd_sun0 = weekday_from_days(days_from_civil(year, 1, 1));
        int first_weekday = ps->week_starts_mon
            ? (jan1_wd_sun0 + 6) % 7             // Monday-start weeks
            : jan1_wd_sun0;                      // Sunday-start weeks
        int dow = ps->weekday;                   // Mon0..Sun6
        if (!ps->week_starts_mon) dow = (dow + 1) % 7;
        int week_0_length = (7 - first_weekday) % 7;
        if (ps->week_of_year == 0) {
            julian = 1 + dow - first_weekday;
        } else {
            int days_to_week = week_0_length + 7 * (ps->week_of_year - 1);
            julian = 1 + days_to_week + dow;
        }
        if (julian <= 0) {
            year -= 1;
            julian += is_leap(year) ? 366 : 365;
        }
    }

    // ISO week-date -> civil directly (no julian path).
    if (ps->iso_year >= 0 && ps->iso_week >= 0 && ps->weekday >= 0) {
        civil_from_iso_week(ps->iso_year, ps->iso_week, ps->weekday + 1,
                            &year, &month, &day);
        julian = -1;  // civil already set; skip julian conversion below
    }

    // Julian -> civil (julian wins over explicit month/day -- CPython).
    if (julian > 0) {
        long long days = days_from_civil(year, 1, 1) + (julian - 1);
        civil_from_days(days, &year, &month, &day);
    }

    // 12-hour clock + AM/PM (CPython semantics).
    int hour = ps->hour;
    if (ps->have_ampm) {
        if (ps->pm) {
            if (hour != 12) hour += 12;   // 12 PM == 12
        } else {
            if (hour == 12) hour = 0;     // 12 AM == 0
        }
    }

    // Range checks (CPython validates in the datetime constructor).
    if (month < 1 || month > 12) {
        fprintf(stderr, "Error: datetime: month must be in 1..12.\n");
        return false;
    }
    if (day < 1 || day > days_in_month(year, month)) {
        fprintf(stderr, "Error: datetime: day is out of range for month.\n");
        return false;
    }
    if (hour > 23 || ps->minute > 59 || ps->second > 59) {
        fprintf(stderr, "Error: datetime: hour must be in 0..23, minute/second in 0..59.\n");
        return false;
    }

    if (leap_year_fix) year = 1900;
    *out_y = year; *out_mo = month; *out_d = day;
    *out_h = hour; *out_mi = ps->minute; *out_s = ps->second;
    return true;
}

// Shared struct builder: fields appended in order (the old code built the
// linked list in reverse and never set ->definition, so printing a datetime
// struct read malloc garbage). One definition, stable field order.
static Value make_datetime_struct(long long year, int month, int day,
                                  int hour, int minute, int second,
                                  long long unix_ts) {
    StructInstance* instance = malloc(sizeof(StructInstance));
    if (!instance) return make_null();
    instance->definition = datetime_def();
    instance->fields = NULL;
    long long vals[7] = { year, month, day, hour, minute, second, unix_ts };
    static const char* const names[7] =
        { "year","month","day","hour","minute","second","unix" };
    FieldValue* tail = NULL;
    for (int i = 0; i < 7; i++) {
        FieldValue* f = malloc(sizeof(FieldValue));
        if (!f) return make_null();
        f->name = strdup(names[i]);
        f->value = malloc(sizeof(Value));
        f->value->type = VAL_INT;
        f->value->int_val = vals[i];
        f->next = NULL;
        if (tail) tail->next = f; else instance->fields = f;
        tail = f;
    }
    Value result;
    result.type = VAL_STRUCT_INSTANCE;
    result.struct_instance = instance;
    return result;
}

// Field extractor: find an int field on a datetime struct by name.
static bool dt_get_field(Value* dt, const char* name, long long* out) {
    if (dt->type != VAL_STRUCT_INSTANCE) return false;
    for (FieldValue* f = dt->struct_instance->fields; f; f = f->next) {
        if (strcmp(f->name, name) == 0 && f->value->type == VAL_INT) {
            *out = f->value->int_val;
            return true;
        }
    }
    return false;
}

// Pull all 7 fields out of a datetime struct (false if not a datetime).
static bool dt_unpack(Value* dt, long long* y, int* mo, int* d,
                      int* h, int* mi, int* s, long long* ux) {
    // NB: fetch through a long long temp -- casting int* to long long* and
    // writing 8 bytes would smash the adjacent stack slots (found the hard
    // way: month became 0, MONTH_FULL[-1] segfaulted the interpreter).
    long long t;
    if (!dt_get_field(dt, "year", y)) return false;
    if (!dt_get_field(dt, "month", &t)) return false;
    *mo = (int)t;
    if (!dt_get_field(dt, "day", &t)) return false;
    *d = (int)t;
    if (!dt_get_field(dt, "hour", &t)) return false;
    *h = (int)t;
    if (!dt_get_field(dt, "minute", &t)) return false;
    *mi = (int)t;
    if (!dt_get_field(dt, "second", &t)) return false;
    *s = (int)t;
    return dt_get_field(dt, "unix", ux);
}

// glibc-compatible: known strftime directive letters. Anything else in a
// format string passes through literally ("100%% %q" renders as "100% %q"), like
// glibc strftime (and CPython on top of it) does. Parse side still rejects.
static bool is_fmt_directive(char dir) {
    return dir && strchr("YymBbdeHIMSjaAwupUWGVzZCnt%cxXDFTRr", dir) != NULL;
}

// Forward decl: composite %c recurses into the walker.
static bool fmt_strftime_impl(GrowBuf* out, const char* fmt,
                              long long year, int month, int day,
                              int hour, int minute, int second,
                              int yday, int weekday_sun0, bool has_tz,
                              long long tz_offset);

// strftime side: render one directive into the output buffer. tz_offset is
// the %z offset in seconds (0 when naive; naive %z/%Z render empty, like
// CPython). Returns false on a bad directive.
static bool fmt_directive(GrowBuf* out, char dir,
                          long long year, int month, int day,
                          int hour, int minute, int second,
                          int yday, int weekday_sun0, bool has_tz,
                          long long tz_offset) {
    int wd = weekday_sun0;                       // 0=Sun..6=Sat
    switch (dir) {
        case 'Y': return gb_printf(out, "%04lld", year);
        case 'y': return gb_printf(out, "%02d", (int)(year % 100));
        case 'm': return gb_printf(out, "%02d", month);
        case 'B': return gb_str(out, MONTH_FULL[month - 1]);
        case 'b': return gb_str(out, MONTH_ABBR[month - 1]);
        case 'd': return gb_printf(out, "%02d", day);
        case 'e': return gb_printf(out, "%2d", day);
        case 'H': return gb_printf(out, "%02d", hour);
        case 'I': {
            int h12 = hour % 12; if (h12 == 0) h12 = 12;
            return gb_printf(out, "%02d", h12);
        }
        case 'M': return gb_printf(out, "%02d", minute);
        case 'S': return gb_printf(out, "%02d", second);
        case 'j': return gb_printf(out, "%03d", yday);
        case 'a': return gb_str(out, DAY_ABBR[wd]);
        case 'A': return gb_str(out, DAY_FULL[wd]);
        case 'w': return gb_printf(out, "%d", wd);
        case 'u': return gb_printf(out, "%d", wd == 0 ? 7 : wd);
        case 'p': return gb_str(out, hour < 12 ? AM_PM[0] : AM_PM[1]);
        case 'U': case 'W': {
            bool mon = (dir == 'W');
            return gb_printf(out, "%02d", week_number(yday, wd, mon));
        }
        case 'G': case 'V': {
            int iy, iw, iwd;
            iso_week_of(year, month, day, &iy, &iw, &iwd);
            return dir == 'G' ? gb_printf(out, "%04d", iy)
                              : gb_printf(out, "%02d", iw);
        }
        case 'z': {
            if (!has_tz) return gb_str(out, "");   // naive -> empty (CPython)
            long long off = tz_offset;
            char sign = off < 0 ? '-' : '+';
            if (off < 0) off = -off;
            int hh = (int)(off / 3600);
            int mm = (int)((off % 3600) / 60);
            return gb_printf(out, "%c%02d%02d", sign, hh, mm);
        }
        case 'Z': {
            if (!has_tz) return gb_str(out, "");   // naive -> empty (CPython)
            if (tz_offset == 0) return gb_str(out, "UTC");
            return gb_printf(out, "UTC%c%02d:%02d",
                             tz_offset < 0 ? '-' : '+',
                             (int)(llabs(tz_offset) / 3600),
                             (int)((llabs(tz_offset) % 3600) / 60));
        }
        case 'C': return gb_printf(out, "%02lld", year / 100);
        case 'n': return gb_str(out, "\n");
        case 't': return gb_str(out, "\t");
        case '%': return gb_str(out, "%");
        case 'c': return fmt_strftime_impl(out, "%a %b %e %H:%M:%S %Y",
                             year, month, day, hour, minute, second,
                             yday, wd, has_tz, tz_offset);
        default:
            return false;
    }
}

// strftime walker: render a full format string. Composite %c/%x/%X and the
// format-only %D/%F/%T/%R/%r are expanded through expand_token recursion.
// Returns false on bad directive; out holds the partial render then.
static bool fmt_strftime_impl(GrowBuf* out, const char* fmt,
                              long long year, int month, int day,
                              int hour, int minute, int second,
                              int yday, int weekday_sun0, bool has_tz,
                              long long tz_offset) {
    size_t flen = strlen(fmt);
    size_t i = 0;
    while (i < flen) {
        // Legacy RADS tokens render like the old format() did.
        if (is_yyyy(fmt, i)) {
            if (!fmt_directive(out, 'Y', year, month, day, hour, minute,
                               second, yday, weekday_sun0, has_tz, tz_offset))
                return false;
            i += 4;
            continue;
        }
        if (i + 2 <= flen) {
            const char* legacy = expand_token(&fmt[i], 2);
            if (legacy && legacy[0] == '%' && strchr("mdHMS", legacy[1])
                && fmt[i] != '%') {
                if (!fmt_directive(out, legacy[1], year, month, day, hour,
                                   minute, second, yday, weekday_sun0,
                                   has_tz, tz_offset)) return false;
                i += 2;
                continue;
            }
        }
        if (fmt[i] == '%' && i + 1 < flen) {
            char dir = fmt[i + 1];
            const char* exp = expand_token(&fmt[i], 2);
            if (exp) {
                if (!fmt_strftime_impl(out, exp, year, month, day, hour,
                                       minute, second, yday, weekday_sun0,
                                       has_tz, tz_offset)) return false;
                i += 2;
                continue;
            }
            if (dir == '%' || dir == 'n' || dir == 't') {
                if (!fmt_directive(out, dir, year, month, day, hour, minute,
                                   second, yday, weekday_sun0, has_tz, tz_offset))
                    return false;
                i += 2;
                continue;
            }
            if (!is_fmt_directive(dir)) {
                // Unknown directive: emit "%" + dir literally (glibc law).
                char unk[3] = { '%', dir, '\0' };
                if (!gb_str(out, unk)) return false;
                i += 2;
                continue;
            }
            if (!fmt_directive(out, dir, year, month, day, hour, minute,
                               second, yday, weekday_sun0, has_tz, tz_offset))
                return false;
            i += 2;
            continue;
        }
        char lit[2] = { fmt[i], '\0' };
        if (!gb_str(out, lit)) return false;
        i++;
    }
    return true;
}

// datetime.strptime(string, format) -> DateTime struct (null on error).
// CPython parity: full-string match, 1900 default year, %j over month/day,
// week/ISO rules, case-insensitive names, %z offsets honored (aware unix
// timestamp; the struct stays naive). Legacy "YYYY-MM-DD" tokens still work.
Value stdlib_datetime_strptime(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: datetime.strptime(string, format) needs two strings.\n");
        return make_null();
    }
    const char* date_str = args[0].string_val;
    const char* fmt = args[1].string_val;

    // Cache lookup / compile.
    CachedFormat* entry = NULL;
    for (int i = 0; i < fmt_cache_count; i++) {
        if (strcmp(fmt_cache[i].fmt, fmt) == 0) { entry = &fmt_cache[i]; break; }
    }
    int dirs[256];
    int ndirs = 0;
    if (!entry) {
        if (fmt_cache_count >= MAX_FMT_CACHE) {
            // Recompile in slot 0 (simple bound, formats rarely vary).
            regfree(&fmt_cache[0].compiled);
            free(fmt_cache[0].fmt);
            free(fmt_cache[0].ere);
            free(fmt_cache[0].dirs);
            fmt_cache_count = 0;
        }
        char* ere = format_to_ere(fmt, dirs, &ndirs, true);
        if (!ere) return make_null();
        if (regcomp(&fmt_cache[fmt_cache_count].compiled, ere,
                    REG_EXTENDED) != 0) {
            fprintf(stderr, "Error: datetime: could not compile format \"%s\".\n", fmt);
            free(ere);
            return make_null();
        }
        CachedFormat* slot = &fmt_cache[fmt_cache_count];
        slot->fmt = strdup(fmt);
        slot->ere = ere;
        slot->dirs = malloc(sizeof(int) * (ndirs > 0 ? ndirs : 1));
        memcpy(slot->dirs, dirs, sizeof(int) * ndirs);
        slot->dir_count = ndirs;
        slot->valid = true;
        fmt_cache_count++;
        entry = slot;
    }
    // Use the entry's own dirs (cache hit path skips the compile above).
    int* gdirs = entry->dirs;
    int gcount = entry->dir_count;

    regmatch_t matches[257];
    if (regexec(&entry->compiled, date_str, (size_t)gcount + 1, matches, 0) != 0) {
        return make_null();   // CPython: ValueError on no match
    }
    // CPython rejects trailing garbage ("unconverted data remains").
    if (matches[0].rm_eo < (regoff_t)strlen(date_str)) {
        fprintf(stderr, "Error: datetime: unconverted data remains: \"%s\".\n",
                &date_str[matches[0].rm_eo]);
        return make_null();
    }

    ParseState ps;
    ps_init(&ps);
    char buf[32];
    for (int g = 1; g <= gcount; g++) {
        if (matches[g].rm_so < 0) continue;
        int len = matches[g].rm_eo - matches[g].rm_so;
        if (len > 30) {
            fprintf(stderr, "Error: datetime: field too long.\n");
            return make_null();
        }
        memcpy(buf, &date_str[matches[g].rm_so], (size_t)len);
        buf[len] = '\0';
        if (!apply_group(&ps, gdirs[g - 1], buf)) return make_null();
    }

    int y, mo, d, h, mi, s;
    if (!resolve_state(&ps, &y, &mo, &d, &h, &mi, &s)) return make_null();

    long long epoch = naive_epoch(y, mo, d, h, mi, s);
    if (ps.have_gmtoff) epoch -= ps.gmtoff;   // aware timestamp
    return make_datetime_struct(y, mo, d, h, mi, s, epoch);
}

// datetime.now() - Current local time as a DateTime struct.
Value stdlib_datetime_now(Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (!tm_info) return make_null();
    return make_datetime_struct(
        tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
        tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
        (long long)now);
}

// datetime.from_unix(unix_timestamp) - local-time interpretation.
Value stdlib_datetime_from_unix(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) {
        fprintf(stderr, "Error: datetime.from_unix() requires an integer unix timestamp.\n");
        return make_null();
    }
    time_t timestamp = (time_t)args[0].int_val;
    struct tm* tm_info = localtime(&timestamp);
    if (!tm_info) return make_null();
    return make_datetime_struct(
        tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
        tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
        (long long)timestamp);
}

// datetime.format(datetime_obj, format_string) - strftime with C-locale
// names, grow-on-demand output, %-directives plus legacy tokens.
Value stdlib_datetime_format(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRUCT_INSTANCE || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: datetime.format(datetime, format) needs a DateTime and a string.\n");
        return make_null();
    }
    long long y = 0, ux = 0;
    int mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (!dt_unpack(&args[0], &y, &mo, &d, &h, &mi, &s, &ux)) {
        fprintf(stderr, "Error: datetime.format() argument is not a DateTime struct.\n");
        return make_null();
    }
    GrowBuf out = {0};
    bool ok = fmt_strftime_impl(&out, args[1].string_val, y, mo, d, h, mi, s,
                                yday_of(y, mo, d), weekday_from_days(days_from_civil(y, mo, d)),
                                false, 0);
    if (!ok) {
        fprintf(stderr, "Error: datetime: bad directive in format \"%s\".\n", args[1].string_val);
        free(out.data);
        return make_string("");
    }
    Value result = make_string(out.data ? out.data : "");
    free(out.data);
    return result;
}

// datetime.parse(string, format) - strptime alias (old name kept working).
Value stdlib_datetime_parse(Interpreter* interp, int argc, Value* args) {
    return stdlib_datetime_strptime(interp, argc, args);
}

// datetime.unix(datetime_obj) - Unix timestamp field.
Value stdlib_datetime_unix(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    long long ux;
    if (argc < 1 || !dt_get_field(&args[0], "unix", &ux)) {
        fprintf(stderr, "Error: datetime.unix() requires a DateTime struct.\n");
        return make_int(0);
    }
    return make_int(ux);
}

// Field getters - one shared implementation for year/month/day/hour/
// minute/second (the old code repeated this loop six times).
static Value dt_field_getter(Interpreter* interp, int argc, Value* args, const char* field) {
    (void)interp;
    long long v;
    if (argc < 1 || !dt_get_field(&args[0], field, &v)) {
        fprintf(stderr, "Error: datetime.%s() requires a DateTime struct.\n", field);
        return make_int(0);
    }
    return make_int(v);
}

Value stdlib_datetime_year(Interpreter* interp, int argc, Value* args) {
    return dt_field_getter(interp, argc, args, "year");
}
Value stdlib_datetime_month(Interpreter* interp, int argc, Value* args) {
    return dt_field_getter(interp, argc, args, "month");
}
Value stdlib_datetime_day(Interpreter* interp, int argc, Value* args) {
    return dt_field_getter(interp, argc, args, "day");
}
Value stdlib_datetime_hour(Interpreter* interp, int argc, Value* args) {
    return dt_field_getter(interp, argc, args, "hour");
}
Value stdlib_datetime_minute(Interpreter* interp, int argc, Value* args) {
    return dt_field_getter(interp, argc, args, "minute");
}
Value stdlib_datetime_second(Interpreter* interp, int argc, Value* args) {
    return dt_field_getter(interp, argc, args, "second");
}

// datetime.weekday(datetime_obj) - Day of week (0=Sunday, 6=Saturday).
Value stdlib_datetime_weekday(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    long long y = 0, ux = 0;
    int mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (argc < 1 || !dt_unpack(&args[0], &y, &mo, &d, &h, &mi, &s, &ux)) {
        fprintf(stderr, "Error: datetime.weekday() requires a DateTime struct.\n");
        return make_int(0);
    }
    return make_int(weekday_from_days(days_from_civil(y, mo, d)));
}

// datetime.add(datetime_obj, days, hours=0, minutes=0, seconds=0) - Add time
// via exact civil math (the old code round-tripped through localtime).
Value stdlib_datetime_add(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRUCT_INSTANCE) {
        fprintf(stderr, "Error: datetime.add(datetime, days, [hours, minutes, seconds]).\n");
        return make_null();
    }
    long long y = 0, ux = 0;
    int mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (!dt_unpack(&args[0], &y, &mo, &d, &h, &mi, &s, &ux)) return make_null();
    long long days    = (argc >= 2 && args[1].type == VAL_INT) ? args[1].int_val : 0;
    long long hours   = (argc >= 3 && args[2].type == VAL_INT) ? args[2].int_val : 0;
    long long minutes = (argc >= 4 && args[3].type == VAL_INT) ? args[3].int_val : 0;
    long long seconds = (argc >= 5 && args[4].type == VAL_INT) ? args[4].int_val : 0;
    long long delta = days * 86400LL + hours * 3600LL + minutes * 60LL + seconds;
    long long new_epoch = naive_epoch(y, mo, d, h, mi, s) + delta;
    int ny, nmo, nd, nh, nmi, ns;
    civil_from_epoch_helper(new_epoch, &ny, &nmo, &nd, &nh, &nmi, &ns);
    return make_datetime_struct(ny, nmo, nd, nh, nmi, ns, new_epoch);
}

// datetime.diff(datetime1, datetime2) - Difference in seconds.
Value stdlib_datetime_diff(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    long long ux1 = 0, ux2 = 0;
    if (argc < 2
        || !dt_get_field(&args[0], "unix", &ux1)
        || !dt_get_field(&args[1], "unix", &ux2)) {
        fprintf(stderr, "Error: datetime.diff(d1, d2) requires two DateTime structs.\n");
        return make_int(0);
    }
    return make_int(ux1 - ux2);
}

void stdlib_datetime_register(void) {
    register_native("datetime.now", stdlib_datetime_now);
    register_native("datetime.from_unix", stdlib_datetime_from_unix);
    register_native("datetime.format", stdlib_datetime_format);
    register_native("datetime.strptime", stdlib_datetime_strptime);
    register_native("datetime.parse", stdlib_datetime_parse);
    register_native("datetime.unix", stdlib_datetime_unix);
    register_native("datetime.year", stdlib_datetime_year);
    register_native("datetime.month", stdlib_datetime_month);
    register_native("datetime.day", stdlib_datetime_day);
    register_native("datetime.hour", stdlib_datetime_hour);
    register_native("datetime.minute", stdlib_datetime_minute);
    register_native("datetime.second", stdlib_datetime_second);
    register_native("datetime.weekday", stdlib_datetime_weekday);
    register_native("datetime.add", stdlib_datetime_add);
    register_native("datetime.diff", stdlib_datetime_diff);
}
