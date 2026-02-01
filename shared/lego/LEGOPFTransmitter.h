#pragma once

#include "../core/LegoPFIR.h"
#include "../ble/IDBTCharacteristic.h"

class LEGOPFTransmitter : ScheduledRunner {
public:
  LEGOPFTransmitter(Scheduler& scheduler, BLEServiceRunner& ble, int pin);

  void begin();

private:
  LegoPFIR _ir;
  LegoPFIR::Command _value;
  IDBTCharacteristic _transmitChar;
  TaskThunk _task;

  static void transmit(BLEDevice device, BLECharacteristic characteristic);
  virtual void loop(Task&);
};
