# NIGHTSHIFT PLAN — RADS autonomous parity program

Mission: make RADS genuinely match Python and TypeScript (STDLIB PARITY + SYNTAX/ERGONOMICS PARITY tracks).
House laws: comments on every function (why/extend), VW Fusca (C11 + existing libs only, zero new deps),
every feature ships tests in tests/*.rads, conversational errors, functions <50 lines, conventional commits.
Git law: branches `nightshift/<yyyymmdd>-<slug>` only, never push main, never force-push, green build+tests
(`make -j4 && ./run_tests.sh`) before every commit, push branch + `gh pr create --repo zarigata/RADS`.

## ⚠️ CRITICAL DISCOVERY (2026-10-13)
`main` (f9d2c82) **does not build from a clean tree**: `make clean && make` fails —
`stdlib_heapq/logging/statistics(.c/.h)` on main use an imagined API (`Value.as.*` union members,
`../core/value.h` header that never existed, `VAL_ERROR` not in enum, `register_native("heapq_x")`
underscore names). It only ever "built" because stale Oct-12 `build/*.o` objects satisfied make.
Branch `nightshift/20261013-stdlib-float-parity` carries the completed port to the real API
(`int_val/float_val/array_val`, `interpreter.h`, dotted `heapq.` names) and is verified
**clean-build green** (trio 11+8+17 PASS, runner 9/9, exit 0). Merge that branch to unblock main.

## Night Log

 - **2026-10-31**: (1) sync: fetch clean, PRs #19-#31 all still open, none merged; cut
   nightshift/20261031-js-syntax-parity from struct-leak head d68c72b. (2) f2faa02
   feat(core): **BACKLOG 9 CLOSED — JS syntax parity night**. test_v0.0.12 (and test_http)
   were written in JS-style syntax RADS could not parse; v0.0.12 had NEVER compiled.
   Shipped: (a) fn(params){} lambdas — contextual via lexer-snapshot lookahead (Lexer is a
   self-contained struct: pointers + fixed tpl stack, plain copy = valid snapshot; fn stays
   a plain identifier — fn(5) is still a call, bare fn still a variable); lowers to the same
   nameless AST_FUNCTION_DECL blast() lambdas use, runtime path already worked (probe:
   blast-lambda-as-arg returned 42 on base). (b) let — contextual let NAME same-line
   lookahead -> AST_VARIABLE_DECL. (c) ASI — consume_semi(): ';', newline, '}' or EOF
   terminates; return uses the JS restriction production (value must start on the return's
   line); for-headers stay strict. (d) SCRIPT MODE — files without main() now execute
   top-level statements in order (before tonight, top-level test.describe()/test.run() ran
   ZERO statements — a whole test framework existed but nothing could invoke it from a
   script). (3) THE FRAMEWORK WAS LYING: test.expect(1+2).toBe(4) printed a green check.
   Chained calls on call RESULTS never dispatched (callee's object was a call expr, not an
   identifier, so no native name could be built) — every assertion in every framework suite
   silently passed. Fix: chained dispatch — inner callee module-shape (IDENT.member(...)) ->
   dispatch "module.matcher" with the inner result as FIRST arg; string results also get
   str.* method dispatch. Framework failures now print grep-able "FAIL:" lines (the runner
   greps for exactly that). toBe dropped a double-strdup leak. (4) json.parse LEAKED
   ~256B/object + originals (jp_parse_object/jp_parse_array push-then-orphan — same
   array_push-clone class as PR #31); 1860B repro on base -> 0 after. (5) NULL-name guards:
   anonymous top-level lambdas hit strcmp(NULL,"main") in the legacy main-search and
   env_set(NULL,...) in exec_function. (6) test_v0.0.12: escaped-JSON strings rewritten as
   template literals (RADS strings are RAW — backticks hold JSON verbatim, zero escaping);
   the \n expectation is a real newline in a template. 50/50 PASSED, wired as suite 22.
   (7) tests/test_js_syntax.rads — 15 pins (fn-as-identifier, fn(...)-as-call non-hijack,
   ASI, let, script mode, chained str methods), negative-tested with a planted failure;
   wired as suite 23. (8) My own corpse bugs caught by probes: first suite draft had a
   self-defeating collision test (blast fn + let fn rebind order) and an expected-count
   that could lie; three regex/patch misfires on the suite file (one ate "hello" and
   doubled a paren) — all caught by parse errors and the planted-failure gate. Verified:
   make clean && make -j4 exit 0; 23/23 suites PASSED 0 FAIL lines grepped twice; ASan+
   UBSan+LSan clean on both new suites AND a repo-wide tests/*.rads sweep — 0 LEAKY SUITES
   (8 on base two nights ago, 1 last night); bare top-level lambda safe. test_http now
   PARSES end-to-end (blocks only on the bare-{ object-literal gap — backlog 11). PR #32:
   https://github.com/zarigata/RADS/pull/32
 - **2026-10-30**: (1) sync: fetch clean, PRs #19-#30 all still open, none merged; cut nightshift/20261030-struct-ast-leak from sci-notation head 38874ce. (2) 07220e7 fix(core,stdlib): **BACKLOG 3b CLOSED — the struct-literal leak night, and the leak CLASS it was hiding**. Four cooperating defects, every one LSan-verified on base first: (a) parser struct-literal branch borrowed expr->identifier.name then DROPPED the node without ast_free — identifier node + name leaked per literal; (b) interpreter mallocd a fresh StructDef + strdup name per INSTANCE, value_free never released (clones share the pointer; per-instance free = double-free) — now INTERNED per struct name, same ownership model as stdlib static defs, freed at teardown; (c) member-assign (p.x=v) released contents but leaked the Value BOX; (d) interpret() exit paths only ran env_free() — struct/enum registries leaked EVERY file run; full teardown now on success + uncaught-throw paths. (3) THE CLASS: array_push DEEP-COPIES (value_clone) every push, so any native that strdups a string then pushes it leaks the original. 14 stdlib sites swept (queue x4, string x2, string_advanced, csv, cli x3, env, copy, heapq x2) + regex findall/split/groups/group_names + 7 translate_pattern ERROR paths (freed the names array, not the stored name strings) + stdlib_regex_cache_flush() (pattern cache died at exit holding its heap) + fs.readdir (6KB PER CALL) + csv.parse (256B) + copy.deepcopy per-item + parser ...rest temp + struct-literal panic path (leaked callee+args on Expected-struct-name errors). (4) DISCOVERED: tests/test_v0.0.11 and test_v0.0.12 have NEVER COMPILED — they use fn() {} anonymous-function syntax the language DOES NOT HAVE (fn lexes as identifier -> fn() parses as call -> { becomes struct literal -> error). Their leaks were parse-error leaks. fn-as-lambda is a real parity gap: BACKLOG 9. (5) tests/test_memory_truth.rads — 16 behavior pins over every fixed construct, wired as suite 21; runs clean under ASan+UBSan+LSan. (6) My own corpse-quality bugs en route: deepcopy first draft pushed *v (infinite recursion) — caught before build; three stray-quote artifacts from printf patching — all caught by compiler; heapq/string_advanced pushes had stale comments. Verified: clean build exit 0 plain+sanitize, runner 21/21 PASSED 0 FAIL lines (grepped) twice on BOTH builds, probe numbers base->branch: struct x2 152B/8alloc -> 0/0, test_structs 102B/5 -> 0/0, repo-wide tests LSan sweep 8 suites leaky -> 1 (pre-existing async_utils callback leaks, BACKLOG 10). PR #31.


 - **2026-10-29**: (1) sync: fetch clean, PRs #19-#29 all still open, none merged, base unchanged; cut
   nightshift/20261029-sci-notation from build-truth head faa4627. (2) 8bd87aa feat(core): **scientific-notation
   literals + Python-parity float display** (backlog 7 CLOSED). 1e2/2.5E-3/2E+3/1.5e-3 previously lexed as
   INTEGER+IDENTIFIER -> "Expected ';'" parse error. number() now scans e/E + optional sign + MANDATORY digits,
   with rollback: bare "1e"/"1e+" rewinds so the e-token stays a clean identifier (same two-token split Python
   makes before rejecting). Exponent forces FLOAT (7e0 is float, typeof-pinned). Collision check: grepped every
   test+example for [0-9]e[0-9] OUTSIDE strings — zero hits; number() only fires when a digit starts the token,
   so UUID/CSS-hex strings are structurally immune. (3) BUG the feature exposed, fixed same unit: float DISPLAY
   was lying. echo used %f (2.5 -> "2.500000"), concat/templates used %g 6-sig-digit (1e6+1 printed "1e+06" —
   the +1 display-ROUNDED AWAY; value right, screen wrong). One shared format_float_shortest() now feeds all
   three render sites (value_print, value_to_owned_string, uncaught-throw stderr): shortest ROUND-TRIP string,
   ".0" on integral floats, sci only in CPython's repr window (|x|>=1e16 or <1e-4), -0.0, subnormal 5e-324,
   DBL_MAX exact 1.7976931348623157e+308. All pins from real CPython 3.14 runs (0.30000000000000004, 1e-05,
   1000000000000000.0). Two formatter drafts died honestly: v1 emitted C-style exponents (1.5e300 vs Python's
   1.5e+300, 5e-7 vs 5e-07) — v2 uses %.*e directly in the sci window (C's signed 2-digit exponent IS Python's
   style) and %.*g with skip-wrong-form in the fixed window. (4) tests/test_sci_notation.rads 28 assertions
   wired as suite 20; runner 20/20. Verified: clean build exit 0 (0 warnings from touched files), 20/20 suites
   0 FAIL twice, ASan+UBSan+LSan 28/28 0 findings 0 leaks on new suite (numeric_mixed/itertools/promise/
   decimal/fractions also clean; template/structs still show the KNOWN pre-existing AST leak, backlog 3b,
   identical on base), malformed battery 1e/1e+/5.5e-/0x1F all clean rejections. PR #30.
 
 - **2026-10-28**: (1) sync: fetch clean, PRs #19-#28 all still open, none merged, base unchanged; cut
   nightshift/20261028-build-truth from promise-allsettled head 74b7ee9. (2) 5d01a60 fix(core): **the honest-runner night** — taught run_tests.sh to grep its own output for FAIL lines (exit-code gate had been lying by omission for months), and it IMMEDIATELY caught 3 suites that have been failing silently on main: v0_0_7_comprehensive (string advanced + math extended), test_debugger (watch expressions), test_typecheck (optional types + generics). Root defects, all pre-existing: (a) eval_binary_op had NO mixed int/float arms for OP_ADD and all six comparison ops — 1+1.5 was null, math.clamp(50,0,100)==50 was false; (b) exec_statement had no AST_FUNCTION_DECL case, so a blast declared INSIDE another function fell to default:, got evaluated, and was DISCARDED — nested helper functions never registered (root cause of debugger+typecheck suite failures); (c) native_str_length existed since forever but was never registered — str.len/string.length returned null. All fixed; nested decls register globally (true local scoping waits for closures — deliberate, logged). tests/test_numeric_mixed.rads 17 assertions (CPython-pinned) wired as suite 19. Makefile: -MMD -MP autodeps (backlog 3d CLOSED — touch lexer.h now rebuilds 4 true dependents, not stale-object segfaults); runner: missing-file=fail, off-by-one Total fixed, stale test_stream.rads entry purged. Negative-tested: runner exits 1 on a planted FAIL-line suite. (3) 1f7deab fix(core): **parser infinite error loop** — a malformed input (stray '}' at statement position, e.g. anything rads-mask's buggy else-dedent emits) looped the SAME 'Expected expression' error FOREVER: 12-line file -> 158MB of output, process hung. Three cooperating defects: parse_block had no forward-progress guard (NULL stmt that consumed nothing re-parsed the same token), parse_primary never consumed the offending token, parse_call deref'd the cascading NULL (segfault hiding behind the loop). 18-input malformed battery all terminate now; valid programs unaffected (19/19). Same commit: rstar registry-search snprintf comma bug (format string terminated mid-JSON — the -d body NEVER reached curl; verified via PATH-shimmed curl capture: body now arrives), rads-mask sprintf overflow sweep (13 unbounded appends -> bounded append_formatted, replace_in_line growth bounded, newline-strip so ';' lands on the right line). (4) DISCOVERED, backlog: rads-mask emits DOUBLE closing braces for if/else + for (dedent closes AND the rule closes — its output has never been valid RADS); RADS has NO scientific notation literals (1e2 is a parse error). Verified: clean build exit 0 (0 warnings from touched files), 19/19 suites 0 FAIL twice, ASan+LSan 0 findings on new code paths (numeric_mixed, malformed-input battery), rstar curl-shim end-to-end. PR #29.

 - **2026-10-27**: (1) sync: fetch clean, PRs #19-#27 all still open, none merged, base unchanged; cut nightshift/20261027-promise-allsettled from template-literals head c2250c6. (2) da17110 fix(core): **exception propagation was BROKEN across call boundaries** — try { boom() } catch NEVER caught (callback left the throw pending, eval_call returned null), uncaught throws exited 0 silently, and the REPL would have been poisoned by a stale pending flag. Now: pending throw unwinds at block/statement level (try/finally exempt), finally runs during unwind with TS replace-on-throw semantics (my first cut restored the ORIGINAL exception over a finally-thrown one — backwards, fixed), uncaught -> 'Uncaught error: X' + exit 1 (Python/Node parity), REPL reports per-line. Same commit killed two more pre-existing segfaults, both found by probes on base: (a) anonymous struct instances (NULL definition — EVERY regex.search result!) crashed on missing-field access AND on echo() (NULL->definition->name deref); (b) array-index eval shallow-copied struct items then freed the source array — turbo p = arr[0] handed back freed memory (same UAF class as the 20261024 sweep, index path missed). Plus catch-var parser leak (strdup'd twice). Core gained public interpreter_has_throw()/take_throw()/raise(). (3) ebf17f0 feat(stdlib): **Promise.allSettled/all/race/any — the LAST TS-parity backlog item**. No microtask queue exists, so combinators settle synchronously; semantics preserved: allSettled never rejects ({status,value|reason} per input, input order), all re-raises first rejection, race first settlement, any first fulfillment else AggregateError {name,message,errors}. Rejections modelled as throw-in-body exactly like TS — only possible because of the barrier fix. Pins from real Node v20.19.2. tests/test_promise.rads 18 assertions, runner 18 suites. LSan caught TWO leaks in my own first cut (promise_any errors-array refcount, exposed parser catch-var) — both fixed, 0 leaks now. (4) House lesson x2: 'git add src/core/x.c' trips the 'core' gitignore pattern (bare-dir match) — use -f; make CFLAGS=... sanitizer build leaves stale ASan objects that break the NEXT plain make link (no header deps, again — backlog 3(d) bites). Verified: clean build exit 0 (0 warnings from touched files), 18/18 suites 0 FAIL twice, ASan 18/18 0 findings 0 leaks on new suite, uaf/itertools suites clean under ASan too. PR #28: https://github.com/zarigata/RADS/pull/28
 


 - **2026-10-26**: (1) sync: fetch clean, PRs #19-#26 all still open, none merged, base unchanged; cut
   nightshift/20261026-template-literals from lazy-itertools head 8eeb1c2. (2) d834d54 fix(core): **scoped call frames — recursion was BROKEN on main**. Params bound to the
   single global env, so (a) calls permanently rebound same-named caller vars (g(name) rewrote main's name — found because template tests were position-dependent), and (b) inner
   calls clobbered outer bindings mid-expression: fib(10) returned -80, not 55 (verified on base with a probe). Fix: EnvShadow frame per call (save caller binding, bind, LIFO
   restore on exit) + env_remove() for params with no prior binding. fib(10)=55, leak gone, ASan clean incl. leaks. (3) bbe21bb feat(core): **template literals, TS-parity** —
   backtick templates with ${...} interpolation; lexer emits START/CHUNK/EXPR_BEGIN/EXPR_END/END via a frame stack (nested templates inside ${}, brace counting so struct literals
   in holes don't close early, raw-text law kept: NO unescaping, multi-line text, unterminated diagnostic); parser lowers to left-fold of + seeded with "" so `${7}`=="7"; interpreter
   value_to_owned_string() — dynamic heap conversion replacing OP_ADD's 256-byte stack buffers that silently clipped long concatenations (latent bug, killed). (4) House lesson
   re-learned: make has NO header deps — editing lexer.h (struct layout) without make clean = segfaults from stale objects; burned 20 min on a ghost crash. (5) Known pre-existing
   debt confirmed: struct-literal AST leaks 84B/4alloc (same class as test_structs, in backlog); fn-local `turbo` decls still bind globally (e2 probe prints 1 not 2) — frames fix
   params only, full scope chain is future work. tests/test_template_literals.rads 21 assertions (node v20.19.2-pinned), runner now 17 suites. Verified: clean build exit 0 (0
   warnings from touched files), 17/17 suites 0 FAIL twice, ASan 21/21 twice + 0 runtime findings. PR #27: https://github.com/zarigata/RADS/pull/27

 - **2026-10-25**: (1) sync: fetch clean, PRs #19-#25 all still open, none merged, base unchanged; cut
   nightshift/20261025-lazy-itertools from UAF-sweep head d9b3589. (2) 876c44d feat(stdlib): **first-class lazy iterators + Python-parity itertools** — new
   VAL_ITERATOR core type (refcounted Iterator, pull-vtable next/cleanup, one-shot like Python gens, clone shares cursor like arrays); full itertools rewrite, 24 lazy
   natives (count/cycle/repeat/chain/zip/zip_longest/enumerate/islice/takewhile/dropwhile/filter/filterfalse/map/starmap/accumulate/compress/pairwise/product/combinations/
   permutations + iter/next/list/reduce), combinators nest as lazy pipelines, strings iterate UTF-8 code points, state Values owned via value_retain (UAF law). (3) DISCOVERED:
   cruise (x in [1,2,3]) silently ran ZERO times on main — interpreter only handled the a..b range op; now drives arrays (snapshot count) + iterators (pull to exhaustion,
   break/continue/return honored). (4) My own corpse bugs, caught by probes+ASan before shipping: pairwise never advanced its window; zip_longest emitted a phantom all-fill
   row after both sources died; dropwhile fell into the takewhile branch after the drop phase (now TAKE/DROP/PASS mode machine); permutations seeded used[] past pool when r>n
   (heap overflow, ASan-confirmed); islice cleanup never released its retained source (leak, LSan-confirmed). Also: ?. and ?? were ALREADY wired lexer->parser->interpreter —
   backlog item was stale, marked done. (5) tests/test_itertools_lazy.rads 49 assertions (all python3.13-pinned), runner now 16 suites; old eager test_itertools.rads deleted.
   Verified: clean build exit 0 (0 warnings from touched files), 16/16 suites 0 FAIL twice, ASan 49/49 0 findings 0 leaks twice. PR #26: https://github.com/zarigata/RADS/pull/26
 - **2026-10-24**: (1) sync: fetch clean, PRs #19-#24 all still open, none merged, base unchanged; cut
   nightshift/20261024-decimal-minmax-uaf from fractions head 8b5952c. (2) 159b937 fix(stdlib): **aliasing UAF
   sweep, 17 sites** — the decimal.min/max segfault was one instance of a repo-wide class: the interpreter frees the args
   array right after every native call, so any native returning args[i] / raw array-item aliases = use-after-free.
   Repro-d on base: decimal.min nested (double-free), functools.identity(string), heapq.heappushpop, random.choice,
   collections.peek, array.push (15KB garbage print). Fix: public value_retain() wrapper over value_clone (strdup/
   refcount++/deep-copy) at 17 sites across 11 stdlib files; MOVE semantics kept on destructive pop/shift/heappop
   (blanket cloning there orphaned payloads — leaks; ASan caught my first attempt, 18B/6alloc). heapq stores now clone
   args into slots. (3) tests/test_uaf_regression.rads 20 assertions wired into runner (15 suites). (4) DISCOVERED:
   pre-existing LeakSanitizer debt on base: test_structs/v0_0_7_comprehensive/test_regex leak (3291B regex, 2900B
   v0_0_7) — backlog. Also: heapq cmp_values is numeric-only (strings compare as 0) — semantics gap vs Python,
   backlog. Verified: clean build exit 0 (0 new warnings from touched files), 15/15 suites 0 FAIL twice, ASan 20/20
   0 findings 0 leaks on new suite; base still UAFs the same probes. PR #25.
 - **2026-10-22**: (1) sync: fetch clean, no corpse tonight (working tree clean on decimal branch — a first), PRs #19-#23 all still open, none merged; cut `nightshift/20261022-fractions` from decimal head 96d2b3d (freshest green). (2) `3a2857c` feat(stdlib): **fractions module, Python-parity exact rationals** — self-contained signed bigint (base-1e9) + rational layer (coprime num/den, den>0, CPython invariants), EXACT ops only (no context precision — the point of rationals): add/sub/mul/div/floordiv/mod (Python floor law), pow square-and-multiply w/ sign-correct negative-base inversion, limit_denominator ported line-for-line from CPython Lib/fractions.py (continued fractions + the 2*d*(q0+k*q1)<=den tiebreak), float→exact-rational via raw IEEE-754 bits (F(0.1)==3602879701896397/36028797018963968), parse of "n/d"/decimal/scientific; 23 natives, tests/test_fractions.rads 59 assertions. (3) **INTERPRETER BUG FOUND (affects main today)**: native min/max returning `args[i]` as struct Value = use-after-free — nested `decimal.to_str(decimal.min(...))` SEGFAULTS on current main (verified by probe). fractions re-boxes the winner; decimal.min/max still carry the latent bug — bughunt backlog #1 next shift. (4) My own corpse-quality bugs fixed en route: `str` is a RESERVED KEYWORD in RADS (native had to be `fractions.to_str`), limit_denominator convergent rotation was `a*p0+p1` instead of CPython's `p0+a*p1`, pow invert never actually swapped num/den (and leaked), fr_from_double used frexp math that truncated (0.1 → off-by-2^55; now reads raw IEEE bits), mag_add/bi_mul were not alias-safe (fresh-buffer-first now). (5) Runner now 14 suites. Verified: clean build exit 0 / 0 warnings from fractions.c, 14/14 suites 0 FAIL lines, fractions 59/59, ASan 59/59 0 findings 0 leaks, run twice. PR #24: https://github.com/zarigata/RADS/pull/24
 - **2026-10-21**: (1) sync: fetch green, PRs #19-#22 still open, none merged, base unchanged; found ANOTHER corpse — night run died 04:20 with 1700 lines of uncommitted decimal C on branch nightshift/20261021-decimal (scratch probes p*/q*.rads all over the root). Recovered, deleted junk, finished the unit. (2) e0f9508 feat(stdlib): **decimal module, Python-parity arbitrary precision** — sign/scale limbs base 1e9, exact add/sub/mul, correctly-rounded long division (ROUND_HALF_EVEN, ctx 28 = CPython default), pow square-and-multiply (2**100 exact), sqrt integer Newton, CPython-exact str() (1E+25/1E-7/-0/0E+2 pins), NaN/Inf IEEE total order, spec floordiv/mod toward-zero, quantize half-even, normalize, struct {text,sign,digits,exponent}; 26 natives, tests/test_decimal.rads 40 assertions. (3) corpse bugs fixed: dec_num_digits NaN/Inf limbs[-1] segfault-on-parse, 5 leaks (pow/neg/abs/sqrt result, divmod coef run), dead norm_pow10, uninit digits early-oom, WRONG 1/3 pin (CPython says 28 threes — corpse wrote pin from memory without running python). (4) runner now 13 suites. Verified: clean build exit 0 / 0 warnings from decimal.c, 13/13 suites 0 FAIL lines, decimal 40/40, ASan 40/40 0 leaks. PR #23: https://github.com/zarigata/RADS/pull/23
- **2026-10-20**: (1) sync: fetch origin+fork green, PRs #19/#20/#21 still open, none merged; found last nights run DEAD MID-FLIGHT (HTTP 429) with uncommitted strptime work on branch nightshift/20261019-datetime-strptime — recovered it, deleted junk placeholder file, finished the unit; (2) 4d7ed09 feat(stdlib): **datetime.strptime/strftime Python-parity engine** — full rewrite (+1000/-431): format->POSIX ERE lowered at cache time, CPython _strptime semantics (case-insensitive names, non-padded numbers, %I/%p 12h, %j julian, %U/%W weeks, %G-%V-%u ISO week dates, %z-aware epochs, 1900 default year, strict rejections: trailing garbage/Feb 30/hour 24/%G-without-%V -> null), strftime side exact Howard Hinnant civil math (no mktime/locale), composites %c/%x/%X/%F/%D/%T/%R/%r, glibc unknown-directive passthrough ("100%% %q" -> "100% %q"), legacy YYYY-MM-DD dialect kept; (3) bugs fixed en route: dt_unpack int-smash (month=0 -> MONTH_FULL[-1] segfault), format %% literal wrong output — the FAIL line was buried by the exit-code-only runner AGAIN (house grep law 3-for-3); (4) tests/test_datetime.rads 38 assertions wired into run_tests.sh (12 suites). Verified: clean build exit 0 (0 warnings from stdlib_datetime.c), 12/12 suites 0 FAIL lines, ASan build 38/38 with 0 findings. PR #22 opened: https://github.com/zarigata/RADS/pull/22

- **2026-10-18**: (1) sync: fetch origin+fork green, PRs #19/#20 still open, none merged — cut
  `nightshift/20261018-re-named-groups` from `nightshift/20261017-forge-sync` head (freshest green);
  (2) `6ff4beb` feat(stdlib): **regex named groups** — `(?P<name>...)` + `(?<name>...)` lowered to POSIX ERE
  at cache time (regcomp has no named groups; name->capture-index map kept per cached pattern),
  `regex.named_groups()` groupdict-style struct (null on no match, "" for non-participating optional),
  `regex.group_names()` declaration order, `\g<name>`/`\g<1>`/`\1` backrefs in replace, duplicate names
  rejected, lookarounds rejected with clear errors; (3) **fix: regex.replace heap overflow** — old
  `2*len(str)+2*len(repl)` sizing smashed the heap on many-match expansion (ASan-confirmed WRITE past
  300-byte region, repro preserved); grow-on-demand GrowBuf replaces it; (4) tests/test_regex.rads 20
  assertions wired into run_tests.sh (11 suites). Verified: clean build exit 0, 11 suites 0 FAIL lines,
  regex suite 20/20, ASan build 20/20 with 0 findings. PR #21 opened:
  https://github.com/zarigata/RADS/pull/21
- **2026-10-17**: (1) merged `nightshift/20261013-stdlib-float-parity` into new branch
  `nightshift/20261017-forge-sync` — **CRITICAL DISCOVERY closed**: `make clean && make -j4` exit 0 from
  a clean tree, main-line stdlib unblocked; (2) `79e122e` feat(stdlib): **nostalgia module** — the
  Museum of the 2000s Internet, 19 natives (leek_spin, rickroll, all_your_base, over9000, leeroy,
  badger_badger, pbj_time, hamster_dance, o_rly/ya_rly, the_game, barrel_roll, charlie_bit_me,
  keyboard_cat, dramatic_chipmunk, msn_nudge, asl, away_message, leetspeak) + test_nostalgia.rads
  (26 assertions, wired into run_tests.sh → 10/10) + examples/nostalgia_museum.rads;
  (3) .gitignore: benchmarks/, root symlinks, rads-mask binary (backlog #4 done). gh auth NOW
  AVAILABLE (Dagoth-Ur-69, repo scope) — old blocker dead; branch pushed, PR opened.
- **2026-10-17 (verification fire)**: first fully autonomous pipeline run end-to-end. (1) Sync: fetch origin+fork green, PR #20 (nostalgia+clean-build-fix) verified OPEN and left unmerged; (2) bb7db17 feat(stdlib): **nostalgia.pool_closed()** — Habbo Hotel July 2006 exhibit (canonical cry + backstory plaque), 4 new assertions, museum example Habbo wing rewired; nostalgia suite 30 PASS/0 FAIL, runner 10/10 from clean tree, make clean && make -j4 exit 0; (3) pushed to fork, PR #20 updated with comment (no dup PR): https://github.com/zarigata/RADS/pull/20#issuecomment-6239520889. Runner gap confirmed again: per-suite FAIL lines are swallowed by exit-code-only gate — v2_0_0 tests must always be grepped by hand.
- **2026-10-13**: (1) `27b8b67` on `nightshift/20261013-stdlib-float-parity` — mixed int/float
  arithmetic (`-`,`*`,`/`,`%` w/ zero guards), `make_error()` helper, heapq/logging/statistics real-API
  port + 3 test suites; (2) `5a02649` — runner learns the 3 suites (9/9), exec bit restored;
  (3) `f92114e` on `nightshift/20261013-cleanup` — 33 files / −11,648 lines: .windsurf, .local,
  SESSION_SUMMARY.md, V0.0.7–V0.0.11 plans, dead docs/, docs/README.md dead links fixed; pre-deletion
  grep of CI/scripts/Makefile/run_tests.sh found zero references. `make clean && make -j4` = exit 0,
  `run_tests.sh` 9/9 on stdlib branch. Push attempted, failed (no creds) — commits local per protocol.

## Standing gotchas
- RADS string literals are RAW (parser does no escape unescaping): `.rads` source writes ONE backslash
  per escape (`"\("`, `"\g<1>"`), not Python's two. Tests that assume Python escaping will fail.
- Strings have no `.length` (arrays only) — use `str.count()` or comparisons.
- ASan full-tree build: src/debug has duplicate symbols between debug_protocol.c and
  conditional_breakpoints.c (debug_step_out, debug_get_stack_trace et al.) — build with only
  conditional_breakpoints.c until deduped (bughunt backlog candidate).

## Blockers
- ~~gh auth unavailable~~ **RESOLVED 2026-10-17** — gh logged in as Dagoth-Ur-69 (repo scope);
  stdlib-fix branch merged on `nightshift/20261017-forge-sync`; clean build green.
- MAIN SYNC: origin/main (f9d2c82) still cannot clean-build until the merge lands on main via PR.

## Open PR Review
| PR | Title | Status | Verdict |
|----|-------|--------|---------|
| 20 | nostalgia module + clean-build fix for main | OPEN — head Dagoth-Ur-69:nightshift/20261017-forge-sync, +1 comment (pool_closed, bb7db17) | awaiting master review; carries the main clean-build fix |
| 21 | regex named groups + replace heap-overflow fix | OPEN — head Dagoth-Ur-69:nightshift/20261018-re-named-groups | awaiting master review |
| 22 | datetime strptime/strftime Python-parity engine | OPEN — head Dagoth-Ur-69:nightshift/20261019-datetime-strptime | awaiting master review; strptime done |
| 24 | fractions module, Python-parity exact rationals | OPEN — head Dagoth-Ur-69:nightshift/20261022-fractions | awaiting master review; fractions done, lazy itertools next |
| 27 | template literals + scoped call frames (recursion fix) | OPEN — head Dagoth-Ur-69:nightshift/20261026-template-literals | awaiting master review; fib(10) was -80 on main, now 55 | 
| 29 | honest test runner + mixed numeric ops + nested blast + parser loop fix | OPEN — head Dagoth-Ur-69:nightshift/20261028-build-truth | awaiting master review; 3 suites were silently failing on main |
| 31 | struct-literal leaks + array_push-clone leak class sweep | OPEN — head Dagoth-Ur-69:nightshift/20261030-struct-ast-leak | awaiting master review; backlog 3b closed, fs.readdir leaked 6KB/call |
| 30 | sci-notation literals + Python-parity float display | OPEN — head Dagoth-Ur-69:nightshift/20261029-sci-notation | awaiting master review; echo/concat display used to lie (%g/%f) |
| 28 | Promise combinators + exception propagation fix | OPEN — head Dagoth-Ur-69:nightshift/20261027-promise-allsettled | awaiting master review; try/catch across calls + regex.search structs were broken on main |
| 23 | decimal module, Python-parity arbitrary precision | OPEN — head Dagoth-Ur-69:nightshift/20261021-decimal | awaiting master review; decimal done, fractions next |

## Ranked Backlog (next shifts)
1. **Merge stdlib branch** (unlocks clean builds of main) once auth exists; then CI should run
   `make clean && make` — never trust cached objects.
2. Stdlib parity: ~~itertools lazy generators~~ SHIPPED 2026-10-25 (PR #26). TS parity: ~~optional chaining ?.~~ ~~nullish ??~~ already wired (verified 2026-10-25). TS PARITY TRACK COMPLETE 2026-10-27: template literals PR #27, Promise.allSettled (+all/race/any, + throw-barrier core fix) PR #28.
   ~~template literals~~ SHIPPED 2026-10-26 (PR #27, with recursion fix). Remaining: `Promise.allSettled`. ~~datetime.strptime~~ SHIPPED 2026-10-20 (PR #22). ~~decimal~~ SHIPPED
   2026-10-21 (PR #23). ~~fractions~~ SHIPPED 2026-10-22 (PR #24).
3. **BUGHUNT backlog**: (a) fn-LOCAL `turbo` decls still bind globally (frame fix 2026-10-26 covers PARAMS only; e2 probe: local=1 leaks into caller) — needs a real scope chain
   when closures arrive. (b) CLOSED 2026-10-30 (PR #31): struct-literal leak + the whole strdup-then-array_push clone class swept, 20+ sites.
   (c) sweep `-Wunused`/`-Wformat` warnings (rads-mask sprintf overflow is a real stack-smash risk). (d) CLOSED 2026-10-28: Makefile -MMD -MP autodeps shipped (PR #29).
4. `.gitignore`: add `benchmarks/`, root symlinks (`rads`, `rads-mask`, `radpkg`),
   `tools/rads-mask/rads-mask` build binary.
5. ~~v0_0_7_comprehensive.rads not in runner~~ STALE — it was already wired; the REAL issue was the runner ignoring FAIL lines, fixed 2026-10-28 (PR #29).
6. rads-mask emits DOUBLE closing braces for if/else and for-loops (dedent closes AND the rule closes) — its transpiled output has never been valid RADS. Found 2026-10-28 while verifying the sprintf sweep.
7. ~~scientific-notation float literals~~ SHIPPED 2026-10-29 (PR #30, with the Python-parity float-display fix it exposed).
9. ~~fn-lambda syntax parity gap~~ CLOSED 2026-10-31 (PR #32): fn(){} lambdas + let + ASI + script mode shipped; test_v0.0.12 resurrected 50/50 (suite 22), test_js_syntax 15 pins (suite 23). BONUS: the test framework's assertions never actually RAN before (chained calls silently null) — fixed with module-shaped chained dispatch; json.parse leak class swept.
10. **async_utils callback leaks**: stdlib_async_series/parallel leak ~32B per callback via interpreter_execute_callback value_clone + the result Array (LSan on tests/test_filesystem.rads). Pre-existing, untouched by the 2026-10-30 sweep. Needs an event-loop-owned lifetime audit.
11. **bare-{ object literals at argument position** (JS parity): `json.stringify({"test": "rads"})` — a `{` after `(` parses as struct literal requiring a NAME and fails (`test_http.rads` line 33 is the live repro; the file parses fully once this lands). Needs primary-expression `{` support (map-literal shape like JS) or a clearer diagnostic suggesting named-struct syntax.
