#pragma once
#include "SensorBase.h"
#include <Adafruit_TCS34725.h>

class TCS34725Sensor : public SensorBase {
 public:
  bool begin() override;

  const __FlashStringHelper* name() const override;
  const __FlashStringHelper* sensorType() const override;

  // 4 raw channels (R,G,B,C) + 2 derived (Color Temperature, Lux)
  uint8_t channelCount() const override { return 6; }
  ChannelInfo channel(uint8_t idx) const override;
  bool readChannels(float* out) override;

  GainType gainType() const override { return GainType::GAIN_DISCRETE; }
  uint8_t gainOptionCount() const override { return 4; }
  const __FlashStringHelper* gainOptionLabel(uint8_t idx) const override;
  void setGainIndex(uint8_t idx) override;

  // No formula here: the datasheet only allows 6 fixed integration presets.
  TimeType timeType() const override { return TimeType::TIME_PRESETS; }
  float integrationTimeMs() const override;
  uint8_t timePresetCount() const override { return 6; }
  const __FlashStringHelper* timePresetLabel(uint8_t idx) const override;
  void setTimePreset(uint8_t idx) override;

  // The onboard LED is only on/off — no current control, unlike
  // AS7341/AS7343/AS7262. IMPORTANT: unlike those sensors, this LED is NOT
  // driven through I2C. The Adafruit breakout's LED pin must be wired to a
  // GPIO (pin 7, same on every board in this series) and toggled directly —
  // see .cpp for why setInterrupt()/INT-pin wiring isn't used instead.
  LedType ledType() const override { return LedType::LED_BINARY; }
  void setLedOn(bool on) override;

 private:
  Adafruit_TCS34725 sensor_;
  uint8_t gainIdx_ = 1; // 4X
  uint8_t timeIdx_ = 4; // 154ms

  // Physical GPIO driving the onboard LED directly (see .cpp) — same pin
  // number on every board in this series (ESP32-S3 Feather or Arduino Uno).
  static constexpr uint8_t PIN_LED_INTERNAL = 7;
};
