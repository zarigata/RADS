#!/usr/bin/env python3
"""fib — naive recursive fibonacci."""
import sys
sys.setrecursionlimit(100000)

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

r = fib(28)
print(f"fib(28) = {r}")
assert r == 317811
