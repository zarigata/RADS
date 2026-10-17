#!/usr/bin/env python3
"""file_io — write/read/append cycle."""
import os
path = "benchmarks/tmp_io_py.txt"
for i in range(200):
    with open(path, "w") as f:
        f.write(f"line of benchmark data {i}\n")
    with open(path) as f:
        f.read()
    with open(path, "a") as f:
        f.write(f"appended {i}\n")
    with open(path) as f:
        f.read()
print("file_io done ok=True")
