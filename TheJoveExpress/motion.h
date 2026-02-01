#pragma once

#include <Arduino.h>
#include <TaskScheduler.h>
#include <Wire.h>

#include <Adafruit_LSM6DS3TRC.h>

#include "src/PinIO/I2CHardware.h"

//LSM6DS3TRC
namespace motion
{
  inline Adafruit_LSM6DS3TRC device;

  // Cached latest sample (setup-only storage; interpretation is business logic)
  inline sensors_event_t accel{};
  inline sensors_event_t gyro{};
  inline sensors_event_t temp{};

  inline void _tick()
  {
    // Reads current accel/gyro/temp into our cached structs
    device.getEvent(&accel, &gyro, &temp);
  }

  // Polling task (e.g. 20ms = 50Hz). You can change later.
  inline Task task(20, TASK_FOREVER, &_tick);

  inline void begin(Scheduler& sched)
  {
    I2CHardware::begin();

    if (!device.begin_I2C())
    {
      Serial.println("[motion] LSM6DS3TR-C begin_I2C failed");
      return;
    }

    // Setup-ish defaults (optional)
    device.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
    device.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
    device.setAccelDataRate(LSM6DS_RATE_104_HZ);
    device.setGyroDataRate(LSM6DS_RATE_104_HZ);

    // Start periodic sampling
    sched.addTask(task);
    task.enable();
  }
}
