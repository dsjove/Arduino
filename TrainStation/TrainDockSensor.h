#pragma once

#include "src/task/TaskThunk.h"
#include "src/ble/IDBTCharacteristic.h"
#include "src/PinIO/PinIO.h"

struct TrainDockSensorDefaults
{
  using Pin = PinIO<7, GpioMode::AnalogIn>;
  static constexpr const char * propertyID = "01020002";
  static constexpr int timingMS = 250;
};

template<typename Traits = TrainDockSensorDefaults>
class TrainDockSensor : ScheduledRunner
{
public:
  enum class Docked : uint8_t
  {
    None,
    Passive,
    Charging
  };

  TrainDockSensor(Scheduler& scheduler, BLEServiceRunner& ble)
  : _detected(Docked::None)
  , _sensedChar(ble, Traits::propertyID, &_detected)
  , _task(scheduler, Traits::timingMS, this)
  {
  }

  void begin() {
	  Traits::Pin::begin();
  }

private:
  Docked _detected;
  IDBTCharacteristic _sensedChar;
  TaskThunk _task;

  virtual void loop(Task&)
  {
    auto value = Traits::Pin::read();
    auto detected = value < 5 ? Docked::None : value < 20 ? Docked::Passive : Docked::Charging;
    if (detected != _detected) {
      _sensedChar.ble.writeValue((uint8_t)detected);
    }
  }
};
