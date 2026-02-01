#pragma once

#if defined(ARDUINO) && defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include <cstdint>

#include "ArduinoGpioBackend.h"

// ESP-IDF SoC capability headers (Arduino-ESP32)
#if __has_include("soc/soc_caps.h")
  #include "soc/soc_caps.h"
#endif
#if __has_include("soc/gpio_periph.h")
  #include "soc/gpio_periph.h"
#endif
#if __has_include("soc/adc_channel.h")
  #include "soc/adc_channel.h"
#endif

namespace pinio_esp32
{

static constexpr bool pin_in_mask(uint64_t mask, int pin)
{
  return (pin >= 0 && pin < 64) ? (((mask >> pin) & 0x1ULL) != 0ULL) : false;
}

template <typename Traits>
struct Esp32GpioBackend : ArduinoGpioBackend
{
  static constexpr bool pin_exists(int pin)
  {
    if constexpr (!Traits::knownTarget)
    {
      // permissive only for unknown targets
      return pin >= 0;
    }
    else
    {
      static_assert(Traits::hasGpioMasks,
        "PinIO ESP32 backend: missing SOC_GPIO_VALID_* masks; cannot prove GPIO existence");

      if (pin < 0) return false;
      if (Traits::gpioPinCount > 0 && pin >= Traits::gpioPinCount) return false;
      return pin_in_mask(Traits::validGpioMask, pin);
    }
  }

  static constexpr bool pin_is_reserved(int pin)
  {
    if constexpr (!Traits::knownTarget) return false;
    return pin_exists(pin) && !pin_in_mask(Traits::validOutputMask, pin);
  }

  static constexpr bool pin_supports_pwm(int pin)
  {
    if constexpr (!Traits::knownTarget) return pin_exists(pin);
    return pin_exists(pin) && pin_in_mask(Traits::validOutputMask, pin);
  }

  static constexpr bool pin_supports_analog(int pin)
  {
    if constexpr (!Traits::knownTarget) return pin_exists(pin);

    static_assert(Traits::hasAdcMacros,
      "PinIO ESP32 backend: missing ADC*_CHANNEL_*_GPIO_NUM macros; cannot prove analog pins");

    return pin_exists(pin) && pin_in_mask(Traits::adc_gpio_mask(), pin);
  }
};

// ---------------------- traits ----------------------
struct Esp32UnknownTraits
{
  static constexpr bool knownTarget   = false;
  static constexpr bool hasGpioMasks  = false;
  static constexpr uint64_t validGpioMask   = 0;
  static constexpr uint64_t validOutputMask = 0;
  static constexpr int  gpioPinCount  = -1;
  static constexpr bool hasAdcMacros  = false;
  static constexpr uint64_t adc_gpio_mask() { return 0; }
};

// Shared “mask + count” definitions for known targets (compiled per target)
#define PINIO_ESP32_TRAITS_GPIO_FIELDS                                     \
  static constexpr bool knownTarget = true;                                \
  static constexpr bool hasGpioMasks =                                     \
    (defined(SOC_GPIO_VALID_GPIO_MASK) && defined(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) && defined(SOC_GPIO_PIN_COUNT)); \
  static constexpr uint64_t validGpioMask =                                \
    (defined(SOC_GPIO_VALID_GPIO_MASK) ? static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK) : 0ULL); \
  static constexpr uint64_t validOutputMask =                              \
    (defined(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) ? static_cast<uint64_t>(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) : 0ULL); \
  static constexpr int gpioPinCount = (defined(SOC_GPIO_PIN_COUNT) ? SOC_GPIO_PIN_COUNT : -1)

#undef PINIO_ESP32_TRAITS_GPIO_FIELDS
// (Can’t use defined() inside a macro value reliably across all compilers in this exact style.)
// So we write each traits explicitly below.

// ESP32 classic
struct Esp32ClassicTraits
{
  static constexpr bool knownTarget = true;

#if defined(SOC_GPIO_VALID_GPIO_MASK) && defined(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) && defined(SOC_GPIO_PIN_COUNT)
  static constexpr bool     hasGpioMasks     = true;
  static constexpr uint64_t validGpioMask    = static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK);
  static constexpr uint64_t validOutputMask  = static_cast<uint64_t>(SOC_GPIO_VALID_OUTPUT_GPIO_MASK);
  static constexpr int      gpioPinCount     = SOC_GPIO_PIN_COUNT;
#else
  static constexpr bool     hasGpioMasks     = false;
  static constexpr uint64_t validGpioMask    = 0;
  static constexpr uint64_t validOutputMask  = 0;
  static constexpr int      gpioPinCount     = -1;
#endif

#if defined(ADC1_CHANNEL_0_GPIO_NUM) || defined(ADC2_CHANNEL_0_GPIO_NUM)
  static constexpr bool hasAdcMacros = true;
#else
  static constexpr bool hasAdcMacros = false;
#endif

  static constexpr uint64_t adc_gpio_mask()
  {
    uint64_t m = 0;
    // ADC1 (0..9)
#ifdef ADC1_CHANNEL_0_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_0_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_1_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_1_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_2_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_2_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_3_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_3_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_4_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_4_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_5_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_5_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_6_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_6_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_7_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_7_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_8_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_8_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_9_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_9_GPIO_NUM);
#endif

    // ADC2 (0..9)
#ifdef ADC2_CHANNEL_0_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_0_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_1_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_1_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_2_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_2_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_3_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_3_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_4_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_4_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_5_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_5_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_6_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_6_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_7_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_7_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_8_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_8_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_9_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_9_GPIO_NUM);
#endif
    return m;
  }
};

// ESP32-S2
struct Esp32S2Traits
{
  static constexpr bool knownTarget = true;

#if defined(SOC_GPIO_VALID_GPIO_MASK) && defined(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) && defined(SOC_GPIO_PIN_COUNT)
  static constexpr bool     hasGpioMasks     = true;
  static constexpr uint64_t validGpioMask    = static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK);
  static constexpr uint64_t validOutputMask  = static_cast<uint64_t>(SOC_GPIO_VALID_OUTPUT_GPIO_MASK);
  static constexpr int      gpioPinCount     = SOC_GPIO_PIN_COUNT;
#else
  static constexpr bool     hasGpioMasks     = false;
  static constexpr uint64_t validGpioMask    = 0;
  static constexpr uint64_t validOutputMask  = 0;
  static constexpr int      gpioPinCount     = -1;
#endif

#if defined(ADC1_CHANNEL_0_GPIO_NUM) || defined(ADC2_CHANNEL_0_GPIO_NUM)
  static constexpr bool hasAdcMacros = true;
#else
  static constexpr bool hasAdcMacros = false;
#endif

  static constexpr uint64_t adc_gpio_mask()
  {
    // Macro-driven; only OR-in what exists.
    uint64_t m = 0;
#ifdef ADC1_CHANNEL_0_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_0_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_1_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_1_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_2_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_2_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_3_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_3_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_4_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_4_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_5_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_5_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_6_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_6_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_7_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_7_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_8_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_8_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_9_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_9_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_0_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_0_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_1_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_1_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_2_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_2_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_3_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_3_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_4_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_4_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_5_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_5_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_6_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_6_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_7_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_7_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_8_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_8_GPIO_NUM);
#endif
#ifdef ADC2_CHANNEL_9_GPIO_NUM
    m |= (1ULL << ADC2_CHANNEL_9_GPIO_NUM);
#endif
    return m;
  }
};

// ESP32-S3 (same “macro-driven” pattern; define separately for clarity)
using Esp32S3Traits = Esp32S2Traits;

// ESP32-C3
struct Esp32C3Traits
{
  static constexpr bool knownTarget = true;

#if defined(SOC_GPIO_VALID_GPIO_MASK) && defined(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) && defined(SOC_GPIO_PIN_COUNT)
  static constexpr bool     hasGpioMasks     = true;
  static constexpr uint64_t validGpioMask    = static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK);
  static constexpr uint64_t validOutputMask  = static_cast<uint64_t>(SOC_GPIO_VALID_OUTPUT_GPIO_MASK);
  static constexpr int      gpioPinCount     = SOC_GPIO_PIN_COUNT;
#else
  static constexpr bool     hasGpioMasks     = false;
  static constexpr uint64_t validGpioMask    = 0;
  static constexpr uint64_t validOutputMask  = 0;
  static constexpr int      gpioPinCount     = -1;
#endif

#if defined(ADC1_CHANNEL_0_GPIO_NUM)
  static constexpr bool hasAdcMacros = true;
#else
  static constexpr bool hasAdcMacros = false;
#endif

  static constexpr uint64_t adc_gpio_mask()
  {
    uint64_t m = 0;
#ifdef ADC1_CHANNEL_0_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_0_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_1_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_1_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_2_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_2_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_3_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_3_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_4_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_4_GPIO_NUM);
#endif
    return m;
  }
};

// ESP32-C2 (same ADC1-only shape)
using Esp32C2Traits = Esp32C3Traits;

// ESP32-C6
struct Esp32C6Traits
{
  static constexpr bool knownTarget = true;

#if defined(SOC_GPIO_VALID_GPIO_MASK) && defined(SOC_GPIO_VALID_OUTPUT_GPIO_MASK) && defined(SOC_GPIO_PIN_COUNT)
  static constexpr bool     hasGpioMasks     = true;
  static constexpr uint64_t validGpioMask    = static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK);
  static constexpr uint64_t validOutputMask  = static_cast<uint64_t>(SOC_GPIO_VALID_OUTPUT_GPIO_MASK);
  static constexpr int      gpioPinCount     = SOC_GPIO_PIN_COUNT;
#else
  static constexpr bool     hasGpioMasks     = false;
  static constexpr uint64_t validGpioMask    = 0;
  static constexpr uint64_t validOutputMask  = 0;
  static constexpr int      gpioPinCount     = -1;
#endif

#if defined(ADC1_CHANNEL_0_GPIO_NUM)
  static constexpr bool hasAdcMacros = true;
#else
  static constexpr bool hasAdcMacros = false;
#endif

  static constexpr uint64_t adc_gpio_mask()
  {
    uint64_t m = 0;
#ifdef ADC1_CHANNEL_0_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_0_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_1_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_1_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_2_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_2_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_3_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_3_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_4_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_4_GPIO_NUM);
#endif
#ifdef ADC1_CHANNEL_5_GPIO_NUM
    m |= (1ULL << ADC1_CHANNEL_5_GPIO_NUM);
#endif
    return m;
  }
};

// ESP32-H2 (same ADC1-only shape)
using Esp32H2Traits = Esp32C6Traits;

// ---------------------- model backend aliases ----------------------
using Esp32ClassicGpioBackend = Esp32GpioBackend<Esp32ClassicTraits>;
using Esp32S2GpioBackend      = Esp32GpioBackend<Esp32S2Traits>;
using Esp32S3GpioBackend      = Esp32GpioBackend<Esp32S3Traits>;
using Esp32C3GpioBackend      = Esp32GpioBackend<Esp32C3Traits>;
using Esp32C2GpioBackend      = Esp32GpioBackend<Esp32C2Traits>;
using Esp32C6GpioBackend      = Esp32GpioBackend<Esp32C6Traits>;
using Esp32H2GpioBackend      = Esp32GpioBackend<Esp32H2Traits>;
using Esp32UnknownGpioBackend = Esp32GpioBackend<Esp32UnknownTraits>;

// ---------------------- default ESP32 backend selection ----------------------
// Prefer CONFIG_IDF_TARGET_* if present (ESP-IDF / most Arduino-ESP32 builds).
#if defined(CONFIG_IDF_TARGET_ESP32)
  using DefaultEsp32PinIOBackend = Esp32ClassicGpioBackend;
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  using DefaultEsp32PinIOBackend = Esp32S2GpioBackend;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  using DefaultEsp32PinIOBackend = Esp32S3GpioBackend;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  using DefaultEsp32PinIOBackend = Esp32C3GpioBackend;
#elif defined(CONFIG_IDF_TARGET_ESP32C2)
  using DefaultEsp32PinIOBackend = Esp32C2GpioBackend;
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  using DefaultEsp32PinIOBackend = Esp32C6GpioBackend;
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
  using DefaultEsp32PinIOBackend = Esp32H2GpioBackend;

// Fallback: some builds expose chip macros instead of CONFIG_IDF_TARGET_*.
#elif defined(ESP32S2)
  using DefaultEsp32PinIOBackend = Esp32S2GpioBackend;
#elif defined(ESP32S3)
  using DefaultEsp32PinIOBackend = Esp32S3GpioBackend;
#elif defined(ESP32C3)
  using DefaultEsp32PinIOBackend = Esp32C3GpioBackend;
#elif defined(ESP32C2)
  using DefaultEsp32PinIOBackend = Esp32C2GpioBackend;
#elif defined(ESP32C6)
  using DefaultEsp32PinIOBackend = Esp32C6GpioBackend;
#elif defined(ESP32H2)
  using DefaultEsp32PinIOBackend = Esp32H2GpioBackend;

// Last resort: classic ESP32 (note: some environments define ESP32 for all chips).
#elif defined(ESP32)
  using DefaultEsp32PinIOBackend = Esp32ClassicGpioBackend;

#else
  using DefaultEsp32PinIOBackend = Esp32UnknownGpioBackend; // permissive fallback only here
#endif

#endif
