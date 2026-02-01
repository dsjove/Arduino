#pragma once

#include "BLEUUID.h"
#include "BLEServiceRunner.h"
#include <string>

static inline std::string writeIndex(const char * src, uint8_t value)
{
  static const char hex[16] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
  };
  std::string buf = src;
  if (buf.size() < 8) buf.resize(8, '0');
  buf[4] = hex[(value >> 4) & 0x0F];
  buf[5] = hex[value & 0x0F];
  return buf;
}


static inline BLEUUID makeUuidWithProperty(
    const std::string& propertyId,
    const BLEUUID& serviceId)
{
  BLEUUID out {};
  std::copy_n(serviceId.begin(), 36, out.begin());
  std::copy_n(propertyId.begin(), 8, out.begin());
  out[36] = '\0';
  return out;
}

struct IDBTCharacteristic {
  const BLEUUID uuid; // Must remain in memory with characteristic
  BLECharacteristic ble;

  IDBTCharacteristic(
    BLEServiceRunner& runner,
    const std::string& propertyId, // hex of 4 byte id
    size_t valueSize, // store value of this size, ~180–244 bytes per characteristic write/notify
    const void* value, // initial value of valueSize
    BLECharacteristicEventHandler eventHandler)
  : uuid(makeUuidWithProperty(propertyId, runner.serviceId()))
  , ble(uuid.data(), adjustPermissions(0, value, eventHandler), valueSize)
  {
    if (eventHandler)
    {
      ble.setEventHandler(BLEWritten, eventHandler);
    }
    if (value)
    {
      ble.writeValue(static_cast<const unsigned char*>(value), valueSize);
    }
    runner.addCharacteristic(ble);
  }

  template <typename T>
  IDBTCharacteristic(
    BLEServiceRunner& runner,
    const std::string& propertyId,
    const T* value,
    BLECharacteristicEventHandler eventHandler = NULL)
    : IDBTCharacteristic(runner, propertyId, sizeof(T), value, eventHandler) {}

  template <typename T, std::size_t N>
  IDBTCharacteristic(
    BLEServiceRunner& runner,
    const std::string& propertyId,
    const std::array<T, N>& value,
    BLECharacteristicEventHandler eventHandler = NULL)
    : IDBTCharacteristic(runner, propertyId, sizeof(value), value.data(), eventHandler) {}

	//TODO: use the UUID on eventHandler to route to a lambda extression
	//Create a new static class for this so we don't multiple these constructors

private:
  static unsigned char adjustPermissions(
      unsigned char base,
      const void* value,
      BLECharacteristicEventHandler eventHandler)
  {
    unsigned char p = base;
    if (value)
    {
      p |= BLERead;
      p |= BLENotify;
    }
    if (eventHandler)
    {
      p |= BLEWriteWithoutResponse;
    }
    return p;
  }
};

