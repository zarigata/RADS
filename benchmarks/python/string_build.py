#!/usr/bin/env python3
"""string_build — string concatenation in a loop."""
s = ""
for i in range(5000):
    s = s + "rads" + str(i) + ";"
print(f"done, last={s}")
