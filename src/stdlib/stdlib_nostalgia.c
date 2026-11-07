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

/* ══════════════════════════════════════════════════════════════════════
 * NEW WING (2026-11-07): The Connection Age, the Video Viral Age,
 * and Forum Law. The museum grows a second floor.
 * WHY: parity shipped; the museum is where RADS remembers being human.
 * Every native follows house law: small, deterministic, commented.
 * ══════════════════════════════════════════════════════════════════════ */

/* ── dial-up modem handshake (1995-2005): the sound of the internet arriving ──
 * Three stages: dial, negotiate, connected. Deterministic by step with
 * wraparound (away_message law) so scripts can replay the whole ritual.
 * EXTEND: a busy-line stage for the authentic 7th redial. */
static const char* DIALUP_STAGES[] = {
    "BRRR BRRR... dialing the ISP. Mom picks up the phone = connection dead",
    "KSSSHHHH-EEEE-AH-AH-AH-OOOOO... the 56k handshake. Nobody ever knew what it said.",
    "CONNECTED 56,000 bps. One image at a time. The internet is FULLY LOADED."
};
#define DIALUP_STAGE_COUNT 3

Value native_nostalgia_dial_up(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int step = 0;
    if (argc >= 1 && args[0].type == VAL_INT) step = (int)args[0].int_val;
    if (step < 0) step = 0;               /* you cannot un-dial the phone */
    return make_string(DIALUP_STAGES[step % DIALUP_STAGE_COUNT]);
}

/* ── Winamp (1997): the llama that whipped ──
 * Static exhibit: the tagline every skin-switching teen knew by heart,
 * plus the visualizer that ate more CPU than the music.
 * EXTEND: skins(idx) table of the classic skin names. */
Value native_nostalgia_winamp(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Winamp, it really whips the llama's ass! (1997 -- the player, "
                       "the skins, the visualizer that ate your Pentium)");
}

/* ── LimeWire (2000-2010): every file was exactly what it claimed ──
 * Static exhibit: peer-to-peer law. The song was never the song.
 * EXTEND: variant arg for the kazaa / bearshare / soulseek dialects. */
Value native_nostalgia_limewire(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Downloading: best_song_ever.mp3.exe ... 4 days remaining. "
                       "(LimeWire, 2000s -- it was never the song)");
}

/* ── GeoCities (1994-2009): under construction forever ──
 * Table pick with wraparound, away_message law. Every homepage had a
 * construction gif, a hit counter, and a guestbook demanding entries.
 * EXTEND: a guestbook() native with signed guest entries. */
static const char* GEOCITIES_LINES[] = {
    "UNDER CONSTRUCTION -- (gif of a digging worker, forever, 1998)",
    "You are visitor number 00001337 -- sign my guestbook!!",
    "Best viewed in 800x600 with Internet Explorer 5",
    "~*~ ThIs Is My HoMePaGe ~*~ e-mail me!! ~*~",
    "MIDI autoplaying. There is no mute. There never was."
};
#define GEOCITIES_COUNT 5

Value native_nostalgia_geocities(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int idx = 0;
    if (argc >= 1 && args[0].type == VAL_INT) idx = (int)args[0].int_val;
    if (idx < 0) idx = 0;
    return make_string(GEOCITIES_LINES[idx % GEOCITIES_COUNT]);
}

/* ── Dancing Baby (1996): the museum's prehistoric wing ──
 * The 3D-rendered cha-cha infant that e-mailed itself between offices
 * years before anyone said "viral video". Every meme below descends from it.
 * EXTEND: frame(idx) animation table, leek_spin law. */
Value native_nostalgia_dancing_baby(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("o-|-<  cha-cha-cha... the Dancing Baby (1996, 256 colors -- "
                       "the root of every exhibit in this building)");
}

/* ── Numa Numa (2004): the joyful lip-sync heard round the world ──
 * One guy, one webcam, one headset, Dragostea Din Tei. Pure happiness,
 * zero budget -- the internet at its most harmless.
 * EXTEND: none. Some exhibits are already perfect. */
Value native_nostalgia_numa_numa(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Vrei sa pleci dar nu ma, nu ma iei -- NUMA NUMA YE! "
                       "(2004, the webcam lip-sync that out-happied everything since)");
}

/* ── Chocolate Rain (2007): the bass that launched a thousand remixes ──
 * Tay Zonday's baritone arrived before autotune did; the on-screen note
 * "I move away from the microphone to breathe in" is museum canon.
 * EXTEND: remix(idx) table; the internet made thousands. */
Value native_nostalgia_chocolate_rain(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    return make_string("Chocolate Rain! Some stay dry and others feel the pain. "
                       "(I move away from the microphone to breathe in -- 2007)");
}

/* ── MySpace Top 8 (2003-2008): friendship, ranked and public ──
 * Placement messages for ranks 1-8; everything else fell to the unranked
 * masses. Deterministic: the drama must be reproducible.
 * EXTEND: bulletin() native for the passive-aggressive follow-up post. */
Value native_nostalgia_myspace_top8(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int place = 0;
    if (argc >= 1 && args[0].type == VAL_INT) place = (int)args[0].int_val;
    switch (place) {
        case 1:
            return make_string("#1 -- best friend forever. The top spot was sacred. (MySpace Top 8)");
        case 2: case 3:
            return make_string("#2-3 -- inner circle. One fight from the demotion zone.");
        case 4: case 5: case 6:
            return make_string("#4-6 -- stable mid-table. Nobody checks this deep.");
        case 7: case 8:
            return make_string("#7-8 -- the demotion zone. You saw yourself here. You said nothing.");
        default:
            return make_string("unranked. the Top 8 is full. the friendship survives on profile comments alone.");
    }
}

/* ── Chuck Norris facts (2005): forum law in its final form ──
 * Table pick with wraparound, away_message law. This museum keeps the
 * compiler-flavored subset, as is right and proper.
 * EXTEND: more facts; the forums of 2005 never truly ran out. */
static const char* CHUCK_FACTS[] = {
    "Chuck Norris counted to infinity. Twice. (2005 forum law)",
    "Chuck Norris's keyboard has no Ctrl key -- nothing controls Chuck Norris.",
    "When Chuck Norris throws an exception, the runtime apologizes.",
    "Chuck Norris doesn't unit test. The code proves itself out of respect.",
    "Chuck Norris can divide by zero. The result is classified."
};
#define CHUCK_FACT_COUNT 5

Value native_nostalgia_chuck_norris(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    int idx = 0;
    if (argc >= 1 && args[0].type == VAL_INT) idx = (int)args[0].int_val;
    if (idx < 0) idx = 0;
    return make_string(CHUCK_FACTS[idx % CHUCK_FACT_COUNT]);
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
    register_native("nostalgia.dial_up",           native_nostalgia_dial_up);
    register_native("nostalgia.winamp",            native_nostalgia_winamp);
    register_native("nostalgia.limewire",          native_nostalgia_limewire);
    register_native("nostalgia.geocities",         native_nostalgia_geocities);
    register_native("nostalgia.dancing_baby",      native_nostalgia_dancing_baby);
    register_native("nostalgia.numa_numa",         native_nostalgia_numa_numa);
    register_native("nostalgia.chocolate_rain",    native_nostalgia_chocolate_rain);
    register_native("nostalgia.myspace_top8",      native_nostalgia_myspace_top8);
    register_native("nostalgia.chuck_norris",      native_nostalgia_chuck_norris);
}

/* ⚕ Forged for Master Zarigata 🜂 */
