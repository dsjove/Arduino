//
//  Untitled.h
//  Infrastructure
//
//  Created by David Giovannini on 1/19/26.
//

#pragma once

#include <Arduino.h>

class L298NChannel {
public:
  // enaPin should be a PWM-capable pin if you want speed control via analogWrite.
  L298NChannel(uint8_t enaPin, uint8_t in1Pin, uint8_t in2Pin, bool invert = false)
  : _ena(enaPin), _in1(in1Pin), _in2(in2Pin), _invert(invert) {}

  void begin() {
    pinMode(_ena, OUTPUT);
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    coast();
  }

  // speed: 0..255 (0 => coast)
  void forward(uint8_t speed = 255)  { drive(true,  speed); }
  void backward(uint8_t speed = 255) { drive(false, speed); }

  // Coast: disable bridge (outputs high-Z)
  void coast() {
    digitalWrite(_ena, LOW); // ENA LOW = coast
  }

  // Brake: enable bridge and short motor terminals (IN1==IN2)
  void brake() {
    digitalWrite(_ena, HIGH);
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, LOW); // (or both HIGH)
  }

  // “Full stop” can mean brake or coast; pick one:
  void stopBrake() { brake(); }
  void stopCoast() { coast(); }

private:
  uint8_t _ena, _in1, _in2;
  bool _invert;

  void drive(bool fwd, uint8_t speed) {
    if (speed == 0) { coast(); return; }

    digitalWrite(_ena, HIGH);      // keep enabled; use PWM below if desired
    const bool dir = _invert ? !fwd : fwd;

    digitalWrite(_in1, dir ? HIGH : LOW);
    digitalWrite(_in2, dir ? LOW  : HIGH);

    // If _ena is PWM-capable and timers allow it, this gives speed control.
    // If Servo.h steals that timer on your board, analogWrite may misbehave on some pins.
    analogWrite(_ena, speed);
  }
};
