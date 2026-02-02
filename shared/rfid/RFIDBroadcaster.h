#pragma once

#include "../PinIO/TaskThunk.h"
#include "../ble/IDBTCharacteristic.h"
#include "RFIDDetector.h"

struct RFIDBroadcasterDefaults: RFIDDetectorDefaultTraits
{
  static constexpr const char* propertyID = "01000002";
  static constexpr uint32_t loopFrequencyMs = 20;
};

template<typename Traits = RFIDBroadcasterDefaults>
class RFIDBroadcaster : ScheduledRunner
{
public:
  using Detector = RFIDDetector<Traits>;

  RFIDBroadcaster(Scheduler& scheduler, BLEServiceRunner& ble)
  : _rfid()
  , _idFeedbackChar(ble, writeIndex(Traits::propertyID, Traits::Number), _rfid.lastID().encode())
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
      Serial.print(" -- ");
      RFID::print(encoded);
      Serial.println();
      Serial.println(_idFeedbackChar.uuid.data());
      _idFeedbackChar.ble.writeValue(encoded.data(), detected->encodedSize());
    }
  }
};
