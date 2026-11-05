 - **2026-11-05**: (1) sync: fetch clean, PRs #19-#36 all still open, none merged; cut
   nightshift/20261105-repl-uaf from closures head e6f511e (freshest green, PR #36). (2) fix(core):
   **BACKLOG 12 CLOSED — REPL session lifetime**. Was (re-verified on base in a worktree, rc 139
   deterministic): defining a blast on one REPL line and calling it on the next segfaulted; same
   for turbo f = blast(...){} then f(21), and echo(f) — the printer derefs func_node too. Root
   cause: VAL_FUNCTION values hold a RAW func_node pointer into the AST that parsed them (struct
   defs borrow their AST the same way); file mode is safe ONLY because interpret() tears the env
   down BEFORE ast_free(program) — the REPL inverted that order per line, ast_free(stmt) while
   the env kept values pointing into the freed tree. Fix: REPL statements park in a session
   arena (repl_ast_park: AST + source buffer) and are released ONCE, after
   interpreter_cleanup_environment() drains the env — JS/Python REPL law: session code lives
   for the session; teardown order env -> trees -> regex cache -> event loop. The parse-error
   path frees its half-built tree immediately (nothing references it; also plugged the old leak
   where a non-NULL stmt on had_error was never freed). (3) BONUS defect in the same path: the
   REPL parsed lines with parse_statement, so struct/enum/import/async-blast top-level
   declarations were UNREACHABLE from a session — struct P {...} was a silent parse error
   (Expected expression) and every later P(...) hit Struct-not-defined. parse_declaration
   (the file-mode top-level entry) is now public in parser.h and the REPL uses it; named
   structs (Player {name, score} then p1.name cross-line) and enums (Color.Red -> 0, byte-equal
   to file mode) work in sessions now. (4) Runner: @repl_session suite — scripted 16-line stdin
   session, six markers asserted (cross-line call, lambda-in-var, closures with independent
   captured frames 20/30, named struct, enum arithmetic); a segfault fails via missing markers.
   Gate negative-tested with a planted bogus marker: suite FAILED + runner exit 1; restored
   -> 28/28. (5) Corpses tonight, all caught by gates before commit: two malformed hand-rolled
   unified diffs (patch rejected, tree verified untouched by grep before continuing); the cron
   sandbox blocks heredocs and python -c, so all patching went through printf-to-script-file;
   first runner session had double-escaped quotes (line arrived as backslash-quote -> lexer
   Unexpected character), then the un-escape pass stripped ALL quotes — both caught by the
   MISSING-marker check doing its job. Verified: make clean && make -j4 exit 0 (0 new
   warnings; parser.c:980 update_block warning pre-existing on base); runner **28/28 suites
   PASSED 0 FAIL lines, run twice identical**; ASan+UBSan+LSan: full 18-line REPL session rc 0,
   0 findings 0 leaks (cross-line closures included); closures/structs/object_literals/
   v0.0.12/js_syntax suites clean under sanitizers. Not verified: CI on the PR. PR #37:
   https://github.com/zarigata/RADS/pull/37

