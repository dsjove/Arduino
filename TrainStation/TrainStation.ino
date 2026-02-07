#include <Arduino.h>
#include <array>
#include <algorithm>

#include "src/ble/BLEServiceRunner.h"
#include "src/display/MatrixR4Display.h"
#include "src/rfid/RFIDBroadcaster.h"
#include "TrainDockSensor.h"

struct KnownTrain {
  RFID::ID id;
  MatrixR4Value::Value logo;
};
#include "config.h"

constexpr size_t UnknownLogoIdx = 0;
static std::array<KnownTrain, 10> _knownTrains;
static size_t _knownTrainCount = std::min(_knownTrains.size(), config::knownTrains.size());
static size_t _selectedTrainLogo = UnknownLogoIdx;
static RFID::ID _lastId = {};

Scheduler _runner;
BLEServiceRunner _ble(_runner, config::serviceName);

static inline void bridgeMatrix(const MatrixR4Value::Value& edited) {
  if (_selectedTrainLogo != UnknownLogoIdx) {
    _knownTrains[_selectedTrainLogo].logo = edited;
  }
  else if (_knownTrainCount < _knownTrains.size()) {
    _selectedTrainLogo = _knownTrainCount;
    _knownTrains[_selectedTrainLogo].id = _lastId;
    _knownTrains[_selectedTrainLogo].logo = edited;
    _knownTrainCount++;
  }
}

MatrixR4Display<config::MatrixR4DTraits> _matrixR4(_runner, _ble, bridgeMatrix);

static inline void bridgeRFID(const RFID& detected) {
  _lastId = detected._uuid;
  _selectedTrainLogo = UnknownLogoIdx;
  for (size_t i = UnknownLogoIdx + 1; i < _knownTrainCount; ++i) {
    if (_knownTrains[i].id == _lastId) {
	    _selectedTrainLogo = i;
      break;
    }
  }
  _matrixR4.update(_knownTrains[_selectedTrainLogo].logo);
}

RFIDBroadcaster<config::RFIDBroadcasterTraits> _rfidBroadcaster(_runner, _ble, &bridgeRFID);
TrainDockSensor<config::TrainDockSensorTraits> _trainDockSensor(_runner, _ble, nullptr);

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
  std::copy(config::knownTrains.data(), config::knownTrains.data() + _knownTrainCount, _knownTrains.data());
  
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
