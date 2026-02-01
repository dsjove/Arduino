#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

#ifndef I2C_ON_ERROR
  #define I2C_ON_ERROR(msg) ((void)0)
#endif

class I2CHardware
{
public:
  using BeginFn = void (*)(int sdaPin, int sclPin);
  using ErrorFn = void (*)(const char* message);

  static void init(int sdaPin, int sclPin, uint32_t hz, BeginFn fn = nullptr)
  {
    auto& s = state();

    if (s.begun)
    {
      reportError("[I2C] init() after begin()");
      return;
    }

    s.sda   = sdaPin;
    s.scl   = sclPin;
    s.clock = hz;

    if (fn)
    {
      s.beginFn = fn;
    }
  }

  static bool valid()
  {
    return state().valid();
  }

  static void onError(ErrorFn fn)
  {
    state().errorFn = fn;
  }

  static bool begin()
  {
    auto& s = state();

    if (s.begun)
    {
      return true;
    }

    if (!s.valid())
    {
      reportError(
        "[I2C] Invalid config. Call I2CHardware::init(sda, scl, clock[, beginFn]) in setup()."
      );
      return false;
    }

    s.beginFn(s.sda, s.scl);
    Wire.setClock(s.clock);

    s.begun = true;
    return true;
  }

#ifdef UNIT_TEST
  static void resetForTests()
  {
    state() = State{};
  }
#endif

private:
  static void reportError(const char* msg)
  {
    auto& s = state();
    if (s.errorFn) s.errorFn(msg);
    else           I2C_ON_ERROR(msg);
  }

  static void defaultBegin(int sdaPin, int sclPin)
  {
    (void)sdaPin;
    (void)sclPin;
#if __cpp_concepts
    if constexpr (requires { Wire.begin(sdaPin, sclPin); })
    {
      Wire.begin(sdaPin, sclPin);
    }
    else
    {
      Wire.begin();
    }
#else
    Wire.begin();
#endif
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

    bool valid() const
    {
      return
        (sda >= 0) &&
        (scl >= 0) &&
        (sda != scl) &&
        (beginFn != nullptr);
    }
  };

  static State& state()
  {
    static State s;
    return s;
  }
};
