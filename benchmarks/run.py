#!/usr/bin/env python3
"""RADS benchmark runner: RADS vs CPython vs Node.

Usage: python3 benchmarks/run.py [--quick] [--iterations N]
Times wall-clock of each workload, best-of-N, and writes
benchmarks/results/<timestamp>.json plus a stdout table.
"""
import json
import os
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "benchmarks" / "results"

WORKLOADS = {
    # name: (rads_cmd, python_cmd, node_cmd) — None = N/A
    "fib": (
        ["./bin/rads", "benchmarks/rads/fib.rads"],
        ["python3", "benchmarks/python/fib.py"],
        ["node", "benchmarks/node/fib.js"],
    ),
    "string_build": (
        ["./bin/rads", "benchmarks/rads/string_build.rads"],
        ["python3", "benchmarks/python/string_build.py"],
        ["node", "benchmarks/node/string_build.js"],
    ),
    "array_churn": (
        ["./bin/rads", "benchmarks/rads/array_churn.rads"],
        ["python3", "benchmarks/python/array_churn.py"],
        ["node", "benchmarks/node/array_churn.js"],
    ),
    "file_io": (
        ["./bin/rads", "benchmarks/rads/file_io.rads"],
        ["python3", "benchmarks/python/file_io.py"],
        ["node", "benchmarks/node/file_io.js"],
    ),
}


def best_of(cmd, iterations):
    """Run cmd `iterations` times, return best wall time in ms (float) or None."""
    best = None
    for _ in range(iterations):
        t0 = time.perf_counter()
        proc = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL)
        dt = (time.perf_counter() - t0) * 1000.0
        if proc.returncode != 0:
            return None  # workload failed — report, don't fake a number
        if best is None or dt < best:
            best = dt
    return round(best, 1) if best is not None else None


def git_desc():
    try:
        return subprocess.run(["git", "describe", "--tags", "--always"],
                              cwd=ROOT, capture_output=True, text=True,
                              check=True).stdout.strip()
    except subprocess.CalledProcessError:
        return "dev"


def cpu_info():
    try:
        return open("/proc/cpuinfo").read().split("model name")[1].split("\n")[0].split(":")[1].strip()
    except Exception:
        return f"{os.cpu_count()} cores"


def main():
    iterations = 1 if "--quick" in sys.argv else 3
    if "--iterations" in sys.argv:
        iterations = int(sys.argv[sys.argv.index("--iterations") + 1])

    meta = {
        "date": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "rads": git_desc(),
        "python": sys.version.split()[0],
        "node": subprocess.run(["node", "--version"], capture_output=True,
                               text=True).stdout.strip(),
        "iterations": iterations,
        "cpu": cpu_info(),
    }
    results = {}
    header = f"{'WORKLOAD':<14}{'RADS(ms)':>12}{'PY(ms)':>12}{'NODE(ms)':>12}"
    print(header)
    print("-" * len(header))
    for name, (rads_cmd, py_cmd, node_cmd) in WORKLOADS.items():
        r = best_of(rads_cmd, iterations)
        p = best_of(py_cmd, iterations)
        n = best_of(node_cmd, iterations)
        results[name] = {"rads": r, "python": p, "node": n}
        print(f"{name:<14}{str(r):>12}{str(p):>12}{str(n):>12}")

    RESULTS.mkdir(exist_ok=True)
    out = RESULTS / (datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + ".json")
    out.write_text(json.dumps({"meta": meta, "results": results}, indent=2))
    print("-" * len(header))
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
