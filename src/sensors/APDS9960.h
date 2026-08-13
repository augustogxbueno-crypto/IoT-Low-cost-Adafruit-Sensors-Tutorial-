#pragma once
#include "SensorBase.h"
#include <Adafruit_APDS9960.h>

// =============================================================================
// APDS9960 — Adafruit Proximity, Light, RGB, and Gesture Sensor (STEMMA QT/Qwiic)
// https://www.adafruit.com/product/3595
// -----------------------------------------------------------------------------
// Replaces the discontinued TCS34725 as this project's "simple 4-channel color
// sensor" option. Only the chip's ALS/color engine is used here — proximity
// and gesture sensing are separate features of the same chip that this
// tutorial doesn't touch.
//
// IMPORTANT: unlike TCS34725 (whose Adafruit breakout has a solder-optional
// white LED that this project drives from GPIO pin 7), the APDS9960 breakout
// has NO controllable white/broadband LED for illuminating a sample. The
// chip does contain its own IR LED + driver, but that LED is wired
// internally to the proximity/gesture engine only — the Adafruit library
// exposes no way to fire it during a color/ALS reading, and it isn't the
// right wavelength for reflectance work anyway. So this sensor never offers
// Reflectance mode; it only ever uses the project's two fixed EXTERNAL LEDs
// (pin 6 = Absorbance/180°, pin 5 = Fluorescence/90°), exactly like the
// other LED-less photodiode sensors (BH1750, LTR303/329/390, TSL2591).
// =============================================================================
class APDS9960Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override; // "color"

  // 4 raw channels (R,G,B,Clear) + 2 derived (Color Temperature, Lux) —
  // same shape as TCS34725, computed via the same kind of library helpers.
  uint8_t channelCount() const override { return 6; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  // apds9960AGain_t: 1X / 4X / 16X / 64X — 4 discrete steps.
  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 4; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  // setADCIntegrationTime(uint16_t iTimeMS) takes a direct millisecond value
  // (internally converted to the ATIME register via
  // ATIME = 256 - iTimeMS/2.78, so effectively ~2.8-712ms) — unlike
  // AS7262/AS7341's ATIME/ASTEP pair, there's exactly one numeric knob here,
  // so it's modeled as a 1-parameter formula rather than a preset list.
  TimeType timeType() const override { return TimeType::TIME_FORMULA; }
  const __FlashStringHelper* integrationFormula() const override;
  float integrationTimeMs() const override;
  uint8_t timeParamCount() const override { return 1; }
  const char* timeParamKey(uint8_t idx) const override;
  const __FlashStringHelper* timeParamLabel(uint8_t idx) const override;
  int32_t timeParamMin(uint8_t idx) const override;
  int32_t timeParamMax(uint8_t idx) const override;
  void setTimeParam(uint8_t idx, int32_t value) override;

  // No usable illumination source for this application — see note above.
  // Reflectance mode is hidden by the frontend automatically because of this.
  LedType ledType() const override { return LedType::LED_NONE; }

 private:
  Adafruit_APDS9960 sensor_;
  uint8_t gainIdx_ = 1;     // APDS9960_AGAIN_4X (library default)
  uint16_t itimeMs_ = 10;   // library default integration time
};
