// RADS fractions module — Python-parity exact rationals (fractions.Fraction).
//
// Mission: 1/3 + 1/6 must be *exactly* 1/2 in RADS, the way it is with
// Python's fractions.Fraction and the way IEEE doubles never can be.
// RADS has no bigint core, so this file IS the engine: signed arbitrary-
// precision integers (base-1e9 limbs) + an exact rational layer on top
// (numerator/denominator always coprime, denominator always positive —
// the CPython invariants). Every operation is EXACT; there is no context
// precision, because exactness is the entire point of rationals.
//
// Semantics pinned against python3.13 fractions on this box (pins live in
// tests/test_fractions.rads):
//   - Fraction(6,4) == 3/2 (auto-reduced); sign lives in the numerator
//   - floor-div/mod use Python FLOOR law: F(-7,2)//2 == -2, F(-7,2)%2 ==
//     1/2, F(7,2)%2 == 3/2
//   - pow: integer exponents exact; 0**-n errors; negative base with
//     negative exponent keeps sign in numerator (F(3)**-2 == 1/9)
//   - limit_denominator: CPython's continued-fraction algorithm, ported
//     from Lib/fractions.py line for line (best upper/lower approx,
//     tie -> lower bound)
//   - float inputs convert to the EXACT binary rational (F(0.1) ==
//     3602879701896397/36028797018963968), not the short decimal
//   - str(): "num/den", or bare "num" when den == 1
//
// Representation: BigInt = sign (+1/-1, zero always +) + little-endian
// base-1e9 limbs. Fraction = BigInt num / BigInt den, den > 0,
// gcd(num,den) == 1. Zero denominator is rejected at construction
// (conversational stderr error + null — Python raises ZeroDivisionError).
//
// Zero new deps: C11 stdlib only. VW Fusca law holds.
// ⚕ Forged for Master Zarigata 🜂

#include "stdlib_fractions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);

// forward decls (shared by the division engine below)
static bool limbs_mul_small_new(const unsigned int* v, int vn, unsigned int m,
                                unsigned int** t, int* tn);
static void limbs_sub_inplace(unsigned int* a, int an, const unsigned int* b, int bn);

// ---------------------------------------------------------------------------
// BigInt core: sign + base-1e9 limbs, little-endian. Zero = {1, [0]}.
// ---------------------------------------------------------------------------

#define FRAC_MAX_LIMBS 4000 // runaway guard, ~36000 decimal digits

typedef struct {
    int sign;            // +1 / -1; zero is always +1
    int nlimbs;          // >= 1; limbs[nlimbs-1] is most significant
    unsigned int* limbs; // base 1e9, little-endian
} BigInt;

// Release limb storage; safe on already-released values.
static void bi_free(BigInt* a) {
    free(a->limbs);
    a->limbs = NULL;
    a->nlimbs = 0;
}

// True when every limb is zero.
static bool bi_is_zero(const BigInt* a) {
    for (int i = 0; i < a->nlimbs; i++) if (a->limbs[i]) return false;
    return true;
}

// Drop leading zero limbs (keep at least one so limbs stays valid).
static void bi_trim(BigInt* a) {
    while (a->nlimbs > 1 && a->limbs[a->nlimbs - 1] == 0) a->nlimbs--;
}

// Canonical zero (fresh single limb).
static bool bi_zero(BigInt* out) {
    out->sign = 1;
    out->limbs = calloc(1, sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = 1;
    return true;
}

// Canonical one.
static bool bi_one(BigInt* out) {
    if (!bi_zero(out)) return false;
    out->limbs[0] = 1;
    return true;
}

// From small signed value (any long long, LLONG_MIN safe).
static bool bi_from_ll(BigInt* out, long long v) {
    out->sign = v < 0 ? -1 : 1;
    unsigned long long m = v < 0 ? (unsigned long long)(-(v + 1)) + 1 : (unsigned long long)v;
    out->limbs = calloc(3, sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = 3;
    out->limbs[0] = (unsigned int)(m % 1000000000u);
    out->limbs[1] = (unsigned int)((m / 1000000000u) % 1000000000u);
    out->limbs[2] = (unsigned int)(m / 1000000000000000000ull);
    bi_trim(out);
    if (bi_is_zero(out)) out->sign = 1;
    return true;
}

// Deep copy (independent limb buffer).
static bool bi_copy(BigInt* out, const BigInt* in) {
    out->sign = in->sign;
    out->nlimbs = in->nlimbs > 0 ? in->nlimbs : 1;
    out->limbs = malloc((size_t)out->nlimbs * sizeof(unsigned int));
    if (!out->limbs) return false;
    for (int i = 0; i < in->nlimbs; i++) out->limbs[i] = in->limbs[i];
    for (int i = in->nlimbs; i < out->nlimbs; i++) out->limbs[i] = 0;
    return true;
}

// Build from a bare ASCII digit run (no sign; leading zeros ok).
static bool bi_from_digits(BigInt* out, const char* digits) {
    const char* p = digits;
    while (*p == '0' && p[1] != '\0') p++;
    size_t nd = strlen(p);
    out->sign = 1;
    out->nlimbs = (int)((nd + 8) / 9);
    if (out->nlimbs < 1) out->nlimbs = 1;
    if (out->nlimbs > FRAC_MAX_LIMBS) return false;
    out->limbs = calloc((size_t)out->nlimbs, sizeof(unsigned int));
    if (!out->limbs) return false;
    long pos = 0;
    for (long i = (long)nd - 1; i >= 0; i--) {
        int dv = p[i] - '0';
        if (dv < 0 || dv > 9) { bi_free(out); return false; }
        int off = (int)(pos % 9);
        unsigned int mult = 1;
        for (int t = 0; t < off; t++) mult *= 10;
        out->limbs[pos / 9] += (unsigned int)dv * mult;
        pos++;
    }
    if (bi_is_zero(out)) out->sign = 1;
    return true;
}

// 10^k for k >= 0 (used by float rendering and decimal-string parsing).
static bool bi_pow10(BigInt* out, long k) {
    if (k < 0) return false;
    int nlimbs = (int)(k / 9) + 1;
    if (nlimbs > FRAC_MAX_LIMBS) return false;
    out->sign = 1;
    out->limbs = calloc((size_t)nlimbs, sizeof(unsigned int));
    if (!out->limbs) return false;
    out->nlimbs = nlimbs;
    int off = (int)(k % 9);
    unsigned int mult = 1;
    for (int t = 0; t < off; t++) mult *= 10;
    out->limbs[k / 9] = mult;
    return true;
}

// Magnitude compare of limb runs (leading zeros ignored either side).
static int limbs_cmp(const unsigned int* a, int na, const unsigned int* b, int nb) {
    int ha = na - 1; while (ha >= 0 && a[ha] == 0) ha--;
    int hb = nb - 1; while (hb >= 0 && b[hb] == 0) hb--;
    if (ha != hb) return ha < hb ? -1 : 1;
    for (int i = ha; i >= 0; i--) if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}

// Full signed compare: -1 / 0 / +1.
static int bi_cmp(const BigInt* a, const BigInt* b) {
    bool az = bi_is_zero(a), bz = bi_is_zero(b);
    if (az && bz) return 0;
    if (az) return b->sign < 0 ? 1 : -1;
    if (bz) return a->sign < 0 ? -1 : 1;
    if (a->sign != b->sign) return a->sign < 0 ? -1 : 1;
    int mag = limbs_cmp(a->limbs, a->nlimbs, b->limbs, b->nlimbs);
    return a->sign < 0 ? -mag : mag;
}

// out = |a| + |b| (fresh buffer; out->limbs released first).
static bool mag_add(const unsigned int* a, int na, const unsigned int* b, int nb, BigInt* out) {
    int n = (na > nb ? na : nb) + 1;
    unsigned int* res = calloc((size_t)n, sizeof(unsigned int)); // fresh buffer first: makes out-aliases-input safe
    if (!res) return false;
    out->nlimbs = n;
    unsigned long long carry = 0;
    for (int i = 0; i < n; i++) {
        unsigned long long s = carry
            + (unsigned long long)(i < na ? a[i] : 0)
            + (unsigned long long)(i < nb ? b[i] : 0);
        res[i] = (unsigned int)(s % 1000000000u);
        carry = s / 1000000000u;
    }
    free(out->limbs);
    out->limbs = res;
    out->nlimbs = n;
    bi_trim(out);
    return true;
}

// out = |a| - |b|; REQUIRES |a| >= |b|. Result computed into a fresh
// buffer BEFORE the old one is freed, so aliasing (out == a) is safe.
static bool mag_sub(const unsigned int* a, int na, const unsigned int* b, int nb, BigInt* out) {
    unsigned int* res = malloc((size_t)na * sizeof(unsigned int));
    if (!res) return false;
    long long borrow = 0;
    for (int i = 0; i < na; i++) {
        long long d = (long long)a[i] - (i < nb ? b[i] : 0) - borrow;
        if (d < 0) { d += 1000000000LL; borrow = 1; } else borrow = 0;
        res[i] = (unsigned int)d;
    }
    free(out->limbs);
    out->limbs = res;
    out->nlimbs = na;
    bi_trim(out);
    return true;
}

// out = a + b (signed); exact cancel normalizes to +0.
static bool bi_add(BigInt* out, const BigInt* a, const BigInt* b) {
    if (a->sign == b->sign) {
        out->sign = a->sign;
        if (!mag_add(a->limbs, a->nlimbs, b->limbs, b->nlimbs, out)) return false;
    } else {
        int mag = limbs_cmp(a->limbs, a->nlimbs, b->limbs, b->nlimbs);
        if (mag == 0) return bi_zero(out);
        if (mag > 0) {
            out->sign = a->sign;
            if (!mag_sub(a->limbs, a->nlimbs, b->limbs, b->nlimbs, out)) return false;
        } else {
            out->sign = b->sign;
            if (!mag_sub(b->limbs, b->nlimbs, a->limbs, a->nlimbs, out)) return false;
        }
    }
    if (bi_is_zero(out)) out->sign = 1;
    return true;
}

// out = a - b via a + (-b) copy (one path, fewer corners).
static bool bi_sub(BigInt* out, const BigInt* a, const BigInt* b) {
    BigInt nb;
    if (!bi_copy(&nb, b)) return false;
    nb.sign = -nb.sign;
    bool ok = bi_add(out, a, &nb);
    bi_free(&nb);
    return ok;
}

// out = a * b, schoolbook with per-limb carry. Exact.
static bool bi_mul(BigInt* out, const BigInt* a, const BigInt* b) {
    if (bi_is_zero(a) || bi_is_zero(b)) {
        free(out->limbs);
        return bi_zero(out);
    }
    int n = a->nlimbs + b->nlimbs;
    if (n > FRAC_MAX_LIMBS) return false;
    unsigned int* res = calloc((size_t)n, sizeof(unsigned int)); // fresh buffer first: alias-safe
    if (!res) return false;
    int sign = a->sign * b->sign;
    out->nlimbs = n;
    for (int i = 0; i < a->nlimbs; i++) {
        unsigned long long carry = 0;
        for (int j = 0; j < b->nlimbs; j++) {
            unsigned long long cur = (unsigned long long)a->limbs[i] * b->limbs[j]
                                   + res[i + j] + carry;
            res[i + j] = (unsigned int)(cur % 1000000000u);
            carry = cur / 1000000000u;
        }
        int k = i + b->nlimbs;
        while (carry) {
            unsigned long long cur = (unsigned long long)res[k] + carry;
            res[k] = (unsigned int)(cur % 1000000000u);
            carry = cur / 1000000000u;
            k++;
        }
    }
    free(out->limbs);
    out->limbs = res;
    out->sign = sign;
    out->nlimbs = n;
    bi_trim(out);
    if (bi_is_zero(out)) out->sign = 1;
    return true;
}

// a /= m in place (1 <= m < 1e9); *r_out receives the remainder.
static bool bi_divmod_small(BigInt* a, unsigned int m, unsigned int* r_out) {
    if (m == 0) return false;
    unsigned long long r = 0;
    for (int i = a->nlimbs - 1; i >= 0; i--) {
        unsigned long long cur = r * 1000000000u + a->limbs[i];
        a->limbs[i] = (unsigned int)(cur / m);
        r = cur % m;
    }
    bi_trim(a);
    *r_out = (unsigned int)r;
    return true;
}

// a *= m in place (m >= 2); carry propagates to at most one new limb.
static bool bi_mul_small(BigInt* a, unsigned int m) {
    if (m <= 1 || bi_is_zero(a)) return true;
    unsigned long long carry = 0;
    for (int i = 0; i < a->nlimbs; i++) {
        unsigned long long cur = (unsigned long long)a->limbs[i] * m + carry;
        a->limbs[i] = (unsigned int)(cur % 1000000000u);
        carry = cur / 1000000000u;
    }
    if (carry) {
        int cap = a->nlimbs + 2;
        unsigned int* p = realloc(a->limbs, (size_t)cap * sizeof(unsigned int));
        if (!p) return false;
        a->limbs = p;
        a->limbs[a->nlimbs++] = (unsigned int)carry; // carry < 1e9 always
        bi_trim(a);
    }
    return true;
}

// Binary GCD (Stein) on magnitudes: strip shared twos, subtract-swap the
// odd cores, reapply. O(limbs^2) — fine at scripting scale.
static bool bi_gcd(const BigInt* a, const BigInt* b, BigInt* out) {
    if (bi_is_zero(a)) { out->sign = 1; return bi_copy(out, b); }
    if (bi_is_zero(b)) { out->sign = 1; return bi_copy(out, a); }
    BigInt u, v;
    if (!bi_copy(&u, a) || !bi_copy(&v, b)) { bi_free(&u); bi_free(&v); return false; }
    u.sign = 1; v.sign = 1;
    unsigned int r = 0;
    long twos = 0;
    while (u.limbs[0] % 2 == 0 && v.limbs[0] % 2 == 0) {
        if (!bi_divmod_small(&u, 2, &r) || !bi_divmod_small(&v, 2, &r)) goto fail;
        twos++;
    }
    while (u.limbs[0] % 2 == 0) { if (!bi_divmod_small(&u, 2, &r)) goto fail; }
    while (v.limbs[0] % 2 == 0) { if (!bi_divmod_small(&v, 2, &r)) goto fail; }
    for (;;) { // both odd now
        int c = limbs_cmp(u.limbs, u.nlimbs, v.limbs, v.nlimbs);
        if (c == 0) break;
        BigInt* big = c > 0 ? &u : &v;
        const BigInt* small = c > 0 ? &v : &u;
        if (!mag_sub(big->limbs, big->nlimbs, small->limbs, small->nlimbs, big)) goto fail;
        while (big->limbs[0] % 2 == 0) { // odd-odd diff is even: halve
            if (!bi_divmod_small(big, 2, &r)) goto fail;
        }
    }
    bi_free(&v);
    for (long i = 0; i < twos; i++) {
        if (!bi_mul_small(&u, 2)) { bi_free(&u); return false; }
    }
    u.sign = 1;
    *out = u;
    return true;
fail:
    bi_free(&u); bi_free(&v);
    return false;
}

// ---------------------------------------------------------------------------
// Long division engine (quotient + remainder), MSB-first one limb at a
// time. Quotient-limb estimate from the top limbs, corrected both
// directions — sound in base 1e9 without Knuth normalization (the same
// scheme the decimal engine proved over 40 pinned assertions).
// ---------------------------------------------------------------------------

// t = v * m into a fresh buffer (t != v).
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

// a -= b in place (requires a >= b).
static void limbs_sub_inplace(unsigned int* a, int an, const unsigned int* b, int bn) {
    long long borrow = 0;
    for (int i = 0; i < an; i++) {
        long long d = (long long)a[i] - (i < bn ? b[i] : 0) - borrow;
        if (d < 0) { d += 1000000000LL; borrow = 1; } else borrow = 0;
        a[i] = (unsigned int)d;
    }
}

// One division step: q = floor(R/D) (single limb, R < D*1e9), R -= q*D.
static bool div_step(unsigned int* R, int rn, const unsigned int* D, int dn,
                     unsigned int* q_out) {
    int rt = rn - 1; while (rt > 0 && R[rt] == 0) rt--;
    int dt = dn - 1; while (dt > 0 && D[dt] == 0) dt--;
    if (dt < 0 || rt < dt) { *q_out = 0; return true; }
    unsigned long long est;
    if (rt == dt) {
        est = D[dt] ? R[rt] / D[dt] : 0;
    } else if (rt == dt + 1) {
        unsigned long long num = (unsigned long long)R[rt] * 1000000000u + R[rt - 1];
        est = D[dt] ? num / D[dt] : 0;
        if (est > 999999999u) est = 999999999u;
    } else {
        est = 999999998u; // tower case: only reachable on the first step
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

// Signed divmod, quotient cut toward zero: q = a/b, r = a - q*b (sign of
// a). b == 0 -> false (caller reports conversationally). Outputs are
// optional (pass NULL); both receive fresh limb buffers.
static bool bi_divmod(const BigInt* a, const BigInt* b, BigInt* q_out, BigInt* r_out) {
    if (bi_is_zero(b)) return false;
    if (bi_is_zero(a)) {
        if (q_out && !bi_zero(q_out)) return false;
        if (r_out && !bi_zero(r_out)) return false;
        return true;
    }
    int nn = a->nlimbs, dn = b->nlimbs;
    unsigned int* Q = calloc((size_t)(nn + 1), sizeof(unsigned int));
    unsigned int* R = calloc((size_t)(dn + 2), sizeof(unsigned int));
    int rn = 0;
    if (!Q || !R) { free(Q); free(R); return false; }
    for (int i = nn - 1; i >= 0; i--) {
        memmove(&R[1], R, (size_t)rn * sizeof(unsigned int));
        R[0] = a->limbs[i];
        rn++;
        if (!div_step(R, rn, b->limbs, dn, &Q[i])) { free(Q); free(R); return false; }
        while (rn > 0 && R[rn - 1] == 0) rn--;
    }
    int qtop = nn;
    while (qtop > 1 && Q[qtop - 1] == 0) qtop--;
    if (q_out) {
        q_out->sign = a->sign * b->sign;
        q_out->limbs = malloc((size_t)qtop * sizeof(unsigned int));
        if (!q_out->limbs) { free(Q); free(R); return false; }
        q_out->nlimbs = qtop;
        for (int i = 0; i < qtop; i++) q_out->limbs[i] = Q[i];
        if (bi_is_zero(q_out)) q_out->sign = 1;
    }
    if (r_out) {
        int rtop = rn < 1 ? 1 : rn;
        r_out->sign = a->sign;
        r_out->limbs = calloc((size_t)rtop, sizeof(unsigned int));
        if (!r_out->limbs) { free(Q); free(R); return false; }
        r_out->nlimbs = rtop;
        for (int i = 0; i < rn; i++) r_out->limbs[i] = R[i];
        if (bi_is_zero(r_out)) r_out->sign = 1;
    }
    free(Q); free(R);
    return true;
}

// Decimal digits of a (with leading '-') into a fresh string.
static char* bi_to_string(const BigInt* a) {
    if (bi_is_zero(a)) {
        char* s = malloc(2);
        if (!s) return NULL;
        s[0] = '0'; s[1] = '\0';
        return s;
    }
    unsigned int top = a->limbs[a->nlimbs - 1];
    long nd = 1;
    while (top >= 10) { top /= 10; nd++; }
    nd += (long)(a->nlimbs - 1) * 9;
    char* buf = malloc((size_t)nd + 2);
    if (!buf) return NULL;
    int pos = 0;
    if (a->sign < 0) buf[pos++] = '-';
    pos += sprintf(&buf[pos], "%u", a->limbs[a->nlimbs - 1]);
    for (int i = a->nlimbs - 2; i >= 0; i--) pos += sprintf(&buf[pos], "%09u", a->limbs[i]);
    buf[pos] = '\0';
    return buf;
}

// Fit check for long long extraction (numerator/denominator natives).
static bool bi_fits_ll(const BigInt* a, long long* out) {
    if (a->nlimbs > 3) return false;
    unsigned long long m = 0;
    for (int i = a->nlimbs - 1; i >= 0; i--) m = m * 1000000000u + a->limbs[i];
    if (m > 9223372036854775807ull) return false;
    *out = a->sign < 0 ? -(long long)m : (long long)m;
    return true;
}

// ---------------------------------------------------------------------------
// Fraction layer: num/den coprime, den > 0. Zero = 0/1.
// ---------------------------------------------------------------------------

typedef struct {
    BigInt num;
    BigInt den;
} Frac;

static void fr_free(Frac* f) {
    bi_free(&f->num);
    bi_free(&f->den);
}

// Reduce num/den (ownership consumed) to coprime form. Sign convention:
// den always positive, num carries the sign, zero -> 0/1.
static bool fr_reduce(BigInt* num, BigInt* den) {
    if (bi_is_zero(den)) return false;
    if (bi_is_zero(num)) {
        bi_free(num);
        bi_free(den);
        return bi_zero(num) && bi_one(den);
    }
    int sign = num->sign * den->sign;
    BigInt g;
    memset(&g, 0, sizeof(BigInt));
    if (!bi_gcd(num, den, &g)) return false;
    BigInt qn, qd;
    memset(&qn, 0, sizeof(BigInt));
    memset(&qd, 0, sizeof(BigInt));
    bool ok = bi_divmod(num, &g, &qn, NULL) && bi_divmod(den, &g, &qd, NULL);
    bi_free(&g);
    bi_free(num);
    bi_free(den);
    if (!ok) { bi_free(&qn); bi_free(&qd); return false; }
    qn.sign = sign;
    qd.sign = 1;
    if (bi_is_zero(&qn)) qn.sign = 1;
    *num = qn;
    *den = qd;
    return true;
}

// f = num/den reduced; consumes ownership of both inputs either way.
static bool fr_make(Frac* f, BigInt num, BigInt den) {
    if (bi_is_zero(&den) || !fr_reduce(&num, &den)) {
        bi_free(&num);
        bi_free(&den);
        return false;
    }
    f->num = num;
    f->den = den;
    return true;
}

// f = n/d from long longs (the two-int constructor).
static bool fr_from_lls(Frac* f, long long n, long long d) {
    BigInt bn, bd;
    memset(&bn, 0, sizeof(BigInt));
    memset(&bd, 0, sizeof(BigInt));
    if (!bi_from_ll(&bn, n) || !bi_from_ll(&bd, d)) { bi_free(&bn); bi_free(&bd); return false; }
    return fr_make(f, bn, bd);
}

// Deep copy.
static bool fr_copy(Frac* out, const Frac* in) {
    memset(out, 0, sizeof(Frac));
    if (!bi_copy(&out->num, &in->num)) return false;
    if (!bi_copy(&out->den, &in->den)) { bi_free(&out->num); return false; }
    return true;
}

// out = a + b exact: (na*db + nb*da) / (da*db); equal-den fast path.
static bool fr_add(Frac* out, const Frac* a, const Frac* b) {
    if (bi_cmp(&a->den, &b->den) == 0) {
        BigInt nsum, dcopy;
        memset(&nsum, 0, sizeof(BigInt));
        memset(&dcopy, 0, sizeof(BigInt));
        if (!bi_add(&nsum, &a->num, &b->num)) return false;
        if (!bi_copy(&dcopy, &a->den)) { bi_free(&nsum); return false; }
        return fr_make(out, nsum, dcopy);
    }
    BigInt an, bd, nsum, den;
    memset(&an, 0, sizeof(BigInt)); memset(&bd, 0, sizeof(BigInt));
    memset(&nsum, 0, sizeof(BigInt)); memset(&den, 0, sizeof(BigInt));
    if (!bi_mul(&an, &a->num, &b->den)) return false;
    if (!bi_mul(&bd, &b->num, &a->den)) { bi_free(&an); return false; }
    if (!bi_add(&nsum, &an, &bd)) { bi_free(&an); bi_free(&bd); return false; }
    bi_free(&an); bi_free(&bd);
    if (!bi_mul(&den, &a->den, &b->den)) { bi_free(&nsum); return false; }
    return fr_make(out, nsum, den);
}

// out = a - b via a + (-b) copy.
static bool fr_sub(Frac* out, const Frac* a, const Frac* b) {
    Frac negb;
    if (!fr_copy(&negb, b)) return false;
    negb.num.sign = -negb.num.sign;
    if (bi_is_zero(&negb.num)) negb.num.sign = 1;
    bool ok = fr_add(out, a, &negb);
    fr_free(&negb);
    return ok;
}

// out = a * b exact: (na*nb) / (da*db).
static bool fr_mul(Frac* out, const Frac* a, const Frac* b) {
    BigInt n, d;
    memset(&n, 0, sizeof(BigInt)); memset(&d, 0, sizeof(BigInt));
    if (!bi_mul(&n, &a->num, &b->num)) return false;
    if (!bi_mul(&d, &a->den, &b->den)) { bi_free(&n); return false; }
    return fr_make(out, n, d);
}

// out = a / b exact: (na*db) / (da*nb); b == 0 rejected at the native.
static bool fr_div(Frac* out, const Frac* a, const Frac* b) {
    if (bi_is_zero(&b->num)) return false;
    BigInt n, d;
    memset(&n, 0, sizeof(BigInt)); memset(&d, 0, sizeof(BigInt));
    if (!bi_mul(&n, &a->num, &b->den)) return false;
    if (!bi_mul(&d, &a->den, &b->num)) { bi_free(&n); return false; }
    if (bi_is_zero(&d)) { bi_free(&n); bi_free(&d); return false; }
    return fr_make(out, n, d);
}

// out = a^e exact (square-and-multiply on the coprime parts — a reduced
// fraction raised to an integer power stays reduced). e==0 -> 1;
// 0 ** negative fails (caller reports).
static bool fr_pow(Frac* out, const Frac* a, long e) {
    memset(out, 0, sizeof(Frac));
    if (e == 0) return bi_one(&out->num) && bi_one(&out->den);
    if (bi_is_zero(&a->num) && e < 0) return false;
    if (e < -1000000L || e > 1000000L) return false;
    bool invert = e < 0;
    long ae = invert ? -e : e;
    BigInt bn, bd, an, ad;
    memset(&bn, 0, sizeof(BigInt)); memset(&bd, 0, sizeof(BigInt));
    memset(&an, 0, sizeof(BigInt)); memset(&ad, 0, sizeof(BigInt));
    if (!bi_copy(&bn, &a->num) || !bi_copy(&bd, &a->den)
        || !bi_one(&an) || !bi_one(&ad)) {
        bi_free(&bn); bi_free(&bd); bi_free(&an); bi_free(&ad);
        return false;
    }
    while (ae > 0) {
        if (ae & 1) {
            BigInt r1, r2;
            memset(&r1, 0, sizeof(BigInt)); memset(&r2, 0, sizeof(BigInt));
            bool ok = bi_mul(&r1, &an, &bn) && bi_mul(&r2, &ad, &bd);
            bi_free(&an); bi_free(&ad);
            if (!ok) { bi_free(&r1); bi_free(&r2); bi_free(&bn); bi_free(&bd); return false; }
            an = r1; ad = r2;
        }
        ae >>= 1;
        if (ae > 0) {
            BigInt r3, r4;
            memset(&r3, 0, sizeof(BigInt)); memset(&r4, 0, sizeof(BigInt));
            bool ok = bi_mul(&r3, &bn, &bn) && bi_mul(&r4, &bd, &bd);
            bi_free(&bn); bi_free(&bd);
            if (!ok) { bi_free(&r3); bi_free(&r4); bi_free(&an); bi_free(&ad); return false; }
            bn = r3; bd = r4;
        }
    }
    bi_free(&bn); bi_free(&bd);
    if (invert) { // 1/(n/d) = d/n: swap parts; denominator stays positive
        int sign = an.sign;          // sign of the value = sign of the numerator
        BigInt tmp = an; an = ad; ad = tmp; // now an=den-magnitude, ad=num-magnitude
        an.sign = sign;              // new numerator carries the sign
        ad.sign = 1;                 // new denominator always positive
        if (bi_is_zero(&an)) { bi_free(&an); bi_free(&ad); return false; }
    }
    out->num = an; // (splice repair: adopt numerator)
    out->den = ad;
    if (bi_is_zero(&out->num)) out->num.sign = 1;
    return true;
}

// Python floor division on rationals: cross-multiply, integer divmod with
// quotient cut toward zero, then q -= 1 when a remainder survives with
// mixed signs (the floor adjustment).
static bool fr_floordiv(Frac* out, const Frac* a, const Frac* b) {
    if (bi_is_zero(&b->num)) return false;
    BigInt n, d, q, r;
    memset(&n, 0, sizeof(BigInt)); memset(&d, 0, sizeof(BigInt));
    memset(&q, 0, sizeof(BigInt)); memset(&r, 0, sizeof(BigInt));
    if (!bi_mul(&n, &a->num, &b->den) || !bi_mul(&d, &a->den, &b->num)) {
        bi_free(&n); bi_free(&d); return false;
    }
    if (!bi_divmod(&n, &d, &q, &r)) { bi_free(&n); bi_free(&d); return false; }
    if (!bi_is_zero(&r) && n.sign != d.sign) {
        BigInt one, qm;
        memset(&one, 0, sizeof(BigInt)); memset(&qm, 0, sizeof(BigInt));
        bool ok = bi_one(&one) && bi_sub(&qm, &q, &one);
        bi_free(&one); bi_free(&q);
        if (!ok) { bi_free(&qm); bi_free(&n); bi_free(&d); bi_free(&r); return false; }
        q = qm;
    }
    bi_free(&n); bi_free(&d); bi_free(&r);
    BigInt one;
    memset(&one, 0, sizeof(BigInt));
    if (!bi_one(&one)) { bi_free(&q); return false; }
    return fr_make(out, q, one);
}

// Python mod: a - floor(a/b)*b (result sign follows the divisor).
static bool fr_mod(Frac* out, const Frac* a, const Frac* b) {
    Frac q, qb;
    memset(&q, 0, sizeof(Frac)); memset(&qb, 0, sizeof(Frac));
    if (!fr_floordiv(&q, a, b)) return false;
    bool ok = fr_mul(&qb, &q, b) && fr_sub(out, a, &qb);
    fr_free(&q); fr_free(&qb);
    return ok;
}

// Cross-multiplied signed compare: na*db vs nb*da.
static int fr_cmp(const Frac* a, const Frac* b) {
    if (bi_cmp(&a->den, &b->den) == 0) return bi_cmp(&a->num, &b->num);
    BigInt l, r;
    memset(&l, 0, sizeof(BigInt)); memset(&r, 0, sizeof(BigInt));
    if (!bi_mul(&l, &a->num, &b->den) || !bi_mul(&r, &b->num, &a->den)) {
        bi_free(&l); bi_free(&r);
        return -2;
    }
    int c = bi_cmp(&l, &r);
    bi_free(&l); bi_free(&r);
    return c;
}

// str(): "num/den", or bare "num" when den == 1 (CPython __str__).
static char* fr_to_string(const Frac* f) {
    char* n = bi_to_string(&f->num);
    if (!n) return NULL;
    bool den_is_one = f->den.nlimbs == 1 && f->den.limbs[0] == 1;
    if (bi_is_zero(&f->num) || den_is_one) return n;
    char* d = bi_to_string(&f->den);
    if (!d) { free(n); return NULL; }
    char* out = malloc(strlen(n) + strlen(d) + 2);
    if (!out) { free(n); free(d); return NULL; }
    sprintf(out, "%s/%s", n, d);
    free(n); free(d);
    return out;
}

// Python int(): cut toward zero (checked fit; overflow reports at native).
static bool fr_to_ll(const Frac* f, long long* out) {
    BigInt q, r;
    memset(&q, 0, sizeof(BigInt)); memset(&r, 0, sizeof(BigInt));
    if (!bi_divmod(&f->num, &f->den, &q, &r)) return false;
    bi_free(&r);
    bool ok = bi_fits_ll(&q, out);
    bi_free(&q);
    return ok;
}

// Python float(): ~20 significant digits of the exact quotient rendered
// as d.dddE±ee, then strtod (glibc rounds the decimal string correctly).
static bool fr_to_double(const Frac* f, double* out) {
    if (bi_is_zero(&f->num)) { *out = 0.0; return true; }
    bool neg = f->num.sign < 0;
    BigInt n, d;
    memset(&n, 0, sizeof(BigInt)); memset(&d, 0, sizeof(BigInt));
    if (!bi_copy(&n, &f->num) || !bi_copy(&d, &f->den)) { bi_free(&n); bi_free(&d); return false; }
    n.sign = 1; d.sign = 1;
    char* ds_n = bi_to_string(&n); char* ds_d = bi_to_string(&d);
    if (!ds_n || !ds_d) { free(ds_n); free(ds_d); bi_free(&n); bi_free(&d); return false; }
    long len_n = (long)strlen(ds_n), len_d = (long)strlen(ds_d);
    free(ds_n); free(ds_d);
    long shift = len_n - len_d + 20; // target ~20 quotient digits
    BigInt sc, q, r;
    memset(&sc, 0, sizeof(BigInt)); memset(&q, 0, sizeof(BigInt)); memset(&r, 0, sizeof(BigInt));
    bool ok = true;
    if (shift > 0) {
        ok = bi_pow10(&sc, shift); if (ok) { BigInt t; memset(&t, 0, sizeof(BigInt)); ok = bi_mul(&t, &n, &sc); bi_free(&n); n = t; }
    } else if (shift < 0) {
        ok = bi_pow10(&sc, -shift); if (ok) { BigInt t2; memset(&t2, 0, sizeof(BigInt)); ok = bi_mul(&t2, &d, &sc); bi_free(&d); d = t2; }
    }
    if (!ok || !bi_divmod(&n, &d, &q, &r)) {
        bi_free(&sc); bi_free(&n); bi_free(&d); bi_free(&q); bi_free(&r);
        return false;
    }
    bi_free(&n); bi_free(&d); bi_free(&r); bi_free(&sc);
    char* qs = bi_to_string(&q);
    bi_free(&q);
    if (!qs) return false;
    long qlen = (long)strlen(qs);
    long exp10 = shift > 0 ? shift : 0; // digits added below the point
    long adj = qlen - 1 - exp10;        // decimal exponent of the lead digit
    char* sci = malloc((size_t)qlen + 24);
    if (!sci) { free(qs); return false; }
    int pos = 0;
    sci[pos++] = qs[0];
    sci[pos++] = '.';
    if (qlen > 1) { memcpy(&sci[pos], &qs[1], (size_t)(qlen - 1)); pos += (int)(qlen - 1); }
    else { sci[pos++] = '0'; }
    pos += sprintf(&sci[pos], "e%+ld", adj);
    sci[pos] = '\0';
    if (neg) {
        char* fin = malloc((size_t)pos + 2);
        if (!fin) { free(qs); free(sci); return false; }
        fin[0] = '-';
        memcpy(&fin[1], sci, (size_t)pos + 1);
        free(sci);
        sci = fin;
    }
    *out = strtod(sci, NULL);
    free(qs); free(sci);
    return true;
}

// CPython limit_denominator — ported line-for-line from Lib/fractions.py:
// continued-fraction convergents/semiconvergents; best upper and lower
// approximations with denominator <= max are computed and the closer one
// wins; ties go to the smaller denominator, and when denominators tie at
// max==1 the LOWER bound (floor) is taken. All arithmetic exact (BigInt).
//
// Loop invariants (from the Python source): p1/q1 and p0/q0 are the last
// two convergents, n/d the remaining fraction tail.

// ---------------------------------------------------------------------------
// Parsing — CPython Fraction string grammar (subset that matters):
//   [ws] [+-]? ( digits [. digits]? [. digits]? [/ digits]?
//              | digits [/ digits]
//              | digits [eE [+-] digits] ) [ws]
// i.e. "3/4", "-3/4", "7e-6", "0.5", "3.14", " 3/4 ". Leading + allowed.
// Rejected: "1/-4" (sign only on the whole), "3/0" (zero den), junk.
// ---------------------------------------------------------------------------

static bool fr_parse(const char* s, Frac* out) {
    memset(out, 0, sizeof(Frac));
    while (isspace((unsigned char)*s)) s++;
    int sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') sign = -1;
        s++;
    }
    // split on '/' if present (rationals); else decimal/scientific
    const char* slash = strchr(s, '/');
    if (slash) {
        const char* tail = slash + 1;
        // numerator: digits with optional single dot (CPython allows
        // "1.5/2" — it parses both sides as decimal then divides)
        const char* p = s;
        size_t intd = 0, fracd = 0;
        while (isdigit((unsigned char)*p)) { intd++; p++; }
        if (*p == '.') { p++; while (isdigit((unsigned char)*p)) { fracd++; p++; } }
        if (intd + fracd == 0 || p != slash) goto bad;
        size_t ndig = intd + fracd;
        char* nrun = malloc(ndig + 1);
        if (!nrun) return false;
        size_t w = 0;
        for (size_t i = 0; i < intd; i++) nrun[w++] = s[i];
        for (size_t i = 0; i < fracd; i++) nrun[w++] = s[intd + 1 + i];
        nrun[w] = '\0';
        // denominator: bare digits (CPython rejects signs here)
        p = tail;
        size_t ddig = 0;
        while (isdigit((unsigned char)*p)) { ddig++; p++; }
        while (isspace((unsigned char)*p)) p++;
        if (ddig == 0 || *p != '\0') { free(nrun); goto bad; }
        char* drun = malloc(ddig + 1);
        if (!drun) { free(nrun); return false; }
        memcpy(drun, tail, ddig);
        drun[ddig] = '\0';
        BigInt bn, bd, scale;
        memset(&bn, 0, sizeof(BigInt)); memset(&bd, 0, sizeof(BigInt));
        memset(&scale, 0, sizeof(BigInt));
        bool ok = bi_from_digits(&bn, nrun) && bi_from_digits(&bd, drun);
        free(nrun); free(drun);
        if (!ok) { bi_free(&bn); bi_free(&bd); return false; }
        bn.sign = sign;
        if (fracd > 0) { // 1.5/2 == 15/(10*2)
            if (!bi_pow10(&scale, (long)fracd)) { bi_free(&bn); bi_free(&bd); return false; }
            BigInt t;
            memset(&t, 0, sizeof(BigInt));
            bool m_ok = bi_mul(&t, &bd, &scale);
            bi_free(&bd); bi_free(&scale);
            if (!m_ok) { bi_free(&bn); bi_free(&t); return false; }
            bd = t;
        }
        return fr_make(out, bn, bd);
    }
    // decimal / scientific form
    const char* p = s;
    size_t intd = 0, fracd = 0;
    while (isdigit((unsigned char)*p)) { intd++; p++; }
    if (*p == '.') {
        p++;
        while (isdigit((unsigned char)*p)) { fracd++; p++; }
    }
    if (intd + fracd == 0) goto bad;
    long expo = 0;
    if (*p == 'e' || *p == 'E') {
        p++;
        bool eneg = false;
        if (*p == '+' || *p == '-') { eneg = (*p == '-'); p++; }
        if (!isdigit((unsigned char)*p)) goto bad;
        while (isdigit((unsigned char)*p)) {
            expo = expo * 10 + (*p - '0');
            if (expo > 1000000L) goto bad;
            p++;
        }
        if (eneg) expo = -expo;
    }
    while (isspace((unsigned char)*p)) p++;
    if (*p != '\0') goto bad;
    size_t ndig = intd + fracd;
    char* run = malloc(ndig + 1);
    if (!run) return false;
    size_t w = 0;
    for (size_t i = 0; i < intd; i++) run[w++] = s[i];
    for (size_t i = 0; i < fracd; i++) run[w++] = s[intd + 1 + i];
    run[w] = '\0';
    BigInt bn, scale;
    memset(&bn, 0, sizeof(BigInt)); memset(&scale, 0, sizeof(BigInt));
    bool ok = bi_from_digits(&bn, run);
    free(run);
    if (!ok) return false;
    bn.sign = sign;
    long e = (long)fracd - expo; // denominator 10^e (e may be negative)
    if (e > 0) {
        if (!bi_pow10(&scale, e)) { bi_free(&bn); return false; }
        bool m = fr_make(out, bn, scale);
        return m;
    } else if (e < 0) {
        if (!bi_pow10(&scale, -e)) { bi_free(&bn); return false; }
        BigInt t;
        memset(&t, 0, sizeof(BigInt));
        bool m_ok = bi_mul(&t, &bn, &scale);
        bi_free(&bn); bi_free(&scale);
        if (!m_ok) { bi_free(&t); return false; }
        BigInt one;
        memset(&one, 0, sizeof(BigInt));
        if (!bi_one(&one)) { bi_free(&t); return false; }
        return fr_make(out, t, one);
    }
    BigInt one;
    memset(&one, 0, sizeof(BigInt));
    if (!bi_one(&one)) { bi_free(&bn); return false; }
    return fr_make(out, bn, one);
bad:
    fprintf(stderr, "Error: fractions cannot parse \"%s\" (expected forms: \"3/4\", \"0.5\", \"7e-6\").\n", s);
    return false;
}

// Exact binary conversion for float inputs — CPython F(float) takes the
// exact binary value (F(0.1) == 3602879701896397/36028797018963968).

// ---------------------------------------------------------------------------
// Native surface. Fractions flow in/out as structs (definition name
// "Fraction") with fields text (str-form), numerator, denominator — the
// same registry-owned-def pattern as stdlib_decimal.c / stdlib_datetime.c.
// ---------------------------------------------------------------------------

static StructDef* fraction_def(void) {
    static StructDef def;
    static bool init = false;
    if (!init) {
        def.name = strdup("Fraction");
        def.ast_node = NULL;
        init = true;
    }
    return &def;
}

// Box a Frac into a struct Value { text, numerator, denominator }.
static Value fraction_to_value(const Frac* f) {
    char* s = fr_to_string(f);
    if (!s) return make_null();
    StructInstance* inst = malloc(sizeof(StructInstance));
    if (!inst) { free(s); return make_null(); }
    inst->definition = fraction_def();
    inst->fields = NULL;
    long long num_ll = 0, den_ll = 0;
    bool fits = bi_fits_ll(&f->num, &num_ll) && bi_fits_ll(&f->den, &den_ll);
    static const char* const names[3] = { "text", "numerator", "denominator" };
    FieldValue* tail = NULL;
    for (int i = 0; i < 3; i++) {
        FieldValue* fl = malloc(sizeof(FieldValue));
        if (!fl) { free(s); return make_null(); }
        fl->name = strdup(names[i]);
        fl->value = malloc(sizeof(Value));
        if (!fl->value) { free(fl); free(s); return make_null(); }
        if (i == 0) {
            fl->value->type = VAL_STRING;
            fl->value->string_val = s; // ownership moves into the field
        } else {
            fl->value->type = VAL_INT;
            fl->value->int_val = i == 1 ? (fits ? num_ll : 0) : (fits ? den_ll : 0);
        }
        fl->next = NULL;
        if (tail) tail->next = fl; else inst->fields = fl;
        tail = fl;
    }
    Value v;
    v.type = VAL_STRUCT_INSTANCE;
    v.struct_instance = inst;
    return v;
}

// Exact binary conversion for float inputs — CPython F(float) takes the
// exact binary value (F(0.1) == 3602879701896397/36028797018963968).
// Reads the raw IEEE-754 bit pattern: x = M * 2^(E-1075) where M is the
// full 53-bit significand (implicit leading 1 restored for normals) and
// E the biased exponent. No floating multiplies — bits cannot lie.
static bool fr_from_double_impl(Frac* out, double x) {
    if (isnan(x) || isinf(x)) return false;
    unsigned long long bits;
    memcpy(&bits, &x, sizeof(bits));
    bool neg = (bits >> 63) != 0;
    int E = (int)((bits >> 52) & 0x7FF);
    unsigned long long frac = bits & 0xFFFFFFFFFFFFFull;
    long long M;
    long e2;
    if (E == 0) {            // subnormal (or zero — handled below)
        if (frac == 0) return fr_from_lls(out, 0, 1);
        M = (long long)frac;
        e2 = -1074;
    } else {                 // normal: restore the implicit leading 1
        M = (long long)frac | (1LL << 52);
        e2 = (long)E - 1075;
    }
    BigInt bn, bd, two;
    memset(&bn, 0, sizeof(BigInt)); memset(&bd, 0, sizeof(BigInt));
    memset(&two, 0, sizeof(BigInt));
    if (!bi_from_ll(&bn, M) || !bi_from_ll(&two, 2)) { bi_free(&bn); bi_free(&two); return false; }
    bn.sign = neg ? -1 : 1;
    bool ok = true;
    if (e2 >= 0) {
        for (long i = 0; i < e2 && ok; i++) {
            BigInt t; memset(&t, 0, sizeof(BigInt));
            ok = bi_mul(&t, &bn, &two);
            bi_free(&bn);
            if (ok) bn = t;
        }
        BigInt one; memset(&one, 0, sizeof(BigInt));
        if (ok && bi_one(&one)) { bi_free(&two); return fr_make(out, bn, one); }
        bi_free(&one); bi_free(&bn); bi_free(&two);
        return false;
    }
    if (!bi_one(&bd)) { bi_free(&bn); bi_free(&two); return false; }
    for (long i = 0; i < -e2 && ok; i++) {
        BigInt t; memset(&t, 0, sizeof(BigInt));
        ok = bi_mul(&t, &bd, &two);
        bi_free(&bd);
        if (ok) bd = t;
    }
    bi_free(&two);
    if (!ok) { bi_free(&bn); bi_free(&bd); return false; }
    return fr_make(out, bn, bd);
}
// Unbox: Fraction struct (by "text" field), int, float (exact binary), or
// string (rational/decimal/scientific grammar). Errors go to stderr.
static bool fraction_from_value(Value* v, Frac* out, const char* ctx) {
    memset(out, 0, sizeof(Frac));
    if (v->type == VAL_STRING) {
        if (!fr_parse(v->string_val, out)) return false;
        return true;
    }
    if (v->type == VAL_INT) return fr_from_lls(out, v->int_val, 1);
    if (v->type == VAL_FLOAT) {
        if (!fr_from_double_impl(out, v->float_val)) {
            fprintf(stderr, "Error: fractions.%s: float must be finite.\n", ctx);
            return false;
        }
        return true;
    }
    if (v->type == VAL_STRUCT_INSTANCE) {
        FieldValue* fl = v->struct_instance->fields;
        while (fl) {
            if (strcmp(fl->name, "text") == 0 && fl->value->type == VAL_STRING) {
                return fr_parse(fl->value->string_val, out);
            }
            fl = fl->next;
        }
    }
    fprintf(stderr, "Error: fractions.%s: cannot interpret value of type %d as a fraction.\n", ctx, (int)v->type);
    return false;
}

// Shared two-operand arithmetic wrapper: unbox, op, box, free.
static Value fr_binop(Value* args, int argc, const char* name,
                      bool (*op)(Frac*, const Frac*, const Frac*)) {
    if (argc != 2) {
        fprintf(stderr, "Error: fractions.%s(a, b) takes two arguments.\n", name);
        return make_null();
    }
    Frac a, b, r;
    memset(&r, 0, sizeof(Frac));
    if (!fraction_from_value(&args[0], &a, name)) return make_null();
    if (!fraction_from_value(&args[1], &b, name)) { fr_free(&a); return make_null(); }
    bool ok = op(&r, &a, &b);
    if (!ok) {
        if (strcmp(name, "div") == 0 || strcmp(name, "floordiv") == 0 || strcmp(name, "mod") == 0)
            fprintf(stderr, "Error: fractions.%s division by zero.\n", name);
        else
            fprintf(stderr, "Error: fractions.%s failed (out of memory or invalid).\n", name);
        fr_free(&a); fr_free(&b); fr_free(&r);
        return make_null();
    }
    Value v = fraction_to_value(&r);
    fr_free(&a); fr_free(&b); fr_free(&r);
    return v;
}

static bool fr_op_add(Frac* r, const Frac* a, const Frac* b) { return fr_add(r, a, b); }
static bool fr_op_sub(Frac* r, const Frac* a, const Frac* b) { return fr_sub(r, a, b); }
static bool fr_op_mul(Frac* r, const Frac* a, const Frac* b) { return fr_mul(r, a, b); }
static bool fr_op_div(Frac* r, const Frac* a, const Frac* b) { return fr_div(r, a, b); }
static bool fr_op_floordiv(Frac* r, const Frac* a, const Frac* b) { return fr_floordiv(r, a, b); }
static bool fr_op_mod(Frac* r, const Frac* a, const Frac* b) { return fr_mod(r, a, b); }

// Native entry points -------------------------------------------------------

Value stdlib_fractions_make(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    // fractions(3, 4) / fractions("3/4") / fractions(0.5) / fractions(7)
    if (argc == 2) {
        if (args[0].type == VAL_INT && args[1].type == VAL_INT) {
            if (args[1].int_val == 0) {
                fprintf(stderr, "Error: fractions(n, 0) — denominator cannot be zero (Python raises ZeroDivisionError).\n");
                return make_null();
            }
            Frac f;
            if (!fr_from_lls(&f, args[0].int_val, args[1].int_val)) return make_null();
            Value v = fraction_to_value(&f);
            fr_free(&f);
            return v;
        }
        // numerator/denominator as strings or mixed: route through parse
        Frac fn, fd;
        if (!fraction_from_value(&args[0], &fn, "make") || !fraction_from_value(&args[1], &fd, "make")) {
            return make_null(); // fraction_from_value already freed nothing shared
        }
        Frac r;
        memset(&r, 0, sizeof(Frac));
        bool ok = fr_div(&r, &fn, &fd);
        if (!ok) fprintf(stderr, "Error: fractions division by zero in constructor.\n");
        fr_free(&fn); fr_free(&fd);
        if (!ok) { fr_free(&r); return make_null(); }
        Value v = fraction_to_value(&r);
        fr_free(&r);
        return v;
    }
    if (argc == 1) {
        Frac f;
        if (!fraction_from_value(&args[0], &f, "make")) return make_null();
        Value v = fraction_to_value(&f);
        fr_free(&f);
        return v;
    }
    fprintf(stderr, "Error: fractions() takes one value or (numerator, denominator).\n");
    return make_null();
}

Value stdlib_fractions_to_str(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.to_str(f) takes one argument.\n"); return make_null(); }
    Frac f;
    if (!fraction_from_value(&args[0], &f, "str")) return make_null();
    char* s = fr_to_string(&f);
    fr_free(&f);
    if (!s) return make_null();
    Value v = make_string(s);
    free(s);
    return v;
}

Value stdlib_fractions_add(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_binop(args, argc, "add", fr_op_add);
}
Value stdlib_fractions_sub(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_binop(args, argc, "sub", fr_op_sub);
}
Value stdlib_fractions_mul(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_binop(args, argc, "mul", fr_op_mul);
}
Value stdlib_fractions_div(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_binop(args, argc, "div", fr_op_div);
}
Value stdlib_fractions_floordiv(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_binop(args, argc, "floordiv", fr_op_floordiv);
}
Value stdlib_fractions_mod(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_binop(args, argc, "mod", fr_op_mod);
}

Value stdlib_fractions_pow(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2) { fprintf(stderr, "Error: fractions.pow(f, n) takes a fraction and an integer exponent.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "pow")) return make_null();
    if (args[1].type != VAL_INT) {
        fprintf(stderr, "Error: fractions.pow exponent must be an integer.\n");
        fr_free(&a);
        return make_null();
    }
    Frac r;
    if (!fr_pow(&r, &a, (long)args[1].int_val)) {
        fprintf(stderr, "Error: fractions.pow: 0 cannot be raised to a negative power (Python raises ZeroDivisionError).\n");
        fr_free(&a);
        return make_null();
    }
    fr_free(&a);
    Value v = fraction_to_value(&r);
    fr_free(&r);
    return v;
}

Value stdlib_fractions_neg(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.neg(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "neg")) return make_null();
    a.num.sign = -a.num.sign;
    if (bi_is_zero(&a.num)) a.num.sign = 1;
    Value v = fraction_to_value(&a);
    fr_free(&a);
    return v;
}

Value stdlib_fractions_abs(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.abs(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "abs")) return make_null();
    a.num.sign = 1;
    Value v = fraction_to_value(&a);
    fr_free(&a);
    return v;
}

// Comparison family: eq/lt/gt/lte/gte/cmp via cross-multiplied fr_cmp.
static Value fr_cmpop(Value* args, int argc, const char* name, int mode) {
    if (argc != 2) { fprintf(stderr, "Error: fractions.%s(a, b) takes two arguments.\n", name); return make_null(); }
    Frac a, b;
    if (!fraction_from_value(&args[0], &a, name) || !fraction_from_value(&args[1], &b, name)) {
        return make_null();
    }
    int c = fr_cmp(&a, &b);
    fr_free(&a); fr_free(&b);
    if (c == -2) { fprintf(stderr, "Error: fractions.%s internal comparison failure.\n", name); return make_null(); }
    switch (mode) {
        case 0: return make_bool(c == 0);
        case 1: return make_bool(c < 0);
        case 2: return make_bool(c > 0);
        case 3: return make_bool(c <= 0);
        default: return make_bool(c >= 0);
    }
}

Value stdlib_fractions_eq(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_cmpop(args, argc, "eq", 0);
}
Value stdlib_fractions_lt(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_cmpop(args, argc, "lt", 1);
}
Value stdlib_fractions_gt(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_cmpop(args, argc, "gt", 2);
}
Value stdlib_fractions_lte(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_cmpop(args, argc, "lte", 3);
}
Value stdlib_fractions_gte(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_cmpop(args, argc, "gte", 4);
}

Value stdlib_fractions_to_int(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.to_int(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "to_int")) return make_null();
    long long out = 0;
    bool ok = fr_to_ll(&a, &out);
    fr_free(&a);
    if (!ok) { fprintf(stderr, "Error: fractions.to_int: value does not fit a 64-bit integer.\n"); return make_null(); }
    return make_int(out);
}

Value stdlib_fractions_to_float(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.to_float(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "to_float")) return make_null();
    double out = 0.0;
    bool ok = fr_to_double(&a, &out);
    fr_free(&a);
    if (!ok) return make_null();
    Value v = { .type = VAL_FLOAT, .float_val = out };
    return v;
}

Value stdlib_fractions_numerator(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.numerator(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "numerator")) return make_null();
    long long out = 0;
    bool ok = bi_fits_ll(&a.num, &out);
    fr_free(&a);
    if (!ok) { fprintf(stderr, "Error: fractions.numerator: value exceeds 64 bits (use .text).\n"); return make_null(); }
    return make_int(out);
}

Value stdlib_fractions_denominator(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.denominator(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "denominator")) return make_null();
    long long out = 0;
    bool ok = bi_fits_ll(&a.den, &out);
    fr_free(&a);
    if (!ok) { fprintf(stderr, "Error: fractions.denominator: value exceeds 64 bits (use .text).\n"); return make_null(); }
    return make_int(out);
}

Value stdlib_fractions_is_integer(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 1) { fprintf(stderr, "Error: fractions.is_integer(f) takes one argument.\n"); return make_null(); }
    Frac a;
    if (!fraction_from_value(&args[0], &a, "is_integer")) return make_null();
    bool is_int = a.den.nlimbs == 1 && a.den.limbs[0] == 1;
    fr_free(&a);
    return make_bool(is_int);
}

// ⚕ Forged for Master Zarigata 🜂
// CPython limit_denominator — continued-fraction convergents + semi-
// convergents, ported line-for-line from Lib/fractions.py. Best upper and
// lower approximations with denominator <= max are computed; the closer
// one wins; ties take the smaller denominator; when both denominators are
// equal (max == 1 and self midway between integers) the LOWER bound
// (floor) is taken. All arithmetic exact (BigInt).
static bool fr_limit_denominator(Frac* out, const Frac* self, long max_den) {
    if (max_den < 1) return false;
    BigInt maxb;
    memset(&maxb, 0, sizeof(BigInt));
    if (!bi_from_ll(&maxb, max_den)) return false;
    // CPython early return: denominator already within the bound.
    if (bi_cmp(&self->den, &maxb) <= 0) { bi_free(&maxb); return fr_copy(out, self); }
    BigInt p0, q0, p1, q1, n, d;
    memset(&p0, 0, sizeof(BigInt)); memset(&q0, 0, sizeof(BigInt));
    memset(&p1, 0, sizeof(BigInt)); memset(&q1, 0, sizeof(BigInt));
    memset(&n, 0, sizeof(BigInt)); memset(&d, 0, sizeof(BigInt));
    if (!bi_from_ll(&p0, 0) || !bi_from_ll(&q0, 1)
        || !bi_from_ll(&p1, 1) || !bi_from_ll(&q1, 0)
        || !bi_copy(&n, &self->num) || !bi_copy(&d, &self->den)) {
        goto fail;
    }
    for (;;) {
        // a = n // d with Python FLOOR semantics (n may be negative)
        BigInt a, q2, aq1, np1, nd, nn, rem;
        memset(&a, 0, sizeof(BigInt)); memset(&q2, 0, sizeof(BigInt));
        memset(&aq1, 0, sizeof(BigInt)); memset(&np1, 0, sizeof(BigInt));
        memset(&nd, 0, sizeof(BigInt)); memset(&nn, 0, sizeof(BigInt));
        memset(&rem, 0, sizeof(BigInt));
        if (!bi_divmod(&n, &d, &a, &rem)) goto loop_fail;
        if (!bi_is_zero(&rem) && n.sign != d.sign) { // floor adjust
            BigInt one, am;
            memset(&one, 0, sizeof(BigInt)); memset(&am, 0, sizeof(BigInt));
            bool adj_ok = bi_one(&one) && bi_sub(&am, &a, &one);
            bi_free(&one);
            if (!adj_ok) { bi_free(&am); goto loop_fail; }
            bi_free(&a);
            a = am;
        }
        bi_free(&rem);
        // q2 = q0 + a*q1; stop when it passes the bound
        if (!bi_mul(&aq1, &a, &q1) || !bi_add(&q2, &q0, &aq1)) goto loop_fail;
        if (bi_cmp(&q2, &maxb) > 0) {
            bi_free(&a); bi_free(&q2); bi_free(&aq1);
            bi_free(&np1); bi_free(&nd); bi_free(&nn);
            break;
        }
        // rotate: (p0,q0,p1,q1) = (p1,q1, p0+a*p1, q2)
        if (!bi_mul(&np1, &a, &p1) || !bi_add(&np1, &np1, &p0)) goto loop_fail; // CPython: p1_new = p0 + a*p1
        bi_free(&p0); p0 = p1; // old p0 buffer released; p0 now owns ex-p1 buffer
        bi_free(&q0); q0 = q1; // old q0 released; q0 owns ex-q1 buffer — do NOT free q1 again
        p1 = np1;
        memset(&np1, 0, sizeof(BigInt)); // moved: prevent alias double-free
        q1 = q2; // adopt: q1 buffer was handed to q0 above (no free)
        memset(&q2, 0, sizeof(BigInt)); // q2 must not alias q1 for loop_fail frees
        // tail: (n,d) = (d, n - a*d)
        if (!bi_mul(&nd, &a, &d) || !bi_sub(&nn, &n, &nd)) goto loop_fail;
        bi_free(&n); n = d; d = nn;
        memset(&nn, 0, sizeof(BigInt)); // moved: prevent alias double-free
        bi_free(&nd);
        bi_free(&a); bi_free(&aq1);
        continue;
    loop_fail:
        bi_free(&a); bi_free(&q2); bi_free(&aq1); bi_free(&np1);
        bi_free(&nd); bi_free(&nn); bi_free(&rem);
        goto fail;
    }
    // final semiconvergent: k = (max - q0) // q1 (operands nonneg: cut == floor)
    BigInt diff, k, kq1, pk, qk, lhs, rhs, two;
    memset(&diff, 0, sizeof(BigInt)); memset(&k, 0, sizeof(BigInt));
    memset(&kq1, 0, sizeof(BigInt)); memset(&pk, 0, sizeof(BigInt));
    memset(&qk, 0, sizeof(BigInt)); memset(&lhs, 0, sizeof(BigInt));
    memset(&rhs, 0, sizeof(BigInt)); memset(&two, 0, sizeof(BigInt));
    if (!bi_sub(&diff, &maxb, &q0) || !bi_divmod(&diff, &q1, &k, NULL)) goto tail_fail;
    if (!bi_mul(&kq1, &k, &q1) || !bi_add(&qk, &q0, &kq1)) goto tail_fail;
    if (!bi_mul(&pk, &k, &p1) || !bi_add(&pk, &pk, &p0)) goto tail_fail;
    if (!bi_from_ll(&two, 2)) goto tail_fail;
    bool cond = false;
    {
        BigInt lt; memset(&lt, 0, sizeof(BigInt));
        bool m1 = bi_mul(&lt, &two, &d);        // lt = 2*d
        bool m2 = m1 && bi_mul(&lhs, &lt, &qk);  // lhs = lt*qk (fresh out)
        bi_free(&lt);
        if (m2 && bi_copy(&rhs, &self->den)) {
            cond = bi_cmp(&lhs, &rhs) <= 0;   // 2*d*(q0+k*q1) <= self.den
        }
    }
    bool ok;
    if (cond) {                            // p1/q1 wins
        bi_free(&out->num); bi_free(&out->den);
        memset(out, 0, sizeof(Frac));
        ok = bi_copy(&out->num, &p1) && bi_copy(&out->den, &q1); // p1/q1 wins: denominator is q1, not 1
    } else {                               // (p0+k*p1)/(q0+k*q1) wins
        bi_free(&out->num); bi_free(&out->den);
        memset(out, 0, sizeof(Frac));
        ok = bi_copy(&out->num, &pk) && bi_copy(&out->den, &qk);
    }
    bi_free(&diff); bi_free(&k); bi_free(&kq1); bi_free(&pk);
    bi_free(&qk); bi_free(&lhs); bi_free(&rhs); bi_free(&two);
    bi_free(&p0); bi_free(&q0); bi_free(&p1); bi_free(&q1);
    bi_free(&n); bi_free(&d); bi_free(&maxb);
    return ok;
tail_fail:
    bi_free(&diff); bi_free(&k); bi_free(&kq1); bi_free(&pk);
    bi_free(&qk); bi_free(&lhs); bi_free(&rhs); bi_free(&two);
fail:
    bi_free(&p0); bi_free(&q0); bi_free(&p1); bi_free(&q1);
    bi_free(&n); bi_free(&d); bi_free(&maxb);
    return false;
}

Value stdlib_fractions_limit_denominator(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc != 2 || args[1].type != VAL_INT) {
        fprintf(stderr, "Error: fractions.limit_denominator(f, max) takes a fraction and an integer max >= 1.\n");
        return make_null();
    }
    if (args[1].int_val < 1) {
        fprintf(stderr, "Error: fractions.limit_denominator max must be at least 1 (Python raises ValueError).\n");
        return make_null();
    }
    Frac a, r;
    memset(&r, 0, sizeof(Frac));
    if (!fraction_from_value(&args[0], &a, "limit_denominator")) return make_null();
    bool ok = fr_limit_denominator(&r, &a, args[1].int_val);
    fr_free(&a);
    if (!ok) { fr_free(&r); return make_null(); }
    Value v = fraction_to_value(&r);
    fr_free(&r);
    return v;
}



// Exact binary conversion for float inputs — CPython F(float) takes the
// exact binary value (F(0.1) == 3602879701896397/36028797018963968).
// Uses the IEEE-754 bit pattern: x = mantissa_int * 2^(exp2-52), where
// mantissa_int is the full 52-bit significand (frexp normalizes m to
// [0.5,1), so m*2^53 is always an exact integer — 2^53 is the crossover).
// min/max: re-box the winner instead of returning the caller's Value —
// native args are released by the interpreter after the call, so an
// aliased struct return would dangle (same latent bug decimal.min/max
// carry; fixed here first because fractions tests nest these calls).
static Value fr_minmax(Value* args, int argc, bool want_min) {
    const char* name = want_min ? "min" : "max";
    if (argc != 2) { fprintf(stderr, "Error: fractions.%s(a, b) takes two arguments.\n", name); return make_null(); }
    Frac a, b, r;
    memset(&r, 0, sizeof(Frac));
    if (!fraction_from_value(&args[0], &a, name) || !fraction_from_value(&args[1], &b, name)) {
        return make_null();
    }
    int c = fr_cmp(&a, &b);
    bool ok = false;
    if (c != -2) {
        bool take_a = want_min ? (c <= 0) : (c >= 0);
        ok = fr_copy(&r, take_a ? &a : &b);
    }
    fr_free(&a); fr_free(&b);
    if (!ok) { fr_free(&r); return make_null(); }
    Value v = fraction_to_value(&r);
    fr_free(&r);
    return v;
}
Value stdlib_fractions_min(Interpreter* interp, int argc, Value* args); // fwd decls (register sits above definitions)
Value stdlib_fractions_max(Interpreter* interp, int argc, Value* args);

Value stdlib_fractions_max(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_minmax(args, argc, false);
}

Value stdlib_fractions_min(Interpreter* interp, int argc, Value* args) {
    (void)interp; return fr_minmax(args, argc, true);
}
void stdlib_fractions_register(void) {
    register_native("fractions.make", stdlib_fractions_make);
    register_native("fractions.to_str", stdlib_fractions_to_str);
    register_native("fractions.add", stdlib_fractions_add);
    register_native("fractions.sub", stdlib_fractions_sub);
    register_native("fractions.mul", stdlib_fractions_mul);
    register_native("fractions.div", stdlib_fractions_div);
    register_native("fractions.floordiv", stdlib_fractions_floordiv);
    register_native("fractions.mod", stdlib_fractions_mod);
    register_native("fractions.pow", stdlib_fractions_pow);
    register_native("fractions.neg", stdlib_fractions_neg);
    register_native("fractions.abs", stdlib_fractions_abs);
    register_native("fractions.eq", stdlib_fractions_eq);
    register_native("fractions.lt", stdlib_fractions_lt);
    register_native("fractions.gt", stdlib_fractions_gt);
    register_native("fractions.lte", stdlib_fractions_lte);
    register_native("fractions.gte", stdlib_fractions_gte);
    register_native("fractions.to_int", stdlib_fractions_to_int);
    register_native("fractions.to_float", stdlib_fractions_to_float);
    register_native("fractions.numerator", stdlib_fractions_numerator);
    register_native("fractions.denominator", stdlib_fractions_denominator);
    register_native("fractions.is_integer", stdlib_fractions_is_integer);
    register_native("fractions.limit_denominator", stdlib_fractions_limit_denominator);
    register_native("fractions.min", stdlib_fractions_min);
    register_native("fractions.max", stdlib_fractions_max);
}
