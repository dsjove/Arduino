#pragma once

#include <Arduino_LED_Matrix.h>
#include "../ble/IDBTCharacteristic.h"

#include "MatrixR4Value.h"

/*
Hardware:
Arduino UNO R4 WiFi
 */
class MatrixR4Display
{
public:
  using Value = MatrixR4Value;

  MatrixR4Display(BLEServiceRunner& ble, bool flipY = false, bool flipX = false, bool invert = false)
  : _current()
  , _flipY(flipY)
  , _flipX(flipX)
  , _invert(invert)
  , _displayChar(ble, "07020000", _current.size(), _current.data(), bleUpdate)
  {
    matrixRefR4 = this;
  }

  void begin()
  {
    _matrix.begin();
    _matrix.loadFrame(_current.data());
  }

  void update(const MatrixR4Value::Value& value, bool writeBLE = true)
  {
    if (_current.update(value, _flipY, _flipX, _invert))
    {
      if (writeBLE)
      {
        _displayChar.ble.writeValue(_current.data(), _current.size());
	  }
      loadFrame();
    }
}

private:
  Value _current;
  const bool _flipY;
  const bool _flipX;
  const bool _invert;
  
  inline static MatrixR4Display* matrixRefR4 = NULL;

  IDBTCharacteristic _displayChar;
  static void bleUpdate(BLEDevice, BLECharacteristic characteristic)
  {
    //Serial.println(characteristic.uuid());
    MatrixR4Value::Value value;
    characteristic.readValue(value.data(), sizeof(value));
    matrixRefR4->update(value, false);
  }

  ArduinoLEDMatrix _matrix;

  void loadFrame()
  {
    _matrix.loadFrame(_current.data());
  }
};
