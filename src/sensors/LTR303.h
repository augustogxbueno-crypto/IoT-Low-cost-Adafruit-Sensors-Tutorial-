#pragma once
#include "SensorBase.h"
#include <Adafruit_LTR329_LTR303.h>

// NOTE ON LTR303 vs LTR329: both chips share the exact same register layout
// and the same Adafruit_LTR329_LTR303 library file, just two different
// classes (Adafruit_LTR303 / Adafruit_LTR329) with the same method surface.
// LTR329.h/.cpp is intentionally a near-duplicate of this file rather than a
// template, because the two vendor classes aren't polymorphic to each other.
// SensorBase.h doesn't need to change either way if you later merge them.
class LTR303Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override; // "photodiode"

  // CH0 (Visible+IR), CH1 (IR only), derived Visible (CH0 - CH1)
  uint8_t channelCount() const override { return 3; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 6; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  TimeType timeType() const override { return TimeType::TIME_PRESETS; }
  float integrationTimeMs() const override;
  uint8_t timePresetCount() const override { return 8; }
  const __FlashStringHelper* timePresetLabel(uint8_t idx) const override;
  void setTimePreset(uint8_t idx) override;

  // Purely passive ambient light sensor — no LED at all.
  LedType ledType() const override { return LedType::LED_NONE; }

  // Measurement Rate: how often the sensor auto-repeats a reading. This
  // doesn't exist on any other sensor in the series, hence the generic
  // "extra parameter" escape hatch instead of a dedicated SensorBase field.
  uint8_t extraParamCount() const override { return 1; }
  ExtraParamInfo extraParam(uint8_t idx) const override;
  void setExtraParam(uint8_t idx, int value) override;

 protected:
  Adafruit_LTR303 sensor_;
  uint8_t gainIdx_ = 0; // 1X
  uint8_t timeIdx_ = 1; // 100ms
  uint8_t rateIdx_ = 1; // 100ms
};
