#pragma once
#include "SensorBase.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>

class TSL2591Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override; // "photodiode"

  // Full spectrum, IR (both raw) + derived Visible and derived Lux
  uint8_t channelCount() const override { return 4; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  // Gain steps here aren't evenly spaced multipliers like other sensors —
  // they span a huge, non-uniform dynamic range (1x to ~9876x).
  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 4; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  TimeType timeType() const override { return TimeType::TIME_PRESETS; }
  float integrationTimeMs() const override;
  uint8_t timePresetCount() const override { return 6; }
  const __FlashStringHelper* timePresetLabel(uint8_t idx) const override;
  void setTimePreset(uint8_t idx) override;

  // Purely passive high-dynamic-range lux sensor — no LED at all.
  LedType ledType() const override { return LedType::LED_NONE; }

 private:
  Adafruit_TSL2591 sensor_{2591};
  uint8_t gainIdx_ = 1; // MED (25x)
  uint8_t timeIdx_ = 0; // 100ms
};
