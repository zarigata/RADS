# Temperature Converter - Practical Example

def celsius_to_fahrenheit(celsius):
    return (celsius * 9 / 5) + 32

def fahrenheit_to_celsius(fahrenheit):
    return (fahrenheit - 32) * 5 / 9

# Test conversions
c_temp = 25
f_temp = celsius_to_fahrenheit(c_temp)
print("25°C =", f_temp, "°F")

f_temp2 = 77
c_temp2 = fahrenheit_to_celsius(f_temp2)
print("77°F =", c_temp2, "°C")
