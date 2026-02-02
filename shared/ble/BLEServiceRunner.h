#pragma once

#include <ArduinoBLE.h>
#include "BLEUUID.h"
#include "../PinIO/TaskThunk.h"

class BLEServiceRunner
{
public:
  BLEServiceRunner(Scheduler& scheduler, const std::string& serviceName, int pollMS = 100, const std::string& overrideId = "")
  : _name(serviceName)
  , _serviceId(makeUuidWithService(serviceName, overrideId))
  , _bleService(_serviceId.data())
  , _bluetoothTask(pollMS, TASK_FOREVER, &loop, &scheduler, true)
  {
  }

  const BLEUUID& serviceId() const { return _serviceId; }

  void addCharacteristic(BLECharacteristic& ble)
  {
    _bleService.addCharacteristic(ble);
  }

  void begin()
  {
    if (!BLE.begin())
    {
      Serial.println("Starting Bluetooth® Low Energy module failed!");
      while (1);
    }
    BLE.setLocalName(_name.c_str());
    BLE.setEventHandler(BLEConnected, bluetooth_connected);
    BLE.setEventHandler(BLEDisconnected, bluetooth_disconnected);

    BLE.setAdvertisedService(_bleService);
    BLE.addService(_bleService);

    int r = BLE.advertise();
    if (r == 1)
    {
      Serial.println("Bluetooth® device active.");
      Serial.print(_name.c_str());
      Serial.print(": ");
      Serial.println(_serviceId.data());
    }
    else
    {
      Serial.println("Bluetooth® activation failed!");
      while (1);
    }
  }

private:
  const std::string _name;
  const BLEUUID _serviceId;
  BLEService _bleService;
  Task _bluetoothTask;

  static void loop()
  {
    BLE.poll();
  }

  static void bluetooth_connected(BLEDevice device)
  {
    Serial.println();
    Serial.print("BT Connected: ");
    Serial.println(device.address());
  }

  static void bluetooth_disconnected(BLEDevice device)
  {
    Serial.println();
    Serial.print("BT Disconnected: ");
    Serial.println(device.address());
  }
};
