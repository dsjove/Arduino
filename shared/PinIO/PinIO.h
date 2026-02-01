#pragma once

#include <cstdint>
#include <type_traits>

#include "DefaultPinIOBackend.h"
#include "GpioModeTraits.h"
#include "GpioTypes.h"

template <int PIN, GpioMode MODE, typename Backend = DefaultPinIOBackend>
struct PinIO
{
  static constexpr int pin = PIN;
  static constexpr GpioMode mode = MODE;

  static constexpr bool enabled  = (PIN != PINIO_DISABLED_PIN);
  static constexpr bool disabled = (PIN == PINIO_DISABLED_PIN);

private:
  static constexpr bool pin_mode_allowed()
  {
    // Delegated means "PinIO is not responsible for validating this pin/mode".
    if constexpr (MODE == GpioMode::Delegated)
    {
      return true;
    }
    // Disabled pins are always allowed for any non-delegated mode (becomes no-op).
    if constexpr (disabled)
    {
      return true;
    }
    // Hard disallow reserved pins for non-delegated use.
    if constexpr (Backend::pin_is_reserved(PIN))
    {
      return false;
    }
    // Mode-specific capabilities.
    if constexpr (MODE == GpioMode::AnalogIn)
    {
      return Backend::pin_supports_analog(PIN);
    }
    if constexpr (MODE == GpioMode::PWMOut)
    {
      return Backend::pin_supports_pwm(PIN);
    }
    // DigitalIn / DigitalInPullup / DigitalOut: OK if pin is otherwise valid.
    return true;
  }

public:
  static_assert(
    (MODE == GpioMode::Delegated) || disabled || Backend::pin_exists(PIN),
    "Pin number is not valid for this backend (use PINIO_DISABLED_PIN to disable)"
  );

  static_assert(
    pin_mode_allowed(),
    "Pin has illegal mode for this backend"
  );

  using Traits = GpioModeTraits<MODE, Backend>;
  using value_type = typename Traits::value_type;

private:
  static constexpr uint8_t u8pin()
  {
    if constexpr (disabled)
    {
      return 0; // Never executed but required for compilation.
    }
    else
    {
      static_assert(PIN >= 0 && PIN <= 255, "Pin number out of uint8_t range");
      return static_cast<uint8_t>(PIN);
    }
  }

public:
  // Single readiness API (no duplicate ready()/isReady()).
  static bool isReady()
  {
    if constexpr (disabled)
    {
      return true;
    }

    if constexpr (Backend::alwaysReady)
    {
      return true;
    }
    else
    {
      return Backend::verifyReady();
    }
  }

  template <
    GpioMode M = MODE,
    typename std::enable_if_t<GpioModeTraits<M, Backend>::beginable, int> = 0>
  static void begin()
  {
    if constexpr (disabled) { return; }
    if (isReady() == false) { return; }

    Traits::begin(u8pin());
  }

  template <
    GpioMode M = MODE,
    typename std::enable_if_t<GpioModeTraits<M, Backend>::beginable &&
                              GpioModeTraits<M, Backend>::writable, int> = 0>
  static void begin(typename GpioModeTraits<M, Backend>::value_type initial)
  {
    if constexpr (disabled) { return; }
    if (isReady() == false) { return; }

    Traits::begin(u8pin());
    GpioModeTraits<M, Backend>::write(u8pin(), initial);
  }

  template <
    GpioMode M = MODE,
    typename std::enable_if_t<GpioModeTraits<M, Backend>::readable, int> = 0>
  static typename GpioModeTraits<M, Backend>::value_type read()
  {
    if constexpr (disabled) { return {}; }
    if (isReady() == false) { return {}; }

    return GpioModeTraits<M, Backend>::read(u8pin());
  }

  template <
    GpioMode M = MODE,
    typename std::enable_if_t<GpioModeTraits<M, Backend>::writable, int> = 0>
  static void write(typename GpioModeTraits<M, Backend>::value_type v)
  {
    if constexpr (disabled) { return; }
    if (isReady() == false) { return; }

    GpioModeTraits<M, Backend>::write(u8pin(), v);
  }

  template <
    GpioMode M = MODE,
    typename std::enable_if_t<(M == GpioMode::PWMOut) &&
                              GpioModeTraits<M, Backend>::writable, int> = 0>
  static void writeScaled(uint32_t value, uint32_t scaleMax)
  {
    using pwm_type = typename GpioModeTraits<M, Backend>::value_type;

    if constexpr (disabled) { return; }
    if (isReady() == false) { return; }

    if (scaleMax == 0)
    {
      write(static_cast<pwm_type>(0));
      return;
    }

    const uint32_t maxv = static_cast<uint32_t>(Backend::pwmMax(u8pin()));

    if (value >= scaleMax)
    {
      write(static_cast<pwm_type>(maxv));
      return;
    }

    const uint32_t mapped = (value * maxv + (scaleMax / 2)) / scaleMax;
    write(static_cast<pwm_type>(mapped));
  }

  template <
    GpioMode M = MODE,
    typename std::enable_if_t<(M == GpioMode::PWMOut) &&
                              GpioModeTraits<M, Backend>::writable, int> = 0>
  static void writeNormalized(float x)
  {
    using pwm_type = typename GpioModeTraits<M, Backend>::value_type;

    if constexpr (disabled) { return; }
    if (isReady() == false) { return; }

    if (x <= 0.0f) { write(static_cast<pwm_type>(0)); return; }
    if (x >= 1.0f) { write(static_cast<pwm_type>(Backend::pwmMax(u8pin()))); return; }

    constexpr uint32_t SCALE = 65535u;
    const uint32_t v = static_cast<uint32_t>(x * static_cast<float>(SCALE) + 0.5f);
    writeScaled(v, SCALE);
  }
};
