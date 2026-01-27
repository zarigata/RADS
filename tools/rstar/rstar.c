#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>

#define REGISTRY_API_BASE "https://api.registry.rads-lang.org"

#ifdef RSTAR_TESTING
#define RSTAR_INTERNAL
#else
#define RSTAR_INTERNAL static
#endif

#ifndef RSTAR_PLUGIN_ROOT_ENV
#define RSTAR_PLUGIN_ROOT_ENV "RADS_PLUGIN_HOME"
#endif

#ifndef RSTAR_DEFAULT_SUBDIR
#define RSTAR_DEFAULT_SUBDIR ".rads/plugins"
#endif

#define RSTAR_MAX_PATH 4096

/*
 * RADStar (rstar) — RADS plugin manager CLI.
 *
 * Why it exists:
 *   Provide a lightweight, fast, reliable way to install/list/run RADS plugins
 *   from a Git-hosted index. This is the seed skeleton to wire HTTP fetch and
 *   archive extraction later without changing callers.
 *
 * How to upgrade later:
 *   - Implement HTTP index fetch and repository download in cmd_install.
 *   - Add checksum/signature verification before unpacking.
 *   - Extend cmd_run to resolve plugin search paths and env isolation.
 *   - Keep each command under 50 lines; add helpers for I/O and parsing.
 */

/* print_usage
 * Why: Centralizes CLI help so new commands stay discoverable.
 * How to upgrade: Add command summaries here whenever a new verb is added.
 */
RSTAR_INTERNAL void print_usage(void) {
    printf("RADStar (rstar) - RADS Plugin Manager v0.1.0-skeleton\n");
    printf("Usage: rstar <command> [args]\n\n");
     printf("Commands:\n");
    printf("  install <name>   Install a plugin from registry\n");
    printf("  search <query>   Search packages in registry\n");
    printf("  info <name>      Get package information\n");
    printf("  list             List installed plugins (plugins.lock)\n");
    printf("  run <name> [...](Execute plugin entry with rads)\n");
    printf("  help             Show this help\n");
}

/* cmd_install
 * Why: Entry point to fetch and place a plugin locally from the remote index.
 * How to upgrade: Replace TODO steps with HTTP fetch + git/zip extraction,
 * manifest validation, checksum verification, and dependency resolution.
 */
/* ensure_dir_recursive
 * Why: mkdir -p equivalent for plugin store and lock location.
 * How to upgrade: Add permission controls and error translation.
 */
RSTAR_INTERNAL bool ensure_dir_recursive(const char *path) {
    if (!path || !*path) return false;
    char buffer[RSTAR_MAX_PATH];
    size_t len = strnlen(path, RSTAR_MAX_PATH - 1);
    memcpy(buffer, path, len);
    buffer[len] = '\0';
    for (size_t i = 1; i < len; ++i) {
        if (buffer[i] == '/') {
            buffer[i] = '\0';
            if (mkdir(buffer, 0755) != 0 && errno != EEXIST) return false;
            buffer[i] = '/';
        }
    }
    if (mkdir(buffer, 0755) != 0 && errno != EEXIST) return false;
    return true;
}

/* plugin_root_path
 * Why: Resolve plugin store root honoring RADS_PLUGIN_HOME or HOME fallback.
 * How to upgrade: Support XDG-style paths and Windows APPDATA.
 */
RSTAR_INTERNAL bool plugin_root_path(char *out, size_t out_len) {
    const char *custom = getenv(RSTAR_PLUGIN_ROOT_ENV);
    if (custom && *custom) {
        return snprintf(out, out_len, "%s", custom) < (int)out_len;
    }
    const char *home = getenv("HOME");
    if (!home) return false;
    return snprintf(out, out_len, "%s/%s", home, RSTAR_DEFAULT_SUBDIR) < (int)out_len;
}

/* join_path
 * Why: Safe path concatenation for store and manifest paths.
 * How to upgrade: Normalize separators across platforms.
 */
RSTAR_INTERNAL bool join_path(char *out, size_t out_len, const char *a, const char *b) {
    if (!out || !a || !b) return false;
    size_t la = strnlen(a, out_len);
    size_t lb = strnlen(b, out_len);
    if (la + 1 + lb + 1 > out_len) return false;
    memcpy(out, a, la);
    out[la] = '/';
    memcpy(out + la + 1, b, lb);
    out[la + 1 + lb] = '\0';
    return true;
}

/* append_lock_entry
 * Why: Record installs in a dedicated lock file instead of package.rads comments.
 * How to upgrade: Add version pinning, checksums, and JSON/TOML structure.
 */
RSTAR_INTERNAL bool append_lock_entry(const char *lock_path, const char *name, const char *version, const char *entry, const char *install_path) {
    FILE *f = fopen(lock_path, "a");
    if (!f) return false;
    fprintf(f, "%s|%s|%s|%s\n", name, version ? version : "0.0.0", entry ? entry : "main.rads", install_path ? install_path : "");
    fclose(f);
    return true;
}

/* read_manifest_entry
 * Why: Minimal parser to read entry = "file" from plugin.radsmeta.
 * How to upgrade: Implement full INI/TOML parser and validation.
 */
RSTAR_INTERNAL bool read_manifest_entry(const char *manifest_path, char *entry_out, size_t entry_len) {
    FILE *f = fopen(manifest_path, "r");
    if (!f) return false;
    char line[512];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "entry", 5) == 0) {
            char *eq = strchr(line, '=');
            if (!eq) continue;
            char *val = eq + 1;
            while (*val == ' ' || *val == '\t') val++;
            char *quote = strchr(val, '"');
            if (quote) {
                char *end = strchr(quote + 1, '"');
                if (end) {
                    size_t len = (size_t)(end - (quote + 1));
                    if (len >= entry_len) len = entry_len - 1;
                    memcpy(entry_out, quote + 1, len);
                    entry_out[len] = '\0';
                    found = true;
                    break;
                }
            }
        }
    }
    fclose(f);
    return found;
}

/* copy_manifest_if_present
 * Why: Keep a manifest alongside installed files even for stub installs.
 * How to upgrade: Copy full plugin payload after download/unpack.
 */
RSTAR_INTERNAL void copy_manifest_if_present(const char *dest_manifest_path) {
    FILE *src = fopen("plugin.radsmeta", "r");
    if (!src) {
        FILE *f = fopen(dest_manifest_path, "w");
        if (f) {
            fprintf(f, "name = \"unknown\"\nentry = \"main.rads\"\nversion = \"0.0.0\"\n");
            fclose(f);
        }
        return;
    }
    FILE *dst = fopen(dest_manifest_path, "w");
    if (!dst) {
        fclose(src);
        return;
    }
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }
    fclose(src);
    fclose(dst);
}

/* cmd_install
 * Why: Entry point to fetch and place a plugin locally from the remote index.
 * How to upgrade: Replace TODO steps with HTTP fetch + git/zip extraction,
 * manifest validation, checksum verification, and dependency resolution.
 */
RSTAR_INTERNAL void cmd_install(const char *package) {
    if (!package) {
        fprintf(stderr, "Error: package name required. Try: rstar install media-kit\n");
        return;
    }
    char root[RSTAR_MAX_PATH];
    if (!plugin_root_path(root, sizeof(root))) {
        fprintf(stderr, "Error: could not resolve plugin root path.\n");
        return;
    }
    if (!ensure_dir_recursive(root)) {
        fprintf(stderr, "Error: could not create plugin root at %s\n", root);
        return;
    }

    char plugin_dir[RSTAR_MAX_PATH];
    if (!join_path(plugin_dir, sizeof(plugin_dir), root, package)) {
        fprintf(stderr, "Error: plugin path too long.\n");
        return;
    }
    if (!ensure_dir_recursive(plugin_dir)) {
        fprintf(stderr, "Error: could not create plugin directory %s\n", plugin_dir);
        return;
    }

    char manifest_path[RSTAR_MAX_PATH];
    if (!join_path(manifest_path, sizeof(manifest_path), plugin_dir, "plugin.radsmeta")) {
        fprintf(stderr, "Error: manifest path too long.\n");
        return;
    }

    printf("📡 Resolving %s in remote index (TODO: HTTP fetch)...\n", package);
    printf("🌐 Downloading repository/archive for %s (TODO: git/zip)...\n", package);
    printf("🗄️  Placing files into %s ...\n", plugin_dir);
    copy_manifest_if_present(manifest_path);

    char lock_path[RSTAR_MAX_PATH];
    if (!join_path(lock_path, sizeof(lock_path), root, "plugins.lock")) {
        fprintf(stderr, "Error: lock path too long.\n");
        return;
    }
    append_lock_entry(lock_path, package, "0.1.0", "main.rads", plugin_dir);
    printf("✅ Installed %s (stub). Manifest stored at %s\n", package, manifest_path);
}

/* cmd_list
 * Why: Minimal visibility into what is recorded as installed in package.rads.
 * How to upgrade: Parse a dedicated plugins.lock with versions and statuses,
 * and show install locations plus updates available.
 */
/* cmd_list
 * Why: Minimal visibility into what is recorded as installed in plugins.lock.
 * How to upgrade: Parse a dedicated plugins.lock with versions and statuses,
 * and show install locations plus updates available.
 */
RSTAR_INTERNAL void cmd_list(void) {
    char root[RSTAR_MAX_PATH];
    if (!plugin_root_path(root, sizeof(root))) {
        fprintf(stderr, "Error: could not resolve plugin root path.\n");
        return;
    }
    char lock_path[RSTAR_MAX_PATH];
    if (!join_path(lock_path, sizeof(lock_path), root, "plugins.lock")) {
        fprintf(stderr, "Error: lock path too long.\n");
        return;
    }
    FILE *f = fopen(lock_path, "r");
    if (!f) {
        printf("(No plugins.lock found at %s)\n", lock_path);
        return;
    }
    printf("Installed Plugins (from %s):\n", lock_path);
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *name = strtok(line, "|");
        char *ver = strtok(NULL, "|");
        char *entry = strtok(NULL, "|");
        char *path = strtok(NULL, "|\n");
        if (name && ver && entry && path) {
            printf("- %s @ %s (entry=%s) [%s]\n", name, ver, entry, path);
        }
    }
    fclose(f);
}

/* cmd_run
 * Why: Stub to execute a plugin entry via RADS in its install directory.
 * How to upgrade: Resolve plugin.radsmeta entry, chdir to plugin path,
 * set env vars, and spawn rads with forwarded args.
 */
RSTAR_INTERNAL void cmd_run(const char *package, int __attribute__((unused)) argc, char ** __attribute__((unused)) argv) {
    if (!package) {
        fprintf(stderr, "Error: package name required. Try: rstar run ai-discord-bot -- args\n");
        return;
    }
    char root[RSTAR_MAX_PATH];
    if (!plugin_root_path(root, sizeof(root))) {
        fprintf(stderr, "Error: could not resolve plugin root path.\n");
        return;
    }
    char plugin_dir[RSTAR_MAX_PATH];
    if (!join_path(plugin_dir, sizeof(plugin_dir), root, package)) {
        fprintf(stderr, "Error: plugin path too long.\n");
        return;
    }
    char manifest_path[RSTAR_MAX_PATH];
    if (!join_path(manifest_path, sizeof(manifest_path), plugin_dir, "plugin.radsmeta")) {
        fprintf(stderr, "Error: manifest path too long.\n");
        return;
    }
    char entry[256] = "main.rads";
    read_manifest_entry(manifest_path, entry, sizeof(entry));
    printf("🚀 Running plugin %s (entry=%s) from %s\n", package, entry, plugin_dir);
    printf("Hint: future version will chdir to %s and exec: rads %s", plugin_dir, entry);
    if (argc > 0) {
        printf(" with forwarded args (count=%d)", argc);
    }
    printf(".\n");
}

/* fetch_registry_packages
 * Why: Fetch package list from RADS registry via GitHub Actions API
 * How to upgrade: Add local caching, pagination support, error handling
 */
RSTAR_INTERNAL int fetch_registry_packages(const char *query, const char *category, const char *license) {
    char url[512];
    char cmd[1024];

    snprintf(url, sizeof(url), "%s/dispatch", REGISTRY_API_BASE);

    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST %s "
        "-H 'Content-Type: application/json' "
        "-d '{\"event_type\":\"api-search\",\"client_payload\":{",
        "\"query\":\"%s\",\"category\":\"%s\",\"license\":\"%s\"}}'",
        url, query ? query : "", category ? category : "", license ? license : "");

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to connect to registry\n");
        return 1;
    }

    char response[8192];
    size_t response_len = fread(response, 1, sizeof(response), fp);
    response[response_len] = '\0';
    pclose(fp);

    printf("%s\n", response);
    return 0;
}

/* get_package_info
 * Why: Get detailed information about a specific package from registry
 * How to upgrade: Add caching, handle network errors gracefully
 */
RSTAR_INTERNAL int get_package_info(const char *package_name) {
    char url[512];
    char cmd[1024];

    snprintf(url, sizeof(url), "%s/dispatch", REGISTRY_API_BASE);

    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST %s "
        "-H 'Content-Type: application/json' "
        "-d '{\"event_type\":\"api-list-packages\",\"client_payload\":{}}'",
        url);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to connect to registry\n");
        return 1;
    }

    char response[8192];
    size_t response_len = fread(response, 1, sizeof(response), fp);
    response[response_len] = '\0';
    pclose(fp);

    printf("%s\n", response);
    return 0;
}

/* cmd_search
 * Why: Search packages in RADS registry with filters
 * How to upgrade: Add pagination, sorting options, local caching
 */
RSTAR_INTERNAL void cmd_search(const char *query, const char *category, const char *license) {
    if (!query || strlen(query) == 0) {
        fprintf(stderr, "Error: search query required. Try: rstar search database\n");
        return;
    }

    printf("🔍 Searching registry...\n");
    fetch_registry_packages(query, category, license);
}

/* cmd_info
 * Why: Display detailed information about a specific package
 * How to upgrade: Format output, show versions, dependencies
 */
RSTAR_INTERNAL void cmd_info(const char *package_name) {
    if (!package_name || strlen(package_name) == 0) {
        fprintf(stderr, "Error: package name required. Try: rstar info database-sqlite\n");
        return;
    }

    printf("📦 Getting package info...\n");
    get_package_info(package_name);
}

/* cmd_publish
 * Why: Guide user to publish package via GitHub workflow
 * How to upgrade: Automated publishing with CLI flags
 */
RSTAR_INTERNAL void cmd_publish(void) {
    printf("📤 To publish a package:\n\n");
    printf("1. Ensure package.rads exists in repository root\n");
    printf("2. Commit and push changes to GitHub\n");
    printf("3. Create GitHub Release: gh release create v1.0.0\n");
    printf("4. The GitHub Action will automatically publish to registry\n\n");
    printf("See: docs/registry-architecture/PUBLISHING_WORKFLOW.md\n\n");
}

#ifndef RSTAR_TESTING
int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    const char *cmd = argv[1];

    if (strcmp(cmd, "install") == 0) {
        const char *pkg = (argc >= 3) ? argv[2] : NULL;
        cmd_install(pkg);
    } else if (strcmp(cmd, "search") == 0) {
        const char *query = (argc >= 3) ? argv[2] : NULL;
        const char *category = (argc >= 4) ? argv[3] : NULL;
        const char *license = (argc >= 5) ? argv[4] : NULL;
        cmd_search(query, category, license);
    } else if (strcmp(cmd, "info") == 0) {
        const char *pkg = (argc >= 3) ? argv[2] : NULL;
        cmd_info(pkg);
    } else if (strcmp(cmd, "list") == 0) {
        cmd_list();
    } else if (strcmp(cmd, "run") == 0) {
        const char *pkg = (argc >= 3) ? argv[2] : NULL;
        cmd_run(pkg, argc - 3, argv + 3);
    } else if (strcmp(cmd, "publish") == 0) {
        cmd_publish();
    } else {
        print_usage();
        return 1;
    }
    return 0;
}
#endif
