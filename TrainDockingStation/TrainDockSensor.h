#pragma once

#include "src/task/TaskThunk.h"
#include "src/ble/IDBTCharacteristic.h"
#include "src/PinIO/PinIO.h"

struct TrainDockSensorDefaults
{
  using Pin = PinIO<7, GpioMode::DigitalIn>;
  static constexpr const char * propertyID = "01020002";
  static constexpr int timingMS = 250;
};

template<typename Traits = TrainDockSensorDefaults>
class TrainDockSensor : ScheduledRunner
{
public:
  TrainDockSensor(Scheduler& scheduler, BLEServiceRunner& ble)
  : _detected(GpioLevel::Low)
  , _sensedChar(ble, Traits::propertyID, &_detected)
  , _task(scheduler, Traits::timingMS, this)
  {
  }

  void begin() {
	  Traits::Pin::begin();
  }

private:
  GpioLevel _detected;
  IDBTCharacteristic _sensedChar;
  TaskThunk _task;

  virtual void loop(Task&)
  {
	_detected = Traits::Pin::read();
  }
};
