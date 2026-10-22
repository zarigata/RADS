// RADS decimal module — Python-parity arbitrary-precision decimals.
//
// Mission: 0.1 + 0.2 == 0.3 must be *exactly* true in RADS, the way it is
// with Python's decimal.Decimal and the way it never is with IEEE doubles.
// RADS has no bigint core, so this file IS the engine: sign/scale limbs in
// base 1e9, add/sub/mul with full precision, correctly-rounded long
// division to context precision with ROUND_HALF_EVEN (the CPython
// default), integer powers, sqrt via Newton on scaled integers, and
// CPython-exact str() formatting. All numeric corners pinned against
// python3.13 decimal on this box (pins recorded in tests/test_decimal.rads).
//
// Representation: value = sign * (sum limb[i] * 1e9^i) * 10^-scale.
//   sign  : +1 / -1. Signed zero is kept from input ("−0" round-trips,
//           exact cancels in add yield +0) — matches CPython.
//   scale : digits after the point. MAY BE NEGATIVE — that is how CPython
//           stores exponents ("1E+25" = coefficient 1, scale -25) and how
//           normalize()/quantize() to negative places express themselves.
//   limbs : base-1e9 digits, little-endian, trailing zero limbs allowed
//           (exact coefficient identity — 1.50 vs 1.5 — is the point).
//
// Specials: NaN / +-Infinity parse, compare (IEEE total order) and answer
// predicates; arithmetic on them errors to stderr + returns null (Python
// raises InvalidOperation — RADS has no exceptions in natives yet).
//
// Division family is TRUNCATED in the decimal-spec sense (pins: -7//2 =
// -3, 7//-2 = -3, -7%2 = -1, 7%-2 = 1) — quotient toward zero, remainder
// sign follows the dividend. NOT Python's int floor-mod.
//
// Zero new deps: C11 stdlib only. VW Fusca law holds.
// ⚕ Forged for Master Zarigata 🜂

#include "stdlib_decimal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Value make_error(const char* msg);

// ---------------------------------------------------------------------------
// Core bignum: kind + sign + base-1e9 limbs + decimal scale (may be < 0).
// ---------------------------------------------------------------------------

enum { DEC_NAN = 0, DEC_INF = 1, DEC_NORMAL = 2 };

typedef struct {
    int kind;            // DEC_* tag
    int sign;            // +1 / -1 (normal & inf)
    long scale;          // value = coef * 10^-scale; negative allowed
    int nlimbs;          // limb count, >= 1 (limb[0] least significant)
    unsigned int* limbs; // base 1e9, little-endian
} Dec;

// Context precision for divide/sqrt/pow — CPython default context.
static long g_precision = 28;

// Runaway guard: ~36000 decimal digits is far past anything a script
// needs; refuse instead of thrashing the allocator.
#define DEC_MAX_LIMBS 4000

static bool dec_is_zero(const Dec* d) {
    if (d->kind != DEC_NORMAL) return false;
    for (int i = 0; i < d->nlimbs; i++) if (d->limbs[i]) return false;
    return true;
}

// Drop leading zero limbs (keep at least one so limbs stays valid).
static void dec_trim(Dec* d) {
    while (d->nlimbs > 1 && d->limbs[d->nlimbs - 1] == 0) d->nlimbs--;
}

// Realloc to exactly n limbs, zero-filling new tail slots.

// Deep copy — limbs duplicated, copies release independently.
static bool dec_copy(Dec* out, const Dec* in) {
    memset(out, 0, sizeof(Dec));
    out->kind = in->kind;
    out->sign = in->sign;
    out->scale = in->scale;
    int n = in->nlimbs > 0 ? in->nlimbs : 1;
    out->limbs = malloc((size_t)n * sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = n;
    for (int i = 0; i < in->nlimbs && i < n; i++) out->limbs[i] = in->limbs[i];
    for (int i = in->nlimbs; i < n; i++) out->limbs[i] = 0;
    return true;
}

static void dec_free(Dec* d) {
    free(d->limbs);
    d->limbs = NULL;
    d->nlimbs = 0;
}

// Significant decimal digits of the coefficient (zero -> 1).
static long dec_num_digits(const Dec* d) {
    if (d->kind != DEC_NORMAL) return 1; // specials carry no coefficient
    if (dec_is_zero(d)) return 1;
    int top = d->nlimbs - 1;
    while (top > 0 && d->limbs[top] == 0) top--;
    unsigned int v = d->limbs[top];
    long digits = (long)top * 9;
    while (v) { digits++; v /= 10; }
    return digits;
}

// In-place coefficient *= 10^k (k > 0); scale deliberately untouched —
// callers adjust it to keep the value invariant. Fresh-alloc up-shift
static bool dec_scale_up(Dec* d, long k) {
    if (k <= 0 || d->kind != DEC_NORMAL) return true;
    if (dec_is_zero(d)) return true;
    long whole = k / 9, part = k % 9;
    int add = (int)whole;
    int newn = d->nlimbs + add + 1;
    if (newn > DEC_MAX_LIMBS) return false;
    unsigned int* p = calloc((size_t)newn, sizeof(unsigned int));
    if (!p) return false;
    memcpy(&p[add], d->limbs, (size_t)d->nlimbs * sizeof(unsigned int));
    free(d->limbs);
    d->limbs = p;
    d->nlimbs = newn;
    if (part > 0) {
        unsigned int mult = 1;
        for (long i = 0; i < part; i++) mult *= 10;
        unsigned long long carry = 0;
        for (int i = 0; i < d->nlimbs; i++) {
            unsigned long long cur = (unsigned long long)d->limbs[i] * mult + carry;
            d->limbs[i] = (unsigned int)(cur % 1000000000u);
            carry = cur / 1000000000u;
        }
        // top limb headroom was allocated (+1); carry always fits
    }
    dec_trim(d);
    return true;
}

// ---------------------------------------------------------------------------
// Construction from a bare digit run (no sign, no dot, leading zeros ok).
// scale may be negative (the CPython negative-exponent form: "1E+25" is
// coefficient 1 at scale -25). Sign applied separately; "-0" keeps sign.
// ---------------------------------------------------------------------------

static bool dec_from_digits(Dec* out, const char* digits, long scale, int sign) {
    // Skip leading zeros but remember whether the value is zero.
    const char* p = digits;
    while (*p == '0' && p[1] != '\0') p++;
    size_t nd = strlen(p);
    // NB: does NOT free out->limbs -- callers own the input state; the
    // in-place rebuild paths (round_context, quantize, sqrt) free first.
    out->kind = DEC_NORMAL;
    out->sign = sign;
    out->scale = scale;
    int nlimbs = (int)((nd + 8) / 9);
    if (nlimbs < 1) nlimbs = 1;
    if (nlimbs > DEC_MAX_LIMBS) return false;
    out->limbs = calloc((size_t)nlimbs, sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = nlimbs;
    // Fill from least significant decimal digit upward.
    long pos = 0;
    for (long i = (long)nd - 1; i >= 0; i--) {
        int dv = p[i] - '0';
        if (dv < 0 || dv > 9) { dec_free(out); return false; }
        long limb = pos / 9;
        int off = (int)(pos % 9);
        unsigned int mult = 1;
        for (int t = 0; t < off; t++) mult *= 10;
        out->limbs[limb] += (unsigned int)dv * mult;
        pos++;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Parsing: [+-]?digits[.digits]?(eE[+-]digits)? / Infinity / NaN / sNaN.
// Whitespace tolerated. Everything else (empty, bare dot, trailing junk,
// exponent > 1e6) is a conversational parse failure (false).
// ---------------------------------------------------------------------------

static bool dec_parse(const char* s, Dec* out) {
    memset(out, 0, sizeof(Dec));
    while (isspace((unsigned char)*s)) s++;
    out->sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') out->sign = -1;
        s++;
    }
    if (strcasecmp(s, "Infinity") == 0 || strcasecmp(s, "inf") == 0) {
        out->kind = DEC_INF;
        return true;
    }
    if (strcasecmp(s, "NaN") == 0 || strcasecmp(s, "sNaN") == 0
        || strcasecmp(s, "nan") == 0) {
        out->kind = DEC_NAN;
        return true;
    }
    const char* p = s;
    size_t int_digits = 0, frac_digits = 0;
    while (isdigit((unsigned char)*p)) { int_digits++; p++; }
    if (*p == '.') {
        p++;
        while (isdigit((unsigned char)*p)) { frac_digits++; p++; }
    }
    if (int_digits + frac_digits == 0) return false;
    long expo = 0;
    if (*p == 'e' || *p == 'E') {
        p++;
        bool neg = false;
        if (*p == '+' || *p == '-') { neg = (*p == '-'); p++; }
        if (!isdigit((unsigned char)*p)) return false;
        while (isdigit((unsigned char)*p)) {
            expo = expo * 10 + (*p - '0');
            if (expo > 1000000L) return false;
            p++;
        }
        if (neg) expo = -expo;
    }
    while (isspace((unsigned char)*p)) p++;
    if (*p != '\0') return false;
    // Concatenate the digit run (dot skipped) and keep the exponent as a
    // possibly-negative scale — zero-folding happens nowhere (CPython law).
    size_t total = int_digits + frac_digits;
    char* run = malloc(total + 1);
    if (!run) return false;
    size_t w = 0;
    for (size_t i = 0; i < int_digits; i++) run[w++] = s[i];
    for (size_t i = 0; i < frac_digits; i++) run[w++] = s[int_digits + 1 + i];
    run[w] = '\0';
    bool ok = dec_from_digits(out, run, (long)frac_digits - expo, out->sign);
    free(run);
    return ok;
}

// ---------------------------------------------------------------------------
// Formatting — CPython str() exactly (pins in tests/test_decimal.rads):
//   adjusted = nd - 1 - scale
//   zero:     scale 0 -> "0"/"-0"; 1..6 -> "0.000..."; >6 -> "0E-<scale>";
//             <0 -> "0E+<|scale|>"
//   nonzero:  scientific iff scale < 0 (positive exponent stored) or
//             adjusted < -6; mantissa d[.ddd]E+xx (sign always, no pad)
//   plain:    integer part = first adjusted+1 digits, "0.00" pad when
//             adjusted < 0
// ---------------------------------------------------------------------------

// i-th significant decimal digit, 0 = most significant.
static char dec_digit_at(const Dec* d, long i) {
    long nd = dec_num_digits(d);
    if (i < 0 || i >= nd) return '0';
    long from_lsd = nd - 1 - i;
    long limb = from_lsd / 9;
    int off = (int)(from_lsd % 9);
    unsigned int mult = 1;
    for (int t = 0; t < off; t++) mult *= 10;
    unsigned int v = (d->limbs[limb] / mult) % 10;
    return (char)('0' + v);
}

// Coefficient digits, most significant first, leading zeros stripped.
static char* dec_coef_string(const Dec* d) {
    long nd = dec_num_digits(d);
    char* buf = malloc((size_t)nd + 1);
    if (!buf) return NULL;
    for (long i = 0; i < nd; i++) buf[i] = dec_digit_at(d, i);
    buf[nd] = '\0';
    return buf;
}

// Scientific form helper (nonzero coefficient only).
static char* dec_sci_string(const Dec* d, long adjusted) {
    char* coef = dec_coef_string(d);
    if (!coef) return NULL;
    size_t len = strlen(coef);
    char* out = malloc(len + 20);
    if (!out) { free(coef); return NULL; }
    int pos = 0;
    if (d->sign < 0) out[pos++] = '-';
    out[pos++] = coef[0];
    if (len > 1) {
        out[pos++] = '.';
        memcpy(&out[pos], &coef[1], len - 1);
        pos += (int)(len - 1);
    }
    pos += sprintf(&out[pos], "E%+ld", adjusted);
    out[pos] = '\0';
    free(coef);
    return out;
}

// Main formatter: plain or scientific per the pinned rules.
static char* dec_to_string(const Dec* d) {
    if (d->kind == DEC_NAN) return strdup("NaN");
    if (d->kind == DEC_INF) return strdup(d->sign < 0 ? "-Infinity" : "Infinity");
    long nd = dec_num_digits(d);
    long adjusted = nd - 1 - d->scale;
    if (dec_is_zero(d)) {
        // Signed zero round-trips ("-0" -> "-0"), matching CPython.
        const char* neg = d->sign < 0 ? "-" : "";
        if (d->scale == 0) {
            char* b = malloc(3);
            if (!b) return NULL;
            snprintf(b, 3, "%s0", neg);
            return b;
        }
        if (d->scale > 0 && d->scale <= 6) {
            char* b = malloc((size_t)d->scale + 4);
            if (!b) return NULL;
            snprintf(b, (size_t)d->scale + 4, "%s0.%0*ld", neg, (int)d->scale, 0L);
            return b;
        }
        char b[40];
        if (d->scale < 0) snprintf(b, sizeof(b), "%s0E+%ld", neg, -d->scale);
        else snprintf(b, sizeof(b), "%s0E-%ld", neg, d->scale);
        return strdup(b);
    }
    if (d->scale < 0 || adjusted < -6) return dec_sci_string(d, adjusted);
    // Plain decimal with inserted point.
    char* coef = dec_coef_string(d);
    if (!coef) return NULL;
    size_t len = strlen(coef);
    size_t zeros = adjusted < 0 ? (size_t)(-adjusted - 1) : 0;
    size_t cap = len + zeros + 4;
    char* out = malloc(cap);
    if (!out) { free(coef); return NULL; }
    char* w = out;
    if (d->sign < 0) *w++ = '-';
    if (adjusted >= 0) {
        long ip = adjusted + 1;
        memcpy(w, coef, (size_t)ip);
        w += ip;
        if ((long)len > ip) {
            *w++ = '.';
            memcpy(w, &coef[ip], len - (size_t)ip);
            w += len - (size_t)ip;
        }
    } else {
        *w++ = '0'; *w++ = '.';
        for (size_t g = 0; g < zeros; g++) *w++ = '0';
        memcpy(w, coef, len);
        w += len;
    }
    *w = '\0';
    free(coef);
    return out;
}

// ---------------------------------------------------------------------------
// Comparison — IEEE total order (NaN unordered, -Inf < everything).
// ---------------------------------------------------------------------------

static int limbs_cmp(const unsigned int* a, int na, const unsigned int* b, int nb) {
    int ha = na - 1; while (ha >= 0 && a[ha] == 0) ha--;
    int hb = nb - 1; while (hb >= 0 && b[hb] == 0) hb--;
    if (ha != hb) return ha < hb ? -1 : 1;
    for (int i = ha; i >= 0; i--) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

static int dec_cmp(const Dec* a, const Dec* b) {
    if (a->kind == DEC_NAN || b->kind == DEC_NAN) return -2; // unordered
    if (a->kind == DEC_INF && b->kind == DEC_INF)
        return a->sign == b->sign ? 0 : (a->sign < b->sign ? -1 : 1);
    if (a->kind == DEC_INF) return a->sign < 0 ? -1 : 1;
    if (b->kind == DEC_INF) return b->sign > 0 ? -1 : 1;
    bool az = dec_is_zero(a), bz = dec_is_zero(b);
    if (az && bz) return 0;
    if (az) return b->sign < 0 ? 1 : -1;
    if (bz) return a->sign < 0 ? -1 : 1;
    if (a->sign != b->sign) return a->sign < 0 ? -1 : 1;
    // same sign: compare by adjusted exponent, then aligned coefficient
    long adj_a = dec_num_digits(a) - 1 - a->scale;
    long adj_b = dec_num_digits(b) - 1 - b->scale;
    int mag;
    if (adj_a != adj_b) {
        mag = adj_a < adj_b ? -1 : 1;
    } else {
        Dec ta, tb;
        if (!dec_copy(&ta, a) || !dec_copy(&tb, b)) return -2;
        long target = ta.scale > tb.scale ? ta.scale : tb.scale;
        bool ok = dec_scale_up(&ta, target - ta.scale);
        ok = ok && dec_scale_up(&tb, target - tb.scale);
        if (!ok) { dec_free(&ta); dec_free(&tb); return -2; }
        mag = limbs_cmp(ta.limbs, ta.nlimbs, tb.limbs, tb.nlimbs);
        dec_free(&ta); dec_free(&tb);
        if (mag == -2) return -2;
    }
    return a->sign < 0 ? -mag : mag;
}

// ---------------------------------------------------------------------------
// Add/sub — align to the coarser scale (finer coefficient scaled up),
// then magnitude add or sub per signs. Exactness preserved (no rounding
// in decimal + - *).
// ---------------------------------------------------------------------------

// Unsigned limb add (any lengths). out reset first.
static bool limbs_add(unsigned int* a, int na, unsigned int* b, int nb, Dec* out) {
    int n = (na > nb ? na : nb) + 1;
    free(out->limbs);
    out->limbs = calloc((size_t)n, sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = n;
    unsigned long long carry = 0;
    for (int i = 0; i < n; i++) {
        unsigned long long s = carry
            + (unsigned long long)(i < na ? a[i] : 0)
            + (unsigned long long)(i < nb ? b[i] : 0);
        out->limbs[i] = (unsigned int)(s % 1000000000u);
        carry = s / 1000000000u;
    }
    dec_trim(out);
    return true;
}

// Unsigned limb sub a - b (a >= b). Fresh result buffer.
static bool limbs_sub(unsigned int* a, int na, unsigned int* b, int nb, Dec* out) {
    unsigned int* res = malloc((size_t)na * sizeof(unsigned int));
    if (!res) return false;
    long long borrow = 0;
    for (int i = 0; i < na; i++) {
        long long ai = a[i];
        long long bi = i < nb ? b[i] : 0;
        long long d = ai - bi - borrow;
        if (d < 0) { d += 1000000000LL; borrow = 1; } else borrow = 0;
        res[i] = (unsigned int)d;
    }
    free(out->limbs);
    out->limbs = res;
    out->nlimbs = na;
    dec_trim(out);
    return true;
}

// Align copies of a and b to a common scale by scaling the COARSER one
// up (multiply coefficient by 10^diff, lower its scale): 2.50 (scale 2)
// and 1 (scale 0) both land at scale 2 as 250 / 100. The old version
// scaled the wrong operand and every division since birth was off.
static bool dec_align(const Dec* a, const Dec* b, Dec* ta, Dec* tb) {
    if (!dec_copy(ta, a) || !dec_copy(tb, b)) return false;
    if (ta->scale == tb->scale) return true;
    Dec* coarser = ta->scale < tb->scale ? ta : tb;
    Dec* finer = ta->scale < tb->scale ? tb : ta;
    long diff = finer->scale - coarser->scale;
    if (!dec_scale_up(coarser, diff)) return false;
    coarser->scale = finer->scale;
    return true;
}

static bool round_sig(char** digits_io, long* scale_io, long n, bool sticky);

// Context rounding (the CPython law this engine was missing): add, sub,
// mul and div all round their RESULT to g_precision significant digits
// when it exceeds it (pins: 1E30+1E-30 -> 28 digits, big29*1 -> 1.0E+28,
// big29/2 -> 5.0E+27). Exact coefficient shape is preserved when digits
// <= precision (1.50*2.0 = 3.000 keeps its zeros).
static bool dec_round_context(Dec* d) {
    if (d->kind != DEC_NORMAL) return true;
    long nd = dec_num_digits(d);
    if (nd <= g_precision) return true;
    char* ds = dec_coef_string(d);
    if (!ds) return false;
    long scale = d->scale;
    bool sticky = nd > g_precision + 1;
    if (!round_sig(&ds, &scale, g_precision, sticky)) { free(ds); return false; }
    int sign = d->sign;
    dec_free(d); // rebuilding in place: release the old coefficient first
    bool ok = dec_from_digits(d, ds, scale, sign);
    free(ds);
    return ok;
}

// out = a + b (kind NORMAL only; specials handled by the native layer).
static bool dec_add(const Dec* a, const Dec* b, Dec* out) {
    Dec ta, tb;
    if (!dec_align(a, b, &ta, &tb)) return false;
    out->kind = DEC_NORMAL;
    bool ok;
    if (ta.sign == tb.sign) {
        out->sign = ta.sign;
        out->scale = ta.scale;
        ok = limbs_add(ta.limbs, ta.nlimbs, tb.limbs, tb.nlimbs, out);
    } else {
        int mag = limbs_cmp(ta.limbs, ta.nlimbs, tb.limbs, tb.nlimbs);
        if (mag == 0) {
            // exact cancel -> +0 at the aligned scale (CPython: 1.5 + -1.5 = 2.0E+0? no:
            // 0.0 at scale 1... pinned: 0.1 + -0.1 = 0.0)
            out->sign = 1;
            out->scale = ta.scale;
            free(out->limbs);
            out->limbs = calloc(1, sizeof(unsigned int));
            if (!out->limbs) { ok = false; goto done; }
            out->nlimbs = 1;
            ok = true;
        } else if (mag > 0) {
            out->sign = ta.sign;
            out->scale = ta.scale;
            ok = limbs_sub(ta.limbs, ta.nlimbs, tb.limbs, tb.nlimbs, out);
        } else {
            out->sign = tb.sign;
            out->scale = tb.scale;
            ok = limbs_sub(tb.limbs, tb.nlimbs, ta.limbs, ta.nlimbs, out);
        }
    }
done:
    if (ok && dec_is_zero(out) && out->sign < 0) out->sign = 1; // no -0 from cancel
    if (ok) ok = dec_round_context(out);
    dec_free(&ta); dec_free(&tb);
    return ok;
}

// ---------------------------------------------------------------------------
// Multiply: schoolbook with carry, scales add. Exact (decimal never
// rounds * / + -).
// ---------------------------------------------------------------------------

static bool dec_mul(const Dec* a, const Dec* b, Dec* out) {
    out->kind = DEC_NORMAL;
    out->sign = a->sign * b->sign;
    out->scale = a->scale + b->scale;
    int n = a->nlimbs + b->nlimbs;
    if (n > DEC_MAX_LIMBS) return false;
    free(out->limbs);
    out->limbs = calloc((size_t)n, sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = n;
    for (int i = 0; i < a->nlimbs; i++) {
        unsigned long long carry = 0;
        for (int j = 0; j < b->nlimbs; j++) {
            unsigned long long cur = (unsigned long long)a->limbs[i] * b->limbs[j]
                                   + out->limbs[i + j] + carry;
            out->limbs[i + j] = (unsigned int)(cur % 1000000000u);
            carry = cur / 1000000000u;
        }
        int k = i + b->nlimbs;
        while (carry) {
            unsigned long long cur = (unsigned long long)out->limbs[k] + carry;
            out->limbs[k] = (unsigned int)(cur % 1000000000u);
            carry = cur / 1000000000u;
            k++;
        }
        if (k > n - 1 && carry) return false; // unreachable guard
    }
    dec_trim(out);
    if (dec_is_zero(out)) out->sign = 1;
    return dec_round_context(out);
}


// ---------------------------------------------------------------------------
// Divide — binary long division on aligned integer coefficients with
// half-even rounding at context precision. Simpler than Knuth D and
// plenty fast at scripting scale (per-bit subtract with shifts).
//
//   align both to common scale S, giving integers N = A*10^(shift),
//   D = B. Compute integer Q, R with N = Q*D + R, 0 <= R < D.
//   quotient value = Q * 10^-(shift)  [scale = shift]
//   then round to g_precision digits, half-even; exact results keep
//   their exact digits (no zero-padding, no rounding).
//   Zero dividend: +0 at dividend scale (0.00/8 = 0.00 pinned).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Divide — limb-wise schoolbook long division on aligned integer
// coefficients, fraction digits 9 at a time, ROUND_HALF_EVEN at context
// precision. Exact results keep exact digits (no padding, no rounding)
// and strip trailing zeros only down to the ideal scale sa - sb
// (pins: 2.50/1 = 2.50, 10/2.5 = 4, 10000000000000000000000000001/2
// keeps all 28 digits, 1/0.3 = 3.333...3 with 28 threes).
// ---------------------------------------------------------------------------

// u *= m in place (m < 1e9). Grows u by >= 1 limb. n in/out.
static bool limbs_mul_small(unsigned int** buf, int* n, unsigned int m) {
    if (m == 1) return true;
    int cap = *n + 2;
    unsigned int* p = realloc(*buf, (size_t)cap * sizeof(unsigned int));
    if (!p) return false;
    unsigned long long carry = 0;
    for (int i = 0; i < *n; i++) {
        unsigned long long cur = (unsigned long long)p[i] * m + carry;
        p[i] = (unsigned int)(cur % 1000000000u);
        carry = cur / 1000000000u;
    }
    int top = *n;
    while (carry) {
        if (top >= cap) {
            p = realloc(p, (size_t)(cap + 1) * sizeof(unsigned int));
            if (!p) { *buf = NULL; return false; }
            cap++;
        }
        p[top++] = (unsigned int)(carry % 1000000000u);
        carry /= 1000000000u;
    }
    *buf = p;
    *n = top;
    return true;
}

// t = v * m into freshly allocated buffer (t != v). tl out.
static bool limbs_mul_small_new(const unsigned int* v, int vn, unsigned int m,
                                unsigned int** t, int* tn) {
    *t = calloc((size_t)vn + 2, sizeof(unsigned int));
    if (!*t) return false;
    unsigned long long carry = 0;
    for (int i = 0; i < vn; i++) {
        unsigned long long cur = (unsigned long long)v[i] * m + carry;
        (*t)[i] = (unsigned int)(cur % 1000000000u);
        carry = cur / 1000000000u;
    }
    int top = vn;
    while (carry) {
        (*t)[top++] = (unsigned int)(carry % 1000000000u);
        carry /= 1000000000u;
    }
    *tn = top;
    return true;
}

// in-place a -= b (a >= b required).
static void limbs_sub_inplace(unsigned int* a, int an, const unsigned int* b, int bn) {
    long long borrow = 0;
    for (int i = 0; i < an; i++) {
        long long ai = a[i];
        long long bi = i < bn ? b[i] : 0;
        long long d = ai - bi - borrow;
        if (d < 0) { d += 1000000000LL; borrow = 1; } else borrow = 0;
        a[i] = (unsigned int)d;
    }
}

// Round digit string to n significant digits, half-even, using the next
// digit and a sticky flag. digits/scale updated in place (strings owned).
static bool round_sig(char** digits_io, long* scale_io, long n, bool sticky) {
    char* s = *digits_io;
    long len = (long)strlen(s);
    if (n >= len) return true; // nothing to round
    char next = s[n];
    bool up;
    if (next > '5') up = true;
    else if (next < '5') up = false;
    else up = sticky || ((s[n - 1] - '0') % 2 == 1);
    long new_scale = *scale_io - (len - n);
    if (!up) {
        s[n] = '\0';
        *scale_io = new_scale;
        return true;
    }
    // round up with carry ripple
    for (long i = n - 1; i >= 0; i--) {
        if (s[i] < '9') { s[i]++; s[n] = '\0'; *scale_io = new_scale; return true; }
        s[i] = '0';
    }
    // full carry out: "999" -> "100" with one more digit of magnitude;
    // keeping the same numeric value needs scale+1 (or equivalently the
    // extra digit shifts the point), value = digits * 10^-scale.
    char* ns = malloc((size_t)n + 2);
    if (!ns) return false;
    ns[0] = '1';
    memset(&ns[1], '0', (size_t)n);
    ns[n + 1] = '\0';
    free(s);
    *digits_io = ns;
    *scale_io = new_scale + 1;
    return true;
}

// Grow a heap digit string to cap (returns false on OOM).
static bool grow_str(char** buf, size_t cap) {
    char* p = realloc(*buf, cap);
    if (!p) return false;
    *buf = p;
    return true;
}


// One long-division step: q = floor(R / D), R reduced in place (R -= q*D).
// Invariant R < D*1e9 keeps q < 1e9. Knuth-D two-limb estimate over the
// TOP limb of D, low-biased (+1) so correction is bounded both directions.
static bool div_step(unsigned int* R, int rn, const unsigned int* D, int dn,
                     unsigned int* q_out) {
    int rt = rn - 1; while (rt > 0 && R[rt] == 0) rt--;
    int dt = dn - 1; while (dt > 0 && D[dt] == 0) dt--;
    if (dt < 0 || rt < dt) { *q_out = 0; return true; } // R < D entirely
    // Exact two-limb-over-one-limb estimate (Knuth D without the +1 bias:
    // that bias is only sound when D is limb-normalized, which base-1e9
    // powers-of-ten scaling cannot guarantee). Est may overshoot by a
    // handful of units; the correct-down loop below is bounded by it.
    unsigned long long est;
    if (rt == dt) {
        est = D[dt] ? R[rt] / D[dt] : 0;
    } else if (rt == dt + 1) {
        unsigned long long num = (unsigned long long)R[rt] * 1000000000u
                               + (rt > 0 ? R[rt - 1] : 0);
        est = D[dt] ? num / D[dt] : 0;
        if (est > 999999999u) est = 999999999u;
    } else {
        // R towers over D by 2+ limbs: quotient limb is a full B-1... only
        // possible on the very first step of the integer part; use the
        // top-vs-top ratio directly (clamped, then corrected down).
        est = 999999998u;
    }
    unsigned int* t = NULL; int tn = 0;
    while (est > 0) {
        if (!limbs_mul_small_new(D, dn, (unsigned int)est, &t, &tn)) return false;
        if (limbs_cmp(R, rn, t, tn) >= 0) break;
        est--;
        free(t); t = NULL;
    }
    while (est < 999999999u) {
        unsigned int* t2; int t2n;
        if (!limbs_mul_small_new(D, dn, (unsigned int)(est + 1), &t2, &t2n)) { free(t); return false; }
        if (limbs_cmp(R, rn, t2, t2n) < 0) { free(t2); break; }
        free(t); t = t2; tn = t2n; est++;
    }
    *q_out = (unsigned int)est;
    if (t) { limbs_sub_inplace(R, rn, t, tn); free(t); }
    return true;
}

// out = a / b. Returns false on OOM or b == 0 (native layer distinguishes).
//
// Aligned integers N/D at common scale S: value = (N/D) * 10^-S.
// Emit integer digits of Q, then fraction digits 9-at-a-time until
// g_precision+1 significant digits; exactness = remainder hits zero.
// Result scale = S + fraction_digits_emitted, then:
//   inexact -> round_sig to precision (half-even, sticky from remainder)
//   exact   -> strip trailing zeros down to ideal scale sa - sb
static bool dec_div(const Dec* a, const Dec* b, Dec* out) {
    if (dec_is_zero(b)) return false;
    memset(out, 0, sizeof(Dec));
    char* digits = NULL;  // declared up top: early oom paths below must free(NULL) safely
    out->kind = DEC_NORMAL;
    if (dec_is_zero(a)) {
        out->sign = 1;
        out->limbs = calloc(1, sizeof(unsigned int));
        if (!out->limbs) return false;
        out->nlimbs = 1;
        out->scale = a->scale;
        return true;
    }
    long sa = a->scale, sb = b->scale;
    Dec ta, tb;
    if (!dec_align(a, b, &ta, &tb)) return false;
    unsigned int* N = ta.limbs;
    const unsigned int* D = tb.limbs;
    int nn = ta.nlimbs, dn = tb.nlimbs;
    unsigned int* Q = calloc((size_t)(nn + 1), sizeof(unsigned int));
    unsigned int* R = calloc((size_t)(dn + 2), sizeof(unsigned int));
    int rn = 0;
    if (!Q || !R) { free(Q); free(R); dec_free(&ta); dec_free(&tb); return false; }
    // Integer part: MSB-first limb division.
    for (int i = nn - 1; i >= 0; i--) {
        memmove(&R[1], R, (size_t)rn * sizeof(unsigned int));
        R[0] = N[i];
        rn++;
        if (!div_step(R, rn, D, dn, &Q[i])) goto oom;
        while (rn > 0 && R[rn - 1] == 0) rn--;
    }
    size_t cap = 64, dlen = 0;
    digits = malloc(cap);
    if (!digits) goto oom;
    long sig = 0;
    bool started = false;
    int qtop = nn;
    while (qtop > 0 && Q[qtop - 1] == 0) qtop--;
    for (int i = qtop - 1; i >= 0; i--) {
        char lb[16];
        if (i == qtop - 1) snprintf(lb, sizeof(lb), "%u", Q[i]);
        else snprintf(lb, sizeof(lb), "%09u", Q[i]);
        for (char* c = lb; *c; c++) {
            if (dlen + 2 >= cap) { cap *= 2; if (!grow_str(&digits, cap)) goto oom; }
            digits[dlen++] = *c;
            if (!started && *c != '0') started = true;
            if (started) sig++;
        }
    }
    long frac = 0;            // fraction digits appended
    long wanted = g_precision + 1;
    bool exact = false;
    for (;;) {
        int rz = 0; while (rz < rn && R[rz] == 0) rz++;
        if (rz >= rn) { exact = true; break; }
        if (sig >= wanted) break;
        if (!limbs_mul_small(&R, &rn, 1000000000u)) goto oom;
        unsigned int q9 = 0;
        if (!div_step(R, rn, D, dn, &q9)) goto oom;
        char lb[16];
        snprintf(lb, sizeof(lb), "%09u", q9);
        for (int k = 0; k < 9; k++) {
            if (dlen + 2 >= cap) { cap *= 2; if (!grow_str(&digits, cap)) goto oom; }
            digits[dlen++] = lb[k];
            frac++;
            if (!started && lb[k] != '0') started = true;
            if (started) sig++;
            if (sig >= wanted) break;
        }
        while (rn > 0 && R[rn - 1] == 0) rn--;
    }
    digits[dlen] = '\0';
    bool sticky = false;
    {
        int rz = 0; while (rz < rn && R[rz] == 0) rz++;
        sticky = rz < rn;
    }
    free(Q); free(R);
    dec_free(&ta); dec_free(&tb);
    if (!exact && sig > wanted) {
        // Examine already-emitted digits beyond the wanted significance.
        long pos = (long)dlen - (sig - wanted);
        if (pos >= 0 && pos < (long)dlen) {
            if (digits[pos] > '5') sticky = true;
            else if (digits[pos] == '5') {
                for (long z = pos + 1; z < (long)dlen; z++) if (digits[z] != '0') { sticky = true; break; }
            }
        }
    }
    if (!started) {
        free(digits);
        out->sign = 1;
        out->limbs = calloc(1, sizeof(unsigned int));
        if (!out->limbs) return false;
        out->limbs[0] = 0; out->nlimbs = 1;
        out->scale = 0;
        return true;
    }
    // Ratio law: (N*10^-S)/(D*10^-S) = N/D -- the aligned scale cancels.
    // Result scale is pure fraction-digit count (ideal sa-sb governs the
    // exact-case zero strip below).
    long scale = frac;
    out->sign = a->sign * b->sign;
    if (!exact) {
        if (!round_sig(&digits, &scale, g_precision, sticky)) { free(digits); return false; }
    } else {
        long ideal = sa - sb;
        long len = (long)strlen(digits);
        while (len > 1 && digits[len - 1] == '0' && scale > ideal) {
            digits[--len] = '\0';
            scale--;
        }
        if (len > g_precision) {
            // exact but wider than the context: round like CPython
            bool st = len > g_precision + 1;
            if (!round_sig(&digits, &scale, g_precision, st)) { free(digits); return false; }
        }
    }
    bool ok = dec_from_digits(out, digits, scale, out->sign);
    free(digits);
    if (ok && dec_is_zero(out)) out->sign = 1;
    return ok;
oom:
    free(Q); free(R); free(digits);
    dec_free(&ta); dec_free(&tb);
    return false;
}


// ---------------------------------------------------------------------------
// Quantize / normalize / truncated floor-div & mod / rounding helpers.
// ---------------------------------------------------------------------------

// Drop digits below `places` with half-even rounding (places may be
// negative: quantize to tens/hundreds). Rebuilds d in place.
static bool dec_quantize(Dec* d, long places) {
    if (d->kind != DEC_NORMAL) return true; // specials pass through
    if (d->scale <= places) return true;    // already finer or equal
    long drop = d->scale - places;
    long len = dec_num_digits(d);
    if (drop >= len) {
        // everything below the leading digit goes; result is 0 at `places`
        // unless rounding bumps it (0.d5 -> could carry to 1).
        char* ds = dec_coef_string(d);
        if (!ds) return false;
        bool up = false;
        if (len >= 2 && ds[1] >= '5') up = (ds[1] > '5') || ((ds[0] - '0') % 2 == 1);
        char nd[3];
        if (up && len >= 1) { nd[0] = ds[0] + 1; }
        else if (len >= 1) { nd[0] = ds[0]; }
        else nd[0] = '0';
        nd[1] = '\0';
        // carry ripple on the single kept digit
        if (nd[0] > '9') { nd[0] = '1'; places = places - (len - 1) - 1; }
        int sign = d->sign;
        dec_free(d); // in-place rebuild
        bool ok = dec_from_digits(d, nd, places, sign);
        free(ds);
        return ok;
    }
    char* ds = dec_coef_string(d);
    if (!ds) return false;
    long keep = len - drop;
    long newscale = d->scale - (len - keep);
    int sign = d->sign;
    bool ok = round_sig(&ds, &newscale, keep, false);
    if (!ok) { free(ds); return false; }
    dec_free(d); // in-place rebuild
    ok = dec_from_digits(d, ds, places, sign);
    free(ds);
    return ok;
}

// Normalize: strip trailing coefficient zeros while scale > 0 (CPython:
// Decimal("2.500").normalize() = Decimal("2.5"); 1000 -> 1E+3).
static void dec_normalize(Dec* d) {
    if (d->kind != DEC_NORMAL || dec_is_zero(d)) return;
    // CPython normalize(): remove trailing zeros entirely -- the exponent
    // goes NEGATIVE when the coefficient shrinks (1000 -> 1E+3), so the
    // loop must not stop at scale 0.
    while (d->limbs[0] % 10 == 0) {
        unsigned long long r = 0;
        for (int i = d->nlimbs - 1; i >= 0; i--) {
            unsigned long long cur = r * 1000000000u + d->limbs[i];
            d->limbs[i] = (unsigned int)(cur / 10);
            r = cur % 10;
        }
        d->scale--;
        dec_trim(d);
    }
}

// Truncated integer division + remainder (decimal spec: toward zero).
// 7.5//2 = 3, -7//2 = -3, 7//-2 = -3; -7%2 = -1, 7%-2 = 1.
static bool dec_divmod_trunc(const Dec* a, const Dec* b, Dec* q_out, Dec* r_out) {
    Dec q;
    memset(&q, 0, sizeof(Dec)); // dec_div frees/overwrites out->limbs
    if (!dec_div(a, b, &q)) return false;
    if (q.scale <= 0) {
        if (q_out) { if (!dec_copy(q_out, &q)) { dec_free(&q); return false; } }
        if (r_out) {
            Dec qb, neg_qb, r;
            memset(&qb, 0, sizeof(Dec)); memset(&r, 0, sizeof(Dec));
            if (!dec_mul(&q, b, &qb)) { dec_free(&q); return false; }
            neg_qb = qb; neg_qb.sign = -qb.sign;
            if (!dec_add(a, &neg_qb, &r)) { dec_free(&q); dec_free(&qb); return false; }
            dec_free(&qb);
            *r_out = r;
        }
        dec_free(&q);
        return true;
    }
    // Fractional q: cut to integer toward zero (no rounding).
    char* qs = dec_coef_string(&q);
    if (!qs) { dec_free(&q); return false; }
    long len = (long)strlen(qs);
    long drop = q.scale;
    if (drop >= len) {
        // |q| < 1: integer part is zero
        free(qs);
        dec_free(&q);
        if (q_out) {
            memset(q_out, 0, sizeof(Dec));
            q_out->kind = DEC_NORMAL; q_out->sign = 1; q_out->scale = 0;
            q_out->limbs = calloc(1, sizeof(unsigned int));
            if (!q_out->limbs) return false;
            q_out->limbs[0] = 0; q_out->nlimbs = 1;
        }
        if (r_out) return dec_copy(r_out, a);
        return true;
    }
    char* ip = malloc((size_t)(len - drop) + 1);
    if (!ip) { free(qs); dec_free(&q); return false; }
    memcpy(ip, qs, (size_t)(len - drop));
    free(qs); // copied into ip -- release the full-precision run
    ip[len - drop] = '\0';
    int sign = q.sign;
    dec_free(&q);
    if (q_out && !dec_from_digits(q_out, ip, 0, sign)) { free(ip); return false; }
    Dec qint;
    memset(&qint, 0, sizeof(Dec));
    bool ok = dec_from_digits(&qint, ip, 0, sign);
    free(ip);
    if (!ok) return false;
    if (r_out) {
        Dec qb, neg_qb, r;
        memset(&qb, 0, sizeof(Dec)); memset(&r, 0, sizeof(Dec));
        if (!dec_mul(&qint, b, &qb)) { dec_free(&qint); return false; }
        neg_qb = qb; neg_qb.sign = -qb.sign;
        if (!dec_add(a, &neg_qb, &r)) { dec_free(&qint); dec_free(&qb); return false; }
        dec_free(&qb);
        *r_out = r;
    }
    dec_free(&qint);
    return true;
}

// ---------------------------------------------------------------------------
// Native surface. Values flow in/out as Decimal structs (definition name
// "Decimal", string field "str" carrying the CPython-exact rendering) so
// they print, clone, and release like datetime structs already do.
// ---------------------------------------------------------------------------

// Registry-owned definition singleton (same pattern as stdlib_regex.c /
// stdlib_datetime.c — value_release() never frees definitions).
static StructDef* decimal_def(void) {
    static StructDef def;
    static bool init = false;
    if (!init) {
        def.name = strdup("Decimal");
        def.ast_node = NULL;
        init = true;
    }
    return &def;
}

static bool dec_pow(const Dec* a, long e, Dec* out);

// Box a Dec into a struct Value { str: string, sign: int, digits: int, exponent: int }.
static Value decimal_to_value(const Dec* d) {
    char* s = dec_to_string(d);
    if (!s) return make_null();
    StructInstance* inst = malloc(sizeof(StructInstance));
    if (!inst) { free(s); return make_null(); }
    inst->definition = decimal_def();
    inst->fields = NULL;
    static const char* const names[4] = { "text", "sign", "digits", "exponent" };
    long long vals[3] = { d->sign, dec_num_digits(d), -d->scale };
    FieldValue* tail = NULL;
    for (int i = 0; i < 4; i++) {
        FieldValue* f = malloc(sizeof(FieldValue));
        if (!f) { free(s); return make_null(); }
        f->name = strdup(names[i]);
        f->value = malloc(sizeof(Value));
        if (!f->value) { free(f); free(s); return make_null(); }
        if (i == 0) {
            f->value->type = VAL_STRING;
            f->value->string_val = s; // ownership moves into the field
        } else {
            f->value->type = VAL_INT;
            f->value->int_val = vals[i - 1];
        }
        f->next = NULL;
        if (tail) tail->next = f; else inst->fields = f;
        tail = f;
    }
    Value v;
    v.type = VAL_STRUCT_INSTANCE;
    v.struct_instance = inst;
    return v;
}

// Unbox: accept a Decimal struct (match by field "str" + int sign field) or
// a plain string/int/float. Errors are conversational (stderr + null).
static bool decimal_from_value(Value* v, Dec* out, const char* ctx) {
    if (v->type == VAL_STRING) return dec_parse(v->string_val, out);
    if (v->type == VAL_INT) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lld", v->int_val);
        return dec_parse(buf, out);
    }
    if (v->type == VAL_FLOAT) {
        // CPython Decimal(float) is the exact binary expansion; printing
        // %.17g round-trips the double, which matches Python float->str.
        char buf[40];
        snprintf(buf, sizeof(buf), "%.17g", v->float_val);
        return dec_parse(buf, out);
}
    if (v->type == VAL_STRUCT_INSTANCE) {
        FieldValue* f = v->struct_instance->fields;
        while (f) {
            if (strcmp(f->name, "text") == 0 && f->value->type == VAL_STRING) {
                return dec_parse(f->value->string_val, out);
            }
            f = f->next;
        }
    }
    fprintf(stderr, "Error: decimal.%s: cannot interpret %d as a number.\n", ctx, (int)v->type);
    return false;
}

// Two-operand arithmetic native wrapper: unbox, special-check, compute, box.
static Value dec_binop(Value* args, int argc, const char* name, bool (*op)(const Dec*, const Dec*, Dec*)) {
    if (argc != 2) {
        fprintf(stderr, "Error: decimal.%s(a, b) takes two arguments.\n", name);
        return make_null();
    }
    Dec a, b, r;
    memset(&r, 0, sizeof(Dec)); // ops free/overwrite out->limbs; must start NULL
    if (!decimal_from_value(&args[0], &a, name)) return make_null();
    if (!decimal_from_value(&args[1], &b, name)) { dec_free(&a); return make_null(); }
    if (a.kind != DEC_NORMAL || b.kind != DEC_NORMAL) {
        fprintf(stderr, "Error: decimal.%s: Infinity/NaN arithmetic is invalid (Python raises InvalidOperation).\n", name);
        dec_free(&a); dec_free(&b);
        return make_null();
    }
    bool ok = op(&a, &b, &r);
    if (!ok) {
        if (strcmp(name, "div") == 0 || strcmp(name, "floordiv") == 0 || strcmp(name, "mod") == 0)
            fprintf(stderr, "Error: decimal division by zero.\n");
        dec_free(&a); dec_free(&b); dec_free(&r);
        return make_null();
    }
    Value v = decimal_to_value(&r);
    dec_free(&a); dec_free(&b); dec_free(&r);
    return v;
}

// Check structure supports struct returns
static bool dec_op_add(const Dec* a, const Dec* b, Dec* r) { return dec_add(a, b, r); }
// b -> -b on a copy, then reuse dec_add (exact, shared alignment path)
static bool dec_op_sub(const Dec* a, const Dec* b, Dec* r) {
    Dec nb;
    if (!dec_copy(&nb, b)) return false;
    nb.sign = -nb.sign;
    bool ok = dec_add(a, &nb, r);
    dec_free(&nb);
    return ok;
}
static bool dec_op_mul(const Dec* a, const Dec* b, Dec* r) { return dec_mul(a, b, r); }
static bool dec_op_div(const Dec* a, const Dec* b, Dec* r) { return dec_div(a, b, r); }

// floordiv/mod via the shared truncated divmod.
static bool dec_op_floordiv(const Dec* a, const Dec* b, Dec* r) {
    return dec_divmod_trunc(a, b, r, NULL);
}
static bool dec_op_mod(const Dec* a, const Dec* b, Dec* r) {
    return dec_divmod_trunc(a, b, NULL, r);
}

// Neg: sign flip on a copy (parse-round-trip is exact and simpler).
static bool dec_op_neg(const Dec* a, const Dec* b, Dec* r) {
    (void)b;
    if (!dec_copy(r, a)) return false;
    r->sign = -r->sign;
    return true;
}
static bool dec_op_abs(const Dec* a, const Dec* b, Dec* r) {
    (void)b;
    if (!dec_copy(r, a)) return false;
    r->sign = r->sign < 0 ? 1 : r->sign;
    return true;
}

// ---------------------------------------------------------------------------
static bool dec_sqrt(const Dec* a, Dec* out);

// Native entry points.
// ---------------------------------------------------------------------------

Value stdlib_decimal_parse(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) {
        fprintf(stderr, "Error: decimal(value) takes one argument (string/int/float/Decimal).\n");
        return make_null();
    }
    Dec d;
    if (!decimal_from_value(&args[0], &d, "parse")) return make_null();
    Value v = decimal_to_value(&d);
    dec_free(&d);
    return v;
}

Value stdlib_decimal_str(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.str(d) takes one argument.\n"); return make_null(); }
    Dec d;
    if (!decimal_from_value(&args[0], &d, "str")) return make_null();
    char* s = dec_to_string(&d);
    dec_free(&d);
    if (!s) return make_null();
    Value v = make_string(s);
    free(s);
    return v;
}

Value stdlib_decimal_add(Interpreter* interp, int argc, Value* args) {
    (void)interp; return dec_binop(args, argc, "add", dec_op_add);
}
Value stdlib_decimal_sub(Interpreter* interp, int argc, Value* args) {
    (void)interp; return dec_binop(args, argc, "sub", dec_op_sub);
}
Value stdlib_decimal_mul(Interpreter* interp, int argc, Value* args) {
    (void)interp; return dec_binop(args, argc, "mul", dec_op_mul);
}
Value stdlib_decimal_div(Interpreter* interp,  int argc, Value* args) {
    (void)interp; return dec_binop(args, argc, "div", dec_op_div);
}
Value stdlib_decimal_floordiv(Interpreter* interp, int argc, Value* args) {
    (void)interp; return dec_binop(args, argc, "floordiv", dec_op_floordiv);
}
Value stdlib_decimal_mod(Interpreter* interp, int argc, Value* args) {
    (void)interp; return dec_binop(args, argc, "mod", dec_op_mod);
}
Value stdlib_decimal_pow(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) {
        fprintf(stderr, "Error: decimal.pow(base, exp) takes two arguments (integer exponent).\n");
        return make_null();
    }
    Dec a; long e;
    if (!decimal_from_value(&args[0], &a, "pow")) return make_null();
    if (args[1].type != VAL_INT) {
        fprintf(stderr, "Error: decimal.pow exponent must be an integer.\n");
        dec_free(&a);
        return make_null();
    }
    e = args[1].int_val;
    Dec r;
    if (a.kind != DEC_NORMAL) {
        fprintf(stderr, "Error: decimal.pow: Infinity/NaN arithmetic is invalid.\n");
        dec_free(&a);
        return make_null();
    }
    if (!dec_pow(&a, e, &r)) { dec_free(&a); return make_null(); }
    dec_free(&a);
    Value v = decimal_to_value(&r);
    dec_free(&r);
    return v;
}

// ---------------------------------------------------------------------------
// Pow — integer exponents. Positive: exact square-and-multiply (CPython
// expands precision to hold the full result; 2**100 is never rounded).
// Negative: reciprocal via division at context precision. x**0 = 1.
// ---------------------------------------------------------------------------

static bool dec_pow(const Dec* a, long e, Dec* out) {
    if (e == 0) {
        memset(out, 0, sizeof(Dec));
        out->kind = DEC_NORMAL;
        out->sign = 1;
        out->limbs = calloc(1, sizeof(unsigned int));
        if (!out->limbs) return false;
        out->limbs[0] = 1;
        out->nlimbs = 1;
        return true;
    }
    if (e < -1000000L || e > 1000000L) return false;
    bool invert = e < 0;
    long ae = invert ? -e : e;
    Dec base;
    if (!dec_copy(&base, a)) return false;
    Dec acc;
    memset(&acc, 0, sizeof(Dec));
    acc.kind = DEC_NORMAL; acc.sign = 1; acc.scale = 0;
    acc.limbs = calloc(1, sizeof(unsigned int));
    if (!acc.limbs) { dec_free(&base); return false; }
    acc.limbs[0] = 1; acc.nlimbs = 1;
    while (ae > 0) {
        if (ae & 1) {
            Dec r;
            memset(&r, 0, sizeof(Dec)); // dec_mul frees/overwrites out->limbs
            if (!dec_mul(&acc, &base, &r)) { dec_free(&base); dec_free(&acc); return false; }
            dec_free(&acc);
            acc = r;
        }
        ae >>= 1;
        if (ae > 0) {
            Dec r2;
            memset(&r2, 0, sizeof(Dec));
            if (!dec_mul(&base, &base, &r2)) { dec_free(&base); dec_free(&acc); return false; }
            dec_free(&base);
            base = r2;
        }
    }
    dec_free(&base);
    if (invert) {
        Dec one; memset(&one, 0, sizeof(Dec));
        one.kind = DEC_NORMAL; one.sign = 1; one.scale = 0;
        one.limbs = calloc(1, sizeof(unsigned int));
        if (!one.limbs) { dec_free(&acc); return false; }
        one.limbs[0] = 1; one.nlimbs = 1;
        Dec q;
        bool ok = dec_div(&one, &acc, &q);
        dec_free(&one); dec_free(&acc);
        if (!ok) return false;
        *out = q;
        return true;
    }
    *out = acc;
    return true;
}

Value stdlib_decimal_neg(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.neg(d) takes one argument.\n"); return make_null(); }
    Dec a, r;
    if (!decimal_from_value(&args[0], &a, "neg")) return make_null();
    if (a.kind != DEC_NORMAL) { dec_free(&a); return make_null(); }
    if (!dec_op_neg(&a, NULL, &r)) { dec_free(&a); return make_null(); }
    dec_free(&a);
    Value v = decimal_to_value(&r);
    dec_free(&r);
    return v;
}

Value stdlib_decimal_abs(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.abs(d) takes one argument.\n"); return make_null(); }
    Dec a, r;
    if (!decimal_from_value(&args[0], &a, "abs")) return make_null();
    if (a.kind != DEC_NORMAL) { dec_free(&a); return make_null(); }
    if (!dec_op_abs(&a, NULL, &r)) { dec_free(&a); return make_null(); }
    dec_free(&a);
    Value v = decimal_to_value(&r);
    dec_free(&r);
    return v;
}

Value stdlib_decimal_sqrt(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.sqrt(d) takes one argument.\n"); return make_null(); }
    Dec a, r;
    if (!decimal_from_value(&args[0], &a, "sqrt")) return make_null();
    if (a.kind != DEC_NORMAL) { dec_free(&a); return make_null(); }
    if (a.sign < 0) {
        fprintf(stderr, "Error: decimal.sqrt of a negative is invalid (Python raises InvalidOperation).\n");
        dec_free(&a);
        return make_null();
    }
    if (!dec_sqrt(&a, &r)) { dec_free(&a); return make_null(); }
    dec_free(&a);
    Value v = decimal_to_value(&r);
    dec_free(&r);
    return v;
}

Value stdlib_decimal_cmp(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.cmp(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "cmp") || !decimal_from_value(&args[1], &b, "cmp")) {
        return make_null();
    }
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) { fprintf(stderr, "Error: decimal.cmp with NaN is unordered (Python raises InvalidOperation).\n"); return make_null(); }
    return make_int(c);
}

Value stdlib_decimal_eq(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.eq(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "eq") || !decimal_from_value(&args[1], &b, "eq")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) return make_bool(false); // NaN != anything, even itself
    return make_bool(c == 0);
}

Value stdlib_decimal_lt(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.lt(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "lt") || !decimal_from_value(&args[1], &b, "lt")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) return make_bool(false);
    return make_bool(c < 0);
}

Value stdlib_decimal_gt(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.gt(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "gt") || !decimal_from_value(&args[1], &b, "gt")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) return make_bool(false);
    return make_bool(c > 0);
}

Value stdlib_decimal_lte(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.lte(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "lte") || !decimal_from_value(&args[1], &b, "lte")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) return make_bool(false);
    return make_bool(c <= 0);
}

Value stdlib_decimal_gte(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.gte(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "gte") || !decimal_from_value(&args[1], &b, "gte")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) return make_bool(false);
    return make_bool(c >= 0);
}

Value stdlib_decimal_is_zero(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    // (arity checked below)
    if (argc != 1) { fprintf(stderr, "Error: decimal.is_zero(d) takes one argument.\n"); return make_null(); }
    Dec a;
    if (!decimal_from_value(&args[0], &a, "is_zero")) return make_null();
    bool z = dec_is_zero(&a);
    dec_free(&a);
    return make_bool(z);
}

// ---------------------------------------------------------------------------
// Sqrt — Newton on the scaled integer coefficient: pick k so scale+k is
// even and the coefficient has ~2*precision digits, integer-sqrt by
// digit-string Newton (x = (x + n/x)/2), round half-even to precision.
// Pin: sqrt(2) = 1.414213562373095048801688724 (27 digits).
// ---------------------------------------------------------------------------

// Integer sqrt of a decimal digit string, digit-by-digit on LIMBS (the
// 2000s textbook "long division" sqrt, O(d^2), exact, division-free).
// Per digit pair: r = r*100 + pair; find max q with (20p+q)*q <= r;
// r -= (20p+q)*q; p = p*10 + q. All heavy math via limb helpers.
static bool dec_isqrt_digits(const char* n_str, char** out_digits) {
    long dn = (long)strlen(n_str);
    long pad = dn % 2;
    long total = dn + pad;
    char* dig = malloc((size_t)total + 1);
    if (!dig) return false;
    if (pad) { dig[0] = '0'; memcpy(&dig[1], n_str, (size_t)dn); }
    else memcpy(dig, n_str, (size_t)dn);
    dig[total] = '\0';
    // r and p as limb arrays (little-endian, base 1e9).
    unsigned int* r = calloc(8, sizeof(unsigned int));
    unsigned int* p = calloc(8, sizeof(unsigned int));
    int rn = 1, pn = 1; // limb counts (used slots; arrays grow below)
    if (!r || !p) { free(dig); free(r); free(p); return false; }
    for (long i = 0; i < total; i += 2) {
        // r = r*100 + two digits.
        int pair = (dig[i] - '0') * 10 + (dig[i + 1] - '0');
        // r *= 100 via limbs_mul_small twice (x10, x10) into new buffers
        unsigned int* r2 = NULL; int r2n = 0;
        if (!limbs_mul_small_new(r, rn, 10, &r2, &r2n)) { free(dig); free(r); free(p); return false; }
        unsigned int* r3 = NULL; int r3n = 0;
        if (!limbs_mul_small_new(r2, r2n, 10, &r3, &r3n)) { free(r2); free(dig); free(r); free(p); return false; }
        free(r2);
        // r3 += pair
        unsigned long long carry = pair;
        for (int k = 0; k < r3n; k++) {
            unsigned long long cur = (unsigned long long)r3[k] + carry;
            r3[k] = (unsigned int)(cur % 1000000000u);
            carry = cur / 1000000000u;
            if (!carry) break;
        }
        if (carry) {
            // grow r3 by one limb
            unsigned int* nr = realloc(r3, (size_t)(r3n + 1) * sizeof(unsigned int));
            if (!nr) { free(r3); free(dig); free(r); free(p); return false; }
            nr[r3n++] = (unsigned int)carry;
            r3 = nr;
        }
        free(r);
        r = r3; rn = r3n;
        // p20 = p * 20
        unsigned int* p20 = NULL; int p20n = 0;
        if (!limbs_mul_small_new(p, pn, 20, &p20, &p20n)) { free(dig); free(r); free(p); return false; }
        // find max q: (p20 + q) * q <= r
        int q = 0;
        unsigned int* best = NULL; int bestn = 0;
        for (int cand = 9; cand >= 0; cand--) {
            // base = p20 + cand
            unsigned int* base = NULL; int basen = 0;
            if (!limbs_mul_small_new(p20, p20n, 1, &base, &basen)) { free(p20); free(dig); free(r); free(p); return false; }
            unsigned long long c2 = (unsigned long long)cand;
            for (int k = 0; k < basen && c2; k++) {
                unsigned long long cur = (unsigned long long)base[k] + c2;
                base[k] = (unsigned int)(cur % 1000000000u);
                c2 = cur / 1000000000u;
            }
            if (c2) {
                unsigned int* nb = realloc(base, (size_t)(basen + 1) * sizeof(unsigned int));
                if (!nb) { free(base); free(p20); free(dig); free(r); free(p); return false; }
                nb[basen++] = (unsigned int)c2;
                base = nb;
            }
            // prod = base * cand
            unsigned int* prod = NULL; int prodn = 0;
            if (!limbs_mul_small_new(base, basen, (unsigned int)cand, &prod, &prodn)) { free(base); free(p20); free(dig); free(r); free(p); return false; }
            free(base);
            if (limbs_cmp(prod, prodn, r, rn) <= 0) {
                q = cand;
                best = prod; bestn = prodn;
                break;
            }
            free(prod);
        }
        free(p20);
        // r -= best (if any)
        if (best) {
            limbs_sub_inplace(r, rn, best, bestn);
            free(best);
        }
        // p = p*10 + q
        unsigned int* p2 = NULL; int p2n = 0;
        if (!limbs_mul_small_new(p, pn, 10, &p2, &p2n)) { free(dig); free(r); free(p); return false; }
        unsigned long long c3 = (unsigned long long)q;
        for (int k = 0; k < p2n && c3; k++) {
            unsigned long long cur = (unsigned long long)p2[k] + c3;
            p2[k] = (unsigned int)(cur % 1000000000u);
            c3 = cur / 1000000000u;
        }
        if (c3) {
            unsigned int* np = realloc(p2, (size_t)(p2n + 1) * sizeof(unsigned int));
            if (!np) { free(p2); free(dig); free(r); free(p); return false; }
            np[p2n++] = (unsigned int)c3;
            p2 = np;
        }
        free(p);
        p = p2; pn = p2n;
    }
    free(dig);
    free(r);
    // p -> decimal string (top limb bare, rest %09u)
    int top = pn - 1;
    while (top > 0 && p[top] == 0) top--;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", p[top]);
    size_t need = strlen(buf) + (size_t)top * 9 + 1;
    char* out = malloc(need);
    if (!out) { free(p); return false; }
    strcpy(out, buf);
    for (int k = top - 1; k >= 0; k--) {
        char lb[16];
        snprintf(lb, sizeof(lb), "%09u", p[k]);
        strcat(out, lb);
    }
    free(p);
    *out_digits = out;
    return true;
}

// Sqrt: scale to an even total exponent, integer sqrt (digit-by-digit),
// then round to context precision. Pin: sqrt(2) = 1.414213562373095048801688724.
static bool dec_sqrt(const Dec* a, Dec* out) {
    if (a->kind != DEC_NORMAL || a->sign < 0) return false;
    if (dec_is_zero(a)) return dec_copy(out, a);
    long nd = dec_num_digits(a);
    long need = 2 * g_precision + 2;
    long k = need - nd;
    if ((a->scale + k) % 2 != 0) k++;
    if (k < 0) k = 0;
    Dec t;
    if (!dec_copy(&t, a)) return false;
    if (k > 0 && !dec_scale_up(&t, k)) { dec_free(&t); return false; }
    char* coef = dec_coef_string(&t);
    dec_free(&t);
    if (!coef) return false;
    char* r = NULL;
    if (!dec_isqrt_digits(coef, &r)) { free(coef); return false; }
    free(coef);
    long expo = (a->scale + k) / 2;
    memset(out, 0, sizeof(Dec));
    bool ok = dec_from_digits(out, r, expo, 1);
    free(r);
    if (!ok) return false;
    // Round to context precision (half-even), then strip trailing zeros
    // like CPython sqrt does (sqrt(0.25) = 0.5, not 0.5000...).
    long sc = out->scale;
    char* ds = dec_coef_string(out);
    if (!ds) return false;
    long len = (long)strlen(ds);
    if (len > g_precision) {
        bool ok2 = round_sig(&ds, &sc, g_precision, false);
        if (!ok2) { free(ds); return false; }
        int keep_sign = out->sign;
        dec_free(out);
        ok = dec_from_digits(out, ds, sc, keep_sign);
        if (!ok) { free(ds); return false; }
    }
    dec_normalize(out);
    free(ds);
    return true;
}

Value stdlib_decimal_to_int(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.to_int(d) takes one argument.\n"); return make_null(); }
    Dec a;
    if (!decimal_from_value(&args[0], &a, "to_int")) return make_null();
    if (a.kind != DEC_NORMAL) { dec_free(&a); return make_null(); }
    // Python int(Decimal) cuts toward zero on the plain digit view.
    bool neg = a.sign < 0;
    char* cs = dec_coef_string(&a);
    if (!cs) { dec_free(&a); return make_null(); }
    long len = (long)strlen(cs);
    long drop = a.scale > 0 ? a.scale : 0;
    if (drop >= len) {
        free(cs);
        dec_free(&a);
        return make_int(0);
    }
    char* ip = malloc((size_t)(len - drop) + 1);
    if (!ip) { free(cs); dec_free(&a); return make_null(); }
    memcpy(ip, cs, (size_t)(len - drop));
    ip[len - drop] = '\0';
    long long v = strtoll(ip, NULL, 10);
    free(ip);
    free(cs);
    dec_free(&a);
    return make_int(neg ? -v : v);
}

Value stdlib_decimal_round(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || argc > 2) { fprintf(stderr, "Error: decimal.round(d [, places]) takes one or two arguments.\n"); return make_null(); }
    Dec a;
    if (!decimal_from_value(&args[0], &a, "round")) return make_null();
    long places = 0;
    if (argc == 2) {
        if (args[1].type != VAL_INT) { dec_free(&a); fprintf(stderr, "Error: decimal.round places must be an integer.\n"); return make_null(); }
        places = args[1].int_val;
    }
    if (a.kind != DEC_NORMAL) { dec_free(&a); return make_null(); }
    if (!dec_quantize(&a, places)) { dec_free(&a); return make_null(); }
    Value v = decimal_to_value(&a);
    dec_free(&a);
    return v;
}

Value stdlib_decimal_normalize(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.normalize(d) takes one argument.\n"); return make_null(); }
    Dec a;
    if (!decimal_from_value(&args[0], &a, "normalize")) return make_null();
    dec_normalize(&a);
    Value v = decimal_to_value(&a);
    dec_free(&a);
    return v;
}

Value stdlib_decimal_is_nan(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.is_nan(d) takes one argument.\n"); return make_null(); }
    Dec a;
    if (!decimal_from_value(&args[0], &a, "is_nan")) return make_null();
    bool r = a.kind == DEC_NAN;
    dec_free(&a);
    return make_bool(r);
}

Value stdlib_decimal_is_infinite(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: decimal.is_infinite(d) takes one argument.\n"); return make_null(); }
    Dec a;
    if (!decimal_from_value(&args[0], &a, "is_infinite")) return make_null();
    bool r = a.kind == DEC_INF;
    dec_free(&a);
    return make_bool(r);
}

Value stdlib_decimal_precision(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 0 || argc > 1) { fprintf(stderr, "Error: decimal.precision([n]) takes zero or one argument.\n"); return make_null(); }
    if (argc == 0) return make_int(g_precision);
    if (args[0].type != VAL_INT || args[0].int_val < 1 || args[0].int_val > 100000) {
        fprintf(stderr, "Error: decimal.precision must be an integer in 1..100000.\n");
        return make_null();
    }
    g_precision = args[0].int_val;
    return make_int(g_precision);
}

Value stdlib_decimal_min(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.min(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "min") || !decimal_from_value(&args[1], &b, "min")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) { fprintf(stderr, "Error: decimal.min with NaN is unordered.\n"); return make_null(); }
    return c <= 0 ? args[0] : args[1];
}

Value stdlib_decimal_max(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: decimal.max(a, b) takes two arguments.\n"); return make_null(); }
    Dec a, b;
    if (!decimal_from_value(&args[0], &a, "max") || !decimal_from_value(&args[1], &b, "max")) return make_null();
    int c = dec_cmp(&a, &b);
    dec_free(&a); dec_free(&b);
    if (c == -2) { fprintf(stderr, "Error: decimal.max with NaN is unordered.\n"); return make_null(); }
    return c >= 0 ? args[0] : args[1];
}

void stdlib_decimal_register(void) {
    register_native("decimal.parse", stdlib_decimal_parse);
    register_native("decimal.to_str", stdlib_decimal_str);
    register_native("decimal.add", stdlib_decimal_add);
    register_native("decimal.sub", stdlib_decimal_sub);
    register_native("decimal.mul", stdlib_decimal_mul);
    register_native("decimal.div", stdlib_decimal_div);
    register_native("decimal.floordiv", stdlib_decimal_floordiv);
    register_native("decimal.mod", stdlib_decimal_mod);
    register_native("decimal.pow", stdlib_decimal_pow);
    register_native("decimal.neg", stdlib_decimal_neg);
    register_native("decimal.abs", stdlib_decimal_abs);
    register_native("decimal.sqrt", stdlib_decimal_sqrt);
    register_native("decimal.cmp", stdlib_decimal_cmp);
    register_native("decimal.eq", stdlib_decimal_eq);
    register_native("decimal.lt", stdlib_decimal_lt);
    register_native("decimal.gt", stdlib_decimal_gt);
    register_native("decimal.lte", stdlib_decimal_lte);
    register_native("decimal.gte", stdlib_decimal_gte);
    register_native("decimal.is_zero", stdlib_decimal_is_zero);
    register_native("decimal.is_nan", stdlib_decimal_is_nan);
    register_native("decimal.is_infinite", stdlib_decimal_is_infinite);
    register_native("decimal.to_int", stdlib_decimal_to_int);
    register_native("decimal.round", stdlib_decimal_round);
    register_native("decimal.normalize", stdlib_decimal_normalize);
    register_native("decimal.precision", stdlib_decimal_precision);
    register_native("decimal.min", stdlib_decimal_min);
    register_native("decimal.max", stdlib_decimal_max);
}
