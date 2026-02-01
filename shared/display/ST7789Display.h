#pragma once

#include "../pinio/PinIO.h"

#include <Adafruit_ST7789.h>
#include <stdint.h>

struct DefaultST7789Traits
{
  using Device = Adafruit_ST7789;

  inline static constexpr PinIO<D0, GpioMode::DigitalOut> Cs{};
  inline static constexpr PinIO<D1, GpioMode::DigitalOut> Dc{};
  inline static constexpr PinIO<PINIO_DISABLED_PIN, GpioMode::DigitalOut> Rst{};

  struct Spi
  {
    inline static void begin()
    {
      SPIHardware::begin();
    }
  };

  inline static constexpr uint16_t Width  = 240;
  inline static constexpr uint16_t Height = 240;

  inline static constexpr uint8_t Rotation = 0;

  inline static constexpr uint8_t XOffset = 0;
  inline static constexpr uint8_t YOffset = 0;

  inline static constexpr uint16_t BootFillColor = ST77XX_BLACK;
};


template <class Traits = DefaultST7789Traits>
struct ST7789Display
{
  using Device = typename Traits::Device;

  inline static Device device{
    Traits::Cs.pin,
    Traits::Dc.pin,
    Traits::Rst.pin // -1 if disabled / not connected
  };

  inline static void begin()
  {
    // Pins (reset begin becomes no-op if disabled)
    Traits::Cs.begin(GpioLevel::High);
    Traits::Dc.begin(GpioLevel::Low);
    Traits::Rst.begin(GpioLevel::High);

    // SPI
    Traits::Spi::begin();

    // Panel
    device.init(Traits::Width, Traits::Height);
    device.setRotation(Traits::Rotation);

    if constexpr ((Traits::XOffset != 0) || (Traits::YOffset != 0))
    {
      device.setRowColStart(Traits::XOffset, Traits::YOffset);
    }

    device.fillScreen(Traits::BootFillColor);
  }
};
