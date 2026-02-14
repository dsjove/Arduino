#pragma once

#include <Arduino.h>

#include "../PinIO/PinIO.h"
#include "../PinIO/SBJTask.h"
#include "Veml7700AutoRange.h"

struct DefaultLightingTraits
{
  using SensorType = Veml7700AutoRange;

  static bool sensorBegin(SensorType& s)
  {
    return s.begin();
  }

  static float readLux(SensorType& s)
  {
    return s.read().lux;
  }
};

template <typename Traits = DefaultLightingTraits>
class LightingSubsystem
{
public:
  LightingSubsystem()
  : _task("lighting", 4096, this, TaskPriority::Low, 0, LightingTaskDesc{})
  {
  }

  void begin()
  {
//    Traits::HeadLightPwm.begin(0);
//    Traits::CarLightPwm.begin(0);
//    Traits::CarLightEnable.begin(GpioLevel::Low);
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
