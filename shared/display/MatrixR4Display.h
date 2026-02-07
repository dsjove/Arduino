#pragma once

#include "../PinIO/TaskThunk.h"
#include <Arduino_LED_Matrix.h>
#include "../ble/IDBTCharacteristic.h"

#include "MatrixR4Value.h"

/*
Hardware:
Arduino UNO R4 WiFi
*/
struct MatrixR4DTraitsDft
{
	constexpr static const char* bleProperty = "07020000";
	constexpr static bool flipY = false;
	constexpr static bool flipX = false;
	constexpr static bool invert = false;
	constexpr static int animateMS = 100;
};

template <typename Traits = MatrixR4DTraitsDft>
class MatrixR4Display : ScheduledRunner
{
public:
  using Value = MatrixR4Value;
  using Callback = void (*)(const Value::Value&);

  MatrixR4Display(Scheduler& scheduler, BLEServiceRunner& ble, Callback callback = nullptr)
  : _current(Traits::invert)
  , _showing()
  , _callback(callback)
  , _displayChar(ble, Traits::bleProperty, _current.size(), _current.data(), bleUpdate)
  , _animationTask(scheduler, Traits::animateMS, this, false)
  {
    matrixRefR4 = this;
  }

  void begin()
  {
    _matrix.begin();
    _showing = _current;
    _matrix.loadFrame(_showing.data());
  }

  void update(const MatrixR4Value::Value& value, bool fromBLE = false)
  {
    if (_current.update(value, Traits::flipY, Traits::flipX, Traits::invert))
    {
      if (fromBLE)
      {
        _callback(value);
      }
      else
      {
        _displayChar.ble.writeValue(value.data(), _current.size());
      }
      startAnimation();
    }
  }

private:
  Value _current;
  Value _showing;
  Callback _callback;
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
    matrixRefR4->update(value, true);
  }

  void startAnimation()
  {
    _frameCount = 0;
    _frameIndex = 0;
    if (Traits::animateMS > 0) _showing.animateTo(_current, _frames, _frameCount);

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
