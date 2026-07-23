#pragma once
#include "SensorBase.h"
#include <Adafruit_LTR390.h>

class LTR390Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override; // "photodiode"

  // Only one channel exists at a time — which one depends on the "mode"
  // extra parameter (ALS vs UVS). We always report it as channel 0; the
  // frontend just relabels it if it cares (see README.md protocol notes).
  uint8_t channelCount() const override { return 1; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 5; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  // "Resolution" doubles as integration time here — higher bit depth takes
  // longer to convert. ms values per bit depth come from the LTR390
  // measurement-rate register table (see .cpp), not stated directly by the
  // datasheet's headline spec.
  TimeType timeType() const override { return TimeType::TIME_PRESETS; }
  float integrationTimeMs() const override;
  uint8_t timePresetCount() const override { return 6; }
  const __FlashStringHelper* timePresetLabel(uint8_t idx) const override;
  void setTimePreset(uint8_t idx) override;

  // Purely passive UV/ambient-light sensor — no LED at all.
  LedType ledType() const override { return LedType::LED_NONE; }

  // Which physical measurement is active: Ambient Light or UV. Unique to
  // this sensor, hence the generic "extra parameter" escape hatch.
  uint8_t extraParamCount() const override { return 1; }
  ExtraParamInfo extraParam(uint8_t idx) const override;
  void setExtraParam(uint8_t idx, int value) override;

 private:
  Adafruit_LTR390 sensor_;
  uint8_t gainIdx_ = 1; // LTR390_GAIN_3 (library default)
  uint8_t resIdx_  = 4; // LTR390_RESOLUTION_16BIT (18-bit/100ms is the library default; 16-bit chosen here for headroom)
  uint8_t modeIdx_ = 0; // 0 = ALS, 1 = UVS
};
