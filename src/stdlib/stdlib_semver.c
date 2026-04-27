#define _POSIX_C_SOURCE 200809L
#include "stdlib_semver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);

/* ── Semantic Versioning module ──
 * Parse, compare, and manipulate semver strings.
 * Zero external dependencies.
 */

typedef struct {
    int major, minor, patch;
    char prerelease[128];
    char build[128];
    int valid;
} SemVer;

static int parse_semver(const char* s, SemVer* sv) {
    memset(sv, 0, sizeof(*sv));
    sv->major = sv->minor = sv->patch = -1;

    const char* p = s;
    /* Skip leading 'v' or '=' */
    if (*p == 'v' || *p == '=') p++;

    /* Major */
    if (!isdigit((unsigned char)*p)) return 0;
    sv->major = 0;
    while (isdigit((unsigned char)*p)) {
        sv->major = sv->major * 10 + (*p - '0');
        p++;
    }
    if (*p != '.') return 0;
    p++;

    /* Minor */
    if (!isdigit((unsigned char)*p)) return 0;
    sv->minor = 0;
    while (isdigit((unsigned char)*p)) {
        sv->minor = sv->minor * 10 + (*p - '0');
        p++;
    }
    if (*p != '.') return 0;
    p++;

    /* Patch */
    if (!isdigit((unsigned char)*p)) return 0;
    sv->patch = 0;
    while (isdigit((unsigned char)*p)) {
        sv->patch = sv->patch * 10 + (*p - '0');
        p++;
    }

    /* Prerelease */
    if (*p == '-') {
        p++;
        size_t i = 0;
        while (*p && *p != '+' && i < sizeof(sv->prerelease) - 1)
            sv->prerelease[i++] = *p++;
        sv->prerelease[i] = '\0';
    }

    /* Build */
    if (*p == '+') {
        p++;
        size_t i = 0;
        while (*p && i < sizeof(sv->build) - 1)
            sv->build[i++] = *p++;
        sv->build[i] = '\0';
    }

    sv->valid = 1;
    return 1;
}

/* Compare two semvers: returns -1, 0, 1 */
static int cmp_semver(const SemVer* a, const SemVer* b) {
    if (a->major != b->major) return a->major < b->major ? -1 : 1;
    if (a->minor != b->minor) return a->minor < b->minor ? -1 : 1;
    if (a->patch != b->patch) return a->patch < b->patch ? -1 : 1;
    /* Prerelease: no prerelease > with prerelease */
    int a_has_pre = a->prerelease[0] != '\0';
    int b_has_pre = b->prerelease[0] != '\0';
    if (!a_has_pre && b_has_pre) return 1;
    if (a_has_pre && !b_has_pre) return -1;
    if (a_has_pre && b_has_pre) return strcmp(a->prerelease, b->prerelease);
    return 0;
}

/* semver.parse(str) -> struct {major, minor, patch, prerelease, build, valid} */
Value stdlib_semver_parse(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) {
        /* Return struct with valid=false */
        /* Since we can't easily create structs from C, return null for invalid */
        return make_null();
    }

    /* Return as a formatted string: "major.minor.patch[-pre][+build]|valid" */
    char buf[512];
    int len = snprintf(buf, sizeof(buf), "%d.%d.%d", sv.major, sv.minor, sv.patch);
    if (sv.prerelease[0])
        len += snprintf(buf + len, sizeof(buf) - len, "-%s", sv.prerelease);
    if (sv.build[0])
        len += snprintf(buf + len, sizeof(buf) - len, "+%s", sv.build);

    return make_string(buf);
}

/* semver.valid(str) -> bool */
Value stdlib_semver_valid(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_bool(false);
    SemVer sv;
    return make_bool(parse_semver(args[0].string_val, &sv));
}

/* semver.major(str) -> int */
Value stdlib_semver_major(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(-1);
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_int(-1);
    return make_int(sv.major);
}

/* semver.minor(str) -> int */
Value stdlib_semver_minor(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(-1);
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_int(-1);
    return make_int(sv.minor);
}

/* semver.patch(str) -> int */
Value stdlib_semver_patch(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(-1);
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_int(-1);
    return make_int(sv.patch);
}

/* semver.prerelease(str) -> string|null */
Value stdlib_semver_prerelease(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_null();
    if (sv.prerelease[0] == '\0') return make_null();
    return make_string(sv.prerelease);
}

/* semver.build(str) -> string|null */
Value stdlib_semver_build(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_null();
    if (sv.build[0] == '\0') return make_null();
    return make_string(sv.build);
}

/* semver.compare(a, b) -> int (-1, 0, 1) */
Value stdlib_semver_compare(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_int(-2);
    SemVer a, b;
    if (!parse_semver(args[0].string_val, &a) || !parse_semver(args[1].string_val, &b))
        return make_int(-2);
    return make_int(cmp_semver(&a, &b));
}

/* semver.gt(a, b) -> bool */
Value stdlib_semver_gt(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_bool(false);
    SemVer a, b;
    if (!parse_semver(args[0].string_val, &a) || !parse_semver(args[1].string_val, &b))
        return make_bool(false);
    return make_bool(cmp_semver(&a, &b) > 0);
}

/* semver.gte(a, b) -> bool */
Value stdlib_semver_gte(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_bool(false);
    SemVer a, b;
    if (!parse_semver(args[0].string_val, &a) || !parse_semver(args[1].string_val, &b))
        return make_bool(false);
    return make_bool(cmp_semver(&a, &b) >= 0);
}

/* semver.lt(a, b) -> bool */
Value stdlib_semver_lt(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_bool(false);
    SemVer a, b;
    if (!parse_semver(args[0].string_val, &a) || !parse_semver(args[1].string_val, &b))
        return make_bool(false);
    return make_bool(cmp_semver(&a, &b) < 0);
}

/* semver.lte(a, b) -> bool */
Value stdlib_semver_lte(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_bool(false);
    SemVer a, b;
    if (!parse_semver(args[0].string_val, &a) || !parse_semver(args[1].string_val, &b))
        return make_bool(false);
    return make_bool(cmp_semver(&a, &b) <= 0);
}

/* semver.eq(a, b) -> bool */
Value stdlib_semver_eq(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_bool(false);
    SemVer a, b;
    if (!parse_semver(args[0].string_val, &a) || !parse_semver(args[1].string_val, &b))
        return make_bool(false);
    return make_bool(cmp_semver(&a, &b) == 0);
}

/* semver.bump_major(str) -> string */
Value stdlib_semver_bump_major(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_null();
    char buf[256];
    snprintf(buf, sizeof(buf), "%d.0.0", sv.major + 1);
    return make_string(buf);
}

/* semver.bump_minor(str) -> string */
Value stdlib_semver_bump_minor(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_null();
    char buf[256];
    snprintf(buf, sizeof(buf), "%d.%d.0", sv.major, sv.minor + 1);
    return make_string(buf);
}

/* semver.bump_patch(str) -> string */
Value stdlib_semver_bump_patch(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    SemVer sv;
    if (!parse_semver(args[0].string_val, &sv)) return make_null();
    char buf[256];
    snprintf(buf, sizeof(buf), "%d.%d.%d", sv.major, sv.minor, sv.patch + 1);
    return make_string(buf);
}

void stdlib_semver_register(void) {
    register_native("semver.parse", stdlib_semver_parse);
    register_native("semver.valid", stdlib_semver_valid);
    register_native("semver.major", stdlib_semver_major);
    register_native("semver.minor", stdlib_semver_minor);
    register_native("semver.patch", stdlib_semver_patch);
    register_native("semver.prerelease", stdlib_semver_prerelease);
    register_native("semver.build", stdlib_semver_build);
    register_native("semver.compare", stdlib_semver_compare);
    register_native("semver.gt", stdlib_semver_gt);
    register_native("semver.gte", stdlib_semver_gte);
    register_native("semver.lt", stdlib_semver_lt);
    register_native("semver.lte", stdlib_semver_lte);
    register_native("semver.eq", stdlib_semver_eq);
    register_native("semver.bump_major", stdlib_semver_bump_major);
    register_native("semver.bump_minor", stdlib_semver_bump_minor);
    register_native("semver.bump_patch", stdlib_semver_bump_patch);
}
