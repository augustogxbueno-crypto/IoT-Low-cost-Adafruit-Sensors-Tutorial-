#include "APDS9960.h"
#include "../core/Utils.h"

static const char CH_NAME_0[] PROGMEM = "Red";
static const char CH_NAME_1[] PROGMEM = "Green";
static const char CH_NAME_2[] PROGMEM = "Blue";
static const char CH_NAME_3[] PROGMEM = "Clear";
static const char CH_NAME_4[] PROGMEM = "Color Temp (K)";
static const char CH_NAME_5[] PROGMEM = "Lux";
static const char* const CH_NAMES[] PROGMEM = {
  CH_NAME_0, CH_NAME_1, CH_NAME_2, CH_NAME_3, CH_NAME_4, CH_NAME_5
};

static const char CH_COLOR_0[] PROGMEM = "#FF0000";
static const char CH_COLOR_1[] PROGMEM = "#00C000";
static const char CH_COLOR_2[] PROGMEM = "#0000FF";
static const char CH_COLOR_3[] PROGMEM = "#FFFFFF";
static const char CH_COLOR_4[] PROGMEM = "#FFA500"; // derived, no real wavelength
static const char CH_COLOR_5[] PROGMEM = "#CCCCCC"; // derived, no real wavelength
static const char* const CH_COLORS[] PROGMEM = {
  CH_COLOR_0, CH_COLOR_1, CH_COLOR_2, CH_COLOR_3, CH_COLOR_4, CH_COLOR_5
};

static const char GAIN_0[] PROGMEM = "1X";
static const char GAIN_1[] PROGMEM = "4X";
static const char GAIN_2[] PROGMEM = "16X";
static const char GAIN_3[] PROGMEM = "64X";
static const char* const GAIN_NAMES[] PROGMEM = { GAIN_0, GAIN_1, GAIN_2, GAIN_3 };
static const apds9960AGain_t GAIN_VALUES[4] = {
  APDS9960_AGAIN_1X, APDS9960_AGAIN_4X, APDS9960_AGAIN_16X, APDS9960_AGAIN_64X
};

bool APDS9960Sensor::begin() {
  // Adafruit_APDS9960::begin(iTimeMS, gain, addr, wire) also configures
  // proximity/gesture defaults internally, but leaves them disabled — only
  // enableColor(true) below turns on the ALS/color engine this project uses.
  if (!sensor_.begin(itimeMs_, GAIN_VALUES[gainIdx_])) return false;
  sensor_.enableColor(true);
  return true;
}

const __FlashStringHelper* APDS9960Sensor::name() const { return F("APDS9960"); }
const __FlashStringHelper* APDS9960Sensor::sensorType() const { return F("color"); }

ChannelInfo APDS9960Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool APDS9960Sensor::readChannels(float* out) {
  if (!sensor_.colorDataReady()) return false;
  uint16_t r, g, b, c;
  sensor_.getColorData(&r, &g, &b, &c);
  out[0] = r;
  out[1] = g;
  out[2] = b;
  out[3] = c;
  out[4] = sensor_.calculateColorTemperature(r, g, b);
  out[5] = sensor_.calculateLux(r, g, b);
  return true;
}

const __FlashStringHelper* APDS9960Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void APDS9960Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setADCGain(GAIN_VALUES[gainIdx_]);
}

const __FlashStringHelper* APDS9960Sensor::integrationFormula() const {
  return F("value ms (direct)");
}

float APDS9960Sensor::integrationTimeMs() const { return (float)itimeMs_; }

const char* APDS9960Sensor::timeParamKey(uint8_t) const { return "itime"; }
const __FlashStringHelper* APDS9960Sensor::timeParamLabel(uint8_t) const {
  return F("Integration Time (ms)");
}
// Library range: ATIME = 256 - iTimeMS/2.78, clamped to a uint8_t register,
// so the practical millisecond range is roughly 3-700ms.
int32_t APDS9960Sensor::timeParamMin(uint8_t) const { return 3; }
int32_t APDS9960Sensor::timeParamMax(uint8_t) const { return 700; }

void APDS9960Sensor::setTimeParam(uint8_t, int32_t value) {
  itimeMs_ = (uint16_t)value;
  sensor_.setADCIntegrationTime(itimeMs_);
}
