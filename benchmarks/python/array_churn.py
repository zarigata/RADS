#!/usr/bin/env python3
"""array_churn — list push/pop/map churn."""
arr = []
for i in range(20000):
    arr.append(i % 97)
acc = 0
for _ in range(5000):
    acc += arr.pop()
mapped = list(map(lambda x: x * 2, arr))
print(f"acc = {acc}, last mapped = {mapped.pop()}")
