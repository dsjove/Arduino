# PinIO

PinIO is a small, header-only GPIO abstraction for Arduino-class microcontrollers and Raspberry Pi / Linux systems.

It allows pins, GPIO expanders, and even “virtual” pins to be expressed as
**types**, enabling compile-time validation, zero dynamic allocation, no virtual methods, no state, and highly reusable application code.

This approach improves **readability**, **maintainability**, and **testability** by pushing GPIO intent and correctness into the **type system**, while compiling down to direct hardware access with no runtime cost.

---

## Why PinIO?

Traditional Arduino GPIO code is imperative and loosely coupled:

```cpp
pinMode(7, OUTPUT);
digitalWrite(7, HIGH);
```

This style makes it easy to:
- reuse the wrong pin number
- write to an input pin
- read from an output pin
- forget where a pin is physically connected
- break behavior during refactors

PinIO replaces this with **declarative pin definitions** and **compile-time safety**.

---

## Core Concept

### A Pin Is a Type - all behavior and no runtime state

A GPIO pin is represented as a **type**, not an integer.

```cpp
using Led = PinIO<LED_BUILTIN, GpioMode::DigitalOut>;
```

This single line declares at compile time:
- *which* pin
- *how* it is used
- *what operations are valid*
- *named* for its purpose

Invalid configurations (nonexistent pins, unsupported modes, reserved pins)
fail at compile time whenever possible.

### Zero-Overhead Abstraction

PinIO avoids:
- virtual functions
- dynamic allocation
- runtime dispatch
- stateful issues

When optimized, PinIO calls compile down to the same instructions as direct
`digitalWrite()` or backend-specific register writes.

The abstraction exists only at **compile time**.

### Two-way separation of concerns

PinIO’s HAL works in both directions:

- **Backends** implement hardware access (MCU GPIO, I²C expanders, SPI devices)
- **Application code** consumes typed pin capabilities

This allows business logic to be written once and reused across hardware targets.

### Mode Is Compile-Time Safety

Because the pin mode is part of the type:
- calling `read()` on a `DigitalOut` pin is a compile-time error
- calling `write()` on a `DigitalIn` pin is a compile-time error
- invalid capability combinations fail to compile

---

## Basic example

```cpp
using Led = PinIO<LED_BUILTIN, GpioMode::DigitalOut>;

void setup()
{
  Led::begin(GpioLevel::High);
}

void loop()
{
  Led::write(GpioLevel::Low);
  delay(500);
  Led::write(GpioLevel::High);
  delay(500);
}
```

---

## Backends

PinIO separates *what* a pin is from *how* it is implemented.

A backend provides the low-level GPIO operations for a platform or device.

Examples include:

- Arduino built-in GPIO
- Arduino UNO R4 GPIO
- ESP32 GPIO
- Raspberry Pi GPIO (via `libgpiod`)
- Unit-test / no-op backends

`PinIO.h` is intended to be the primary include; it selects a default backend
based on the platform.

---

## GPIO expanders

PinIO supports GPIO expanders such as the MCP23017 by using a backend that
routes GPIO operations through the expander.

Expander backends may require explicit initialization before use:

```cpp
Mcp23017Device expander;
expander.begin();

using ExpanderPin = PinIO<9, GpioMode::DigitalOut, Mcp23017PinIO<>>;
```

Unlike MCU GPIO, expander backends may perform runtime readiness checks.

---

## Adapters and composition

Because pins are types, they can be adapted or wrapped.

A common example is **active-low hardware**, where a logical “High” turns an
output *off*:

```cpp
template <typename PIN>
struct ActiveLow
{
  static void begin(GpioLevel v) { PIN::begin(invert(v)); }
  static void write(GpioLevel v) { PIN::write(invert(v)); }
  static GpioLevel read()        { return invert(PIN::read()); }

private:
  static constexpr GpioLevel invert(GpioLevel v)
  {
    return (v == GpioLevel::High) ? GpioLevel::Low : GpioLevel::High;
  }
};
```

This allows hardware quirks to be handled without changing application logic.

---

## Testing

PinIO includes a unit-test backend that allows code to compile and run without
real hardware.

```cpp
using TestPin = PinIO<7, GpioMode::DigitalOut, UnitTestPinIOBackend<32>>;
```

This makes it possible to validate higher-level logic on desktop platforms.

---

## Requirements

- **C++17**
- **Arduino builds**
  - An Arduino core that supports C++17
- **Raspberry Pi / Linux builds**
  - `libgpiod` development headers (for example: `libgpiod-dev`)

---

## Design goals

- Header-only, no `.cpp` files
- No dynamic allocation or stateful variables
- Compile-time validation of pin existence and capabilities
- Strongly typed GPIO modes
- No runtime overhead
- Works with:
  - Built-in MCU GPIO
  - GPIO expanders (e.g. MCP23017)
  - Test and simulation backends

## Notes

- `PinIO.h` is the umbrella header intended for most users.

## License

Modifications to PinIO source files must be made available under the same license.
Use in proprietary firmware is permitted.
