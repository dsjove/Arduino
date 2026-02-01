#include <Arduino.h>
#include <TaskScheduler.h>

//Buses
#include "src/PinIO/Mcp23017PinIO.h"
#include "src/PinIO/Mcp23017Device.h"
#include "src/PinIO/SPIHardware.h"
#include "src/PinIO/I2CHardware.h"

// Communications
//#include <NimBLEDevice.h> //Designates BLE impl
//#include "src/ble/TheBLE.h"
#include "src/wifi/TheTime.h"
#include "src/wifi/TheWifi.h"

// MCU
#include "src/fs/TheSDCard.h"
#include "mic.h"
#include "camera.h"

// Motor
#include "src/motor/TB6612Motor.h"
#include "docking.h"

// Environment
#include "motion.h"
#include "LightingSubsystem.h"

// Display
#include "src/display/ST7789Display.h"
#include "MAX98357AAudio.h"

const std::string serviceName = "The Jove Express";
Scheduler _taskScheduler;
using expander = Mcp23017Device<>;
namespace sdcard = TheSDCard;
//TheBLE _ble(_taskScheduler, serviceName);
TheWifi _wifi(_taskScheduler, serviceName);
using lighting = LightingSubsystem<>;
using display = ST7789Display<>;
namespace audio = MAX98357AAudio;

struct MotorPins
{
  inline static constexpr PinIO<D3, GpioMode::PWMOut> MotorPwma{};
  inline static constexpr PinIO<A1, GpioMode::PWMOut> MotorPwmb{};
  inline static constexpr PinIO<0, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorAin1{}; // GPA0
  inline static constexpr PinIO<1, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorAin2{}; // GPA1
  inline static constexpr PinIO<8, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorBin1{}; // GPB0
  inline static constexpr PinIO<4, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorStby{}; // GPA4
  inline static constexpr PinIO<9, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorBin2{}; // GPB1
};
using motor = TB6612Motor<MotorPins>;

void setup()
{
// Buses
  Serial.begin(115200);
  expander::begin();

// Communications
  _ble.begin();
  _wifi.begin();

// MCU
  sdcard::begin();
  mic::begin(_taskScheduler);
  camera::begin(_taskScheduler);

// Motor
  motor::begin();
  docking::begin(_taskScheduler);

// Environment
  motion::begin(_taskScheduler);
  lighting::begin(_taskScheduler);

// Display
  display::begin();
  audio::begin();
}

void loop()
{
  _taskScheduler.execute();
}
