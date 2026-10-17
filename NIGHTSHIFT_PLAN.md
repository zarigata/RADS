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
- **2026-10-13**: (1) `27b8b67` on `nightshift/20261013-stdlib-float-parity` — mixed int/float
  arithmetic (`-`,`*`,`/`,`%` w/ zero guards), `make_error()` helper, heapq/logging/statistics real-API
  port + 3 test suites; (2) `5a02649` — runner learns the 3 suites (9/9), exec bit restored;
  (3) `f92114e` on `nightshift/20261013-cleanup` — 33 files / −11,648 lines: .windsurf, .local,
  SESSION_SUMMARY.md, V0.0.7–V0.0.11 plans, dead docs/, docs/README.md dead links fixed; pre-deletion
  grep of CI/scripts/Makefile/run_tests.sh found zero references. `make clean && make -j4` = exit 0,
  `run_tests.sh` 9/9 on stdlib branch. Push attempted, failed (no creds) — commits local per protocol.

## Blockers
- **gh auth unavailable** — `gh auth login` never run, no GH_TOKEN; cannot list/review PRs, cannot
  push branches or open PRs. All commits held locally on branches as instructed.
- credentials needed to merge the main-unlocking stdlib branch (see CRITICAL DISCOVERY).

## Open PR Review
| PR | Title | Status | Verdict |
|----|-------|--------|---------|
| — | (none reviewable) | gh auth unavailable | skip |

## Ranked Backlog (next shifts)
1. **Merge stdlib branch** (unlocks clean builds of main) once auth exists; then CI should run
   `make clean && make` — never trust cached objects.
2. Stdlib parity: Python `re` gaps (named groups), `datetime.strptime` round-trip, `decimal`,
   `fractions`, `itertools` lazy generators; TS parity: optional chaining `?.`, nullish `??`,
   template literals, `Promise.allSettled`.
3. Sweep `-Wunused`/`-Wformat` warnings (rads-mask sprintf overflow is a real stack-smash risk).
4. `.gitignore`: add `benchmarks/`, root symlinks (`rads`, `rads-mask`, `radpkg`),
   `tools/rads-mask/rads-mask` build binary.
5. v0_0_7_comprehensive.rads still absent from run_tests.sh array (exists in tests/, 155-line diff
   landed in 27b8b67) — wire it in or fold into trio suites.
