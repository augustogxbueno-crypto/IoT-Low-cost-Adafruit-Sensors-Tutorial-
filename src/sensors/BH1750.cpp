#include "BH1750.h"

static const uint8_t MODE_VALUES[3] = {
  BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750::CONTINUOUS_HIGH_RES_MODE_2, BH1750::CONTINUOUS_LOW_RES_MODE
};
static const float MODE_MS[3] = { 120.0f, 120.0f, 16.0f };

bool BH1750Sensor::begin() {
  if (!sensor_.begin((BH1750::Mode)MODE_VALUES[modeIdx_])) return false;
  sensor_.setMTreg(mtreg_);
  return true;
}

const __FlashStringHelper* BH1750Sensor::name() const { return F("BH1750"); }
const __FlashStringHelper* BH1750Sensor::sensorType() const { return F("photodiode"); }

ChannelInfo BH1750Sensor::channel(uint8_t) const {
  return { F("Lux"), F("#CCCCCC") };
}

bool BH1750Sensor::readChannels(float* out) {
  out[0] = sensor_.readLightLevel();
  return true;
}

void BH1750Sensor::setGainContinuous(float value) {
  mtreg_ = (byte)value;
  sensor_.setMTreg(mtreg_);
}

float BH1750Sensor::integrationTimeMs() const {
  return MODE_MS[modeIdx_];
}

const __FlashStringHelper* BH1750Sensor::timePresetLabel(uint8_t idx) const {
  switch (idx) {
    case 0: return F("High Res (1.0 lx, 120ms)");
    case 1: return F("High Res 2 (0.5 lx, 120ms)");
    default: return F("Low Res (4.0 lx, 16ms)");
  }
}

void BH1750Sensor::setTimePreset(uint8_t idx) {
  modeIdx_ = idx;
  applyMode();
}

void BH1750Sensor::applyMode() {
  // The BH1750 library changes resolution mode by re-initializing.
  sensor_.begin((BH1750::Mode)MODE_VALUES[modeIdx_]);
  sensor_.setMTreg(mtreg_);
}
