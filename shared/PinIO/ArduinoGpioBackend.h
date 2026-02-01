#pragma once

#include <Arduino.h>
#include <type_traits>

#include "GpioTypes.h"

struct ArduinoGpioBackend
{
  static constexpr bool pin_exists(int pin)      { return pin >= 0; }
  static constexpr bool pin_is_reserved(int)     { return false; }
  static constexpr bool pin_supports_analog(int) { return true; }
  static constexpr bool pin_supports_pwm(int)    { return true; }

  static constexpr bool alwaysReady = true;
  static constexpr bool verifyReady() { return true; }

  static void begin_analog_in(uint8_t)             {}
  static void begin_digital_in(uint8_t pin)        { pinMode(pin, INPUT); }
  static void begin_digital_in_pullup(uint8_t pin) { pinMode(pin, INPUT_PULLUP); }
  static void begin_digital_out(uint8_t pin)       { pinMode(pin, OUTPUT); }
  static void begin_pwm_out(uint8_t pin)           { pinMode(pin, OUTPUT); }

  static constexpr GpioArchTypes::pwm_type pwmMax(uint8_t)
  {
    return static_cast<GpioArchTypes::pwm_type>(255);
  }

  static GpioLevel read_digital(uint8_t pin)
  {
    return digitalRead(pin) ? GpioLevel::High : GpioLevel::Low;
  }

  static GpioArchTypes::analog_type read_analog(uint8_t pin)
  {
    return static_cast<GpioArchTypes::analog_type>(analogRead(pin));
  }

  static void write_digital(uint8_t pin, GpioLevel v)
  {
    digitalWrite(pin, (v == GpioLevel::High) ? HIGH : LOW);
  }

  static void write_pwm(uint8_t pin, GpioArchTypes::pwm_type v)
  {
    analogWrite(pin, v);
  }
};
