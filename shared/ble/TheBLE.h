#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>

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

#if SBJ_BLE_USE_ARDUINOBLE
#define _TASK_LTS_POINTER
#include <TaskScheduler.h>
#else
class Scheduler;
#endif

class TheBLE
{
public:
  using BLEUUID = std::array<char, 37>;

  struct Characteristic {
    const BLEUUID id;
    const size_t valueSize;
    const void* value;
    int (*handler)(const char *arg);
  };

  inline TheBLE(
    Scheduler& sched,
    const std::string& name)
  : _serviceName(name)
  , _id({})
  {
  }

  void begin()
  {
  }

private:
	std::string _serviceName;
	BLEUUID _id;
};
