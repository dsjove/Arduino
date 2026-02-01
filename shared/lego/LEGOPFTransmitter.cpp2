#include "LEGOPFTransmitter.h"
#include "../core/mapEven.h"

static LEGOPFTransmitter* pfTranbsmitterRef = NULL;

LEGOPFTransmitter::LEGOPFTransmitter(Scheduler& scheduler, BLEServiceRunner& ble, int pin)
: _ir(pin)
, _value({0, LegoPFIR::Port::A, 0, LegoPFIR::Mode::ComboSpurt})
, _transmitChar(ble, "05020000", 4, NULL, transmit)
, _task(scheduler, 1000, this, false)
{
  pfTranbsmitterRef = this;
}

void LEGOPFTransmitter::begin()
{
  _ir.begin();
}

void LEGOPFTransmitter::loop(Task&) {
  _ir.refreshAll();
}

void LEGOPFTransmitter::transmit(BLEDevice, BLECharacteristic characteristic)
{
  std::array<uint8_t, 4> value;
  characteristic.readValue(value.data(), value.size());

  uint8_t channel = value[0];
  if (channel < 1 || channel > 4) return;
  auto port = (LegoPFIR::Port)value[1];
  int8_t inPower = value[2];
  //TODO: have a 3rd mode of combo w/ task enabled, lineOfSight
  auto mode = (LegoPFIR::Mode)value[3];

  uint8_t outPower = inPower == -128 ? 0 : mapEven(inPower, -127, 127, 1, 15);
  if (outPower > 8) outPower -= 8;
  else if (outPower < 8) outPower += 8;

  LegoPFIR::Command command = {
    channel,
    port,
    outPower,
    mode
  };
  if (pfTranbsmitterRef->_value != command) {
//	  Serial.print("Power: ");
//	  Serial.print(inPower);
//	  Serial.print(" -> ");
//	  Serial.println(outPower);
    pfTranbsmitterRef->_value = command;
    pfTranbsmitterRef->_ir.apply(command);
  }
}
