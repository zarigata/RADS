# Math Module

Comprehensive mathematical functions for RADS, providing trigonometric, hyperbolic, logarithmic, and utility operations.

## Constants

### `math.PI()`
Returns the mathematical constant π (pi).

```rads
let pi = math.PI()
println(pi)  # 3.141592653589793
```

### `math.E()`
Returns the mathematical constant e (Euler's number).

```rads
let e = math.E()
println(e)  # 2.718281828459045
```

### `math.INF()`
Returns positive infinity.

```rads
let inf = math.INF()
println(inf)  # inf
```

### `math.NAN()`
Returns Not-a-Number (NaN).

```rads
let nan = math.NAN()
println(math.isnan(nan))  # 1
```

## Basic Operations

### `math.abs(x)`
Returns the absolute value of x.

```rads
println(math.abs(-5))     # 5
println(math.abs(-3.7))   # 3.7
```

### `math.min(a, b)`
Returns the smaller of a and b.

```rads
println(math.min(5, 10))      # 5
println(math.min(3.5, 2.1))   # 2.1
```

### `math.max(a, b)`
Returns the larger of a and b.

```rads
println(math.max(5, 10))      # 10
println(math.max(3.5, 2.1))   # 3.5
```

### `math.sign(x)`
Returns the sign of x: -1 for negative, 0 for zero, 1 for positive.

```rads
println(math.sign(5))     # 1
println(math.sign(-5))    # -1
println(math.sign(0))     # 0
```

### `math.clamp(x, min, max)`
Clamps x to the range [min, max].

```rads
println(math.clamp(5, 0, 10))   # 5
println(math.clamp(-5, 0, 10))  # 0
println(math.clamp(15, 0, 10))  # 10
```

### `math.lerp(a, b, t)`
Linear interpolation between a and b by factor t.

```rads
println(math.lerp(0, 10, 0.5))  # 5.0
println(math.lerp(0, 10, 0.25)) # 2.5
```

## Rounding

### `math.floor(x)`
Returns the largest integer ≤ x.

```rads
println(math.floor(3.7))   # 3
println(math.floor(-3.7))  # -4
```

### `math.ceil(x)`
Returns the smallest integer ≥ x.

```rads
println(math.ceil(3.7))    # 4
println(math.ceil(-3.7))   # -3
```

### `math.round(x)`
Returns x rounded to the nearest integer.

```rads
println(math.round(3.7))   # 4
println(math.round(3.2))   # 3
println(math.round(-3.7))  # -4
```

### `math.trunc(x)`
Returns x truncated toward zero.

```rads
println(math.trunc(3.7))   # 3
println(math.trunc(-3.7))  # -3
```

## Powers and Roots

### `math.sqrt(x)`
Returns the square root of x.

```rads
println(math.sqrt(16))  # 4.0
println(math.sqrt(2))   # 1.4142135623730951
```

### `math.cbrt(x)`
Returns the cube root of x.

```rads
println(math.cbrt(27))  # 3.0
println(math.cbrt(-8))  # -2.0
```

### `math.pow(x, y)`
Returns x raised to the power y.

```rads
println(math.pow(2, 3))   # 8.0
println(math.pow(9, 0.5)) # 3.0
```

### `math.hypot(x, y)`
Returns the length of the hypotenuse of a right-angled triangle (√(x² + y²)).

```rads
println(math.hypot(3, 4))  # 5.0
```

## Trigonometric Functions

All trigonometric functions work with radians. Use `deg_to_rad` to convert degrees to radians.

### `math.sin(x)`
Returns the sine of x.

```rads
println(math.sin(0))              # 0.0
println(math.sin(math.PI() / 2))  # 1.0
```

### `math.cos(x)`
Returns the cosine of x.

```rads
println(math.cos(0))              # 1.0
println(math.cos(math.PI()))      # -1.0
```

### `math.tan(x)`
Returns the tangent of x.

```rads
println(math.tan(0))              # 0.0
println(math.tan(math.PI() / 4))  # 1.0
```

### `math.asin(x)`
Returns the arcsine (inverse sine) of x. Input must be in [-1, 1].

```rads
println(math.asin(0))   # 0.0
println(math.asin(1))   # 1.5707963267948966 (π/2)
```

### `math.acos(x)`
Returns the arccosine (inverse cosine) of x. Input must be in [-1, 1].

```rads
println(math.acos(1))   # 0.0
println(math.acos(0))   # 1.5707963267948966 (π/2)
```

### `math.atan(x)`
Returns the arctangent (inverse tangent) of x.

```rads
println(math.atan(0))   # 0.0
println(math.atan(1))   # 0.7853981633974483 (π/4)
```

### `math.atan2(y, x)`
Returns the arctangent of y/x, using the signs of both to determine the quadrant.

```rads
println(math.atan2(1, 1))   # 0.7853981633974483 (π/4)
println(math.atan2(1, -1))  # 2.356194490192345 (3π/4)
```

## Hyperbolic Functions

### `math.sinh(x)`
Returns the hyperbolic sine of x.

```rads
println(math.sinh(0))  # 0.0
println(math.sinh(1))  # 1.1752011936438014
```

### `math.cosh(x)`
Returns the hyperbolic cosine of x.

```rads
println(math.cosh(0))  # 1.0
println(math.cosh(1))  # 1.5430806348152437
```

### `math.tanh(x)`
Returns the hyperbolic tangent of x.

```rads
println(math.tanh(0))  # 0.0
println(math.tanh(1))  # 0.7615941559557649
```

## Logarithmic and Exponential

### `math.exp(x)`
Returns e raised to the power x.

```rads
println(math.exp(1))   # 2.718281828459045
println(math.exp(0))   # 1.0
```

### `math.log(x)`
Returns the natural logarithm (base e) of x.

```rads
println(math.log(math.E()))  # 1.0
println(math.log(10))        # 2.302585092994046
```

### `math.log2(x)`
Returns the base-2 logarithm of x.

```rads
println(math.log2(8))   # 3.0
println(math.log2(1))   # 0.0
```

### `math.log10(x)`
Returns the base-10 logarithm of x.

```rads
println(math.log10(100))  # 2.0
println(math.log10(10))   # 1.0
```

## Modulo

### `math.fmod(x, y)`
Returns the floating-point remainder of x/y.

```rads
println(math.fmod(10.5, 3.0))   # 1.5
println(math.fmod(-10.5, 3.0))  # -1.5
```

## Special Value Checks

### `math.isnan(x)`
Returns 1 if x is NaN, 0 otherwise.

```rads
println(math.isnan(math.NAN()))  # 1
println(math.isnan(42.0))        # 0
```

### `math.isinf(x)`
Returns 1 if x is infinite, 0 otherwise.

```rads
println(math.isinf(math.INF()))  # 1
println(math.isinf(42.0))        # 0
```

### `math.isfinite(x)`
Returns 1 if x is a finite number, 0 otherwise.

```rads
println(math.isfinite(42.0))     # 1
println(math.isfinite(math.INF()))  # 0
println(math.isfinite(math.NAN()))  # 0
```

## Angle Conversion

### `math.deg_to_rad(degrees)`
Converts degrees to radians.

```rads
println(math.deg_to_rad(180))  # 3.141592653589793
println(math.deg_to_rad(90))   # 1.5707963267948966
```

### `math.rad_to_deg(radians)`
Converts radians to degrees.

```rads
println(math.rad_to_deg(math.PI()))  # 180.0
println(math.rad_to_deg(math.PI()/2))  # 90.0
```

## Array Operations

### `math.sum(array)`
Returns the sum of all numeric values in array.

```rads
let arr = [1.0, 2.0, 3.0, 4.0, 5.0]
println(math.sum(arr))  # 15.0
```

### `math.avg(array)`
Returns the arithmetic mean of all numeric values in array.

```rads
let arr = [1.0, 2.0, 3.0, 4.0, 5.0]
println(math.avg(arr))  # 3.0
```

### `math.median(array)`
Returns the median (middle value) of all numeric values in array.

```rads
let arr = [1.0, 2.0, 3.0, 4.0, 5.0]
println(math.median(arr))  # 3.0
```

## Random (Legacy)

**Note:** The `random` module provides better random number generation. These functions are retained for compatibility.

### `math.random()`
Returns a random float in [0, 1).

```rads
println(math.random())  # e.g., 0.54321
```

### `math.random_int(max)` or `math.random_int(min, max)`
Returns a random integer in the specified range.

```rads
println(math.random_int(100))     # 0-100
println(math.random_int(10, 20))  # 10-20
```

## Summary

The math module provides 40+ functions covering:
- **Constants:** PI, E, INF, NAN
- **Basic:** abs, min, max, sign, clamp, lerp
- **Rounding:** floor, ceil, round, trunc
- **Powers/Roots:** sqrt, cbrt, pow, hypot
- **Trigonometric:** sin, cos, tan, asin, acos, atan, atan2
- **Hyperbolic:** sinh, cosh, tanh
- **Log/Exp:** exp, log, log2, log10
- **Modulo:** fmod
- **Special Checks:** isnan, isinf, isfinite
- **Angles:** deg_to_rad, rad_to_deg
- **Arrays:** sum, avg, median
- **Random:** random, random_int (legacy)
