#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "rstar.c" /* reuse helpers under RSTAR_TESTING */

/*
 * RADStar helper tests.
 * Why: Ensure path resolution, mkdir -p, manifest parsing, and lock writes are stable.
 * How to upgrade: Split into dedicated test runner and add more cases for HTTP fetch and exec wiring.
 */

static bool test_plugin_root_path_custom(void) {
    char buf[RSTAR_MAX_PATH];
    setenv(RSTAR_PLUGIN_ROOT_ENV, "/tmp/rstar-root", 1);
    bool ok = plugin_root_path(buf, sizeof(buf));
    unsetenv(RSTAR_PLUGIN_ROOT_ENV);
    return ok && strcmp(buf, "/tmp/rstar-root") == 0;
}

static bool test_ensure_dir_recursive(void) {
    char templ[] = "/tmp/rstar-test-XXXXXX";
    char *base = mkdtemp(templ);
    if (!base) return false;
    char nested[RSTAR_MAX_PATH];
    snprintf(nested, sizeof(nested), "%s/a/b/c", base);
    bool ok = ensure_dir_recursive(nested);
    return ok;
}

static bool test_join_path(void) {
    char out[64];
    bool ok = join_path(out, sizeof(out), "/tmp", "plugin");
    return ok && strcmp(out, "/tmp/plugin") == 0;
}

static bool test_append_lock_and_read(void) {
    char templ[] = "/tmp/rstar-lock-XXXXXX";
    char *dir = mkdtemp(templ);
    if (!dir) return false;
    char lock[RSTAR_MAX_PATH];
    snprintf(lock, sizeof(lock), "%s/plugins.lock", dir);
    bool ok = append_lock_entry(lock, "p1", "1.0.0", "main.rads", "/tmp/p1");
    if (!ok) return false;
    FILE *f = fopen(lock, "r");
    if (!f) return false;
    char line[128];
    char *res = fgets(line, sizeof(line), f);
    fclose(f);
    return res && strstr(line, "p1|1.0.0|main.rads|/tmp/p1");
}

static bool test_manifest_entry(void) {
    char templ[] = "/tmp/rstar-manifest-XXXXXX";
    char *dir = mkdtemp(templ);
    if (!dir) return false;
    char manifest[RSTAR_MAX_PATH];
    snprintf(manifest, sizeof(manifest), "%s/plugin.radsmeta", dir);
    FILE *f = fopen(manifest, "w");
    if (!f) return false;
    fprintf(f, "entry = \"demo.rads\"\n");
    fclose(f);
    char entry[64] = {0};
    bool ok = read_manifest_entry(manifest, entry, sizeof(entry));
    return ok && strcmp(entry, "demo.rads") == 0;
}

int main(void) {
    int failed = 0;
    if (!test_plugin_root_path_custom()) { fprintf(stderr, "FAIL plugin_root_path_custom\n"); failed++; }
    if (!test_ensure_dir_recursive()) { fprintf(stderr, "FAIL ensure_dir_recursive\n"); failed++; }
    if (!test_join_path()) { fprintf(stderr, "FAIL join_path\n"); failed++; }
    if (!test_append_lock_and_read()) { fprintf(stderr, "FAIL append_lock_and_read\n"); failed++; }
    if (!test_manifest_entry()) { fprintf(stderr, "FAIL manifest_entry\n"); failed++; }
    if (failed == 0) {
        printf("OK\n");
        return 0;
    }
    return 1;
}
