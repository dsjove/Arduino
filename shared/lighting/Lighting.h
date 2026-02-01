#pragma once

#include "../core/TaskThunk.h"
#include "../ble/IDBTCharacteristic.h"
#include <vector>

struct LightOutput
{
	int pin;
	bool dimmable;
};

class Lighting: ScheduledRunner
{
public:
  Lighting(Scheduler& scheduler, BLEServiceRunner& ble, std::vector<LightOutput> output, int sensor = -1);

  void begin();

private:
  const std::vector<LightOutput> _output;
  const int _sensor;
  uint8_t _currentPower;
  uint8_t _currentCalibration;
  uint8_t _currentAmbient;
  uint8_t _currentSignal;
  
  IDBTCharacteristic _powerControlChar;
  IDBTCharacteristic _powerFeedbackChar;
  IDBTCharacteristic _calibrationChar;
  IDBTCharacteristic _sensedFeedbackChar;
  static void updatePower(BLEDevice device, BLECharacteristic characteristic);
  static void updateCalibration(BLEDevice device, BLECharacteristic characteristic);
  static void updateSensed(BLEDevice device, BLECharacteristic characteristic);

  virtual void loop(Task&);
  TaskThunk _lightingTask;
  
  void update();
};
