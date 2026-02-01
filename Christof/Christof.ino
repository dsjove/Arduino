#include <Arduino.h>

// Camera
// SDCard
// LEGO PF
// Lighting sensor and activation

//#include "shared/core/LegoPFIR.cpp"
//#include "shared/adapters/Lighting.cpp"

//Lighting _lighting(_runner, _ble, {{3, true}, {0, false}}, A0);
//LEGOPFTransmitter _pfTransmitter(_runner, _ble, 7);

void setup()
{
  Serial.begin(115200);
  //_lighting.begin();
  //_pfTransmitter.begin();
}

void loop()
{
  //_runner.execute();
}
