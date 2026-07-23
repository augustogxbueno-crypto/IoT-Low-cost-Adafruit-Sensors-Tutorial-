#include "TSL2591.h"
#include "../core/Utils.h"

static const char CH_NAME_0[] PROGMEM = "Full Spectrum";
static const char CH_NAME_1[] PROGMEM = "IR";
static const char CH_NAME_2[] PROGMEM = "Visible (est.)";
static const char CH_NAME_3[] PROGMEM = "Lux";
static const char* const CH_NAMES[] PROGMEM = { CH_NAME_0, CH_NAME_1, CH_NAME_2, CH_NAME_3 };

static const char CH_COLOR_0[] PROGMEM = "#CCCCCC";
static const char CH_COLOR_1[] PROGMEM = "#800000";
static const char CH_COLOR_2[] PROGMEM = "#FFFF66";
static const char CH_COLOR_3[] PROGMEM = "#FFD700";
static const char* const CH_COLORS[] PROGMEM = { CH_COLOR_0, CH_COLOR_1, CH_COLOR_2, CH_COLOR_3 };

static const char GAIN_0[] PROGMEM = "1X (Low)";
static const char GAIN_1[] PROGMEM = "25X (Med)";
static const char GAIN_2[] PROGMEM = "428X (High)";
static const char GAIN_3[] PROGMEM = "9876X (Max)";
static const char* const GAIN_NAMES[] PROGMEM = { GAIN_0, GAIN_1, GAIN_2, GAIN_3 };
static const tsl2591Gain_t GAIN_VALUES[4] = {
  TSL2591_GAIN_LOW, TSL2591_GAIN_MED, TSL2591_GAIN_HIGH, TSL2591_GAIN_MAX
};

static const char TIME_0[] PROGMEM = "100 ms";
static const char TIME_1[] PROGMEM = "200 ms";
static const char TIME_2[] PROGMEM = "300 ms";
static const char TIME_3[] PROGMEM = "400 ms";
static const char TIME_4[] PROGMEM = "500 ms";
static const char TIME_5[] PROGMEM = "600 ms";
static const char* const TIME_NAMES[] PROGMEM = {
  TIME_0, TIME_1, TIME_2, TIME_3, TIME_4, TIME_5
};
static const tsl2591IntegrationTime_t TIME_VALUES[6] = {
  TSL2591_INTEGRATIONTIME_100MS, TSL2591_INTEGRATIONTIME_200MS, TSL2591_INTEGRATIONTIME_300MS,
  TSL2591_INTEGRATIONTIME_400MS, TSL2591_INTEGRATIONTIME_500MS, TSL2591_INTEGRATIONTIME_600MS
};
static const float TIME_MS[6] = { 100, 200, 300, 400, 500, 600 };

bool TSL2591Sensor::begin() {
  if (!sensor_.begin()) return false;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
  sensor_.setTiming(TIME_VALUES[timeIdx_]);
  return true;
}

const __FlashStringHelper* TSL2591Sensor::name() const { return F("TSL2591"); }
const __FlashStringHelper* TSL2591Sensor::sensorType() const { return F("photodiode"); }

ChannelInfo TSL2591Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool TSL2591Sensor::readChannels(float* out) {
  uint32_t lum  = sensor_.getFullLuminosity();
  uint16_t ir   = lum >> 16;
  uint16_t full = lum & 0xFFFF;
  out[0] = full;
  out[1] = ir;
  out[2] = (full > ir) ? (full - ir) : 0;
  out[3] = sensor_.calculateLux(full, ir);
  return true;
}

const __FlashStringHelper* TSL2591Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void TSL2591Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
}

float TSL2591Sensor::integrationTimeMs() const { return TIME_MS[timeIdx_]; }

const __FlashStringHelper* TSL2591Sensor::timePresetLabel(uint8_t idx) const {
  return Utils::flashStr(TIME_NAMES, idx);
}

void TSL2591Sensor::setTimePreset(uint8_t idx) {
  timeIdx_ = idx;
  sensor_.setTiming(TIME_VALUES[timeIdx_]);
}
