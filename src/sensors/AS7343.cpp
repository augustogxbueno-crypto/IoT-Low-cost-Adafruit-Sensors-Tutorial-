#include "AS7343.h"
#include "../core/Utils.h"

// NOTE: AS7343's readAllChannels() fills a flat buffer indexed by the
// AS7343_CHANNEL_* constants (unlike AS7341, which reads one named channel
// at a time). This mirrors the reference sketch this driver was ported from.
static uint16_t g_rawBuffer[18]; // library reads into a buffer sized for its internal channel count

static const char CH_NAME_0[]  PROGMEM = "405nm";
static const char CH_NAME_1[]  PROGMEM = "425nm";
static const char CH_NAME_2[]  PROGMEM = "450nm";
static const char CH_NAME_3[]  PROGMEM = "475nm";
static const char CH_NAME_4[]  PROGMEM = "515nm";
static const char CH_NAME_5[]  PROGMEM = "550nm";
static const char CH_NAME_6[]  PROGMEM = "555nm";
static const char CH_NAME_7[]  PROGMEM = "600nm";
static const char CH_NAME_8[]  PROGMEM = "640nm";
static const char CH_NAME_9[]  PROGMEM = "690nm";
static const char CH_NAME_10[] PROGMEM = "745nm";
static const char CH_NAME_11[] PROGMEM = "855nm";
static const char CH_NAME_12[] PROGMEM = "Clear";
static const char* const CH_NAMES[] PROGMEM = {
  CH_NAME_0, CH_NAME_1, CH_NAME_2, CH_NAME_3, CH_NAME_4, CH_NAME_5, CH_NAME_6,
  CH_NAME_7, CH_NAME_8, CH_NAME_9, CH_NAME_10, CH_NAME_11, CH_NAME_12
};

static const char CH_COLOR_0[]  PROGMEM = "#8B00FF";
static const char CH_COLOR_1[]  PROGMEM = "#6A00FF";
static const char CH_COLOR_2[]  PROGMEM = "#0000FF";
static const char CH_COLOR_3[]  PROGMEM = "#00BFFF";
static const char CH_COLOR_4[]  PROGMEM = "#00C000";
static const char CH_COLOR_5[]  PROGMEM = "#ADFF2F";
static const char CH_COLOR_6[]  PROGMEM = "#CCFF00";
static const char CH_COLOR_7[]  PROGMEM = "#FFA500";
static const char CH_COLOR_8[]  PROGMEM = "#FF3300";
static const char CH_COLOR_9[]  PROGMEM = "#CC0000";
static const char CH_COLOR_10[] PROGMEM = "#800000";
static const char CH_COLOR_11[] PROGMEM = "#400000";
static const char CH_COLOR_12[] PROGMEM = "#FFFFFF";
static const char* const CH_COLORS[] PROGMEM = {
  CH_COLOR_0, CH_COLOR_1, CH_COLOR_2, CH_COLOR_3, CH_COLOR_4, CH_COLOR_5, CH_COLOR_6,
  CH_COLOR_7, CH_COLOR_8, CH_COLOR_9, CH_COLOR_10, CH_COLOR_11, CH_COLOR_12
};

// Index into g_rawBuffer for each of our 13 reported channels, in the same
// wavelength order as the reference sketch's print statements.
static const uint8_t CHANNEL_INDEX[13] = {
  AS7343_CHANNEL_F1, AS7343_CHANNEL_F2, AS7343_CHANNEL_FZ, AS7343_CHANNEL_F3,
  AS7343_CHANNEL_F4, AS7343_CHANNEL_F5, AS7343_CHANNEL_FY, AS7343_CHANNEL_FXL,
  AS7343_CHANNEL_F6, AS7343_CHANNEL_F7, AS7343_CHANNEL_F8, AS7343_CHANNEL_NIR,
  AS7343_CHANNEL_VIS_TL_0
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
static const char GAIN_11[] PROGMEM = "1024X";
static const char GAIN_12[] PROGMEM = "2048X";
static const char* const GAIN_NAMES[] PROGMEM = {
  GAIN_0, GAIN_1, GAIN_2, GAIN_3, GAIN_4, GAIN_5, GAIN_6,
  GAIN_7, GAIN_8, GAIN_9, GAIN_10, GAIN_11, GAIN_12
};

static const as7343_gain_t GAIN_VALUES[13] = {
  AS7343_GAIN_0_5X, AS7343_GAIN_1X,   AS7343_GAIN_2X,   AS7343_GAIN_4X,
  AS7343_GAIN_8X,   AS7343_GAIN_16X,  AS7343_GAIN_32X,  AS7343_GAIN_64X,
  AS7343_GAIN_128X, AS7343_GAIN_256X, AS7343_GAIN_512X, AS7343_GAIN_1024X,
  AS7343_GAIN_2048X
};

bool AS7343Sensor::begin() {
  if (!sensor_.begin()) return false;
  applyGainAndTiming();
  return true;
}

const __FlashStringHelper* AS7343Sensor::name() const { return F("AS7343"); }
const __FlashStringHelper* AS7343Sensor::sensorType() const { return F("color"); }

ChannelInfo AS7343Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool AS7343Sensor::readChannels(float* out) {
  if (!sensor_.readAllChannels(g_rawBuffer)) return false;
  for (uint8_t i = 0; i < 13; i++) out[i] = g_rawBuffer[CHANNEL_INDEX[i]];
  return true;
}

const __FlashStringHelper* AS7343Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void AS7343Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
}

const __FlashStringHelper* AS7343Sensor::integrationFormula() const {
  return F("(ATIME+1) x (ASTEP+1) x 2.78us");
}

float AS7343Sensor::integrationTimeMs() const {
  return (float)(atime_ + 1) * (float)(astep_ + 1) * 2.78e-3f;
}

const char* AS7343Sensor::timeParamKey(uint8_t idx) const {
  return idx == 0 ? "atime" : "astep";
}
const __FlashStringHelper* AS7343Sensor::timeParamLabel(uint8_t idx) const {
  return idx == 0 ? F("ATIME") : F("ASTEP");
}
int32_t AS7343Sensor::timeParamMin(uint8_t) const { return 0; }
int32_t AS7343Sensor::timeParamMax(uint8_t idx) const { return idx == 0 ? 255 : 65535; }

void AS7343Sensor::setTimeParam(uint8_t idx, int32_t value) {
  if (idx == 0) atime_ = value; else astep_ = value;
  applyGainAndTiming();
}

void AS7343Sensor::setLedOn(bool on) {
  sensor_.enableLED(on);
}

void AS7343Sensor::setLedContinuous(int mA) {
  sensor_.setLEDCurrent((uint16_t)mA);
}

void AS7343Sensor::applyGainAndTiming() {
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
  sensor_.setATIME((uint8_t)atime_);
  sensor_.setASTEP((uint16_t)astep_);
}
