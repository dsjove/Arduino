#pragma once

#include <Arduino.h>

#include "ArduinoGpioBackend.h"

#include "GpioTypes.h"

struct UnoR4GpioBackend: ArduinoGpioBackend
{
  static void begin_analog_in(uint8_t pin)
  {
    pinMode(pin, INPUT);
  }

  static constexpr uint16_t pwmMax(uint8_t)
  {
#if defined(PWM_RESOLUTION)
    return static_cast<uint16_t>((1u << PWM_RESOLUTION) - 1u);
#elif defined(ANALOG_WRITE_RESOLUTION)
    return static_cast<uint16_t>((1u << ANALOG_WRITE_RESOLUTION) - 1u);
#else
    return 255;
#endif
  }
};
