#pragma once

#include "src/PinIO/TaskThunk.h"
#include "src/ble/IDBTCharacteristic.h"
#include "src/PinIO/PinIO.h"

struct TrainDockSensorTraitsDft
{
  using Pin = PinIO<7, GpioMode::AnalogIn>;
  static constexpr const char * bleProperty = "01020002";
  static constexpr int timingMS = 250;
};

template<typename Traits = TrainDockSensorTraitsDft>
class TrainDockSensor : ScheduledRunner
{
public:
  enum class Docked : uint8_t
  {
    None,
    Passive,
    Charging
  };
  using Callback = void (*)(Docked);

  TrainDockSensor(Scheduler& scheduler, BLEServiceRunner& ble, Callback callback)
  : _detected(Docked::None)
  , _callback(callback)
  , _sensedChar(ble, Traits::bleProperty, &_detected)
  , _task(scheduler, Traits::timingMS, this)
  {
  }

  void begin() {
	  Traits::Pin::begin();
  }

private:
  Docked _detected;
  Callback _callback;
  IDBTCharacteristic _sensedChar;
  TaskThunk _task;

  virtual void loop(Task&)
  {
    auto value = Traits::Pin::read();
    auto detected = value < 5 ? Docked::None : value < 20 ? Docked::Passive : Docked::Charging;
    if (detected != _detected) {
      if (_callback) _callback(detected);
      _sensedChar.ble.writeValue((uint8_t)detected);
    }
  }
};
