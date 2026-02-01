#include "ServoMotor.h"

static ServoMotor* servoMotorRef = NULL;

ServoMotor::ServoMotor(BLEServiceRunner& ble, int pin)
: _pin(pin)
, _currentPower(0)
, _currentCalibration(_powerMax / 4)
, _currentSignal(_signalStop)
, _powerControlChar(ble, "02020001", (uint8_t*)NULL, updatePower)
, _powerFeedbackChar(ble, "02020002", &_currentPower)
, _calibrationChar(ble, "02010000", &_currentCalibration, updateCalibration)
{
  servoMotorRef = this;
}

void ServoMotor::begin() 
{
  _motor.attach(_pin);
}

void ServoMotor::updatePower(BLEDevice, BLECharacteristic characteristic)
{
  //Serial.println(characteristic.uuid());
  ServoMotor* This = servoMotorRef;
  characteristic.readValue(This->_currentPower);
  This->update();
}

void ServoMotor::updateCalibration(BLEDevice, BLECharacteristic characteristic)
{
  //Serial.println(characteristic.uuid());
  ServoMotor* This = servoMotorRef;
  characteristic.readValue(This->_currentCalibration);
  This->update();
}

void ServoMotor::update()
{
  int8_t actualPower = (abs(_currentPower) < _currentCalibration) ? _powerStop : _currentPower;
  int newSignal = map(actualPower, _powerMin, _powerMax, _signalMin, _signalMax);

  if (newSignal != _currentSignal)
  {
    //Serial.println(_powerFeedbackChar.uuid.data());
    _powerFeedbackChar.ble.writeValue(actualPower);
    _currentSignal = newSignal;
    if (_currentSignal == _signalStop)
    {
      _motor.writeMicroseconds(1500);
    }
    else
    {
      _motor.write(_currentSignal);
    }
  }
}
