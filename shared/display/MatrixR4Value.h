#pragma once

#include <array>
#include <cstdint>

class MatrixR4Value
{
public:
  static constexpr int Width      = 12;
  static constexpr int Height     = 8;
  static constexpr int WordCount  = 3;
  static constexpr int TotalBits  = Width * Height;

  using Value = std::array<uint32_t, WordCount>;

  static constexpr Value allOff = { 0x00000000u, 0x00000000u, 0x00000000u };
  static constexpr Value allOn  = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
  static constexpr Value circle = { 0x06009010u, 0x82042041u, 0x08090060u };

  MatrixR4Value()
  : _value(allOff)
  {
  }

  inline const uint32_t* data() const { return _value.data(); }
  inline size_t size() const { return sizeof(_value); }

  bool update(const Value& input, bool flippingY = false, bool flippingX = false, bool inverting = false)
  {
    Value newValue;
    if (!flippingY && !flippingX)
    {
      newValue = inverting ? Value{ ~input[0], ~input[1], ~input[2] } : input;
    }
    else
    {
      for (int y = 0; y < Height; ++y)
      {
        for (int x = 0; x < Width; ++x)
        {
          const int srcY = flippingY ? flipY(y) : y;
          const int srcX = flippingX ? flipX(x) : x;
          const int srcIndex = getIndex(srcY, srcX);
          const int dstIndex = getIndex(y, x);
          bool pixel = getBit(input, srcIndex);
          if (inverting) pixel = !pixel;
          setBit(newValue, dstIndex, pixel);
        }
      }
    }
    if (_value == newValue) return false;
    _value = newValue;
    return true;
  }

private:
  Value _value;

  inline static int getIndex(int y, int x)
  {
    return y * Width + x;
  }

  inline static int flipY(int y)
  {
    return Height - 1 - y;
  }

  inline static int flipX(int x)
  {
    return Width - 1 - x;
  }

  inline static bool getBit(const Value& frame, int index)
  {
    const uint32_t word = frame[index / 32];
    const uint32_t mask = 1u << (31 - (index % 32));  // MSB-first
    return (word & mask) != 0;
  }

  inline static void setBit(Value& frame, int index, bool value)
  {
    uint32_t& word = frame[index / 32];
    const uint32_t mask = 1u << (31 - (index % 32));  // MSB-first
    word = value ? (word | mask) : (word & ~mask);
  }
};
