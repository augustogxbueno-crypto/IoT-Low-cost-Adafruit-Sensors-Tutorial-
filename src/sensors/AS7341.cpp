#include "AS7341.h"
#include "../core/Utils.h"

// -----------------------------------------------------------------------------
// PROGMEM tables — see core/Utils.h for why these live in flash instead of RAM.
// -----------------------------------------------------------------------------
static const char CH_NAME_0[] PROGMEM = "415nm";
static const char CH_NAME_1[] PROGMEM = "445nm";
static const char CH_NAME_2[] PROGMEM = "480nm";
static const char CH_NAME_3[] PROGMEM = "515nm";
static const char CH_NAME_4[] PROGMEM = "555nm";
static const char CH_NAME_5[] PROGMEM = "590nm";
static const char CH_NAME_6[] PROGMEM = "630nm";
static const char CH_NAME_7[] PROGMEM = "680nm";
static const char* const CH_NAMES[] PROGMEM = {
  CH_NAME_0, CH_NAME_1, CH_NAME_2, CH_NAME_3, CH_NAME_4, CH_NAME_5, CH_NAME_6, CH_NAME_7
};

static const char CH_COLOR_0[] PROGMEM = "#7F00FF";
static const char CH_COLOR_1[] PROGMEM = "#0000FF";
static const char CH_COLOR_2[] PROGMEM = "#00BFFF";
static const char CH_COLOR_3[] PROGMEM = "#00C000";
static const char CH_COLOR_4[] PROGMEM = "#ADFF2F";
static const char CH_COLOR_5[] PROGMEM = "#FFA500";
static const char CH_COLOR_6[] PROGMEM = "#FF0000";
static const char CH_COLOR_7[] PROGMEM = "#8B0000";
static const char* const CH_COLORS[] PROGMEM = {
  CH_COLOR_0, CH_COLOR_1, CH_COLOR_2, CH_COLOR_3, CH_COLOR_4, CH_COLOR_5, CH_COLOR_6, CH_COLOR_7
};

static const char GAIN_0[]  PROGMEM = "0.5X";
static const char GAIN_1[]  PROGMEM = "1X";
static const char GAIN_2[]  PROGMEM = "2X";
static const char GAIN_3[]  PROGMEM = "4X";
static const char GAIN_4[]  PROGMEM = "8X";
static const char GAIN_5[]  PROGMEM = "16X";
static const char GAIN_6[]  PROGMEM = "32X";
static const char GAIN_7[]  PROGMEM = "64X";
static const char GAIN_8[]  PROGMEM = "128X";
static const char GAIN_9[]  PROGMEM = "256X";
static const char GAIN_10[] PROGMEM = "512X";
static const char* const GAIN_NAMES[] PROGMEM = {
  GAIN_0, GAIN_1, GAIN_2, GAIN_3, GAIN_4, GAIN_5, GAIN_6, GAIN_7, GAIN_8, GAIN_9, GAIN_10
};

static const as7341_gain_t GAIN_VALUES[11] = {
  AS7341_GAIN_0_5X, AS7341_GAIN_1X,  AS7341_GAIN_2X,  AS7341_GAIN_4X,
  AS7341_GAIN_8X,   AS7341_GAIN_16X, AS7341_GAIN_32X, AS7341_GAIN_64X,
  AS7341_GAIN_128X, AS7341_GAIN_256X, AS7341_GAIN_512X
};

static const as7341_color_channel_t CHANNEL_VALUES[8] = {
  AS7341_CHANNEL_415nm_F1, AS7341_CHANNEL_445nm_F2, AS7341_CHANNEL_480nm_F3,
  AS7341_CHANNEL_515nm_F4, AS7341_CHANNEL_555nm_F5, AS7341_CHANNEL_590nm_F6,
  AS7341_CHANNEL_630nm_F7, AS7341_CHANNEL_680nm_F8
};

bool AS7341Sensor::begin() {
  if (!sensor_.begin()) return false;
  applyGainAndTiming();
  return true;
}

const __FlashStringHelper* AS7341Sensor::name() const { return F("AS7341"); }
const __FlashStringHelper* AS7341Sensor::sensorType() const { return F("color"); }

ChannelInfo AS7341Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool AS7341Sensor::readChannels(float* out) {
  if (!sensor_.readAllChannels()) return false;
  for (uint8_t i = 0; i < 8; i++) out[i] = sensor_.getChannel(CHANNEL_VALUES[i]);
  return true;
}

const __FlashStringHelper* AS7341Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void AS7341Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
}

const __FlashStringHelper* AS7341Sensor::integrationFormula() const {
  return F("(ATIME+1) x (ASTEP+1) x 2.78us");
}

float AS7341Sensor::integrationTimeMs() const {
  return (float)(atime_ + 1) * (float)(astep_ + 1) * 2.78e-3f;
}

const char* AS7341Sensor::timeParamKey(uint8_t idx) const {
  return idx == 0 ? "atime" : "astep";
}
const __FlashStringHelper* AS7341Sensor::timeParamLabel(uint8_t idx) const {
  return idx == 0 ? F("ATIME") : F("ASTEP");
}
int32_t AS7341Sensor::timeParamMin(uint8_t) const { return 0; }
int32_t AS7341Sensor::timeParamMax(uint8_t idx) const { return idx == 0 ? 255 : 65535; }

void AS7341Sensor::setTimeParam(uint8_t idx, int32_t value) {
  if (idx == 0) atime_ = value; else astep_ = value;
  applyGainAndTiming();
}

void AS7341Sensor::setLedOn(bool on) {
  sensor_.enableLED(on);
}

void AS7341Sensor::setLedContinuous(int mA) {
  sensor_.setLEDCurrent((uint16_t)mA);
}

void AS7341Sensor::applyGainAndTiming() {
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
  sensor_.setATIME((uint8_t)atime_);
  sensor_.setASTEP((uint16_t)astep_);
}
