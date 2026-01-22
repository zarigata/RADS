# Factorial Example - Classic Recursion

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

num = 5
result = factorial(num)
print("Factorial of", num, "is", result)

# Test with different values
test_values = [1, 3, 5, 7, 10]
i = 0
while i < 5:
    val = test_values[i]
    fact = factorial(val)
    print("Factorial of", val, "is", fact)
    i = i + 1
