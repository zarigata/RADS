# RADS Benchmarks

Reproducible comparison of RADS vs CPython 3 vs Node on honest workloads.

- `rads/`, `python/`, `node/` — one file per workload per language, as close
  to identical as the languages allow (RADS has no dict type yet, so the
  hash-churn workload is array-based).
- `run.py` / `run.sh` — the runner. Best-of-N wall-clock (default 3),
  includes process startup. Writes `results/<timestamp>.json`.
- `RESULTS.md` — the human-readable log of runs and takeaways.

```
bash benchmarks/run.sh            # full run, best-of-3
bash benchmarks/run.sh --quick    # smoke, best-of-1
```

Requirements: built binary at `./bin/rads` (run `make`), `python3`, `node`.

Workloads: `fib` (naive recursion), `string_build` (concat loop),
`array_churn` (push/pop/map), `file_io` (write/read/append cycles).
HTTP echo is planned once the HTTP server stdlib is stable enough to
benchmark honestly.
