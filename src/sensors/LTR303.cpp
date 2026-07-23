#include "LTR303.h"
#include "../core/Utils.h"

static const char CH_NAME_0[] PROGMEM = "CH0 (Vis+IR)";
static const char CH_NAME_1[] PROGMEM = "CH1 (IR)";
static const char CH_NAME_2[] PROGMEM = "Visible (est.)";
static const char* const CH_NAMES[] PROGMEM = { CH_NAME_0, CH_NAME_1, CH_NAME_2 };

static const char CH_COLOR_0[] PROGMEM = "#CCCCCC";
static const char CH_COLOR_1[] PROGMEM = "#800000";
static const char CH_COLOR_2[] PROGMEM = "#FFFF66";
static const char* const CH_COLORS[] PROGMEM = { CH_COLOR_0, CH_COLOR_1, CH_COLOR_2 };

static const char GAIN_0[] PROGMEM = "1X";
static const char GAIN_1[] PROGMEM = "2X";
static const char GAIN_2[] PROGMEM = "4X";
static const char GAIN_3[] PROGMEM = "8X";
static const char GAIN_4[] PROGMEM = "48X";
static const char GAIN_5[] PROGMEM = "96X";
static const char* const GAIN_NAMES[] PROGMEM = { GAIN_0, GAIN_1, GAIN_2, GAIN_3, GAIN_4, GAIN_5 };
// NOTE: the real Adafruit_LTR329_LTR303 library names these typedefs with an
// "ltr329_" prefix even for the LTR303 (Adafruit_LTR303 inherits the same
// methods/types from Adafruit_LTR329) — "ltr3xx_*" isn't a real type name,
// only the enum *constants* (LTR3XX_GAIN_1 etc.) use that prefix.
static const ltr329_gain_t GAIN_VALUES[6] = {
  LTR3XX_GAIN_1, LTR3XX_GAIN_2, LTR3XX_GAIN_4, LTR3XX_GAIN_8, LTR3XX_GAIN_48, LTR3XX_GAIN_96
};

static const char TIME_0[] PROGMEM = "50 ms";
static const char TIME_1[] PROGMEM = "100 ms";
static const char TIME_2[] PROGMEM = "150 ms";
static const char TIME_3[] PROGMEM = "200 ms";
static const char TIME_4[] PROGMEM = "250 ms";
static const char TIME_5[] PROGMEM = "300 ms";
static const char TIME_6[] PROGMEM = "350 ms";
static const char TIME_7[] PROGMEM = "400 ms";
static const char* const TIME_NAMES[] PROGMEM = {
  TIME_0, TIME_1, TIME_2, TIME_3, TIME_4, TIME_5, TIME_6, TIME_7
};
static const ltr329_integrationtime_t TIME_VALUES[8] = {
  LTR3XX_INTEGTIME_50,  LTR3XX_INTEGTIME_100, LTR3XX_INTEGTIME_150, LTR3XX_INTEGTIME_200,
  LTR3XX_INTEGTIME_250, LTR3XX_INTEGTIME_300, LTR3XX_INTEGTIME_350, LTR3XX_INTEGTIME_400
};
static const float TIME_MS[8] = { 50, 100, 150, 200, 250, 300, 350, 400 };

static const char RATE_0[] PROGMEM = "50 ms";
static const char RATE_1[] PROGMEM = "100 ms";
static const char RATE_2[] PROGMEM = "200 ms";
static const char RATE_3[] PROGMEM = "500 ms";
static const char RATE_4[] PROGMEM = "1000 ms";
static const char RATE_5[] PROGMEM = "2000 ms";
static const char* const RATE_NAMES[] PROGMEM = {
  RATE_0, RATE_1, RATE_2, RATE_3, RATE_4, RATE_5
};
static const ltr329_measurerate_t RATE_VALUES[6] = {
  LTR3XX_MEASRATE_50,  LTR3XX_MEASRATE_100,  LTR3XX_MEASRATE_200,
  LTR3XX_MEASRATE_500, LTR3XX_MEASRATE_1000, LTR3XX_MEASRATE_2000
};

bool LTR303Sensor::begin() {
  if (!sensor_.begin()) return false;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
  sensor_.setIntegrationTime(TIME_VALUES[timeIdx_]);
  sensor_.setMeasurementRate(RATE_VALUES[rateIdx_]);
  return true;
}

const __FlashStringHelper* LTR303Sensor::name() const { return F("LTR303"); }
const __FlashStringHelper* LTR303Sensor::sensorType() const { return F("photodiode"); }

ChannelInfo LTR303Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool LTR303Sensor::readChannels(float* out) {
  if (!sensor_.newDataAvailable()) return false;
  uint16_t visIr, ir;
  if (!sensor_.readBothChannels(visIr, ir)) return false;
  out[0] = visIr;
  out[1] = ir;
  out[2] = (visIr > ir) ? (visIr - ir) : 0;
  return true;
}

const __FlashStringHelper* LTR303Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void LTR303Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
}

float LTR303Sensor::integrationTimeMs() const { return TIME_MS[timeIdx_]; }

const __FlashStringHelper* LTR303Sensor::timePresetLabel(uint8_t idx) const {
  return Utils::flashStr(TIME_NAMES, idx);
}

void LTR303Sensor::setTimePreset(uint8_t idx) {
  timeIdx_ = idx;
  sensor_.setIntegrationTime(TIME_VALUES[timeIdx_]);
}

ExtraParamInfo LTR303Sensor::extraParam(uint8_t) const {
  return { "measRate", F("Measurement Rate"), (const __FlashStringHelper* const*)RATE_NAMES, 6, 0, 0 };
}

void LTR303Sensor::setExtraParam(uint8_t, int value) {
  if (value < 0 || value >= 6) return;
  rateIdx_ = (uint8_t)value;
  sensor_.setMeasurementRate(RATE_VALUES[rateIdx_]);
}
