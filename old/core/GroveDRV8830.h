#pragma once

#include <Arduino.h>
#include <Wire.h>

class GroveDRV8830 {
public:
  enum Channel : uint8_t { CH1 = 0, CH2 = 1 };
  enum Direction : uint8_t { FORWARD, BACKWARD };

  // Default 7-bit I2C addresses for the Grove board:
  // CH1: 0x60 (0xC0 >> 1), CH2: 0x62 (0xC4 >> 1)
  explicit GroveDRV8830(uint8_t addrCh1 = 0x60, uint8_t addrCh2 = 0x62)
  : _addr{addrCh1, addrCh2} {}

  void begin() {
    Wire.begin();
    coast(CH1);
    coast(CH2);
  }

  // Speed: 0..63 (recommended usable range: 6..63; 0 => coast)
  void forward(Channel ch, uint8_t speed)  { drive(ch, FORWARD,  speed); }
  void backward(Channel ch, uint8_t speed) { drive(ch, BACKWARD, speed); }

  // High-Z / standby (freewheel)
  void coast(Channel ch) { writeControl(_addr[ch], 0 /*vset*/, 0b00 /*IN1 IN2*/); }

  // Full stop = active brake
  void stop(Channel ch)  { writeControl(_addr[ch], 0 /*vset*/, 0b11 /*brake*/); }

  // Optional: read fault register (0x01). Bits indicate OCP/thermal/UVLO, etc.
  uint8_t fault(Channel ch) { return readReg(_addr[ch], 0x01); }

private:
  uint8_t _addr[2];

  void drive(Channel ch, Direction dir, uint8_t speed) {
    if (speed == 0) { coast(ch); return; }
    if (speed < 6)  speed = 6;
    if (speed > 63) speed = 63;

    // IN1 IN2: 10=forward, 01=reverse, 00=coast, 11=brake
    const uint8_t inBits = (dir == FORWARD) ? 0b10 : 0b01;
    writeControl(_addr[ch], speed, inBits);
  }

  void writeControl(uint8_t addr7, uint8_t vset, uint8_t inBits) {
    // CONTROL reg (0x00): [D7..D2]=VSET, [D1..D0]=IN1/IN2
    const uint8_t value = (uint8_t)((vset << 2) | (inBits & 0x03));
    Wire.beginTransmission(addr7);
    Wire.write((uint8_t)0x00);
    Wire.write(value);
    Wire.endTransmission();
  }

  uint8_t readReg(uint8_t addr7, uint8_t reg) {
    Wire.beginTransmission(addr7);
    Wire.write(reg);
    Wire.endTransmission(false);        // repeated start
    Wire.requestFrom(addr7, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
  }
};
