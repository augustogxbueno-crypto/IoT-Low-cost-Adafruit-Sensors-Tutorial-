// =============================================================================
// core/Pins.h — the two external LEDs are wired the same way for every
// sensor and every board (see README.md "Wiring"). The sensor's own
// internal LED (if it has one) is controlled through SensorBase instead,
// since that varies per sensor (I2C register vs GPIO, continuous vs
// discrete vs binary) rather than being a fixed pin.
// =============================================================================
#pragma once
#include <Arduino.h>

constexpr uint8_t PIN_LED_ABSORBANCE   = 6;  // External LED at 180° (Absorbance)
constexpr uint8_t PIN_LED_FLUORESCENCE = 5;  // External LED at 90°  (Fluorescence)
