#pragma once
#include "SensorBase.h"
#include <BH1750.h>

class BH1750Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override; // "photodiode"

  uint8_t channelCount() const override { return 1; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  // BH1750 has no traditional "gain" — MTreg is a continuous sensitivity
  // register that behaves the same way in the UI (a slider).
  GainType gainType() const override { return GainType::GAIN_CONTINUOUS; }
  float gainContinuousMin() const override { return 31; }
  float gainContinuousMax() const override { return 254; }
  void setGainContinuous(float value) override;

  // The resolution mode IS the integration time here (16ms low-res vs
  // 120ms high-res) — modeled as presets, not a formula.
  TimeType timeType() const override { return TimeType::TIME_PRESETS; }
  float integrationTimeMs() const override;
  uint8_t timePresetCount() const override { return 3; }
  const __FlashStringHelper* timePresetLabel(uint8_t idx) const override;
  void setTimePreset(uint8_t idx) override;

  // Purely passive ambient light sensor — no LED at all.
  LedType ledType() const override { return LedType::LED_NONE; }

 private:
  BH1750 sensor_;
  uint8_t modeIdx_ = 0; // CONTINUOUS_HIGH_RES_MODE
  byte    mtreg_   = 69;

  void applyMode();
};
