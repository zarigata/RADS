# Test 2: Functions and Control Flow
# Tests functions, if/elif/else, return statements

def calculate(x, y):
    if x > y:
        return x - y
    elif x < y:
        return y - x
    else:
        return 0

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

result1 = calculate(10, 5)
result2 = factorial(5)
print("Calculate:", result1)
print("Factorial:", result2)
