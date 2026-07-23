#pragma once
#include "SensorBase.h"
#include <Adafruit_AS7341.h>

class AS7341Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override;

  uint8_t channelCount() const override { return 8; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 11; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  TimeType timeType() const override { return TimeType::TIME_FORMULA; }
  const __FlashStringHelper* integrationFormula() const override;
  float integrationTimeMs() const override;
  uint8_t timeParamCount() const override { return 2; }
  const char* timeParamKey(uint8_t idx) const override;
  const __FlashStringHelper* timeParamLabel(uint8_t idx) const override;
  int32_t timeParamMin(uint8_t idx) const override;
  int32_t timeParamMax(uint8_t idx) const override;
  void setTimeParam(uint8_t idx, int32_t value) override;

  LedType ledType() const override { return LedType::LED_CONTINUOUS; }
  int ledMinMA() const override { return 4; }
  int ledMaxMA() const override { return 258; }
  void setLedOn(bool on) override;
  void setLedContinuous(int mA) override;

 private:
  Adafruit_AS7341 sensor_;
  uint8_t gainIdx_  = 4; // 16X
  int     atime_    = 100;
  int     astep_    = 999;

  void applyGainAndTiming();
};
