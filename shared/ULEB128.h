#pragma once

#include <stdint.h>

/*
// Max bytes for a 64-bit ULEB128 / protobuf varint
static constexpr size_t kMaxVarint64Bytes = 10;

// Encodes value into buffer.
// Returns number of bytes written (1..10).
size_t encodeULEB128_u64(uint64_t value, uint8_t* buffer) {
  size_t i = 0;
  do {
    uint8_t byte = static_cast<uint8_t>(value & 0x7FULL); // lower 7 bits
    value >>= 7;
    if (value != 0) byte |= 0x80;                         // continuation
    buffer[i++] = byte;
  } while (value != 0 && i < kMaxVarint64Bytes);

  // For uint64_t, i will never exceed 10 with correct logic above.
  return i;
}

// Decodes from buffer into 'value'.
// Returns true on success and sets:
//   - value
//   - bytesConsumed (1..10)
// Returns false if input is malformed or would overflow uint64_t.
//
// NOTE: You must ensure 'buffer' has at least 10 readable bytes OR stop earlier
// based on your framing (length-delimited packet, etc.).
bool decodeULEB128_u64(const uint8_t* buffer, size_t bufferLen,
                       uint64_t& value, size_t& bytesConsumed) {
  value = 0;
  bytesConsumed = 0;

  uint32_t shift = 0;

  for (size_t i = 0; i < kMaxVarint64Bytes; ++i) {
    if (i >= bufferLen) return false; // not enough data

    uint8_t byte = buffer[i];
    uint64_t slice = static_cast<uint64_t>(byte & 0x7F);

    // Overflow check:
    // - For shifts 0..63, we can place bits.
    // - If shift == 63, only 1 bit could fit; but varints place 7 bits at a time.
    // Practical protobuf rule: on the 10th byte (i==9), only the low 1 bit may be set
    // (because 64 bits total).
    if (shift >= 64) return false;

    if (i == 9) {
      // 10th byte: only bit0 is allowed (values 0 or 1), and continuation must be 0
      if ((byte & 0xFE) != 0) return false;  // any bits beyond bit0 set => overflow
      if ((byte & 0x80) != 0) return false;  // continuation on 10th byte is invalid
    }

    value |= (slice << shift);
    bytesConsumed = i + 1;

    if ((byte & 0x80) == 0) {
      return true; // finished
    }

    shift += 7;
  }

  // If we consumed 10 bytes and still had continuation, it's malformed.
  return false;
}
*/
/*
uint8_t buf[10];
uint64_t original = 300ULL;

size_t n = encodeULEB128_u64(original, buf);

uint64_t decoded = 0;
size_t consumed = 0;
bool ok = decodeULEB128_u64(buf, n, decoded, consumed);
 */
 
//#include <EEPROM.h>
//const int _epromIdxFirstRun = 0;
//bool _firstRun = true;
  //_firstRun = EEPROM.read(_epromIdxFirstRun) == 0;
  //if (_firstRun) {
    //Serial.println("First Run!");
    //EEPROM.write(_epromIdxFirstRun, 1);
  //}
