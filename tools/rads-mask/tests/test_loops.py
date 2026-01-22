# Test 3: Loops
# Tests while loops, for loops, break, continue

def test_while():
    i = 0
    while i < 5:
        print(i)
        i = i + 1

def test_for():
    for i in range(10):
        if i == 3:
            continue
        if i == 7:
            break
        print(i)

def sum_array(numbers):
    total = 0
    for num in numbers:
        total = total + num
    return total

test_while()
test_for()
nums = [1, 2, 3, 4, 5]
print("Sum:", sum_array(nums))
