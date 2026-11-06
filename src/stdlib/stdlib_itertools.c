#define _POSIX_C_SOURCE 200809L
/* itertools — Python-parity LAZY iterator toolkit (nightshift 20261025).
 *
 * Full rewrite of the v0.0.9-era eager module: every combinator here returns
 * a first-class VAL_ITERATOR (pull-based, one-shot, refcounted — see the
 * Iterator protocol in core/interpreter.h) or a plain value for consumers
 * (next/list/reduce). Signatures and semantics follow CPython's itertools:
 *   count(start, step)      — infinite arithmetic sequence
 *   cycle(it)               — repeats a materialized snapshot forever
 *   repeat(obj, [times])    — obj over and over (times<0/omitted = infinite)
 *   chain(*its), zip(*its), zip_longest(a, b, [fill])   (fill is positional;
 *                               Python uses a kwarg — documented deviation)
 *   enumerate(it, [start])  — pairs [i, item] like Python
 *   islice(it, [start], stop, [step]) — stop=null means unbounded; negative
 *                               bounds clamp to 0 (CPython raises — deviation)
 *   takewhile/dropwhile(fn, it), filter/filterfalse(fn|null, it)
 *   map(fn, *its), starmap(fn, it), accumulate(it, [fn])
 *   compress(data, sels), pairwise(it)
 *   product(*its), combinations(it, r), permutations(it, [r])
 *   iter(x) — adapt array (or pass-through iterator); next(it, [default]);
 *   list(it) — materialize; reduce(fn, it, [initial])
 *
 * Memory law (learned the hard way, nightshift 20261024): the interpreter
 * frees the args array right after a native call, so every Value stored in
 * iterator state is an OWNED value_retain() copy, freed by the flavor's
 * cleanup when the last iterator reference drops.
 */
#include "stdlib_itertools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Value make_error(const char* msg);
extern Value value_retain(Value v);
extern bool iterator_next(Value* it, Value* out);
extern Value interpreter_execute_callback(Value callback, int argc, Value* args);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

static Value array_value(Array* arr) {
    Value v;
    v.type = VAL_ARRAY;
    v.array_val = arr;
    return v;
}

/* A pull-source: either an owned array Value (cursor in idx) or an owned
 * iterator Value (idx unused). Arrays are read live: mutation during
 * iteration sees the new count (Python raises instead — deviation). */
typedef struct SrcState {
    Value src;
    size_t idx; /* array/iterator cursor; also char offset for strings */
} SrcState;

/* Pull one OWNED value; false = this source is exhausted. */
static bool src_pull(SrcState* s, Value* out) {
    if (s->src.type == VAL_ARRAY) {
        Array* a = s->src.array_val;
        if (!a || s->idx >= a->count) return false;
        *out = value_retain(a->items[s->idx++]); /* array keeps its item */
        return true;
    }
    if (s->src.type == VAL_ITERATOR) return iterator_next(&s->src, out);
    if (s->src.type == VAL_STRING) { /* Python iterates strings char by char */
        const char* str = s->src.string_val;
        if (!str) return false;
        size_t byte = s->idx;
        if (str[byte] == '\0') return false;
        /* advance one UTF-8 code point (byte-safe fallback for legacy bytes) */
        size_t len = 1;
        unsigned char c = (unsigned char)str[byte];
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        while (len > 1 && str[byte + len - 1] == '\0') len--; /* clamp at NUL */
        char buf[8];
        if (len > sizeof(buf) - 1) len = sizeof(buf) - 1;
        memcpy(buf, str + byte, len);
        buf[len] = '\0';
        s->idx = byte + len;
        *out = make_string(buf); /* one code point as its own string */
        return true;
    }
    return false;
}

/* Own a copy of an argument as a SrcState source (arrays share items via
 * refcount; iterators share the cursor via refcount — aliases observe the
 * same position, same law as Python's shared generators). */
static bool src_from_arg(Value* arg, SrcState* out) {
    if (arg->type != VAL_ARRAY && arg->type != VAL_ITERATOR && arg->type != VAL_STRING) return false;
    out->src = value_retain(*arg);
    out->idx = 0;
    return true;
}

static void src_free(SrcState* s) { value_free(&s->src); }

/* Python truthiness for filter/compress selectors. */
static bool val_truthy(Value* v) {
    switch (v->type) {
        case VAL_BOOL: return v->bool_val;
        case VAL_INT: return v->int_val != 0;
        case VAL_FLOAT: return v->float_val != 0.0;
        case VAL_STRING: return v->string_val && v->string_val[0] != '\0';
        case VAL_ARRAY: return v->array_val && v->array_val->count > 0;
        case VAL_ITERATOR:
        case VAL_FUNCTION:
        case VAL_STRUCT_INSTANCE: return true;
        default: return false;
    }
}

/* Call a 1-arg callback; returns an OWNED result (null fn = passthrough). */
static Value call_fn1(Value* fn, Value* a) {
    if (fn->type != VAL_FUNCTION) return value_retain(*a);
    Value args[1] = { *a }; /* callback clones args internally */
    return interpreter_execute_callback(*fn, 1, args);
}

/* Call a 2-arg callback; returns an OWNED result. */
static Value call_fn2(Value* fn, Value* a, Value* b) {
    Value args[2] = { *a, *b };
    return interpreter_execute_callback(*fn, 2, args);
}

/* ---- count: infinite start, start+step, start+2*step, ... ---- */
typedef struct { long long cur, step; } StCount;

static bool count_next(Iterator* it, Value* out) {
    StCount* st = it->state;
    long long v = st->cur;
    st->cur += st->step; /* ll arithmetic; Python uses bigint — documented cap */
    *out = make_int(v);
    return true;
}
static void st_cleanup(Iterator* it) { free(it->state); }
static const IteratorVTable vt_count = { count_next, st_cleanup };

Value native_itertools_count(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    long long start = 0, step = 1;
    if (argc >= 1 && args[0].type == VAL_INT) start = args[0].int_val;
    else if (argc >= 1 && args[0].type == VAL_FLOAT) start = (long long)args[0].float_val;
    if (argc >= 2 && args[1].type == VAL_INT) step = args[1].int_val;
    else if (argc >= 2 && args[1].type == VAL_FLOAT) step = (long long)args[1].float_val;
    if (step == 0) { make_error("itertools.count: step must not be zero"); return make_null(); }
    StCount* st = malloc(sizeof(StCount));
    st->cur = start; st->step = step;
    return make_iterator(&vt_count, st);
}

/* ---- repeat: one value, n times (or forever when times omitted) ---- */
typedef struct { Value item; long long remaining; } StRepeat;

static bool repeat_next(Iterator* it, Value* out) {
    StRepeat* st = it->state;
    if (st->remaining == 0) return false;
    if (st->remaining > 0) st->remaining--;
    *out = value_retain(st->item);
    return true;
}
static void repeat_cleanup(Iterator* it) {
    StRepeat* st = it->state;
    value_free(&st->item);
    free(st);
}
static const IteratorVTable vt_repeat = { repeat_next, repeat_cleanup };

Value native_itertools_repeat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { make_error("itertools.repeat: expected a value"); return make_null(); }
    StRepeat* st = malloc(sizeof(StRepeat));
    st->item = value_retain(args[0]);
    st->remaining = -1; /* infinite, Python repeat(obj) */
    if (argc >= 2) st->remaining = args[1].type == VAL_INT ? args[1].int_val : -1;
    return make_iterator(&vt_repeat, st);
}

/* ---- cycle: materialize the input once, then loop over it forever ---- */
typedef struct { Array* pool; size_t idx; } StCycle;

static bool cycle_next(Iterator* it, Value* out) {
    StCycle* st = it->state;
    if (st->pool->count == 0) return false; /* Python: cycle of empty is empty */
    *out = value_retain(st->pool->items[st->idx]);
    st->idx = (st->idx + 1) % st->pool->count;
    return true;
}
static void cycle_cleanup(Iterator* it) {
    StCycle* st = it->state;
    Value pool = array_value(st->pool);
    value_free(&pool); /* drop our refcount on the snapshot */
    free(st);
}
static const IteratorVTable vt_cycle = { cycle_next, cycle_cleanup };

/* Drain any source into a fresh array (used by cycle/product pools). */
static Array* materialize(SrcState* s) {
    Array* out = array_create(8);
    Value v;
    while (src_pull(s, &v)) {
        array_push(out, v); /* clones */
        value_free(&v);
    }
    return out;
}

Value native_itertools_cycle(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 1 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.cycle: expected an array or iterator");
        return make_null();
    }
    StCycle* st = malloc(sizeof(StCycle));
    st->pool = materialize(&src); /* cycle saves a snapshot, like CPython */
    st->idx = 0;
    src_free(&src);
    return make_iterator(&vt_cycle, st);
}

/* ---- chain: first iterable, then the next, ... (variadic) ---- */
typedef struct { SrcState* srcs; size_t n, cur; } StChain;

static bool chain_next(Iterator* it, Value* out) {
    StChain* st = it->state;
    while (st->cur < st->n) {
        if (src_pull(&st->srcs[st->cur], out)) return true;
        st->cur++; /* this source is done, move on */
    }
    return false;
}
static void chain_cleanup(Iterator* it) {
    StChain* st = it->state;
    for (size_t i = 0; i < st->n; i++) src_free(&st->srcs[i]);
    free(st->srcs);
    free(st);
}
static const IteratorVTable vt_chain = { chain_next, chain_cleanup };

/* Shared helper: turn the whole args range [0,argc) into owned sources.
 * Returns a malloc'd array of argc SrcStates, or NULL (with an error
 * message) if some argument is not iterable. */
static SrcState* srcs_from_args(int argc, Value* args, const char* who) {
    SrcState* srcs = malloc(sizeof(SrcState) * (size_t)(argc > 0 ? argc : 1));
    for (int i = 0; i < argc; i++) {
        if (!src_from_arg(&args[i], &srcs[i])) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s: argument %d is not an array or iterator", who, i + 1);
            make_error(msg);
            for (int j = 0; j < i; j++) src_free(&srcs[j]);
            free(srcs);
            return NULL;
        }
    }
    return srcs;
}

Value native_itertools_chain(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) { make_error("itertools.chain: expected at least one iterable"); return make_null(); }
    SrcState* srcs = srcs_from_args(argc, args, "itertools.chain");
    if (!srcs) return make_null();
    StChain* st = malloc(sizeof(StChain));
    st->srcs = srcs; st->n = (size_t)argc; st->cur = 0;
    return make_iterator(&vt_chain, st);
}

/* ---- zip: pull one from each; stop at the shortest ---- */
typedef struct { SrcState* srcs; size_t n; } StZip;

static bool zip_next(Iterator* it, Value* out) {
    StZip* st = it->state;
    Value tmp[16]; /* cap arity like a sane person */
    size_t n = st->n < 16 ? st->n : 16;
    for (size_t i = 0; i < n; i++) {
        if (!src_pull(&st->srcs[i], &tmp[i])) {
            while (i-- > 0) value_free(&tmp[i]); /* release the over-pulls */
            return false;
        }
    }
    Array* pair = array_create(n);
    for (size_t i = 0; i < n; i++) {
        array_push(pair, tmp[i]);
        value_free(&tmp[i]);
    }
    *out = array_value(pair);
    return true;
}
static void zip_cleanup(Iterator* it) {
    StZip* st = it->state;
    for (size_t i = 0; i < st->n; i++) src_free(&st->srcs[i]);
    free(st->srcs);
    free(st);
}
static const IteratorVTable vt_zip = { zip_next, zip_cleanup };

Value native_itertools_zip(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) { make_error("itertools.zip: expected at least two iterables"); return make_null(); }
    SrcState* srcs = srcs_from_args(argc, args, "itertools.zip");
    if (!srcs) return make_null();
    StZip* st = malloc(sizeof(StZip));
    st->srcs = srcs; st->n = (size_t)argc;
    return make_iterator(&vt_zip, st);
}

/* ---- zip_longest: fill exhausted slots; stop when all are done ---- */
typedef struct { SrcState* srcs; bool* dead; size_t n; Value fill; bool done; } StZipLongest;

static bool ziplongest_next(Iterator* it, Value* out) {
    StZipLongest* st = it->state;
    if (st->done) return false;
    size_t n = st->n < 16 ? st->n : 16;
    Value tmp[16];
    Array* row = array_create(n);
    bool any_alive = false;
    for (size_t i = 0; i < n; i++) {
        if (!st->dead[i] && src_pull(&st->srcs[i], &tmp[i])) {
            any_alive = true; /* at least one source produced this round */
            array_push(row, tmp[i]);
            value_free(&tmp[i]);
        } else {
            st->dead[i] = true; /* pull failed: this source is finished */
            array_push(row, st->fill); /* clones; fill may be null */
        }
    }
    /* Phantom-row bug, take two: dead flags only flip on FAILED pulls, so
     * after the last real row there is one more round where nobody produces
     * (all fill). That row must NOT be emitted — discard it and stop. */
    if (!any_alive) {
        Value rv = array_value(row);
        value_free(&rv); /* drop the all-fill corpse row */
        st->done = true;
        return false;
    }
    *out = array_value(row);
    return true;
}
static void ziplongest_cleanup(Iterator* it) {
    StZipLongest* st = it->state;
    for (size_t i = 0; i < st->n; i++) src_free(&st->srcs[i]);
    free(st->srcs); free(st->dead);
    value_free(&st->fill);
    free(st);
}
static const IteratorVTable vt_ziplongest = { ziplongest_next, ziplongest_cleanup };

Value native_itertools_zip_longest(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) { make_error("itertools.zip_longest: expected at least two iterables"); return make_null(); }
    /* Last positional is the fill value when it is not itself iterable
     * (CPython takes fillvalue as a keyword — positional here, deviation). */
    int nsrc = argc;
    Value fill = make_null();
    Value* last = &args[argc - 1];
    if (last->type != VAL_ARRAY && last->type != VAL_ITERATOR) {
        fill = value_retain(*last);
        nsrc = argc - 1;
    }
    if (nsrc < 2) {
        value_free(&fill);
        make_error("itertools.zip_longest: expected at least two iterables");
        return make_null();
    }
    SrcState* srcs = srcs_from_args(nsrc, args, "itertools.zip_longest");
    if (!srcs) { value_free(&fill); return make_null(); }
    StZipLongest* st = malloc(sizeof(StZipLongest));
    st->srcs = srcs; st->n = (size_t)nsrc;
    st->dead = calloc((size_t)nsrc, sizeof(bool));
    st->fill = fill;
    st->done = false;
    return make_iterator(&vt_ziplongest, st);
}

/* ---- enumerate: [index, item] pairs from a start (default 0) ---- */
typedef struct { SrcState src; long long i; } StEnum;

static bool enum_next(Iterator* it, Value* out) {
    StEnum* st = it->state;
    Value v;
    if (!src_pull(&st->src, &v)) return false;
    Array* pair = array_create(2);
    array_push(pair, make_int(st->i++));
    array_push(pair, v);
    value_free(&v);
    *out = array_value(pair);
    return true;
}
static void enum_cleanup(Iterator* it) {
    StEnum* st = it->state;
    src_free(&st->src);
    free(st);
}
static const IteratorVTable vt_enum = { enum_next, enum_cleanup };

Value native_itertools_enumerate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 1 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.enumerate: expected an array or iterator");
        return make_null();
    }
    long long start = 0;
    if (argc >= 2 && args[1].type == VAL_INT) start = args[1].int_val;
    StEnum* st = malloc(sizeof(StEnum));
    st->src = src; st->i = start;
    return make_iterator(&vt_enum, st);
}

/* ---- islice: absolute-position window [start, stop) stepping by step ---- */
typedef struct { SrcState src; long long pos, start, stop, step; } StIslice;

static bool islice_next(Iterator* it, Value* out) {
    StIslice* st = it->state;
    for (;;) {
        if (st->pos >= st->stop) return false; /* stop==LLONG_MAX = unbounded */
        Value v;
        if (!src_pull(&st->src, &v)) return false;
        long long pos = st->pos++;
        if (pos < st->start || (pos - st->start) % st->step != 0) {
            value_free(&v); /* not in the window (or not on the step grid) */
            continue;
        }
        *out = v;
        return true;
    }
}
static void islice_cleanup(Iterator* it) {
    StIslice* st = it->state;
    src_free(&st->src); /* release the retained source before freeing state */
    free(st);
}
static const IteratorVTable vt_islice = { islice_next, islice_cleanup };

Value native_itertools_islice(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 2 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.islice: expected (iterable, stop) or (iterable, start, stop[, step])");
        return make_null();
    }
    /* Accept (it, stop) / (it, start, stop) / (it, start, stop, step);
     * null start/stop behave like Python's None (0 / unbounded). */
    long long start = 0, stop = -1, step = 1;
    int nvals = argc - 1;
    Value* v = &args[1];
    if (nvals == 1) {
        if (v[0].type == VAL_INT) stop = v[0].int_val;
    } else {
        if (v[0].type == VAL_INT) start = v[0].int_val;
        if (v[1].type == VAL_INT) stop = v[1].int_val;
        if (nvals >= 3 && v[2].type == VAL_INT) step = v[2].int_val;
    }
    if (start < 0) start = 0;          /* CPython raises on negatives; we clamp */
    if (step < 1) step = 1;            /* CPython requires step >= 1 */
    if (stop < 0) stop = LLONG_MAX;    /* stop == null / omitted: unbounded */
    StIslice* st = malloc(sizeof(StIslice));
    st->src = src; st->pos = 0; st->start = start; st->stop = stop; st->step = step;
    return make_iterator(&vt_islice, st);
}

/* ---- takewhile / dropwhile: one predicate, a small mode machine ----
 * mode TAKE: yield while predicate holds, die on first miss (takewhile).
 * mode DROP: skip while predicate holds; first miss flips to PASS.
 * mode PASS: dropwhile past its drop point — yield everything blindly. */
typedef struct { SrcState src; Value fn; bool done; char mode; } StWhile;

static bool while_next(Iterator* it, Value* out) {
    StWhile* st = it->state;
    if (st->done) return false;
    for (;;) {
        Value v;
        if (!src_pull(&st->src, &v)) { st->done = true; return false; }
        if (st->mode == 'P') { *out = v; return true; } /* passthrough */
        Value ok = call_fn1(&st->fn, &v);
        bool holds = val_truthy(&ok);
        value_free(&ok);
        if (st->mode == 'D') {
            if (holds) { value_free(&v); continue; } /* still dropping */
            st->mode = 'P'; /* first miss: yield it, passthrough from here */
            *out = v;
            return true;
        }
        /* TAKE */
        if (!holds) { value_free(&v); st->done = true; return false; }
        *out = v;
        return true;
    }
}
static void while_cleanup(Iterator* it) {
    StWhile* st = it->state;
    src_free(&st->src);
    value_free(&st->fn);
    free(st);
}
static const IteratorVTable vt_while = { while_next, while_cleanup };

Value native_itertools_takewhile(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_FUNCTION) {
        make_error("itertools.takewhile: expected (function, iterable)");
        return make_null();
    }
    SrcState src;
    if (!src_from_arg(&args[1], &src)) {
        make_error("itertools.takewhile: second argument must be an array or iterator");
        return make_null();
    }
    StWhile* st = malloc(sizeof(StWhile));
    st->src = src; st->fn = value_retain(args[0]); st->done = false; st->mode = 'T';
    return make_iterator(&vt_while, st);
}

Value native_itertools_dropwhile(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_FUNCTION) {
        make_error("itertools.dropwhile: expected (function, iterable)");
        return make_null();
    }
    SrcState src;
    if (!src_from_arg(&args[1], &src)) {
        make_error("itertools.dropwhile: second argument must be an array or iterator");
        return make_null();
    }
    StWhile* st = malloc(sizeof(StWhile));
    st->src = src; st->fn = value_retain(args[0]); st->done = false; st->mode = 'D';
    return make_iterator(&vt_while, st);
}

/* ---- filter / filterfalse: fn==null means Python's filter(None, it) ---- */
typedef struct { SrcState src; Value fn; bool invert; } StFilter;

static bool filter_next(Iterator* it, Value* out) {
    StFilter* st = it->state;
    for (;;) {
        Value v;
        if (!src_pull(&st->src, &v)) return false;
        bool keep;
        if (st->fn.type == VAL_NULL) keep = val_truthy(&v);
        else {
            Value ok = call_fn1(&st->fn, &v);
            keep = val_truthy(&ok);
            value_free(&ok);
        }
        if (keep != st->invert) { *out = v; return true; }
        value_free(&v);
    }
}
static void filter_cleanup(Iterator* it) {
    StFilter* st = it->state;
    src_free(&st->src);
    value_free(&st->fn);
    free(st);
}
static const IteratorVTable vt_filter = { filter_next, filter_cleanup };

static Value make_filter(struct Interpreter* interp, int argc, Value* args, bool invert, const char* who) {
    (void)interp;
    if (argc < 2 || (args[0].type != VAL_FUNCTION && args[0].type != VAL_NULL)) {
        char msg[96];
        snprintf(msg, sizeof(msg), "%s: expected (function or null, iterable)", who);
        make_error(msg);
        return make_null();
    }
    SrcState src;
    if (!src_from_arg(&args[1], &src)) {
        char msg[96];
        snprintf(msg, sizeof(msg), "%s: second argument must be an array or iterator", who);
        make_error(msg);
        return make_null();
    }
    StFilter* st = malloc(sizeof(StFilter));
    st->src = src; st->fn = value_retain(args[0]); st->invert = invert;
    return make_iterator(&vt_filter, st);
}

Value native_itertools_filter(struct Interpreter* interp, int argc, Value* args) {
    return make_filter(interp, argc, args, false, "itertools.filter");
}
Value native_itertools_filterfalse(struct Interpreter* interp, int argc, Value* args) {
    return make_filter(interp, argc, args, true, "itertools.filterfalse");
}

/* ---- map: fn over the shortest of n iterables ---- */
typedef struct { Value fn; SrcState* srcs; size_t n; } StMap;

static bool map_next(Iterator* it, Value* out) {
    StMap* st = it->state;
    size_t n = st->n < 16 ? st->n : 16;
    Value tmp[16];
    for (size_t i = 0; i < n; i++) {
        if (!src_pull(&st->srcs[i], &tmp[i])) {
            while (i-- > 0) value_free(&tmp[i]);
            return false; /* shortest iterable wins, like Python */
        }
    }
    *out = interpreter_execute_callback(st->fn, (int)n, tmp);
    for (size_t i = 0; i < n; i++) value_free(&tmp[i]);
    return true;
}
static void map_cleanup(Iterator* it) {
    StMap* st = it->state;
    value_free(&st->fn);
    for (size_t i = 0; i < st->n; i++) src_free(&st->srcs[i]);
    free(st->srcs);
    free(st);
}
static const IteratorVTable vt_map = { map_next, map_cleanup };

Value native_itertools_map(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_FUNCTION) {
        make_error("itertools.map: expected (function, iterable, ...)");
        return make_null();
    }
    SrcState* srcs = srcs_from_args(argc - 1, &args[1], "itertools.map");
    if (!srcs) return make_null();
    StMap* st = malloc(sizeof(StMap));
    st->fn = value_retain(args[0]); st->srcs = srcs; st->n = (size_t)(argc - 1);
    return make_iterator(&vt_map, st);
}

/* ---- starmap: fn(*item) for arrays of args ---- */
typedef struct { Value fn; SrcState src; } StStarmap;

static bool starmap_next(Iterator* it, Value* out) {
    StStarmap* st = it->state;
    Value row;
    if (!src_pull(&st->src, &row)) return false;
    if (row.type != VAL_ARRAY) { value_free(&row); return false; }
    Array* a = row.array_val;
    *out = interpreter_execute_callback(st->fn, (int)a->count, a->items);
    value_free(&row);
    return true;
}
static void starmap_cleanup(Iterator* it) {
    StStarmap* st = it->state;
    value_free(&st->fn);
    src_free(&st->src);
    free(st);
}
static const IteratorVTable vt_starmap = { starmap_next, starmap_cleanup };

Value native_itertools_starmap(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 2 || args[0].type != VAL_FUNCTION || !src_from_arg(&args[1], &src)) {
        make_error("itertools.starmap: expected (function, iterable of arrays)");
        return make_null();
    }
    StStarmap* st = malloc(sizeof(StStarmap));
    st->fn = value_retain(args[0]); st->src = src;
    return make_iterator(&vt_starmap, st);
}

/* ---- accumulate: running reduce (default +), Python 3 itertools order ---- */
typedef struct { SrcState src; Value fn; bool first, has_fn; Value acc; } StAcc;

static bool acc_next(Iterator* it, Value* out) {
    StAcc* st = it->state;
    Value v;
    if (!src_pull(&st->src, &v)) return false;
    if (st->first) {
        st->first = false;
        st->acc = v;      /* move ownership: first item seeds the accumulator */
        *out = value_retain(v);
        return true;
    }
    if (st->has_fn) {
        Value r = call_fn2(&st->fn, &st->acc, &v);
        value_free(&st->acc);
        st->acc = r;
        value_free(&v);
    } else {
        if (st->acc.type == VAL_INT && v.type == VAL_INT) st->acc.int_val += v.int_val;
        else {
            double a = st->acc.type == VAL_INT ? (double)st->acc.int_val : st->acc.float_val;
            double b = v.type == VAL_INT ? (double)v.int_val : v.float_val;
            st->acc = make_float(a + b);
        }
        value_free(&v);
    }
    *out = value_retain(st->acc);
    return true;
}
static void acc_cleanup(Iterator* it) {
    StAcc* st = it->state;
    src_free(&st->src);
    value_free(&st->fn);
    if (!st->first) value_free(&st->acc);
    free(st);
}
static const IteratorVTable vt_acc = { acc_next, acc_cleanup };

Value native_itertools_accumulate(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 1 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.accumulate: expected an array or iterator");
        return make_null();
    }
    StAcc* st = malloc(sizeof(StAcc));
    st->src = src; st->first = true;
    st->has_fn = argc >= 2 && args[1].type == VAL_FUNCTION;
    st->fn = st->has_fn ? value_retain(args[1]) : make_null();
    st->acc = make_null();
    return make_iterator(&vt_acc, st);
}

/* ---- compress: data items whose selector is truthy ---- */
typedef struct { SrcState data, sels; } StCompress;

static bool compress_next(Iterator* it, Value* out) {
    StCompress* st = it->state;
    for (;;) {
        Value d, s;
        if (!src_pull(&st->data, &d)) return false;
        if (!src_pull(&st->sels, &s)) { value_free(&d); return false; }
        bool keep = val_truthy(&s);
        value_free(&s);
        if (keep) { *out = d; return true; }
        value_free(&d);
    }
}
static void compress_cleanup(Iterator* it) {
    StCompress* st = it->state;
    src_free(&st->data);
    src_free(&st->sels);
    free(st);
}
static const IteratorVTable vt_compress = { compress_next, compress_cleanup };

Value native_itertools_compress(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState data, sels;
    if (argc < 2 || !src_from_arg(&args[0], &data) || !src_from_arg(&args[1], &sels)) {
        make_error("itertools.compress: expected (data iterable, selectors iterable)");
        return make_null();
    }
    StCompress* st = malloc(sizeof(StCompress));
    st->data = data; st->sels = sels;
    return make_iterator(&vt_compress, st);
}

/* ---- pairwise: (prev, cur) sliding window ---- */
typedef struct { SrcState src; bool has_prev; Value prev; } StPairwise;

static bool pairwise_next(Iterator* it, Value* out) {
    StPairwise* st = it->state;
    if (!st->has_prev) {
        if (!src_pull(&st->src, &st->prev)) return false;
        st->has_prev = true;
    }
    Value cur;
    if (!src_pull(&st->src, &cur)) return false;
    Array* pair = array_create(2);
    array_push(pair, st->prev); /* array_push clones both */
    array_push(pair, cur);
    value_free(&st->prev);
    st->prev = cur; /* window slides: old cur is the new prev (move, not free) */
    *out = array_value(pair);
    return true;
}
static void pairwise_cleanup(Iterator* it) {
    StPairwise* st = it->state;
    src_free(&st->src);
    if (st->has_prev) value_free(&st->prev);
    free(st);
}
static const IteratorVTable vt_pairwise = { pairwise_next, pairwise_cleanup };

Value native_itertools_pairwise(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 1 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.pairwise: expected an array or iterator");
        return make_null();
    }
    StPairwise* st = malloc(sizeof(StPairwise));
    st->src = src; st->has_prev = false; st->prev = make_null();
    return make_iterator(&vt_pairwise, st);
}

/* ---- product: cartesian product of snapshotted pools (odometer) ---- */
typedef struct { Array** pools; size_t n, *lens, *idx; bool done, started; } StProduct;

static bool product_next(Iterator* it, Value* out) {
    StProduct* st = it->state;
    if (st->done) return false;
    if (st->started) { /* advance odometer rightmost-first */
        size_t j = st->n;
        while (j-- > 0) {
            st->idx[j]++;
            if (st->idx[j] < st->lens[j]) break;
            st->idx[j] = 0;
            if (j == 0) { st->done = true; return false; }
        }
    }
    st->started = true;
    for (size_t i = 0; i < st->n; i++) if (st->lens[i] == 0) { st->done = true; return false; }
    Array* row = array_create(st->n);
    for (size_t i = 0; i < st->n; i++) array_push(row, st->pools[i]->items[st->idx[i]]);
    *out = array_value(row);
    return true;
}
static void product_cleanup(Iterator* it) {
    StProduct* st = it->state;
    for (size_t i = 0; i < st->n; i++) {
        Value p = array_value(st->pools[i]);
        value_free(&p);
    }
    free(st->pools); free(st->lens); free(st->idx);
    free(st);
}
static const IteratorVTable vt_product = { product_next, product_cleanup };

Value native_itertools_product(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) { make_error("itertools.product: expected at least two iterables"); return make_null(); }
    SrcState* srcs = srcs_from_args(argc, args, "itertools.product");
    if (!srcs) return make_null();
    StProduct* st = malloc(sizeof(StProduct));
    st->n = (size_t)argc;
    st->pools = malloc(sizeof(Array*) * st->n);
    st->lens = malloc(sizeof(size_t) * st->n);
    st->idx = calloc(st->n, sizeof(size_t));
    for (size_t i = 0; i < st->n; i++) {
        st->pools[i] = materialize(&srcs[i]); /* product caches pools, like CPython */
        st->lens[i] = st->pools[i]->count;
        src_free(&srcs[i]);
    }
    free(srcs);
    st->done = false; st->started = false;
    return make_iterator(&vt_product, st);
}

/* ---- combinations: r-subsets in lexicographic pool order ---- */
typedef struct { Array* pool; size_t r, *idx; bool done, started; } StCombo;

static bool combo_next(Iterator* it, Value* out) {
    StCombo* st = it->state;
    if (st->done) return false;
    if (st->r == 0) { /* exactly one empty tuple, like CPython */
        st->done = true;
        *out = array_value(array_create(0));
        return true;
    }
    size_t n = st->pool->count;
    if (st->started) { /* advance rightmost movable index, reset the tail */
        size_t k = st->r;
        bool moved = false;
        while (k-- > 0) {
            if (st->idx[k] != n - (st->r - k)) {
                st->idx[k]++;
                for (size_t j = k + 1; j < st->r; j++) st->idx[j] = st->idx[j - 1] + 1;
                moved = true;
                break;
            }
        }
        if (!moved) { st->done = true; return false; }
    }
    st->started = true;
    Array* row = array_create(st->r);
    for (size_t j = 0; j < st->r; j++) array_push(row, st->pool->items[st->idx[j]]);
    *out = array_value(row);
    return true;
}
static void combo_cleanup(Iterator* it) {
    StCombo* st = it->state;
    Value p = array_value(st->pool);
    value_free(&p);
    free(st->idx);
    free(st);
}
static const IteratorVTable vt_combo = { combo_next, combo_cleanup };

Value native_itertools_combinations(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    long long r = 0;
    if (argc < 2 || !src_from_arg(&args[0], &src) || args[1].type != VAL_INT) {
        make_error("itertools.combinations: expected (iterable, r)");
        return make_null();
    }
    r = args[1].int_val;
    StCombo* st = malloc(sizeof(StCombo));
    st->pool = materialize(&src);
    src_free(&src);
    st->started = false;
    st->r = r > 0 ? (size_t)r : 0;
    st->idx = malloc(sizeof(size_t) * (st->r > 0 ? st->r : 1));
    for (size_t j = 0; j < st->r; j++) st->idx[j] = j;
    /* Python law: r<0 or r>pool yields nothing; r==0 yields one empty tuple. */
    st->done = (r < 0) || ((long long)st->pool->count < r);
    return make_iterator(&vt_combo, st);
}

/* ---- permutations: r-length orderings, lexicographic by pool index ---- */
typedef struct { Array* pool; size_t r, *idx; bool* used; bool done, started; } StPerm;

static bool perm_next(Iterator* it, Value* out) {
    StPerm* st = it->state;
    if (st->done) return false;
    size_t n = st->pool->count;
    if (st->r == 0) { st->done = true; *out = array_value(array_create(0)); return true; }
    if (!st->started) { /* first: 0,1,2,...,r-1 */
        st->started = true;
    } else {
        /* advance the rightmost position to the next unused pool index */
        ssize_t k = (ssize_t)st->r - 1;
        for (;;) {
            if (st->idx[k] != (size_t)-1) st->used[st->idx[k]] = false;
            ssize_t v = (ssize_t)st->idx[k] + 1;
            while (v < (ssize_t)n && st->used[v]) v++;
            if (v < (ssize_t)n) { st->idx[k] = v; st->used[v] = true; break; }
            st->idx[k] = -1; /* exhausted; carry left */
            k--;
            if (k < 0) { st->done = true; return false; }
        }
        /* refill every position right of k with the smallest unused */
        for (size_t j = (size_t)k + 1; j < st->r; j++) {
            ssize_t v2 = 0;
            while (v2 < (ssize_t)n && st->used[v2]) v2++;
            st->idx[j] = v2;
            st->used[v2] = true;
        }
    }
    Array* row = array_create(st->r);
    for (size_t j = 0; j < st->r; j++) array_push(row, st->pool->items[st->idx[j]]);
    *out = array_value(row);
    return true;
}
static void perm_cleanup(Iterator* it) {
    StPerm* st = it->state;
    Value p = array_value(st->pool);
    value_free(&p);
    free(st->idx);
    free(st->used);
    free(st);
}
static const IteratorVTable vt_perm = { perm_next, perm_cleanup };

Value native_itertools_permutations(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    long long r = -1;
    if (argc < 1 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.permutations: expected (iterable, [r])");
        return make_null();
    }
    if (argc >= 2 && args[1].type == VAL_INT) r = args[1].int_val;
    StPerm* st = malloc(sizeof(StPerm));
    st->pool = materialize(&src);
    src_free(&src);
    size_t n = st->pool->count;
    if (r < 0) r = (long long)n; /* default r = pool length, like Python */
    st->r = (size_t)r;
    st->started = false;
    st->done = ((long long)r > (long long)n); /* r>n: nothing (Python law) */
    st->idx = malloc(sizeof(size_t) * (st->r > 0 ? st->r : 1));
    st->used = calloc(n > 0 ? n : 1, sizeof(bool));
    /* Seed only when the permutation is real (r <= n): the naive seed loop
     * wrote used[j] for j < r, smashing the heap when r > n (ASan caught
     * the 1-byte overflow on the r>n test — done flag prevents yielding
     * but the constructor already wrote past the pool). */
    if (!st->done) for (size_t j = 0; j < st->r && j < n; j++) { st->idx[j] = j; st->used[j] = true; }
    return make_iterator(&vt_perm, st);
}

/* ---- iter: adapt an array (or pass an iterator through unchanged) ---- */
typedef struct { SrcState src; } StIter;

static bool iter_next(Iterator* it, Value* out) {
    StIter* st = it->state;
    return src_pull(&st->src, out);
}
static void iter_cleanup(Iterator* it) {
    StIter* st = it->state;
    src_free(&st->src);
    free(st);
}
static const IteratorVTable vt_iter = { iter_next, iter_cleanup };

Value native_itertools_iter(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    SrcState src;
    if (argc < 1 || !src_from_arg(&args[0], &src)) {
        make_error("itertools.iter: expected an array or iterator");
        return make_null();
    }
    StIter* st = malloc(sizeof(StIter));
    st->src = src;
    return make_iterator(&vt_iter, st);
}

/* ---- consumers (not lazy): next / list / reduce ---- */

/* itertools.next(it, [default]) - pull one; null (or default) when done. */
Value native_itertools_next(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ITERATOR) {
        make_error("itertools.next: expected an iterator");
        return make_null();
    }
    Value out;
    if (iterator_next(&args[0], &out)) return out;
    if (argc >= 2) return value_retain(args[1]); /* default on exhaustion */
    return make_null();
}

/* itertools.list(it) - drain any iterable into an array (fresh each call). */
Value native_itertools_list(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || (args[0].type != VAL_ITERATOR && args[0].type != VAL_ARRAY)) {
        make_error("itertools.list: expected an array or iterator");
        return make_null();
    }
    SrcState src;
    src_from_arg(&args[0], &src);
    Array* out = materialize(&src);
    src_free(&src);
    return array_value(out);
}

/* itertools.reduce(fn, it, [initial]) - fold left; Python functools order. */
Value native_itertools_reduce(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_FUNCTION) {
        make_error("itertools.reduce: expected (function, iterable, [initial])");
        return make_null();
    }
    SrcState src;
    if (!src_from_arg(&args[1], &src)) {
        make_error("itertools.reduce: second argument must be an array or iterator");
        return make_null();
    }
    Value acc;
    bool has_acc = false;
    if (argc >= 3) { acc = value_retain(args[2]); has_acc = true; }
    Value v;
    while (src_pull(&src, &v)) {
        if (!has_acc) { acc = v; has_acc = true; continue; } /* first item seeds */
        Value r = call_fn2(&args[0], &acc, &v);
        value_free(&acc);
        acc = r;
        value_free(&v);
    }
    src_free(&src);
    if (!has_acc) return make_null(); /* empty, no initial: Python raises; we null */
    return acc;
}

void stdlib_itertools_register(void) {
    register_native_fn("itertools", "count", native_itertools_count);
    register_native_fn("itertools", "cycle", native_itertools_cycle);
    register_native_fn("itertools", "repeat", native_itertools_repeat);
    register_native_fn("itertools", "chain", native_itertools_chain);
    register_native_fn("itertools", "zip", native_itertools_zip);
    register_native_fn("itertools", "zip_longest", native_itertools_zip_longest);
    register_native_fn("itertools", "enumerate", native_itertools_enumerate);
    register_native_fn("itertools", "islice", native_itertools_islice);
    register_native_fn("itertools", "takewhile", native_itertools_takewhile);
    register_native_fn("itertools", "dropwhile", native_itertools_dropwhile);
    register_native_fn("itertools", "filter", native_itertools_filter);
    register_native_fn("itertools", "filterfalse", native_itertools_filterfalse);
    register_native_fn("itertools", "map", native_itertools_map);
    register_native_fn("itertools", "starmap", native_itertools_starmap);
    register_native_fn("itertools", "accumulate", native_itertools_accumulate);
    register_native_fn("itertools", "compress", native_itertools_compress);
    register_native_fn("itertools", "pairwise", native_itertools_pairwise);
    register_native_fn("itertools", "product", native_itertools_product);
    register_native_fn("itertools", "combinations", native_itertools_combinations);
    register_native_fn("itertools", "permutations", native_itertools_permutations);
    register_native_fn("itertools", "iter", native_itertools_iter);
    register_native_fn("itertools", "next", native_itertools_next);
    register_native_fn("itertools", "list", native_itertools_list);
    register_native_fn("itertools", "reduce", native_itertools_reduce);
}
