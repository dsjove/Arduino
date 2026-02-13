#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <cstdint>
#include <inttypes.h>

#ifndef I2C_ON_ERROR
  #define I2C_ON_ERROR(msg) ((void)0)
#endif

class I2CHardware
{
public:
  using BeginFn = void (*)(int sdaPin, int sclPin, uint32_t hz);
  using ErrorFn = void (*)(const char* message);

#if defined(__cpp_concepts)
  static constexpr bool kHasBeginWithPins = []{
    if constexpr (requires { Wire.begin(0, 0); }) { return true; }
    else { return false; }
  }();
#else
  static constexpr bool kHasBeginWithPins = false;
#endif

  I2CHardware(BeginFn fn = nullptr)
  {
    state().overrideDefaults(0, fn);
  }

  I2CHardware(uint32_t hz, BeginFn fn = nullptr)
  {
    state().overrideDefaults(hz, fn);
  }

  I2CHardware(int sdaPin, int sclPin, uint32_t hz = 0, BeginFn fn = nullptr)
  {
    state().overrideDefaults(sdaPin, sclPin, hz, fn);
  }

  static bool valid()
  {
    return state().valid();
  }

  static void onError(ErrorFn fn)
  {
    state().errorFn = fn;
  }

  // Call from any I2C device
  static bool begin()
  {
    return state().begin();
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
  struct State;   // forward declaration

  static State& state()
  {
    static State s;
    return s;
  }

  static void defaultBegin(int sdaPin, int sclPin, uint32_t hz)
  {
    if constexpr (kHasBeginWithPins) {
      Wire.begin(sdaPin, sclPin);
    } else {
      Wire.begin();
    }
    Wire.setClock(hz);
  }

  struct State
  {
    int sda =
#if defined(SDA)
        static_cast<int>(SDA);
#else
        -1;
#endif

    int scl =
#if defined(SCL)
        static_cast<int>(SCL);
#else
        -1;
#endif

    uint32_t clock =
#if defined(ARDUINO_ARCH_AVR)
        100000UL;
#else
        400000UL;
#endif

    BeginFn beginFn = &defaultBegin;
    ErrorFn errorFn = nullptr;
    bool begun = false;

    void overrideDefaults(uint32_t hz, BeginFn fn = nullptr)
    {
      if (begun)
      {
        reportError("[I2C] init() after begin()");
        return;
      }
      if (hz) { clock = hz; }
      if (fn) { beginFn = fn; }
    }

    void overrideDefaults(int sdaPin, int sclPin, uint32_t hz, BeginFn fn = nullptr)
    {
      if (begun)
      {
        reportError("[I2C] init() after begin()");
        return;
      }
      sda = sdaPin;
      scl = sclPin;
      if (hz) { clock = hz; }
      if (fn) { beginFn = fn; }
    }

    bool valid() const
    {
      if constexpr (kHasBeginWithPins)
      {
        return
          (sda >= 0) &&
          (scl >= 0) &&
          (sda != scl) &&
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
          "[I2C] Invalid config. Call I2CHardware(...) before begin()."
        );
        return false;
      }

      beginFn(sda, scl, clock);

      begun = true;
      return true;
    }

    void reportError(const char* msg)
    {
      if (errorFn) errorFn(msg); else I2C_ON_ERROR(msg);
    }

    void debugPrint() const
    {
#ifdef ARDUINO
      Serial.println("[I2C] Debug");
      Serial.print("  hasBeginWithPins: "); Serial.println(kHasBeginWithPins ? 1 : 0);
      Serial.print("  sda: "); Serial.println(sda);
      Serial.print("  scl: "); Serial.println(scl);
      Serial.print("  clock: "); Serial.println(clock);
      Serial.print("  valid: "); Serial.println(valid() ? 1 : 0);
      Serial.print("  begun: "); Serial.println(begun ? 1 : 0);
#endif
    }
  };
};

