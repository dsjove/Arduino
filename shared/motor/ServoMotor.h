#pragma once

#include <Servo.h>
#include "../ble/IDBTCharacteristic.h"

/*
Hardware:
L298N Motor Drive Controller Board DC Dual H-Bridge Robot Stepper
 */

class ServoMotor
{
public:
  ServoMotor(BLEServiceRunner& ble, int pin);

  void begin();

private:
  const int _pin;
  const int8_t _powerMin = -127;
  const int8_t _powerStop = 0;
  const int8_t _powerMax = 127;
  int8_t _currentPower;
  uint8_t _currentCalibration;

  const int _signalMin = 0;
  const int _signalStop = 90;
  const int _signalMax = 180;
  int _currentSignal;
  
  IDBTCharacteristic _powerControlChar;
  IDBTCharacteristic _powerFeedbackChar;
  IDBTCharacteristic _calibrationChar;
  static void updatePower(BLEDevice device, BLECharacteristic characteristic);
  static void updateCalibration(BLEDevice device, BLECharacteristic characteristic);

  Servo _motor;

  void update();
};
