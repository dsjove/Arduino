#include "Lighting.h"

static Lighting* lightingRef = NULL;

Lighting::Lighting(Scheduler& scheduler, BLEServiceRunner& ble, std::vector<LightOutput> output, int sensor)
: _output(output)
, _sensor(sensor)
, _currentPower(0)
, _currentCalibration(255)
, _currentAmbient(0)
, _currentSignal(0)
, _powerControlChar(ble, "03020001", (uint8_t*)NULL, updatePower)
, _powerFeedbackChar(ble, "03020002", &_currentPower)
, _calibrationChar(ble, "03010000", &_currentCalibration, updateCalibration)
, _sensedFeedbackChar(ble, "03040002", &_currentAmbient, sensor != -1 ? updateSensed : NULL)
, _lightingTask(scheduler, 1000, this, sensor != -1)
{
  lightingRef = this;
}

void Lighting::begin()
{
  for (LightOutput light : _output)
  {  
    pinMode(light.pin, OUTPUT);
  }
}

void Lighting::updatePower(BLEDevice, BLECharacteristic characteristic)
{
  //Serial.println(characteristic.uuid());
  characteristic.readValue(lightingRef->_currentPower);
  lightingRef->update();
}

void Lighting::updateCalibration(BLEDevice, BLECharacteristic characteristic)
{
  //Serial.println(characteristic.uuid());
  characteristic.readValue(lightingRef->_currentCalibration);
  lightingRef->update();
}

void Lighting::loop(Task&)
{
  int sensorValue = analogRead(_sensor);
  uint8_t signal = map(sensorValue, 920, 1014, 0, 255);
  if (signal != _currentAmbient)
  {
    _currentAmbient = signal;
    //Serial.println(_sensedFeedbackChar.uuid.data());
    _sensedFeedbackChar.ble.writeValue(_currentAmbient);
    update();
  }
}

void Lighting::updateSensed(BLEDevice, BLECharacteristic characteristic) {
  //Serial.println(characteristic.uuid());
  characteristic.readValue(lightingRef->_currentAmbient);
  lightingRef->update();
}

void Lighting::update() {
  uint8_t signal;
  if (_currentCalibration == 0 || _currentCalibration < _currentAmbient)
  {
      signal = 0;
  }
  else
  {
      signal = _currentPower;
  }

  if (signal != _currentSignal)
  {
    //Serial.println(_powerFeedbackChar.uuid.data());
    _powerFeedbackChar.ble.writeValue(signal);
    _currentSignal = signal;

  for (LightOutput light : _output)
  {
      if (light.dimmable)
      {
        analogWrite(light.pin, signal);
      }
      else
      {
        digitalWrite(light.pin, signal);
      }
    }
  }
}
