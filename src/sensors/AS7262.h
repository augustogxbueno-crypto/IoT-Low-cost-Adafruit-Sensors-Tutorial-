#pragma once
#include "SensorBase.h"
#include <Adafruit_AS726x.h>

class AS7262Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override;

  uint8_t channelCount() const override { return 6; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 4; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  // Integration time = value * 2.8ms — a simple linear formula, unlike
  // AS7341/AS7343's two-parameter multiplicative one.
  TimeType timeType() const override { return TimeType::TIME_FORMULA; }
  const __FlashStringHelper* integrationFormula() const override;
  float integrationTimeMs() const override;
  uint8_t timeParamCount() const override { return 1; }
  const char* timeParamKey(uint8_t idx) const override;
  const __FlashStringHelper* timeParamLabel(uint8_t idx) const override;
  int32_t timeParamMin(uint8_t idx) const override;
  int32_t timeParamMax(uint8_t idx) const override;
  void setTimeParam(uint8_t idx, int32_t value) override;

  // The AS7262's LED only has 4 fixed current levels, not a continuous range.
  LedType ledType() const override { return LedType::LED_DISCRETE; }
  uint8_t ledOptionCount() const override { return 4; }
  const __FlashStringHelper* ledOptionLabel(uint8_t idx) const override;
  void setLedOn(bool on) override;
  void setLedDiscrete(uint8_t idx) override;

 private:
  Adafruit_AS726x sensor_;
  uint8_t itime_ = 50; // ~140ms
};
