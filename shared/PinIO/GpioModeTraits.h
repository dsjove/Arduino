#pragma once

#include <cstdint>

#include "GpioTypes.h"

template <GpioMode, typename Backend>
struct GpioModeTraits;

template <typename Backend>
struct GpioModeTraits<GpioMode::Delegated, Backend>
{
  using value_type = GpioArchTypes::empty_type;
  static constexpr bool beginable = false;
  static constexpr bool readable  = false;
  static constexpr bool writable  = false;
};

template <typename Backend>
struct GpioModeTraits<GpioMode::DigitalIn, Backend>
{
  using value_type = GpioArchTypes::digital_type;
  static constexpr bool beginable = true;
  static constexpr bool readable  = true;
  static constexpr bool writable  = false;

  static void begin(uint8_t pin) { Backend::begin_digital_in(pin); }
  static value_type read(uint8_t pin) { return Backend::read_digital(pin); }
};

template <typename Backend>
struct GpioModeTraits<GpioMode::DigitalInPullup, Backend>
{
  using value_type = GpioArchTypes::digital_type;
  static constexpr bool beginable = true;
  static constexpr bool readable  = true;
  static constexpr bool writable  = false;

  static void begin(uint8_t pin) { Backend::begin_digital_in_pullup(pin); }
  static value_type read(uint8_t pin) { return Backend::read_digital(pin); }
};

template <typename Backend>
struct GpioModeTraits<GpioMode::AnalogIn, Backend>
{
  using value_type = GpioArchTypes::analog_type;
  static constexpr bool beginable = true;
  static constexpr bool readable  = true;
  static constexpr bool writable  = false;

  static void begin(uint8_t pin) { Backend::begin_analog_in(pin); }
  static value_type read(uint8_t pin) { return Backend::read_analog(pin); }
};

template <typename Backend>
struct GpioModeTraits<GpioMode::DigitalOut, Backend>
{
  using value_type = GpioArchTypes::digital_type;
  static constexpr bool beginable = true;
  static constexpr bool readable  = false;
  static constexpr bool writable  = true;

  static void begin(uint8_t pin) { Backend::begin_digital_out(pin); }
  static void write(uint8_t pin, value_type v) { Backend::write_digital(pin, v); }
};

template <typename Backend>
struct GpioModeTraits<GpioMode::PWMOut, Backend>
{
  using value_type = GpioArchTypes::pwm_type;
  static constexpr bool beginable = true;
  static constexpr bool readable  = false;
  static constexpr bool writable  = true;

  static void begin(uint8_t pin) { Backend::begin_pwm_out(pin); }
  static void write(uint8_t pin, value_type v) { Backend::write_pwm(pin, v); }
};
