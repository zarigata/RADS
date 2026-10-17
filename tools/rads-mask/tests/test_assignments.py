# Regression tests for assignment/comparison transpilation (nightshift 2026-10-11)
# Covers: == mangled into 'turbo a = = b' (bug 1), 'return a == b' captured by
# the assignment rule (bug 2), kwargs must not be assignments, chained compare.

def compare_basics(a, b):
    equal = a == b
    not_equal = a != b
    less = a <= b
    greater = a >= b
    return a == b

def with_kwargs():
    return compute(x=1, y=2)

def compute(x, y):
    return x + y
