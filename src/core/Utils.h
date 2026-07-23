// =============================================================================
// core/Utils.h — small shared helpers used across core/ and sensors/.
// No String class anywhere here on purpose (see main.cpp for why: it
// fragments the heap on AVR boards like the Uno).
// =============================================================================
#pragma once
#include <Arduino.h>

#if defined(__AVR__)
  #include <avr/pgmspace.h>
#endif

namespace Utils {

// Reads a pointer out of a PROGMEM table of PROGMEM strings and returns it
// as a Flash string ArduinoJson can serialize without copying it into RAM.
// Usage pattern (see any sensors/*.cpp for a real example):
//   const char NAME_0[] PROGMEM = "foo";
//   const char* const NAMES[] PROGMEM = { NAME_0, ... };
//   Utils::flashStr(NAMES, 0) -> F("foo")-equivalent pointer
inline const __FlashStringHelper* flashStr(const char* const table[], uint8_t idx) {
  return (const __FlashStringHelper*)pgm_read_ptr(&table[idx]);
}

// Writes the decimal representation of a small non-negative index (0-15) into
// `buf` (must be at least 3 bytes). Used to build JSON object keys ("0".."15")
// without pulling in String/itoa's locale machinery.
inline void smallIndexKey(uint8_t idx, char* buf) {
  if (idx >= 10) {
    buf[0] = '1';
    buf[1] = char('0' + (idx - 10));
    buf[2] = '\0';
  } else {
    buf[0] = char('0' + idx);
    buf[1] = '\0';
  }
}

} // namespace Utils
