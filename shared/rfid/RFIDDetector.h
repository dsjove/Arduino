#pragma once

#include "RFID.h"
#include "../PinIO/PinIO.h"

#include <MFRC522.h>

/*
Hardware:
HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card + Key Ring
Timeskey NFC 20 Pack Mifare Classic 1k NFC Tag RFID Sticker 13.56mhz - ISO14443A Smart 25mm Adhesive Tags
 */

struct RFIDDetectorTraitsDft {
  static constexpr uint8_t Number = 0;
  static constexpr uint8_t SsPin  = 10;
  using RstPin = PinIO<9, GpioMode::DigitalOut>;

  static constexpr uint32_t cooldownMs      = 800;    // Tune for tag movement speed
  static constexpr uint32_t reinitAfterMs   = 30000;  // MFRC522 goes bad after a while
  static constexpr uint8_t  failResetCount  = 5;      // Reset after repeated failures
};

template <typename Traits = RFIDDetectorTraitsDft>
class RFIDDetector {
public:
  static constexpr uint8_t number  = Traits::Number;
  static constexpr uint8_t ss_pin  = Traits::SsPin;

  RFIDDetector()
  : _rfid(Traits::SsPin, Traits::RstPin::pin)
  , _lastID(Traits::Number)
  , _cooldownLimitMs(0)
  , _lastGoodReadMs(0)
  , _failReadCount(0)
  {
  }

  const RFID& lastID() const { return _lastID; }

  void begin()
  {
    Traits::RstPin::begin(GpioLevel::High);
    _rfid.PCD_Init();
  }

  const RFID* loop()
  {
    const uint32_t now = millis();
    // Re-init after long inactivity without a successful read
    if (_lastGoodReadMs != 0 && (now - _lastGoodReadMs) > Traits::reinitAfterMs)
    {
      resetRc522();
      _lastGoodReadMs = now;
    }
    if (_rfid.PICC_IsNewCardPresent())
    {
      // Cooldown gate
      if (now < _cooldownLimitMs)
      {
        return nullptr;
      }
      if (_rfid.PICC_ReadCardSerial())
      {
        _lastGoodReadMs = now;
        _failReadCount = 0;

        update(_rfid.uid, now);

        _rfid.PICC_HaltA();
        _rfid.PCD_StopCrypto1();

        _cooldownLimitMs = now + Traits::cooldownMs;
        return &_lastID;
      }
      else
      {
        Serial.println("RFID: Read Failed");
        _rfid.PCD_StopCrypto1();
        _rfid.PICC_HaltA();
        _failReadCount++;
        if (_failReadCount >= Traits::failResetCount)
        {
          Serial.println("RFID: Fail Count Reset");
          resetRc522();
        }
      }
    }
    return nullptr;
  }

private:
  MFRC522 _rfid;
  RFID _lastID;
  uint32_t _cooldownLimitMs;
  uint32_t _lastGoodReadMs;
  uint8_t  _failReadCount;

  void update(const MFRC522::Uid& u, uint32_t timestamp)
  {
    _lastID._timestamp = timestamp;
    const uint8_t len = (u.size > 10) ? 10 : u.size;
    _lastID._length = len;
    std::copy(u.uidByte, u.uidByte + len, _lastID._uuid.begin());
  }

  void resetRc522()
  {
    Traits::RstPin::write(GpioLevel::Low);
    delay(5);
    Traits::RstPin::write(GpioLevel::High);
    delay(5);
    _rfid.PCD_Init();
    _rfid.PCD_SetAntennaGain(_rfid.RxGain_max);
    _failReadCount = 0;
    // Do not reset _lastGoodReadMs
  }
};
