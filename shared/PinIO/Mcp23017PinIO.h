#pragma once

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#include "ArduinoGpioBackend.h"

template <bool CheckReady = false>
struct Mcp23017PinIO : ArduinoGpioBackend
{
  // If not using a Mcp23017Device, this must be called.
  // Do not call any begin, read, or write until this method is called.
  static void attach(Adafruit_MCP23X17& d) { dev = &d; }

  // --- capabilities / policy overrides ---
  static constexpr bool pin_exists(int pin)      { return pin >= 0 && pin <= 15; }
  static constexpr bool pin_is_reserved(int)     { return false; }
  static constexpr bool pin_supports_analog(int) { return false; }
  static constexpr bool pin_supports_pwm(int)    { return false; }

  // --- readiness ---
  static constexpr bool alwaysReady = !CheckReady;
  static bool verifyReady()
  {
    return dev != nullptr;
  }

  // --- MCP23017 implementations (hide ArduinoGpio versions) ---
  static void begin_analog_in(uint8_t) = delete;
  static void begin_digital_in(uint8_t pin)        { dev->pinMode(pin, INPUT); }
  static void begin_digital_in_pullup(uint8_t pin) { dev->pinMode(pin, INPUT_PULLUP); }
  static void begin_digital_out(uint8_t pin)       { dev->pinMode(pin, OUTPUT); }
  static void begin_pwm_out(uint8_t)   = delete;

  static GpioArchTypes::analog_type read_analog(uint8_t) = delete;

  static GpioLevel read_digital(uint8_t pin)
  {
    return dev->digitalRead(pin) ? GpioLevel::High : GpioLevel::Low;
  }

  static void write_digital(uint8_t pin, GpioLevel v)
  {
    dev->digitalWrite(pin, (v == GpioLevel::High) ? HIGH : LOW);
  }

  static constexpr GpioArchTypes::pwm_type pwmMax(uint8_t) = delete;
  static void write_pwm(uint8_t, GpioArchTypes::pwm_type) = delete;

private:
  static inline Adafruit_MCP23X17* dev = nullptr;
};
