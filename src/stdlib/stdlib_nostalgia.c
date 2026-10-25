/*
 * stdlib_nostalgia.c — the RADS Museum of the 2000s Internet
 *
 * WHY: Master Zarigata decreed easter eggs that remember Habbo Hotel,
 * Club Penguin, early YouTube and the forum meme age. Every language
 * ships a stdlib; only RADS ships a time machine.
 * EXTEND: add new exhibits as one small static table entry + a tiny
 * native wrapper; keep functions under 50 lines, comments on each.
 *
 * ⚕ Forged for Master Zarigata 🜂
 */
#define _POSIX_C_SOURCE 200809L
#include "stdlib_nostalgia.h"
#include <string.h>
#include <stdlib.h>

extern Value make_string(const char* val);
extern Value make_null(void);

/* ── leekspin.net (2006): Orihime and the infinite leek ──
 * Four frames, single-line so scripts can compare with ==.
 * EXTEND: add music by holding your breath and humming. */
static const char* LEEK_FRAMES[] = {
    "(>^-^)~~|== leek ==--      ",
    "<(^-^<)--== leek ==|~~     ",
    " (^o^)\u30ce *leek spin!*       ",
    "\\(^o^)/  L E E E K !       "
};
#define LEEK_FRAME_COUNT 4

/* ── AIM away messages (1999-2005): the original status update ──
 * Deterministic pick by index; default 0 keeps tests stable.
 * EXTEND: add per-buddy customized messages like 2005 you had. */
static const char* AWAY_MESSAGES[] = {
    "brb dinner \xF0\x9F\x8D\xBD\xEF\xB8\x8F",
    "away -- leave one!! xoxo *~*",
    "phone call brb",
    "doing homework (aim = open anyway)",
    "shower brb *steam emoji doesn't exist yet*",
    "idle -- aim me on my celly",
    "gone 2 the mall, bbl"
};
#define AWAY_COUNT 7

Value native_nostalgia_leek_spin(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int step = 0;
    if (argc >= 1 && args[0].type == VAL_INT) step = (int)args[0].int_val;
    if (step < 0) step = -step;               /* negative time not invented until 2010 */
    return make_string(LEEK_FRAMES[step % LEEK_FRAME_COUNT]);
}

Value native_nostalgia_rickroll(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Never gonna give you up \xF0\x9F\x8E\xB5 https://www.youtube.com/watch?v=dQw4w9WgXcQ");
}

Value native_nostalgia_all_your_base(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("In A.D. 2101, war was beginning... ALL YOUR BASE ARE BELONG TO US. "
                       "Somebody set up us the bomb. (Zero Wing, 2001 -- the meme that "
                       "colonized every forum)");
}

Value native_nostalgia_over9000(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("IT'S OVER 9000!!! WHAT?! NINE THOUSAND?! (DBZ, 2006 YouTube)");
}

Value native_nostalgia_leeroy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("LEEROY JENKINS!! ...at least I have chicken. (WoW raid, 2005)");
}

/* ── badgers (2003): variant 0 = mushrooms, 1 = the snake interruption ── */
Value native_nostalgia_badger(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int variant = 0;
    if (argc >= 1 && args[0].type == VAL_INT) variant = (int)args[0].int_val;
    if (variant == 1) {
        return make_string("badger badger badger badger -- SNAKE! SNAKE! OOOH, IT'S A SNAKE!");
    }
    return make_string("badger badger badger badger badger badger -- MUSHROOM! MUSHROOM! (2003)");
}

Value native_nostalgia_pbj_time(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("\xE3\x83\xBE(\xE2\x8C\x90\xE2\x96\x80_\xE2\x96\x80)\xE3\x83\x8E\xE2\x99\xAA IT'S PEANUT BUTTER JELLY TIME!!! (dancing banana, 2002)");
}

Value native_nostalgia_hamster_dance(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("dee da dee da dee di di dee... THE HAMSTER DANCE (1998 -- "
                       "GeoCities' finest rodents, looped forever)");
}

Value native_nostalgia_o_rly(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("O RLY? (the snowy owl, Something Awful 2003)");
}

Value native_nostalgia_ya_rly(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("YA RLY.");
}

Value native_nostalgia_the_game(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("You just lost The Game. You were always playing it. (2003 forum law)");
}

Value native_nostalgia_barrel_roll(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Do a barrel roll! (Star Fox 64, then early Google's best easter egg)");
}

Value native_nostalgia_charlie_bit_me(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Charlie bit me! And that really hurt! (2007 -- early YouTube's "
                       "crown jewel, before the algorithm ate the soul)");
}

Value native_nostalgia_keyboard_cat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Play him off, Keyboard Cat. \xF0\x9F\x90\x88\xF0\x9F\x8E\xB9 (Fatso's 1984 footage, "
                       "meme-canonized 2009)");
}

Value native_nostalgia_dramatic_chipmunk(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("...dramatic turn. DUN DUN DUNNN. (prairie dog zoom, 2007)");
}

Value native_nostalgia_msn_nudge(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("*** You've just been nudged! *** (MSN Messenger shook the "
                       "whole window, and your soul, 1999-2000s)");
}

/* -- Habbo Hotel (2006): the Pool's Closed blockade --
 * On July 6, 2006, raiders in grey suits and enormous afros formed
 * wall-to-wall lines around every Habbo Hotel pool and declared it
 * closed. The mods could not ban them fast enough; the wall held.
 * WHY: the most famous coordinated raid of 2006 -- the museum needs its plaque.
 * EXTEND: a variant arg could return the dress code (grey suit + afro). */
Value native_nostalgia_pool_closed(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("POOL'S CLOSED! (Habbo Hotel, July 2006: a wall of grey suits and afros sealed the pool; the mods banned in vain)");
}

Value native_nostalgia_asl(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("a/s/l? (the three scariest letters on 2000s IRC and AIM)");
}

Value native_nostalgia_away_message(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int idx = 0;
    if (argc >= 1 && args[0].type == VAL_INT) idx = (int)args[0].int_val;
    if (idx < 0) idx = 0;
    return make_string(AWAY_MESSAGES[idx % AWAY_COUNT]);
}

/* ── leetspeak(text): real utility wearing nostalgia clothes ──
 * a->4 e->3 i->1 o->0 s->5 t->7 (case-insensitive), rest untouched.
 * WHY: forums, Counter-Strike names, and 'pwned' Need deterministic
 * output; no locale tricks, pure ASCII mapping.
 * EXTEND: accept a level arg for heavier dialects (ph for f, d4rk). */
Value native_nostalgia_leetspeak(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* src = args[0].string_val;
    size_t n = strlen(src);
    char* out = malloc(n + 1);
    if (out == NULL) return make_null();
    for (size_t i = 0; i < n; i++) {
        char c = src[i];
        switch (c) {
            case 'a': case 'A': out[i] = '4'; break;
            case 'e': case 'E': out[i] = '3'; break;
            case 'i': case 'I': out[i] = '1'; break;
            case 'o': case 'O': out[i] = '0'; break;
            case 's': case 'S': out[i] = '5'; break;
            case 't': case 'T': out[i] = '7'; break;
            default:            out[i] = c;   break;
        }
    }
    out[n] = '\0';
    Value v = make_string(out);
    free(out);
    return v;
}

void stdlib_nostalgia_register(void) {
    register_native("nostalgia.leek_spin",        native_nostalgia_leek_spin);
    register_native("nostalgia.rickroll",         native_nostalgia_rickroll);
    register_native("nostalgia.all_your_base",    native_nostalgia_all_your_base);
    register_native("nostalgia.over9000",         native_nostalgia_over9000);
    register_native("nostalgia.leeroy",           native_nostalgia_leeroy);
    register_native("nostalgia.pool_closed",      native_nostalgia_pool_closed);
    register_native("nostalgia.badger_badger",    native_nostalgia_badger);
    register_native("nostalgia.pbj_time",         native_nostalgia_pbj_time);
    register_native("nostalgia.hamster_dance",    native_nostalgia_hamster_dance);
    register_native("nostalgia.o_rly",            native_nostalgia_o_rly);
    register_native("nostalgia.ya_rly",           native_nostalgia_ya_rly);
    register_native("nostalgia.the_game",         native_nostalgia_the_game);
    register_native("nostalgia.barrel_roll",      native_nostalgia_barrel_roll);
    register_native("nostalgia.charlie_bit_me",   native_nostalgia_charlie_bit_me);
    register_native("nostalgia.keyboard_cat",     native_nostalgia_keyboard_cat);
    register_native("nostalgia.dramatic_chipmunk", native_nostalgia_dramatic_chipmunk);
    register_native("nostalgia.msn_nudge",        native_nostalgia_msn_nudge);
    register_native("nostalgia.asl",              native_nostalgia_asl);
    register_native("nostalgia.away_message",     native_nostalgia_away_message);
    register_native("nostalgia.leetspeak",        native_nostalgia_leetspeak);
}

/* ⚕ Forged for Master Zarigata 🜂 */
