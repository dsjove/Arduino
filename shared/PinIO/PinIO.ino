#include <Arduino.h>

#include "PinIO.h"
#include "UnitTestPinIOBackend.h"
#include "SPIHardware.h"

// Example GPIO expansion board
#include "Mcp23017Device.h"

using LightPinDefault = PinIO<7, GpioMode::DigitalOut>;
using LightPinUnitTest = PinIO<7, GpioMode::DigitalOut, UnitTestPinIOBackend<>>;
using LightPinExpansionBoard = PinIO<9, GpioMode::DigitalOut, Mcp23017PinIO<>>;

// Authentic adapter example: some boards wire LEDs/transistors "active-low".
// This wrapper makes a normal DigitalOut pin behave like an active-low output.
template <typename PIN>
struct ActiveLowDigitalOut
{
  static void begin(GpioLevel initial)
  {
    PIN::begin(invert(initial));
  }

  static void write(GpioLevel v)
  {
    PIN::write(invert(v));
  }

  static GpioLevel read()
  {
    return invert(PIN::read());
  }

private:
  static constexpr GpioLevel invert(GpioLevel v)
  {
    return (v == GpioLevel::High) ? GpioLevel::Low : GpioLevel::High;
  }
};

using BuiltInLed = PinIO<LED_BUILTIN, GpioMode::DigitalOut>;
using BuiltInLedActiveLow = ActiveLowDigitalOut<BuiltInLed>;

template <typename PIN = LightPinDefault>
class Lighting
{
public:
  void begin()
  {
    PIN::begin(GpioLevel::High);
  }

  void tick()
  {
    const auto now = millis();
    if (now - lastToggleMs_ >= intervalMs_)
    {
      lastToggleMs_ = now;
      state_ = (state_ == GpioLevel::High) ? GpioLevel::Low : GpioLevel::High;
      PIN::write(state_);
    }
  }

private:
  uint32_t lastToggleMs_ = 0;
  uint32_t intervalMs_ = 250;
  GpioLevel state_ = GpioLevel::High;
};

Lighting<> light1;
Lighting<LightPinUnitTest> light2;
Lighting<LightPinExpansionBoard> light3;
Lighting<BuiltInLedActiveLow> light4;

Mcp23017Device expansion;

void setup()
{
  Serial.begin(115200);
  SPIHardware::begin();

  expansion.begin();

  light1.begin();
  light2.begin();
  light3.begin();
  light4.begin();
}

void loop()
{
  light1.tick();
  // light2/light3/light4 are here to compile/validate different backends/adapters.
  // You can call tick() on them too if desired.
  delay(1);
}
