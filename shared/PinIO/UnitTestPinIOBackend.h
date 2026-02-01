#pragma once

#include <cstdint>
#include <type_traits>

#include "GpioTypes.h"

// ============================================================================
// UnitTestPinIO
// PinIO backend for unit testing (no Arduino dependency)
// - Stores per-pin mode and values in static arrays
// - Tracks calls (begins/reads/writes) for assertions
// - Lets tests configure per-pin capabilities + reservations
//
// Usage:
//   using UT = UnitTestPinIO<32>;
//   UT::reset();
//   using Led = PinIO<5, GpioMode::DigitalOut, UT>;
//   Led::begin(GpioLevel::Low);
//   Led::write(GpioLevel::High);
//   EXPECT_EQ(UT::digital[5], GpioLevel::High);
//   EXPECT_EQ(UT::write_digital_calls[5], 1);
// ============================================================================
template <int NumPins = 32>
struct UnitTestPinIOBackend
{
  static constexpr bool alwaysReady = true;
  
  static_assert(NumPins > 0 && NumPins <= 256, "NumPins must be 1..256");

  static constexpr bool pin_exists(int pin)
  {
    return pin >= 0 && pin < NumPins;
  }

  static constexpr bool pin_supports_analog(int)
  {
    return true;
  }

  static constexpr bool pin_supports_pwm(int)
  {
    return true;
  }

  static constexpr bool pin_is_reserved(int)
  {
    return false;
  }

  static constexpr GpioArchTypes::pwm_type pwmMax(uint8_t)
  {
    return static_cast<GpioArchTypes::pwm_type>(255);
  }

  // --------------------
  // Stored state
  // --------------------
  static inline GpioMode mode[NumPins] = {};

  static inline GpioLevel digital[NumPins] = {};
  static inline GpioArchTypes::analog_type analog[NumPins] = {};
  static inline GpioArchTypes::pwm_type pwm[NumPins] = {};

  // --------------------
  // Counters for assertions
  // --------------------
  static inline uint32_t begin_digital_in_calls[NumPins] = {};
  static inline uint32_t begin_digital_in_pullup_calls[NumPins] = {};
  static inline uint32_t begin_analog_in_calls[NumPins] = {};
  static inline uint32_t begin_digital_out_calls[NumPins] = {};
  static inline uint32_t begin_pwm_out_calls[NumPins] = {};

  static inline uint32_t read_digital_calls[NumPins] = {};
  static inline uint32_t read_analog_calls[NumPins] = {};

  static inline uint32_t write_digital_calls[NumPins] = {};
  static inline uint32_t write_pwm_calls[NumPins] = {};

  // --------------------
  // Test helpers
  // --------------------
  static void reset()
  {
    for (int i = 0; i < NumPins; ++i)
    {
      mode[i] = GpioMode::Delegated;

      digital[i] = GpioLevel::Low;
      analog[i]  = 0;
      pwm[i]     = 0;

      begin_digital_in_calls[i] = 0;
      begin_digital_in_pullup_calls[i] = 0;
      begin_analog_in_calls[i] = 0;
      begin_digital_out_calls[i] = 0;
      begin_pwm_out_calls[i] = 0;

      read_digital_calls[i] = 0;
      read_analog_calls[i] = 0;

      write_digital_calls[i] = 0;
      write_pwm_calls[i] = 0;
    }
  }

  // Optionally seed read values
  static void seedDigital(int pin, GpioLevel v)
  {
    if (pin >= 0 && pin < NumPins) digital[pin] = v;
  }

  static void seedAnalog(int pin, GpioArchTypes::analog_type v)
  {
    if (pin >= 0 && pin < NumPins) analog[pin] = v;
  }

  // --------------------
  // Backend surface required by PinIO
  // --------------------
  static void begin_digital_in(uint8_t pin)
  {
    mode[pin] = GpioMode::DigitalIn;
    ++begin_digital_in_calls[pin];
  }

  static void begin_digital_in_pullup(uint8_t pin)
  {
    mode[pin] = GpioMode::DigitalInPullup;
    ++begin_digital_in_pullup_calls[pin];
  }

  static void begin_analog_in(uint8_t pin)
  {
    mode[pin] = GpioMode::AnalogIn;
    ++begin_analog_in_calls[pin];
  }

  static void begin_digital_out(uint8_t pin)
  {
    mode[pin] = GpioMode::DigitalOut;
    ++begin_digital_out_calls[pin];
  }

  static void begin_pwm_out(uint8_t pin)
  {
    mode[pin] = GpioMode::PWMOut;
    ++begin_pwm_out_calls[pin];
  }

  static GpioLevel read_digital(uint8_t pin)
  {
    ++read_digital_calls[pin];
    return digital[pin];
  }

  static GpioArchTypes::analog_type read_analog(uint8_t pin)
  {
    ++read_analog_calls[pin];
    return analog[pin];
  }

  static void write_digital(uint8_t pin, GpioLevel v)
  {
    ++write_digital_calls[pin];
    digital[pin] = v;
  }

  static void write_pwm(uint8_t pin, GpioArchTypes::pwm_type v)
  {
    ++write_pwm_calls[pin];
    pwm[pin] = v;
  }
};
/*
void sample() {
	using UT = UnitTestPinIO<32, true>;

	UT::reset();

	using Led = PinIO<5, GpioMode::DigitalOut, UT>;
	using Fan = PinIO<6, GpioMode::PWMOut, UT>;

	Led::begin(GpioLevel::Low);
	Led::write(GpioLevel::High);

	Fan::begin();
	Fan::write(123);

	REQUIRE(UT::digital[5] == GpioLevel::High);
	REQUIRE(UT::write_digital_calls[5] == 1);
	REQUIRE(UT::pwm[6] == 123);
	REQUIRE(UT::write_pwm_calls[6] == 1);
}
*/
