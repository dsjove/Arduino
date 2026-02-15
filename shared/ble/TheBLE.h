#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>

#include "../PinIO/SBJTask.h"
#include "BLEUUID.h"

// Select BLE backend
#if __has_include(<ArduinoBLE.h>)
  #define SBJ_BLE_USE_ARDUINOBLE 1
  #include <ArduinoBLE.h>
#elif __has_include(<NimBLEDevice.h>)
  #define SBJ_BLE_USE_ARDUINOBLE 0
  #include <NimBLEDevice.h>
#else
  #error "No supported BLE backend found. Install ArduinoBLE (UNO R4 etc.) or NimBLE-Arduino (ESP32)."
#endif

class TheBLE
{
public:
  inline TheBLE(
    Scheduler& sched,
    const std::string& name)
  : _serviceName(name)
  , _serviceId(makeUuidWithService(serviceName))
#if SBJ_BLE_USE_ARDUINOBLE
#else
  , _task("BLE", &poll, Schedule{100})
#endif
  {
  }

  void addCharacteristic(BLECharacteristic& ble)
  {
    _bleService.addCharacteristic(ble);
  }

  void begin()
  {
#if SBJ_BLE_USE_ARDUINOBLE
#else
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
#endif
  }

private:
  std::string _serviceName;
  BLEUUID _serviceId;
  SBJTask _task;
  
#if SBJ_BLE_USE_ARDUINOBLE
#else
  BLEService _bleService;
  static void poll()
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
#endif
};
