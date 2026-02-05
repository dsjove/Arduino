#pragma once

#include "../PinIO/TaskThunk.h"
#include <Arduino_LED_Matrix.h>
#include "../ble/IDBTCharacteristic.h"

#include "MatrixR4Value.h"

/*
Hardware:
Arduino UNO R4 WiFi
*/
class MatrixR4Display : ScheduledRunner
{
public:
  using Value = MatrixR4Value;

  MatrixR4Display(Scheduler& scheduler, BLEServiceRunner& ble,
                  bool flipY = false, bool flipX = false, bool invert = false)
  : _flipY(flipY)
  , _flipX(flipX)
  , _invert(invert)
  , _animate(true)
  , _current(invert)
  , _showing()
  , _displayChar(ble, "07020000", _current.size(), _current.data(), bleUpdate)
  , _animationTask(scheduler, 100, this, false)
  {
    matrixRefR4 = this;
  }

  void begin()
  {
    _matrix.begin();
    _showing = _current;
    _matrix.loadFrame(_showing.data());
  }

  void update(const MatrixR4Value::Value& value, bool writeBLE = true)
  {
      if (_current.update(value, _flipY, _flipX, _invert))
      {
        if (writeBLE)
        {
           _displayChar.ble.writeValue(_current.data(), _current.size());
	    }
        startAnimation();
    }
  }

private:
  const bool _flipY;
  const bool _flipX;
  const bool _invert;
  const bool _animate;
  Value _current;
  Value _showing;
  IDBTCharacteristic _displayChar;
  TaskThunk _animationTask;

  ArduinoLEDMatrix _matrix;
  MatrixR4Value::Frames _frames{};
  int  _frameCount = 0;
  int  _frameIndex = 0;
  bool _animating  = false;

  inline static MatrixR4Display* matrixRefR4 = nullptr;
  static void bleUpdate(BLEDevice, BLECharacteristic characteristic)
  {
    MatrixR4Value::Value value;
    characteristic.readValue(value.data(), sizeof(value));
    matrixRefR4->update(value, false);
  }

  void startAnimation()
  {
    _frameCount = 0;
    _frameIndex = 0;
    if (_animate) _showing.animateTo(_current, _frames, _frameCount);

    if (_frameCount == 0)
    {
      _showing = _current;
      _animating = false;
      _animationTask.disable();
      _matrix.loadFrame(_showing.data());
    }
    else
    {
      _animating = true;
      _animationTask.enable();
	}
  }

  virtual void loop(Task&) override
  {
    if (!_animating) return;
    if (_frameIndex < _frameCount)
    {
      _matrix.loadFrame(_frames[_frameIndex].data());
      _showing = _frames[_frameIndex];
      ++_frameIndex;
    }
    if (_frameIndex >= _frameCount)
    {
      _showing = _current;
      _animating = false;
      _animationTask.disable();
    }
  }
};
