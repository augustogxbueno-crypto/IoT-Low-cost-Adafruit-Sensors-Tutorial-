#include "AS7262.h"
#include "../core/Utils.h"

static const char CH_NAME_0[] PROGMEM = "450nm";
static const char CH_NAME_1[] PROGMEM = "500nm";
static const char CH_NAME_2[] PROGMEM = "550nm";
static const char CH_NAME_3[] PROGMEM = "570nm";
static const char CH_NAME_4[] PROGMEM = "600nm";
static const char CH_NAME_5[] PROGMEM = "650nm";
static const char* const CH_NAMES[] PROGMEM = {
  CH_NAME_0, CH_NAME_1, CH_NAME_2, CH_NAME_3, CH_NAME_4, CH_NAME_5
};

static const char CH_COLOR_0[] PROGMEM = "#7F00FF"; // Violet
static const char CH_COLOR_1[] PROGMEM = "#0000FF"; // Blue
static const char CH_COLOR_2[] PROGMEM = "#00C000"; // Green
static const char CH_COLOR_3[] PROGMEM = "#FFFF00"; // Yellow
static const char CH_COLOR_4[] PROGMEM = "#FFA500"; // Orange
static const char CH_COLOR_5[] PROGMEM = "#FF0000"; // Red
static const char* const CH_COLORS[] PROGMEM = {
  CH_COLOR_0, CH_COLOR_1, CH_COLOR_2, CH_COLOR_3, CH_COLOR_4, CH_COLOR_5
};

static const uint8_t CHANNEL_INDEX[6] = {
  AS726x_VIOLET, AS726x_BLUE, AS726x_GREEN, AS726x_YELLOW, AS726x_ORANGE, AS726x_RED
};

static const char GAIN_0[] PROGMEM = "1X";
static const char GAIN_1[] PROGMEM = "3.7X";
static const char GAIN_2[] PROGMEM = "16X";
static const char GAIN_3[] PROGMEM = "64X";
static const char* const GAIN_NAMES[] PROGMEM = { GAIN_0, GAIN_1, GAIN_2, GAIN_3 };
// NOTE: the real Adafruit_AS726x library declares GAIN_1X etc. inside a
// plain (untypedef'd) `enum channel_gain`, and Adafruit_AS726x::setGain()
// takes a plain uint8_t — there is no "as726x_gain_t" type in the library.
static const uint8_t GAIN_VALUES[4] = { GAIN_1X, GAIN_3X7, GAIN_16X, GAIN_64X };

static const char LED_0[] PROGMEM = "12.5 mA";
static const char LED_1[] PROGMEM = "25 mA";
static const char LED_2[] PROGMEM = "50 mA";
static const char LED_3[] PROGMEM = "100 mA";
static const char* const LED_NAMES[] PROGMEM = { LED_0, LED_1, LED_2, LED_3 };
static const drv_led_current_limits DRV_LIMITS[4] = { LIMIT_12MA5, LIMIT_25MA, LIMIT_50MA, LIMIT_100MA };

bool AS7262Sensor::begin() {
  if (!sensor_.begin()) return false;
  sensor_.setGain(GAIN_16X);
  sensor_.setIntegrationTime(itime_);
  sensor_.setConversionType(MODE_2); // continuous read of all 6 channels
  return true;
}

const __FlashStringHelper* AS7262Sensor::name() const { return F("AS7262"); }
const __FlashStringHelper* AS7262Sensor::sensorType() const { return F("color"); }

ChannelInfo AS7262Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool AS7262Sensor::readChannels(float* out) {
  if (!sensor_.dataReady()) return false;
  float calibrated[AS726x_NUM_CHANNELS];
  sensor_.readCalibratedValues(calibrated);
  for (uint8_t i = 0; i < 6; i++) out[i] = calibrated[CHANNEL_INDEX[i]];
  return true;
}

const __FlashStringHelper* AS7262Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void AS7262Sensor::setGainIndex(uint8_t idx) {
  sensor_.setGain(GAIN_VALUES[idx]);
}

const __FlashStringHelper* AS7262Sensor::integrationFormula() const {
  return F("value x 2.8ms");
}

float AS7262Sensor::integrationTimeMs() const {
  return (float)itime_ * 2.8f;
}

const char* AS7262Sensor::timeParamKey(uint8_t) const { return "itime"; }
const __FlashStringHelper* AS7262Sensor::timeParamLabel(uint8_t) const { return F("Integration Time"); }
int32_t AS7262Sensor::timeParamMin(uint8_t) const { return 0; }
int32_t AS7262Sensor::timeParamMax(uint8_t) const { return 255; }

void AS7262Sensor::setTimeParam(uint8_t, int32_t value) {
  itime_ = (uint8_t)value;
  sensor_.setIntegrationTime(itime_);
}

const __FlashStringHelper* AS7262Sensor::ledOptionLabel(uint8_t idx) const {
  return Utils::flashStr(LED_NAMES, idx);
}

void AS7262Sensor::setLedOn(bool on) {
  if (on) sensor_.drvOn(); else sensor_.drvOff();
}

void AS7262Sensor::setLedDiscrete(uint8_t idx) {
  sensor_.setDrvCurrent(DRV_LIMITS[idx]);
}
