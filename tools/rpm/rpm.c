#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// RPM - RADS Package Manager
// "Blast your projects with plugins!"

void print_usage() {
    printf("RPM - RADS Package Manager v0.1.0\n");
    printf("Usage: rpm <command> [args]\n\n");
    printf("Commands:\n");
    printf("  init       Initialize a new RADS package\n");
    printf("  install    Install a package (e.g. rpm install media-kit)\n");
    printf("  list       List installed packages\n");
    printf("  help       Show this help\n");
}

void cmd_init() {
    FILE* f = fopen("package.rads", "w");
    if (!f) {
        fprintf(stderr, "Error: Could not create package.rads\n");
        return;
    }
    
    fprintf(f, "name = \"my-rads-project\"\n");
    fprintf(f, "version = \"0.1.0\"\n");
    fprintf(f, "dependencies = []\n");
    fclose(f);
    
    printf("🚀 Initialized new RADS package environment!\n");
    printf("Created package.rads\n");
}

void cmd_install(const char* package) {
    if (!package) {
        fprintf(stderr, "Error: Package name required\n");
        return;
    }
    
    printf("📦 Downloading %s...\n", package);
    // Dummy download simulation
    printf("⚡ Extracting...\n");
    printf("✅ Installed %s successfully!\n", package);
    
    // Append to package.rads if exists
    FILE* f = fopen("package.rads", "a");
    if (f) {
        fprintf(f, "# Dependency: %s\n", package);
        fclose(f);
    }
}

void cmd_list() {
    printf("Installed Packages:\n");
    FILE* f = fopen("package.rads", "r");
    if (!f) {
        printf("(No package.rads found)\n");
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "# Dependency:")) {
            printf("- %s", line + 14); // Skip "# Dependency: "
        }
    }
    fclose(f);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    const char* cmd = argv[1];
    
    if (strcmp(cmd, "init") == 0) {
        cmd_init();
    } else if (strcmp(cmd, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: rpm install <package>\n");
            return 1;
        }
        cmd_install(argv[2]);
    } else if (strcmp(cmd, "list") == 0) {
        cmd_list();
    } else {
        print_usage();
        return 1;
    }
    
    return 0;
}
