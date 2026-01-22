# Test 4: Comprehensive Feature Test
# Tests multiple Python features in one file

def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

def is_prime(num):
    if num < 2:
        return False
    i = 2
    while i * i <= num:
        if num % i == 0:
            return False
        i = i + 1
    return True

def filter_primes(numbers):
    primes = []
    for num in numbers:
        if is_prime(num):
            primes.append(num)
    return primes

# Test fibonacci
for i in range(10):
    print("Fib", i, "=", fibonacci(i))

# Test prime checking
test_numbers = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
primes = filter_primes(test_numbers)
print("Primes:", primes)
