#pragma once

#include <array>

struct RFID {
  using Encoded = std::array<uint8_t, 4 + 4 + 1 + 10>;

  RFID(uint8_t number)
  : _number(number)
  , _timestamp(0)
  , _length(0)
  , _uuid({0})
  {
  }

  const uint8_t _number;
  uint32_t _timestamp;
  uint8_t _length;
  std::array<uint8_t, 10> _uuid;

  Encoded encode() const
  {
    Encoded encoded;
    const uint32_t number = _number;
    std::copy(
      reinterpret_cast<const uint8_t*>(&number),
      reinterpret_cast<const uint8_t*>(&number) + sizeof(number),
      encoded.begin()
    );
    const uint32_t ts = _timestamp;
    std::copy(
      reinterpret_cast<const uint8_t*>(&ts),
      reinterpret_cast<const uint8_t*>(&ts) + sizeof(ts),
      encoded.begin() + 4
    );
    encoded[8] = _length;
    std::copy(_uuid.begin(), _uuid.begin() + _length, encoded.begin() + 9);
    return encoded;
  }

  size_t encodedSize() const { return 4 + 4 + 1 + _length; }

  void print() const
  {
    Serial.print(_number);
    Serial.print("-");
    Serial.print(_timestamp);
    Serial.print("-");
    Serial.print(_length);
    if (_length) {
      Serial.print("-");
      for (uint8_t i = 0; i < _length; i++) {
        if (_uuid[i] < 0x10) Serial.print('0');
        Serial.print(_uuid[i], HEX);
        if (i < _length-1) Serial.print('.');
      }
	}
  }

  static void print(const Encoded& encoded)
  {
    for (size_t i = 0; i < size_t(4 + 4 + 1 + encoded[8]); i++) {
      if (encoded[i] < 0x10) Serial.print('0');
      Serial.print(encoded[i], HEX);
      if (i == 3 || i == 7 || i == 8) Serial.print('-');
    }
  }
};
