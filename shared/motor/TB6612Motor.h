#pragma once

#include "../pinio/PinIO.h"

#include <stdint.h>
#include <limits>

template <class Traits>
struct TB6612Motor
{
  enum class Channel : uint8_t { A, B };

  enum class StopMode : uint8_t
  {
    Coast,
    Brake
  };

  enum class Direction : uint8_t
  {
    Forward,
    Reverse
  };

  // ======== Begin / enable ========

  inline static void begin()
  {
    // PWM
    Traits::MotorPwma.begin();
    Traits::MotorPwmb.begin();

    // Control pins
    Traits::MotorAin1.begin();
    Traits::MotorAin2.begin();
    Traits::MotorStby.begin();
    Traits::MotorBin1.begin();
    Traits::MotorBin2.begin();

    // Known safe state: disabled, stopped
    setStby(false);
    applyStop(Channel::A, StopMode::Coast);
    applyStop(Channel::B, StopMode::Coast);
  }

  inline static void enable(bool enabled, StopMode stop = StopMode::Coast)
  {
    if (!enabled)
    {
      applyStop(Channel::A, stop);
      applyStop(Channel::B, stop);
      setStby(false);
      return;
    }
    setStby(true);
  }

  // ======== Single minimal command ========

  // Rules:
  // - value == 0        => Brake
  // - value >  0        => Forward, PWM = clamp(value, 0..255)
  // - value <  0        => Reverse, PWM = clamp(abs(value), 0..255)
  // - value == -256     => Coast
  inline static void set(Channel ch, int16_t value)
  {
    constexpr int16_t kCoastSentinel = -256;

    if (value == 0)
    {
      applyStop(ch, StopMode::Brake);
      return;
    }

    if (value == kCoastSentinel)
    {
      applyStop(ch, StopMode::Coast);
      return;
    }

    // Optional “auto-wake”: bring chip out of standby to execute the command.
    setStby(true);

    if (value > 0)
    {
      applyRun(ch, Direction::Forward, clampU8(static_cast<int32_t>(value)));
    }
    else
    {
      // Avoid abs() overflow at -32768 by promoting first
      const int32_t mag = -(static_cast<int32_t>(value));
      applyRun(ch, Direction::Reverse, clampU8(mag));
    }
  }

  inline static void emergencyStop(bool goStandby = true)
  {
    applyStop(Channel::A, StopMode::Brake);
    applyStop(Channel::B, StopMode::Brake);
    if (goStandby) setStby(false);
  }

private:
  // ======== Helpers ========

  inline static uint8_t clampU8(int32_t v)
  {
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
  }

  inline static void setStby(bool enabled)
  {
    Traits::MotorStby.write(enabled ? GpioLevel::High : GpioLevel::Low);
  }

  inline static void writePwmA(uint8_t pwm) { Traits::MotorPwma.write(pwm); }
  inline static void writePwmB(uint8_t pwm) { Traits::MotorPwmb.write(pwm); }

  // TB6612 truth table:
  // IN1 IN2: H L = fwd, L H = rev, L L = coast, H H = brake
  inline static void setInputsA(GpioLevel in1, GpioLevel in2)
  {
    Traits::MotorAin1.write(in1);
    Traits::MotorAin2.write(in2);
  }

  inline static void setInputsB(GpioLevel in1, GpioLevel in2)
  {
    Traits::MotorBin1.write(in1);
    Traits::MotorBin2.write(in2);
  }

  inline static void applyStop(Channel ch, StopMode mode)
  {
    // PWM must be 0 for stop modes
    if (ch == Channel::A) writePwmA(0);
    else                  writePwmB(0);

    if (mode == StopMode::Brake)
    {
      if (ch == Channel::A) setInputsA(GpioLevel::High, GpioLevel::High);
      else                  setInputsB(GpioLevel::High, GpioLevel::High);
    }
    else // Coast
    {
      if (ch == Channel::A) setInputsA(GpioLevel::Low, GpioLevel::Low);
      else                  setInputsB(GpioLevel::Low, GpioLevel::Low);
    }
  }

  inline static void applyRun(Channel ch, Direction dir, uint8_t speed)
  {
    if (ch == Channel::A)
    {
      if (dir == Direction::Forward) setInputsA(GpioLevel::High, GpioLevel::Low);
      else                           setInputsA(GpioLevel::Low,  GpioLevel::High);
      writePwmA(speed);
    }
    else
    {
      if (dir == Direction::Forward) setInputsB(GpioLevel::High, GpioLevel::Low);
      else                           setInputsB(GpioLevel::Low,  GpioLevel::High);
      writePwmB(speed);
    }
  }
};
