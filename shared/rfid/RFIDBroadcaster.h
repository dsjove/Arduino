#pragma once

#include "../PinIO/TaskThunk.h"
#include "../ble/IDBTCharacteristic.h"
#include "RFIDDetector.h"

struct RFIDBroadcasterTraitsDft: RFIDDetectorTraitsDft
{
  static constexpr const char* bleProperty = "01000002";
  static constexpr uint32_t loopFrequencyMs = 20;
};

template<typename Traits = RFIDBroadcasterTraitsDft>
class RFIDBroadcaster : ScheduledRunner
{
public:
  using Detector = RFIDDetector<Traits>;
  using Callback = void (*)(const RFID&);

  RFIDBroadcaster(Scheduler& scheduler, BLEServiceRunner& ble, Callback callback = nullptr)
  : _rfid()
  , _callback(callback)
  , _idFeedbackChar(ble, writeIndex(Traits::bleProperty, Traits::Number), _rfid.lastID().encode())
  , _rfidTask(scheduler, Traits::loopFrequencyMs, this)
  {
  }

  void begin()
  {
    _rfid.begin();
    Serial.print("RFID: ");
    _rfid.lastID().print();
    Serial.println();
  }

private:
  Detector _rfid;
  Callback _callback;
  IDBTCharacteristic _idFeedbackChar;
  TaskThunk _rfidTask;

  virtual void loop(Task&)
  {
    const RFID* detected = _rfid.loop();
    if (detected)
    {
      auto encoded = detected->encode();
      Serial.print("RFID: ");
      detected->print();
      //Serial.print(" -- ");
      //RFID::print(encoded);
      Serial.println();
      //Serial.println(_idFeedbackChar.uuid.data());
      if (_callback) _callback(*detected);
      _idFeedbackChar.ble.writeValue(encoded.data(), detected->encodedSize());
    }
  }
};
