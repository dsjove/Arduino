#pragma once

#if defined(__linux__)

#if __has_include(<gpiod.h>)
  #include <gpiod.h>
#else
  #error "libgpiod headers not found. Install libgpiod-dev (or equivalent)."
#endif

#include <cstdint>
#include <unistd.h>

#include "GpioTypes.h"

struct RaspberryPiGpioBackend
{
  static constexpr bool alwaysReady = true;
  
  static constexpr bool pin_exists(int pin)
  {
    return pin >= 0;
  }

  static constexpr bool pin_is_reserved(int)
  {
    return false;
  }

  static constexpr bool pin_supports_analog(int)
  {
    return false;
  }

  static constexpr bool pin_supports_pwm(int)
  {
    return false;
  }

  static void begin_digital_in(uint8_t pin)
  {
    // libgpiod setup for input would go here
    (void)pin;
  }

  static void begin_digital_out(uint8_t pin)
  {
    // libgpiod setup for output would go here
    (void)pin;
  }

  static void begin_digital_in_pullup(uint8_t pin)
  {
    // No standardized internal pull-up in this backend; treat as normal input.
    begin_digital_in(pin);
  }

  static GpioLevel read_digital(uint8_t pin)
  {
    // Placeholder; real implementation should read from gpiod line
    (void)pin;
    return GpioLevel::Low;
  }

  static void write_digital(uint8_t pin, GpioLevel v)
  {
    // Placeholder; real implementation should write to gpiod line
    (void)pin;
    (void)v;
  }

  static constexpr GpioArchTypes::pwm_type pwmMax(uint8_t)
  {
    return 0;
  }
};

#endif // __linux__
