#pragma once

#include <Arduino.h>
#include <SPI.h>

#ifndef SPI_ON_ERROR
  #define SPI_ON_ERROR(msg) ((void)0)
#endif

class SPIHardware
{
public:
  using BeginFn = void (*)(int sckPin, int misoPin, int mosiPin);
  using ErrorFn = void (*)(const char* message);

  static void init(int sckPin, int misoPin, int mosiPin, BeginFn fn = nullptr)
  {
    auto& s = state();

    if (s.begun)
    {
      reportError("[SPI] init() after begin()");
      return;
    }

    s.sck  = sckPin;
    s.miso = misoPin;
    s.mosi = mosiPin;

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
        "[SPI] Invalid config. Call SPIHardware::init(sck, miso, mosi[, beginFn]) in setup()."
      );
      return false;
    }

    s.beginFn(s.sck, s.miso, s.mosi);
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
    else           SPI_ON_ERROR(msg);
  }

  static void defaultBegin(int sckPin, int misoPin, int mosiPin)
  {
    (void)sckPin;
    (void)misoPin;
    (void)mosiPin;
#if __cpp_concepts
    if constexpr (requires { SPI.begin(sckPin, misoPin, mosiPin); })
    {
      SPI.begin(sckPin, misoPin, mosiPin);
    }
    else
    {
      SPI.begin();
    }
#else
    SPI.begin();
#endif
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

    bool valid() const
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
  };

  static State& state()
  {
    static State s;
    return s;
  }
};
