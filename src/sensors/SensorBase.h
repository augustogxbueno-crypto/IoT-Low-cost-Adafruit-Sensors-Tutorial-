// =============================================================================
// SensorBase.h — the contract every sensor driver implements.
// -----------------------------------------------------------------------------
// This is the ONLY thing core/ knows about sensors. It never includes a
// specific Adafruit/vendor library or references a specific sensor by name.
// If you're adding sensor #10 to this project, this is the file that tells
// you exactly what to implement — see any file in sensors/ for an example.
//
// Design notes (why the interface looks the way it does):
//   - Gain, LED and integration time each vary a lot between real sensors,
//     so each has a "type" enum the frontend uses to decide how to render
//     the control (slider vs dropdown vs on/off toggle vs nothing at all).
//   - Reflectance mode is only offered by the frontend when ledType() is
//     not LED_NONE — there's no separate "which modes are available" list,
//     it's derived straight from whether the sensor can drive its own LED.
//   - "Channels" can be raw sensor readings OR values computed from raw
//     readings (e.g. TCS34725's color temperature, TSL2591's visible/lux).
//     SensorBase doesn't care which — readChannels() just fills the array
//     with whatever channelCount() promised, in the same order as channel().
//   - extraParam() is the escape hatch for anything that doesn't fit gain/
//     time/LED (e.g. LTR303's Measurement Rate, LTR390's ALS/UVS mode).
// =============================================================================
#pragma once
#include <Arduino.h>

enum class LedType : uint8_t {
  LED_NONE,        // no controllable LED at all (Reflectance mode hidden by the UI)
  LED_BINARY,      // on/off only (e.g. TCS34725)
  LED_DISCRETE,    // fixed list of current levels (e.g. AS7262)
  LED_CONTINUOUS   // current adjustable in a numeric range, in mA (e.g. AS7341)
};

enum class GainType : uint8_t {
  GAIN_DISCRETE,    // fixed list of named gain steps (most sensors)
  GAIN_CONTINUOUS   // a raw numeric sensitivity register (e.g. BH1750's MTreg)
};

enum class TimeType : uint8_t {
  TIME_FORMULA,   // 1-2 numeric parameters combined with a known formula
  TIME_PRESETS    // a fixed list of named presets, each with a known ms value
};

struct ChannelInfo {
  const __FlashStringHelper* name;   // e.g. "415nm" or "Lux"
  const __FlashStringHelper* color;  // hex string used only for the swatch in the UI
};

// A generic extra parameter (e.g. LTR303 Measurement Rate, LTR390 ALS/UVS mode).
// If `options` is non-null it's a dropdown (select one by index); otherwise
// it's a plain numeric field between numMin and numMax.
struct ExtraParamInfo {
  const char* key;                             // protocol key, e.g. "measRate"
  const __FlashStringHelper* label;             // shown in the UI
  const __FlashStringHelper* const* options;    // PROGMEM table of PROGMEM strings, or nullptr
  uint8_t optionCount;
  int numMin;
  int numMax;
};

class SensorBase {
 public:
  virtual ~SensorBase() {}

  // ---- Identity ----
  virtual bool begin() = 0;
  virtual const __FlashStringHelper* name() const = 0;
  virtual const __FlashStringHelper* sensorType() const = 0; // "color" or "photodiode"

  // ---- Channels (raw or derived, sensor doesn't need to distinguish) ----
  virtual uint8_t channelCount() const = 0;
  virtual ChannelInfo channel(uint8_t idx) const = 0;
  virtual bool readChannels(float* out) = 0; // fills out[0..channelCount()-1]

  // ---- Gain ----
  virtual GainType gainType() const = 0;
  virtual uint8_t gainOptionCount() const { return 0; }
  virtual const __FlashStringHelper* gainOptionLabel(uint8_t idx) const { return nullptr; }
  virtual void setGainIndex(uint8_t idx) {}
  virtual float gainContinuousMin() const { return 0; }
  virtual float gainContinuousMax() const { return 0; }
  virtual void setGainContinuous(float value) {}

  // ---- Integration time ----
  virtual TimeType timeType() const = 0;
  virtual const __FlashStringHelper* integrationFormula() const { return nullptr; }
  virtual float integrationTimeMs() const = 0; // current value, always available

  // TIME_FORMULA mode (1-2 numeric params, e.g. ATIME/ASTEP or a single ITIME)
  // NOTE: these are int32_t, not int. On AVR (Arduino Uno) plain `int` is
  // only 16 bits (max 32767), but AS7341/AS7343's ASTEP parameter goes up
  // to 65535 — a literal that silently wraps to -1 if returned as a plain
  // `int` there, which breaks constrain()'s clamp in Protocol.cpp on Uno
  // builds. int32_t is 32 bits on every board in this series (AVR and
  // ESP32-S3 alike), so it can't happen again.
  virtual uint8_t timeParamCount() const { return 0; }
  virtual const char* timeParamKey(uint8_t idx) const { return nullptr; }
  virtual const __FlashStringHelper* timeParamLabel(uint8_t idx) const { return nullptr; }
  virtual int32_t timeParamMin(uint8_t idx) const { return 0; }
  virtual int32_t timeParamMax(uint8_t idx) const { return 0; }
  virtual void setTimeParam(uint8_t idx, int32_t value) {}

  // TIME_PRESETS mode (fixed named list, each with a known ms value)
  virtual uint8_t timePresetCount() const { return 0; }
  virtual const __FlashStringHelper* timePresetLabel(uint8_t idx) const { return nullptr; }
  virtual void setTimePreset(uint8_t idx) {}

  // ---- LED ----
  virtual LedType ledType() const = 0;
  virtual int ledMinMA() const { return 0; }
  virtual int ledMaxMA() const { return 0; }
  virtual uint8_t ledOptionCount() const { return 0; }
  virtual const __FlashStringHelper* ledOptionLabel(uint8_t idx) const { return nullptr; }
  virtual void setLedOn(bool on) {}                 // master enable/disable — implement for ANY led type that isn't LED_NONE
  virtual void setLedContinuous(int mA) {}           // sets current level (call setLedOn(true) separately to enable)
  virtual void setLedDiscrete(uint8_t idx) {}        // sets discrete level (call setLedOn(true) separately to enable)

  // ---- Extra sensor-specific parameters ----
  virtual uint8_t extraParamCount() const { return 0; }
  virtual ExtraParamInfo extraParam(uint8_t idx) const { return {}; }
  virtual void setExtraParam(uint8_t idx, int value) {}
};
