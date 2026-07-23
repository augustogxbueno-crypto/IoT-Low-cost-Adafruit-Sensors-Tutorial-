#include "LTR390.h"
#include "../core/Utils.h"

static const char GAIN_0[] PROGMEM = "1X";
static const char GAIN_1[] PROGMEM = "3X";
static const char GAIN_2[] PROGMEM = "6X";
static const char GAIN_3[] PROGMEM = "9X";
static const char GAIN_4[] PROGMEM = "18X";
static const char* const GAIN_NAMES[] PROGMEM = { GAIN_0, GAIN_1, GAIN_2, GAIN_3, GAIN_4 };
static const ltr390_gain_t GAIN_VALUES[5] = {
  LTR390_GAIN_1, LTR390_GAIN_3, LTR390_GAIN_6, LTR390_GAIN_9, LTR390_GAIN_18
};

// Order matches the library's bit-depth ordering (20-bit down to 13-bit).
// ms values come from the LTR390 measurement-rate register table (the same
// one ESPHome's driver uses): 20-bit=400ms, 19-bit=200ms, 18-bit=100ms,
// 17-bit=50ms, 16-bit=25ms, 13-bit=12.5ms.
static const char RES_0[] PROGMEM = "20-bit (400ms)";
static const char RES_1[] PROGMEM = "19-bit (200ms)";
static const char RES_2[] PROGMEM = "18-bit (100ms)";
static const char RES_3[] PROGMEM = "17-bit (50ms)";
static const char RES_4[] PROGMEM = "16-bit (25ms)";
static const char RES_5[] PROGMEM = "13-bit (12.5ms)";
static const char* const RES_NAMES[] PROGMEM = { RES_0, RES_1, RES_2, RES_3, RES_4, RES_5 };
static const ltr390_resolution_t RES_VALUES[6] = {
  LTR390_RESOLUTION_20BIT, LTR390_RESOLUTION_19BIT, LTR390_RESOLUTION_18BIT,
  LTR390_RESOLUTION_17BIT, LTR390_RESOLUTION_16BIT, LTR390_RESOLUTION_13BIT
};
static const float RES_MS[6] = { 400.0f, 200.0f, 100.0f, 50.0f, 25.0f, 12.5f };

static const char MODE_0[] PROGMEM = "Ambient Light (ALS)";
static const char MODE_1[] PROGMEM = "UV Index (UVS)";
static const char* const MODE_NAMES[] PROGMEM = { MODE_0, MODE_1 };

bool LTR390Sensor::begin() {
  if (!sensor_.begin()) return false;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
  sensor_.setResolution(RES_VALUES[resIdx_]);
  sensor_.setMode(modeIdx_ == 0 ? LTR390_MODE_ALS : LTR390_MODE_UVS);
  return true;
}

const __FlashStringHelper* LTR390Sensor::name() const { return F("LTR390"); }
const __FlashStringHelper* LTR390Sensor::sensorType() const { return F("photodiode"); }

ChannelInfo LTR390Sensor::channel(uint8_t) const {
  return (modeIdx_ == 0)
    ? ChannelInfo{ F("Ambient Light"), F("#CCCCCC") }
    : ChannelInfo{ F("UV Index"),      F("#9400D3") };
}

bool LTR390Sensor::readChannels(float* out) {
  if (!sensor_.newDataAvailable()) return false;
  out[0] = (modeIdx_ == 0) ? sensor_.readALS() : sensor_.readUVS();
  return true;
}

const __FlashStringHelper* LTR390Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void LTR390Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
}

float LTR390Sensor::integrationTimeMs() const { return RES_MS[resIdx_]; }

const __FlashStringHelper* LTR390Sensor::timePresetLabel(uint8_t idx) const {
  return Utils::flashStr(RES_NAMES, idx);
}

void LTR390Sensor::setTimePreset(uint8_t idx) {
  resIdx_ = idx;
  sensor_.setResolution(RES_VALUES[resIdx_]);
}

ExtraParamInfo LTR390Sensor::extraParam(uint8_t) const {
  return { "mode", F("Measurement"), (const __FlashStringHelper* const*)MODE_NAMES, 2, 0, 0 };
}

void LTR390Sensor::setExtraParam(uint8_t, int value) {
  if (value < 0 || value > 1) return;
  modeIdx_ = (uint8_t)value;
  sensor_.setMode(modeIdx_ == 0 ? LTR390_MODE_ALS : LTR390_MODE_UVS);
}
