#pragma once

#include <Arduino.h>

/*
Hardware:
Dorhea Digital 38khz Ir Receiver Sensor Module + 4Pcs 38khz Ir Transmitter Sensor Module Kit
 */

class LegoPFIR {
public:
  enum class Port : uint8_t { A = 0, B = 1 };

  enum class Mode : uint8_t {
// Combo PWM packet (needs periodic refresh; receiver times out on signal loss)
    ComboSpurt = 0,
// Single Output PWM (latches speed like LEGO 8879 train dial)
    SingleLatched = 1
  };

  // Value is raw PF PWM nibble 0..15:
  // 0 = float
  // 1..7 = fwd1..fwd7
  // 8 = brake
  // 9..15 = rev7..rev1
  struct Command {
    uint8_t channel; // 1..4
    Port port;
    uint8_t value; // 0..15
    Mode mode;

    bool operator==(const Command& other) const {
      return channel == other.channel
        && port == other.port
        && value == other.value
        && mode == other.mode;
    }

    bool operator!=(const Command& other) const {
      return !(*this == other);
    }
  };

  explicit LegoPFIR(uint8_t irPin);

  void begin();

  // Apply one {channel,port,value,mode} command:
  // - updates cached state (A/B value + per-channel mode)
  // - transmits according to mode
  bool apply(const Command& cmd, uint8_t repeats = 3, uint16_t repeatDelayMs = 30);

  // Transmit current cached values for a channel using that channel's cached mode.
  // (Useful as keep-alive for ComboSpurt; harmless for SingleLatched.)
  bool sendChannel(uint8_t channel, uint8_t repeats = 1, uint16_t repeatDelayMs = 0);

  // Refresh all channels once (recommended periodically for ComboSpurt channels)
  void refreshAll(uint16_t interChannelDelayMs = 5);

  // Read cached values (no transmit)
  uint8_t cachedA(uint8_t channel) const;
  uint8_t cachedB(uint8_t channel) const;
  Mode cachedMode(uint8_t channel) const;

  // Set cache without sending (then call sendChannel/refreshAll when ready)
  bool setCached(const Command& cmd);

private:
  // ======== LEGO PF IR timing (38kHz carrier) ========
  static constexpr uint16_t kHalfPeriodUs = 13;          // ~38kHz half period
  static constexpr uint16_t kMarkCycles = 6;             // mark = 6 cycles
  static constexpr uint16_t kPause0Cycles = 10;
  static constexpr uint16_t kPause1Cycles = 21;
  static constexpr uint16_t kPauseStartStopCycles = 39;

  const uint8_t _irPin;

  uint8_t _a[4];     // cached values per channel, Output A
  uint8_t _b[4];     // cached values per channel, Output B
  Mode    _mode[4];  // cached mode per channel
  uint8_t _toggle[4];// toggle bit for Single Output mode, per channel (0/1)

  static inline void delayCycles(uint16_t cycles);

  void sendMark6Cycles();
  void sendSymbolWithPause(uint16_t pauseCycles);
  void sendStartOrStop();
  void sendBit(bool one);
  void sendNibbleMSB(uint8_t n);

  // Combo PWM packet:
  // N1 = [a 1 C C]
  // N2 = BBBB
  // N3 = AAAA
  // L  = 0xF xor N1 xor N2 xor N3
  void sendComboSpurt(uint8_t channel1to4, uint8_t outB, uint8_t outA);

  // Single Output PWM packet:
  // N1 = [T 0 C C]
  // N2 = [a 1 M O]  (M=0 for PWM, O=0 A / 1 B, a=0)
  // N3 = [DDDD]     (PWM nibble 0..15)
  // L  = 0xF xor N1 xor N2 xor N3
  void sendSingleLatched(uint8_t channel1to4, Port port, uint8_t pwmNibble);
};
