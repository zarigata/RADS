# String Operations Example

def greet(name):
    return "Hello, " + name + "!"

def shout(message):
    return message + "!!!"

name = "RADS"
greeting = greet(name)
excited_greeting = shout(greeting)

print(greeting)
print(excited_greeting)

# Multiple greetings
names = ["Alice", "Bob", "Charlie"]
i = 0
while i < 3:
    msg = greet(names[i])
    print(msg)
    i = i + 1
