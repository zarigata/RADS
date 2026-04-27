#define _POSIX_C_SOURCE 200809L
#include "stdlib_color.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);

/* ── ANSI color/style module ──
 * Provides terminal color, background, and style helpers.
 * Zero external dependencies — builds everywhere.
 */

/* Helper: wrap text with ANSI escape */
static Value ansi_wrap(const char* code, int argc, Value* args) {
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* text = args[0].string_val;
    size_t tlen = strlen(text);
    size_t clen = strlen(code);
    /* \033[<code>m<text>\033[0m */
    char* buf = malloc(clen + tlen + 12);
    if (!buf) return make_null();
    int len = snprintf(buf, clen + tlen + 12, "\033[%sm%s\033[0m", code, text);
    Value v = make_string(buf);
    free(buf);
    return v;
}

/* ── Foreground colors ── */
Value stdlib_color_black(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("30", argc, args);
}
Value stdlib_color_red(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("31", argc, args);
}
Value stdlib_color_green(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("32", argc, args);
}
Value stdlib_color_yellow(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("33", argc, args);
}
Value stdlib_color_blue(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("34", argc, args);
}
Value stdlib_color_magenta(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("35", argc, args);
}
Value stdlib_color_cyan(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("36", argc, args);
}
Value stdlib_color_white(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("37", argc, args);
}

/* ── Bright foreground colors ── */
Value stdlib_color_bright_red(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("91", argc, args);
}
Value stdlib_color_bright_green(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("92", argc, args);
}
Value stdlib_color_bright_yellow(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("93", argc, args);
}
Value stdlib_color_bright_blue(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("94", argc, args);
}
Value stdlib_color_bright_magenta(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("95", argc, args);
}
Value stdlib_color_bright_cyan(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("96", argc, args);
}
Value stdlib_color_bright_white(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("97", argc, args);
}

/* ── Background colors ── */
Value stdlib_color_bg_black(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("40", argc, args);
}
Value stdlib_color_bg_red(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("41", argc, args);
}
Value stdlib_color_bg_green(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("42", argc, args);
}
Value stdlib_color_bg_yellow(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("43", argc, args);
}
Value stdlib_color_bg_blue(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("44", argc, args);
}
Value stdlib_color_bg_magenta(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("45", argc, args);
}
Value stdlib_color_bg_cyan(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("46", argc, args);
}
Value stdlib_color_bg_white(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("47", argc, args);
}

/* ── Styles ── */
Value stdlib_color_bold(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("1", argc, args);
}
Value stdlib_color_dim(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("2", argc, args);
}
Value stdlib_color_italic(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("3", argc, args);
}
Value stdlib_color_underline(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("4", argc, args);
}
Value stdlib_color_blink(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("5", argc, args);
}
Value stdlib_color_reverse(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("7", argc, args);
}
Value stdlib_color_hidden(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("8", argc, args);
}
Value stdlib_color_strikethrough(Interpreter* interp, int argc, Value* args) {
    (void)interp; return ansi_wrap("9", argc, args);
}

/* ── Utilities ── */

/* color.rgb(r, g, b, text) - 256-color support */
Value stdlib_color_rgb(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 4 || args[0].type != VAL_INT || args[1].type != VAL_INT ||
        args[2].type != VAL_INT || args[3].type != VAL_STRING)
        return make_null();

    int r = (int)args[0].int_val;
    int g = (int)args[1].int_val;
    int b = (int)args[2].int_val;
    const char* text = args[3].string_val;

    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;

    size_t tlen = strlen(text);
    char* buf = malloc(tlen + 64);
    if (!buf) return make_null();
    int len = snprintf(buf, tlen + 64, "\033[38;2;%d;%d;%dm%s\033[0m", r, g, b, text);
    Value v = make_string(buf);
    free(buf);
    return v;
}

/* color.bg_rgb(r, g, b, text) - 256-color background */
Value stdlib_color_bg_rgb(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 4 || args[0].type != VAL_INT || args[1].type != VAL_INT ||
        args[2].type != VAL_INT || args[3].type != VAL_STRING)
        return make_null();

    int r = (int)args[0].int_val;
    int g = (int)args[1].int_val;
    int b = (int)args[2].int_val;
    const char* text = args[3].string_val;

    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;

    size_t tlen = strlen(text);
    char* buf = malloc(tlen + 64);
    if (!buf) return make_null();
    snprintf(buf, tlen + 64, "\033[48;2;%d;%d;%dm%s\033[0m", r, g, b, text);
    Value v = make_string(buf);
    free(buf);
    return v;
}

/* color.strip(text) - Remove all ANSI escape sequences */
Value stdlib_color_strip(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();

    const char* src = args[0].string_val;
    size_t slen = strlen(src);
    char* out = malloc(slen + 1);
    if (!out) return make_null();

    size_t j = 0;
    for (size_t i = 0; i < slen; ) {
        if (src[i] == '\033' && i + 1 < slen && src[i + 1] == '[') {
            /* Skip until terminator letter */
            i += 2;
            while (i < slen && !((src[i] >= 'A' && src[i] <= 'Z') ||
                                 (src[i] >= 'a' && src[i] <= 'z')))
                i++;
            if (i < slen) i++;
        } else {
            out[j++] = src[i++];
        }
    }
    out[j] = '\0';
    Value v = make_string(out);
    free(out);
    return v;
}

/* color.code(n, text) - Apply arbitrary ANSI code */
Value stdlib_color_code(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_null();
    return ansi_wrap(args[0].string_val, 1, &args[1]);
}

/* color.reset() - Return reset escape string */
Value stdlib_color_reset(Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("\033[0m");
}

/* color.supports() - Check if stdout is a terminal (basic check) */
Value stdlib_color_supports(Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_bool(isatty(STDOUT_FILENO));
}

void stdlib_color_register(void) {
    /* Foreground */
    register_native("color.black", stdlib_color_black);
    register_native("color.red", stdlib_color_red);
    register_native("color.green", stdlib_color_green);
    register_native("color.yellow", stdlib_color_yellow);
    register_native("color.blue", stdlib_color_blue);
    register_native("color.magenta", stdlib_color_magenta);
    register_native("color.cyan", stdlib_color_cyan);
    register_native("color.white", stdlib_color_white);
    /* Bright foreground */
    register_native("color.bright_red", stdlib_color_bright_red);
    register_native("color.bright_green", stdlib_color_bright_green);
    register_native("color.bright_yellow", stdlib_color_bright_yellow);
    register_native("color.bright_blue", stdlib_color_bright_blue);
    register_native("color.bright_magenta", stdlib_color_bright_magenta);
    register_native("color.bright_cyan", stdlib_color_bright_cyan);
    register_native("color.bright_white", stdlib_color_bright_white);
    /* Background */
    register_native("color.bg_black", stdlib_color_bg_black);
    register_native("color.bg_red", stdlib_color_bg_red);
    register_native("color.bg_green", stdlib_color_bg_green);
    register_native("color.bg_yellow", stdlib_color_bg_yellow);
    register_native("color.bg_blue", stdlib_color_bg_blue);
    register_native("color.bg_magenta", stdlib_color_bg_magenta);
    register_native("color.bg_cyan", stdlib_color_bg_cyan);
    register_native("color.bg_white", stdlib_color_bg_white);
    /* Styles */
    register_native("color.bold", stdlib_color_bold);
    register_native("color.dim", stdlib_color_dim);
    register_native("color.italic", stdlib_color_italic);
    register_native("color.underline", stdlib_color_underline);
    register_native("color.blink", stdlib_color_blink);
    register_native("color.reverse", stdlib_color_reverse);
    register_native("color.hidden", stdlib_color_hidden);
    register_native("color.strikethrough", stdlib_color_strikethrough);
    /* Utilities */
    register_native("color.rgb", stdlib_color_rgb);
    register_native("color.bg_rgb", stdlib_color_bg_rgb);
    register_native("color.strip", stdlib_color_strip);
    register_native("color.code", stdlib_color_code);
    register_native("color.reset", stdlib_color_reset);
    register_native("color.supports", stdlib_color_supports);
}
