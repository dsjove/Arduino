#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "PinIO.h"

#ifndef SPI_ON_ERROR
  #define SPI_ON_ERROR(msg) ((void)0)
#endif

class SPIHardware
{
public:
  using BeginFn = void (*)(int sckPin, int misoPin, int mosiPin);
  using ErrorFn = void (*)(const char* message);

#if defined(__cpp_concepts)
  static constexpr bool kHasBeginWithPins = []{
    if constexpr (requires { SPI.begin(0, 0, 0); }) { return true; }
    else { return false; }
  }();
#else
  static constexpr bool kHasBeginWithPins = false;
#endif

  SPIHardware(BeginFn fn = nullptr)
  {
    state().overrideDefaults(fn);
  }

  SPIHardware(int sckPin, int misoPin, int mosiPin, BeginFn fn = nullptr)
  {
    state().overrideDefaults(sckPin, misoPin, mosiPin, fn);
  }

  static bool valid()
  {
    return state().valid();
  }

  static void onError(ErrorFn fn)
  {
    state().errorFn = fn;
  }

  template<typename PIN>
  static void prepare()
  {
    // Assumes your GPIO type system provides this.
    PIN::begin(GpioLevel::High);
  }

  // Call once after all devices have called prepare()
  static bool begin()
  {
    return state().begin();
  }

  static void reportError(const char* msg)
  {
    state().reportError(msg);
  }

  static void debugPrint()
  {
    state().debugPrint();
  }

#ifdef UNIT_TEST
  static void resetForTests()
  {
    state() = State{};
  }
#endif

private:
  struct State; // forward declaration

  static State& state()
  {
    static State s;
    return s;
  }

  // Must be a free/static function matching BeginFn exactly.
  static void defaultBegin(int sckPin, int misoPin, int mosiPin)
  {
    (void)sckPin;
    (void)misoPin;
    (void)mosiPin;

    if constexpr (kHasBeginWithPins) {
      SPI.begin(sckPin, misoPin, mosiPin);
    } else {
      SPI.begin();
    }
  }

  struct State
  {
    int sck =
#if defined(ARDUINO_AVR_MEGA2560)
        52;
#elif defined(ARDUINO_ARCH_AVR)
        13;
#elif defined(SCK)
        static_cast<int>(SCK);
#else
        -1;
#endif

    int miso =
#if defined(ARDUINO_AVR_MEGA2560)
        50;
#elif defined(ARDUINO_ARCH_AVR)
        12;
#elif defined(MISO)
        static_cast<int>(MISO);
#else
        -1;
#endif

    int mosi =
#if defined(ARDUINO_AVR_MEGA2560)
        51;
#elif defined(ARDUINO_ARCH_AVR)
        11;
#elif defined(MOSI)
        static_cast<int>(MOSI);
#else
        -1;
#endif

    BeginFn beginFn = &defaultBegin;
    ErrorFn errorFn = nullptr;
    bool begun = false;

    void overrideDefaults(int sckPin, int misoPin, int mosiPin, BeginFn fn = nullptr)
    {
      if (begun)
      {
        reportError("[SPI] init() after begin()");
        return;
      }
      sck  = sckPin;
      miso = misoPin;
      mosi = mosiPin;
      if (fn) { beginFn = fn; }
    }

    void overrideDefaults(BeginFn fn = nullptr)
    {
      if (begun)
      {
        reportError("[SPI] init() after begin()");
        return;
      }
      if (fn) { beginFn = fn; }
    }

    bool valid() const
    {
      if constexpr (kHasBeginWithPins)
      {
        return
          (sck >= 0) &&
          (miso >= 0) &&
          (mosi >= 0) &&
          (sck != miso) &&
          (miso != mosi) &&
          (mosi != sck) &&
          (beginFn != nullptr);
      }
      else
      {
        return (beginFn != nullptr);
      }
    }

    bool begin()
    {
      if (begun) { return true; }

      if (!valid())
      {
        reportError(
          "[SPI] Invalid config. Call SPIHardware(sck, miso, mosi[, beginFn]) before begin()."
        );
        return false;
      }

      beginFn(sck, miso, mosi);
      begun = true;
      return true;
    }

    void reportError(const char* msg)
    {
      if (errorFn) errorFn(msg); else SPI_ON_ERROR(msg);
    }

    void debugPrint() const
    {
#ifdef ARDUINO
      Serial.println("[SPI] Debug");
      Serial.print("  hasBeginWithPins: "); Serial.println(kHasBeginWithPins ? 1 : 0);
      Serial.print("  sck: "); Serial.println(sck);
      Serial.print("  miso: "); Serial.println(miso);
      Serial.print("  mosi: "); Serial.println(mosi);
      Serial.print("  valid: "); Serial.println(valid() ? 1 : 0);
      Serial.print("  begun: "); Serial.println(begun ? 1 : 0);
#endif
    }
  };
};
