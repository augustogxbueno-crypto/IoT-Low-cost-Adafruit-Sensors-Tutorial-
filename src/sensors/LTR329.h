#pragma once
#include "SensorBase.h"
#include <Adafruit_LTR329_LTR303.h>

// See the note at the top of LTR303.h: this is intentionally a close
// duplicate of LTR303Sensor, swapped to the Adafruit_LTR329 class. The two
// chips share the same register layout and the same library file.
class LTR329Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override; // "photodiode"

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

  LedType ledType() const override { return LedType::LED_NONE; }

  uint8_t extraParamCount() const override { return 1; }
  ExtraParamInfo extraParam(uint8_t idx) const override;
  void setExtraParam(uint8_t idx, int value) override;

 protected:
  Adafruit_LTR329 sensor_;
  uint8_t gainIdx_ = 0;
  uint8_t timeIdx_ = 1;
  uint8_t rateIdx_ = 1;
};
