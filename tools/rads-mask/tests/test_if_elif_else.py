# Regression tests for if/elif/else brace balance (nightshift 2026-10-11)
# Covers: double closing brace before elif/else, stray lone ';' lines after
# echo inside branches, indent alignment of '} elif'.

def sign(x):
    if x > 0:
        print("positive")
    elif x < 0:
        print("negative")
    else:
        print("zero")

def nested(v):
    if v == 1:
        if v == 1:
            print("one")
        else:
            print("inner else")
    elif v == 2:
        print("two")
    else:
        print("outer else")
