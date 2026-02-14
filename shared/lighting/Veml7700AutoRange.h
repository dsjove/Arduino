#pragma once

#include <Wire.h>
#include <Adafruit_VEML7700.h>

#include "../PinIO/I2CHardware.h"

class Veml7700AutoRange {
public:
  struct Reading {
    float lux = 0.0f;
    uint16_t raw = 0;
    uint8_t gainIdx = 0;
    uint8_t itIdx = 0;
    bool saturated = false;
  };

  // Start sensor and initialize a sensible default (good indoor baseline).
  bool begin() {
    I2CHardware::begin();
    _veml.begin();
    // Reasonable middle setting
    _gainIdx = 1; // GAIN_1
    _itIdx   = 2; // IT_100MS
    apply();
    return true;
  }

  // Read lux with auto-ranging.
  // - If ALS is saturating, reduce sensitivity (shorter IT, then lower gain).
  // - If ALS is very low, increase sensitivity (higher gain, then longer IT).
  Reading read() {
    uint16_t raw = _veml.readALS();
    bool sat = (raw >= kSatHighRaw);

    if (sat) {
      for (uint8_t i = 0; i < 6; ++i) { // bounded
        if (!makeLessSensitive()) break;
        raw = _veml.readALS();
        sat = (raw >= kSatHighRaw);
        if (!sat) break;
      }
    } else if (raw <= kTooLowRaw) {
      for (uint8_t i = 0; i < 6; ++i) { // bounded
        if (!makeMoreSensitive()) break;
        raw = _veml.readALS();
        if (raw > kTooLowRaw) break;
      }
    }

    Reading r;
    r.raw = raw;
    r.lux = _veml.readLux();
    r.gainIdx = _gainIdx;
    r.itIdx = _itIdx;
    r.saturated = (raw >= kSatHighRaw);
    return r;
  }

  void getSettings(uint8_t& gainIdx, uint8_t& itIdx) const {
    gainIdx = _gainIdx;
    itIdx   = _itIdx;
  }

  static const char* gainName(uint8_t idx) {
    switch (idx) {
      case 0: return "GAIN_2";
      case 1: return "GAIN_1";
      case 2: return "GAIN_1_4";
      case 3: return "GAIN_1_8";
      default: return "?";
    }
  }

  static const char* itName(uint8_t idx) {
    switch (idx) {
      case 0: return "IT_25MS";
      case 1: return "IT_50MS";
      case 2: return "IT_100MS";
      case 3: return "IT_200MS";
      case 4: return "IT_400MS";
      case 5: return "IT_800MS";
      default: return "?";
    }
  }

private:
  // Raw thresholds (tune as desired)
  static constexpr uint16_t kSatHighRaw = 60000;
  static constexpr uint16_t kTooLowRaw  = 200;

  // Use counts + mapping functions instead of constexpr arrays.
  // This avoids enum typedef differences and older Arduino C++ quirks.
  static constexpr uint8_t kGainCount = 4;
  static constexpr uint8_t kItCount   = 6;

  static uint8_t gainForIdx(uint8_t idx) {
    switch (idx) {
      case 0: return VEML7700_GAIN_2;
      case 1: return VEML7700_GAIN_1;
      case 2: return VEML7700_GAIN_1_4;
      default:return VEML7700_GAIN_1_8;
    }
  }

  static uint8_t itForIdx(uint8_t idx) {
    switch (idx) {
      case 0: return VEML7700_IT_25MS;
      case 1: return VEML7700_IT_50MS;
      case 2: return VEML7700_IT_100MS;
      case 3: return VEML7700_IT_200MS;
      case 4: return VEML7700_IT_400MS;
      default:return VEML7700_IT_800MS;
    }
  }

  bool makeLessSensitive() {
    // First shorten integration time, then reduce gain (toward 1/8).
    if (_itIdx > 0) {
      _itIdx--;
      apply();
      return true;
    }
    if ((_gainIdx + 1) < kGainCount) {
      _gainIdx++;
      apply();
      return true;
    }
    return false; // already minimum sensitivity
  }

  bool makeMoreSensitive() {
    // First increase gain (toward 2), then lengthen integration time.
    if (_gainIdx > 0) {
      _gainIdx--;
      apply();
      return true;
    }
    if ((_itIdx + 1) < kItCount) {
      _itIdx++;
      apply();
      return true;
    }
    return false; // already maximum sensitivity
  }

  void apply() {
    _veml.setGain(gainForIdx(_gainIdx));
    _veml.setIntegrationTime(itForIdx(_itIdx));
    delay(5); // small settle after changing config
  }

  Adafruit_VEML7700 _veml;

  uint8_t _gainIdx = 1; // GAIN_1
  uint8_t _itIdx   = 2; // IT_100MS
};
