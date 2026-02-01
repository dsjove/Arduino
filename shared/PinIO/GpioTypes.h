#pragma once

#include <cstdint>

// Project-local disabled pin constant.
// Arduino's NOT_A_PIN exists but its numeric value is inconsistent across cores.
inline constexpr int PINIO_DISABLED_PIN = -1;

// ==================== Semantic digital level ====================
enum class GpioLevel : uint8_t
{
  Low  = 0,
  High = 1
};

// ==================== Pin modes ====================
enum class GpioMode : uint8_t
{
  DigitalIn,
  DigitalInPullup,
  AnalogIn,
  DigitalOut,
  PWMOut,
  Delegated
};

// ==================== Architecture-dependent scalar types ====================
struct GpioArchTypes
{
  using digital_type = GpioLevel;

  struct empty_type {};

#if defined(ARDUINO_ARCH_AVR)
  using analog_type = uint16_t;
  using pwm_type    = uint8_t;
#elif defined(ARDUINO_ARCH_RENESAS)
  using analog_type = uint16_t;
  using pwm_type    = uint16_t;
#elif defined(ARDUINO_ARCH_ESP32)
  using analog_type = uint16_t;
  using pwm_type    = uint16_t;
#else
  using analog_type = uint16_t;
  using pwm_type    = uint16_t;
#endif
};
