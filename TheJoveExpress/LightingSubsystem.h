#pragma once

#include <Arduino.h>
#include <TaskScheduler.h>
#include <Wire.h>

#include <Adafruit_VEML7700.h>

#include "src/PinIO/PinIO.h"
#include "src/PinIO/I2CHardware.h"
#include "src/PinIO/SBJTask.h"

struct DefaultLightingTraits
{
  inline static constexpr PinIO<A0, GpioMode::PWMOut>     HeadLightPwm{};
  inline static constexpr PinIO<A2, GpioMode::PWMOut>     CarLightPwm{};
  inline static constexpr PinIO<A3, GpioMode::DigitalOut> CarLightEnable{};

  using SensorType = Adafruit_VEML7700;

  static bool sensorBegin(SensorType& s)
  {
    I2CHardware::begin();
    if(s.begin()) {
      s.setGain(VEML7700_GAIN_1);
      s.setIntegrationTime(VEML7700_IT_100MS);
      return true;
    }
    return false;
  }

  static float readLux(SensorType& s)
  {
    return s.readLux();
  }
};

template <typename Traits = DefaultLightingTraits>
class LightingSubsystem
{
public:
 // Inside LightingSubsystem

  LightingSubsystem()
  : _task("lighting", 4096, this, TaskPriority::Low, 0, LightingTaskDesc{})
  {
  }

  void begin()
  {
    Traits::HeadLightPwm.begin(0);
    Traits::CarLightPwm.begin(0);
    Traits::CarLightEnable.begin(GpioLevel::Low);
    if (!Traits::sensorBegin(sensor))
    {
      Serial.println("[lighting] sensor begin failed");
    }
    _task.begin();
  }

private:
  using SensorType = typename Traits::SensorType;
  float lux = 0.0f;
  SensorType sensor{};

  struct LightingTaskDesc {
    using Obj = LightingSubsystem;
    static constexpr void (Obj::*Method)() = &LightingSubsystem::tick;
    using Timing = SBJTask::TimingTraits<1000, FOREVER, 0>;
  };

  SBJTask _task;

  void tick()
  {
    lux = Traits::readLux(sensor);
    Serial.println(lux);
  }
};
