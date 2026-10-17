# AGENTS.md — RADS Standing Mission (Autonomous Dev Loop)

You are the autonomous RADS development agent. This file is your standing orders.
Every cron run = one focused unit of work. Never try to do everything in one run.

Repo: `/home/zari/Desktop/RADS` (remote `origin` = github.com/zarigata/RADS.git)
Current version: v0.0.12 "QUASAR II". Core is pure C11 (tree-walking interpreter),
plus tools (`rads-mask` transpiler, `radpkg` package manager, webengine, rads-registry web app).

## State file (READ FIRST, WRITE LAST)
`docs/roadmap/AUTONOMOUS-STATE.md` — the handoff journal between runs.
- If it exists: read it, continue the "Next up" item, do NOT redo finished work.
- At the end of EVERY run: rewrite it with (a) what this run changed, (b) test results,
  (c) the single "Next up" task for the next run, (d) open blockers.
- First run ever: create it, starting with Phase 0 below.

## Phase 0 — FIRST RUN ONLY: housekeeping & cleanup
1. Verify no IDE-rule cruft remains anywhere: `.windsurf/`, `*.windsurfrules`,
   `.cursorrules`, `.cursor/` (a prior commit 3fd3336 removed most; confirm zero remain).
2. Finish the in-flight branch `nightshift/20261011-rads-mask-fixes`: the untracked
   `tools/rads-mask/tests/*.rads` files belong in git; the built binaries
   (`tools/rads-mask/rads-mask`, root symlinks `radpkg`/`rads-mask`) do NOT —
   add them to `.gitignore`, commit the tests.
3. Docs consolidation (conservative: move, don't delete):
   - Create `docs/archive/`. Move session logs (`SESSION_*`, `PHASE*`, `*_STATUS.md`,
     `V0.0.*_PLAN/PROGRESS/RECOVERY.md`, `FINAL_COMPLETION_REPORT.md`, `index.v1.html`)
     into it.
   - Root `docs/`: the scattered `stdlib_*.md` fragments move to `docs/stdlib/` with an
     INDEX.md linking them.
   - Deduplicate: where two docs describe the same thing (e.g. WEB_ENGINE_* trio),
     keep the accurate one, archive the rest, fix inbound links.
4. One commit per logical step, conventional messages (`docs:`, `chore:`, `test:`).

## Phase 1+ — the mission: a LEGIT competitor to TypeScript and Python
Pick ONE workstream per run, in this priority order (skip what's done, see state file):

1. **Performance credibility** — a reproducible benchmark suite (`benchmarks/`) comparing
   RADS vs CPython 3 and Node on honest workloads (fib, string build, hash-table churn,
   file I/O, HTTP echo). Machine-readable results, a `benchmarks/RESULTS.md` kept current.
   A competitor without numbers is a hobby.
2. **rads-mask** (the TypeScript-angle: typed RADS → C/transpiled) — finish the test
   suite green, then close gaps: type annotations, inference, error messages a human
   would show a friend.
3. **Stdlib parity with Python** — maintain `docs/stdlib/GAPLIST.md` (Python module →
   RADS status). Knock out the highest-value gaps; keep the 90s aesthetic in naming.
4. **Tooling** — REPL polish, a basic LSP (hover/diagnose), `rads fmt`, better errors
   with line/column and suggestions.
5. **Docs refresh** — after cleanup: README that a stranger understands in 60 seconds,
   one canonical language spec (`docs/LANGUAGE_SPEC.md`), quickstart that WORKS
   (verify every snippet by running it), kill stale version claims.

## Hard rules
- Build & test before every commit: `make` then `./run_tests.sh` (or the Makefile test
  target). Never commit red.
- Atomic commits, conventional messages, no drive-by changes.
- Never force-push. Never rewrite history. Never touch `.git` internals or hooks.
- No destructive deletes — `docs/archive/` instead. Built artifacts never enter git.
- If a task blocks >10 min: note it in the state file as a blocker, pick the next task.
- If `git push` fails on auth, commit locally and note it; do NOT fiddle with credentials.
- Keep the repo buildable at every commit — a stranger cloning mid-loop must succeed.

## Reporting
End-of-run summary (max 6 lines): what changed, test status, next up. That summary is
what the user sees. The detailed journal lives in AUTONOMOUS-STATE.md.
