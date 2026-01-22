# Fibonacci Example - Recursion Test

def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

i = 0
while i < 10:
    result = fibonacci(i)
    print("Fib(" + str(i) + ") =", result)
    i = i + 1
