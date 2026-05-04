#define _POSIX_C_SOURCE 200809L
#include "stdlib_tempfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* ── tempfile Module ──
 * Temporary file and directory creation.
 * Uses mkstemp/mkdtemp for safe, race-free temp creation.
 * Lightweight — works on all POSIX platforms including 32-bit.
 */

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_null(void);

/* ── tempfile.name() → system temp directory path ── */
Value native_tempfile_name(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = getenv("TEMP");
    if (!tmpdir) tmpdir = getenv("TMP");
    if (!tmpdir) tmpdir = "/tmp";
    return make_string(tmpdir);
}

/* ── tempfile.file(suffix="", prefix="tmp") → path string ──
 * Creates an empty temporary file, returns its path.
 * File is created with 0600 permissions (owner-only read/write).
 */
Value native_tempfile_file(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    const char* suffix = "";
    const char* prefix = "tmp";
    if (argc >= 1 && args[0].type == VAL_STRING) suffix = args[0].string_val;
    if (argc >= 2 && args[1].type == VAL_STRING) prefix = args[1].string_val;

    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = "/tmp";

    /* Build template: tmpdir/prefixXXXXXXsuffix */
    size_t pathlen = strlen(tmpdir) + 1 + strlen(prefix) + 6 + strlen(suffix) + 1;
    char* template = (char*)malloc(pathlen);
    if (!template) return make_null();
    snprintf(template, pathlen, "%s/%sXXXXXX%s", tmpdir, prefix, suffix);

    /* Append suffix after mkstemp: need to do it manually */
    /* mkstemp only handles trailing X's, so create without suffix first */
    size_t tmplen = strlen(tmpdir) + 1 + strlen(prefix) + 6 + 1;
    char* tmpl_base = (char*)malloc(tmplen);
    if (!tmpl_base) { free(template); return make_null(); }
    snprintf(tmpl_base, tmplen, "%s/%sXXXXXX", tmpdir, prefix);

    int fd = mkstemp(tmpl_base);
    if (fd == -1) { free(template); free(tmpl_base); return make_null(); }
    close(fd);

    /* If suffix provided, rename to add suffix */
    if (suffix[0] != '\0') {
        snprintf(template, pathlen, "%s%s", tmpl_base, suffix);
        if (rename(tmpl_base, template) != 0) {
            unlink(tmpl_base);
            free(template);
            free(tmpl_base);
            return make_null();
        }
        Value v = make_string(template);
        free(template);
        free(tmpl_base);
        return v;
    }

    Value v = make_string(tmpl_base);
    free(template);
    free(tmpl_base);
    return v;
}

/* ── tempfile.directory(suffix="", prefix="tmp") → path string ──
 * Creates a temporary directory with 0700 permissions.
 */
Value native_tempfile_directory(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    const char* suffix = "";
    const char* prefix = "tmp";
    if (argc >= 1 && args[0].type == VAL_STRING) suffix = args[0].string_val;
    if (argc >= 2 && args[1].type == VAL_STRING) prefix = args[1].string_val;

    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = "/tmp";

    size_t tmplen = strlen(tmpdir) + 1 + strlen(prefix) + 6 + 1;
    char* template = (char*)malloc(tmplen);
    if (!template) return make_null();
    snprintf(template, tmplen, "%s/%sXXXXXX", tmpdir, prefix);

    char* result = mkdtemp(template);
    if (!result) { free(template); return make_null(); }

    /* If suffix, rename */
    if (suffix[0] != '\0') {
        size_t newlen = strlen(result) + strlen(suffix) + 1;
        char* newpath = (char*)malloc(newlen);
        if (!newpath) { free(template); return make_null(); }
        snprintf(newpath, newlen, "%s%s", result, suffix);
        if (rename(result, newpath) != 0) {
            /* rmdir on failure */
            rmdir(result);
            free(newpath);
            free(template);
            return make_null();
        }
        Value v = make_string(newpath);
        free(newpath);
        free(template);
        return v;
    }

    Value v = make_string(template);
    free(template);
    return v;
}

/* ── tempfile.remove(path) → bool ──
 * Removes a temporary file or directory.
 */
Value native_tempfile_remove(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    const char* path = args[0].string_val;
    /* Try unlink first (file), then rmdir (directory) */
    if (unlink(path) == 0) return make_int(1);
    if (rmdir(path) == 0) return make_int(1);
    return make_int(0);
}

/* ── tempfile.exists(path) → bool ── */
Value native_tempfile_exists(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    struct stat st;
    return make_int(stat(args[0].string_val, &st) == 0 ? 1 : 0);
}

void stdlib_tempfile_register(void) {
    register_native("tempfile.name", native_tempfile_name);
    register_native("tempfile.file", native_tempfile_file);
    register_native("tempfile.directory", native_tempfile_directory);
    register_native("tempfile.remove", native_tempfile_remove);
    register_native("tempfile.exists", native_tempfile_exists);
}
