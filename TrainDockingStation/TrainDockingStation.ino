#include <Arduino.h>

#include "src/ble/BLEServiceRunner.h"
#include "src/display/MatrixR4Display.h"
#include "src/rfid/RFIDBroadcaster.h"
#include "TrainDockSensor.h"

Scheduler _runner;
BLEServiceRunner _ble(_runner, "Train Docking");
MatrixR4Display _matrixR4(_ble, true, true);
RFIDBroadcaster<> _rfidBroadcaster(_runner, _ble);
TrainDockSensor<> _trainDockSensor(_runner, _ble);

constexpr int announceCount = 2;
constexpr int announceTime = 500;
int announceCounter = 0;
void announce()
{
  if (announceCounter == 0) {
    _matrixR4.update(MatrixR4Value::allOn);
  }
  else if (announceCounter == 1) {
    _matrixR4.update(MatrixR4Value::circle);
  }
  announceCounter++;
}
Task announceTask(announceTime, announceCount, &announce, &_runner);

void setup()
{
  Serial.begin(115200);
   _ble.begin();
  _matrixR4.begin();
  _rfidBroadcaster.begin();
  _trainDockSensor.begin();

    announceTask.enable();
}

void loop()
{
  _runner.execute();
}
