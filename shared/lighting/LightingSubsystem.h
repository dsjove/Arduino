#pragma once

#include <Arduino.h>

#include "Veml7700AutoRange.h"
#include "../PinIO/SBJTask.h"

struct DefaultLightingTraits
{
  using SensorType = Veml7700AutoRange;

  static bool begin(SensorType& s) { return s.begin(); }

  static float readLux(SensorType& s)
  {
    return s.read().lux;
  }
};

template <typename Traits = DefaultLightingTraits>
class LightingSubsystem
{
public:
  using SensorType = typename Traits::SensorType;

  LightingSubsystem()
  : _task("lighting", this, LightingTaskDesc{})
  {
  }

  void begin()
  {
    (void)Traits::begin(_sensor);
    _task.begin();
  }

  float lux() const { return _lux; }

private:
  void tick()
  {
    _lux = Traits::readLux(_sensor);
    // Optional: comment out if you don't want serial spam
    // Serial.printf("[lighting] lux=%.2f\n", _lux);
  }

  struct LightingTaskDesc
  {
    using Obj = LightingSubsystem;
    static constexpr void (Obj::*Method)() = &Obj::tick;
    static inline const SBJTask::Schedule schedule{
      1000, FOREVER, 0,
      4096, TaskPriority::Low, 0
    };
  };

  float      _lux = 0.0f;
  SensorType _sensor{};

  SBJTask _task;
};
