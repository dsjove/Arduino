#include <Arduino.h>

// #include "src/PinIO/Mcp23017PinIO.h"
// #include "src/PinIO/Mcp23017Device.h"
// #include "src/motor/TB6612Motor.h"
// #include "src/wifi/TheTime.h"
// #include "src/wifi/TheWifi.h"
// #include <NimBLEDevice.h> //Designates BLE impl
// #include "src/ble/TheBLE.h"
// #include "mic.h"
// #include "camera.h"
// #include "docking.h"
// #include "motion.h"

#include "src/PinIO/I2CHardware.h"
#include "src/PinIO/SPIHardware.h"
#include "src/PinIO/SBJTask.h"
#include "src/fs/TheSDCard.h"

//#include "src/audio/SbjAudioPlayerEsp32S3.h"
#include "src/display/ST7789Display.h"
#include "LightingSubsystem.h"

//Wire color	I²C Function	Signal
//Red	Power	VCC (3.3 V)
//Black	Ground	GND
//Blue	Data	SDA
//Yellow	Clock	SCL

// Camera (internal)
// ...
// SPI
//   SCK = D8
//   MISO = D9
//   MOSI = D10
// SDCard (internal)
//   CS = 21
// I2C
//   SDA = D4
//   SCL = D5
// Display
//   VCC = 5v
//   GND = GND
//   Backlight = 3.3V
//   TFT_CS = D3
//   TFT_DC = D1
//   TFT_RST = D0
//   Clock = SPI-SCK - D8
//   DIN = SPI-MOSI - D10
// I2S audio
//   DOUT = D2
//   BCLK = D6
//   LRCLK/WS = D7

// const std::string serviceName = "The Jove Express";
// TheBLE _ble(_taskScheduler, serviceName);
// TheWifi _wifi(_taskScheduler, serviceName);

// using expander = Mcp23017Device<>;
SPIHardware spi(D8, D9, D10);
I2CHardware i2c(D4, D5);
//TheSDCard sdcard;
//SbjAudioPlayerEsp32S3 audio(D2, D6, D7);
//ST7789Display<ST7789TraitsDft> display;
LightingSubsystem<> lighting;
// struct MotorPins
// {
//   inline static constexpr PinIO<D3, GpioMode::PWMOut> MotorPwma{};
//   inline static constexpr PinIO<A1, GpioMode::PWMOut> MotorPwmb{};
//   inline static constexpr PinIO<0, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorAin1{}; // GPA0
//   inline static constexpr PinIO<1, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorAin2{}; // GPA1
//   inline static constexpr PinIO<8, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorBin1{}; // GPB0
//   inline static constexpr PinIO<4, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorStby{}; // GPA4
//   inline static constexpr PinIO<9, GpioMode::DigitalOut, Mcp23017PinIO<>> MotorBin2{}; // GPB1
// };
// using motor = TB6612Motor<MotorPins>;

void setup()
{
// Serial
  Serial.begin(115200);
  delay(2000);              // Give USB CDC time to enumerate
  while (!Serial) { }       // Wait for USB Serial connection
  Serial.println("Serial ready");
  Serial.flush();

// SPI
  // sdcard.prepare();
  // display.prepare();
  spi.begin();

// SDCard
  // if (!sdcard.begin()) {
  //   Serial.println("SDCard begin failed.");
  //   return;
  // }

// Display
  // if (!display.begin()) {
  //   Serial.println("Display begin failed.");
  //   return;
  // }

// Audio
  // if (!audio.begin()) {
  //   Serial.println("Audio begin failed.");
  //   return;
  // }

// Comminications
  // _ble.begin();
  // _wifi.begin();

  lighting.begin();

  // mic::begin(_taskScheduler);
  // camera::begin(_taskScheduler);
  // motor::begin();
  // docking::begin(_taskScheduler);
  // motion::begin(_taskScheduler);
}

void loop()
{
  // SPIHardware::debugPrint();
  // sdcard.debugPrint();
  // display.drawBMP("/image.bmp");
  // audio.debugPrint();
  // if (!audio.playOnce("/pink_panther.wav"))
  // {
  //   Serial.println("audio play failed");
  // }
  // delay(10000);
  SBJTask::loop();
}
