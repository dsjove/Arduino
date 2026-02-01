#pragma once

#if defined(ARDUINO_ARCH_RENESAS_UNO)

  #include "UnoR4GpioBackend.h"
  using DefaultPinIOBackend = UnoR4GpioBackend;

#elif defined(ARDUINO_ARCH_ESP32)

  #include "Esp32GpioBackend.h"
  using DefaultPinIOBackend = pinio_esp32::DefaultEsp32PinIOBackend;

#elif defined(ARDUINO)

  #include "ArduinoGpioBackend.h"
  using DefaultPinIOBackend = ArduinoGpioBackend;

#elif defined(__linux__)

  #include "RaspberryPiGpioBackend.h"
  using DefaultPinIOBackend = RaspberryPiGpioBackend;

#else

  #error "No DefaultPinIOBackend available for this platform"

#endif
