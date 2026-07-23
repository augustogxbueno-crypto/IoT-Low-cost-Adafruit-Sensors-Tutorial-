#include "TCS34725.h"
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
static const char GAIN_3[] PROGMEM = "60X";
static const char* const GAIN_NAMES[] PROGMEM = { GAIN_0, GAIN_1, GAIN_2, GAIN_3 };
static const tcs34725Gain_t GAIN_VALUES[4] = {
  TCS34725_GAIN_1X, TCS34725_GAIN_4X, TCS34725_GAIN_16X, TCS34725_GAIN_60X
};

static const char TIME_0[] PROGMEM = "2.4 ms";
static const char TIME_1[] PROGMEM = "24 ms";
static const char TIME_2[] PROGMEM = "50 ms";
static const char TIME_3[] PROGMEM = "101 ms";
static const char TIME_4[] PROGMEM = "154 ms";
static const char TIME_5[] PROGMEM = "614 ms";
static const char* const TIME_NAMES[] PROGMEM = {
  TIME_0, TIME_1, TIME_2, TIME_3, TIME_4, TIME_5
};
// NOTE: the real Adafruit_TCS34725 library exposes these presets as plain
// #define register values (not an enum), so setIntegrationTime() takes a
// bare uint8_t — there is no "tcs34725IntegrationTime_t" type. Also, the
// library's longest preset is 614.4ms (TCS34725_INTEGRATIONTIME_614MS);
// TCS34725_INTEGRATIONTIME_700MS doesn't exist.
static const uint8_t TIME_VALUES[6] = {
  TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_INTEGRATIONTIME_24MS,  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_INTEGRATIONTIME_101MS, TCS34725_INTEGRATIONTIME_154MS, TCS34725_INTEGRATIONTIME_614MS
};
static const float TIME_MS[6] = { 2.4f, 24.0f, 50.0f, 101.0f, 154.0f, 614.4f };

bool TCS34725Sensor::begin() {
  if (!sensor_.begin()) return false;
  sensor_.setIntegrationTime(TIME_VALUES[timeIdx_]);
  sensor_.setGain(GAIN_VALUES[gainIdx_]);

  pinMode(PIN_LED_INTERNAL, OUTPUT);
  digitalWrite(PIN_LED_INTERNAL, LOW);
  return true;
}

const __FlashStringHelper* TCS34725Sensor::name() const { return F("TCS34725"); }
const __FlashStringHelper* TCS34725Sensor::sensorType() const { return F("color"); }

ChannelInfo TCS34725Sensor::channel(uint8_t idx) const {
  return { Utils::flashStr(CH_NAMES, idx), Utils::flashStr(CH_COLORS, idx) };
}

bool TCS34725Sensor::readChannels(float* out) {
  uint16_t r, g, b, c;
  sensor_.getRawData(&r, &g, &b, &c);
  out[0] = r;
  out[1] = g;
  out[2] = b;
  out[3] = c;
  out[4] = sensor_.calculateColorTemperature(r, g, b);
  out[5] = sensor_.calculateLux(r, g, b);
  return true;
}

const __FlashStringHelper* TCS34725Sensor::gainOptionLabel(uint8_t idx) const {
  return Utils::flashStr(GAIN_NAMES, idx);
}

void TCS34725Sensor::setGainIndex(uint8_t idx) {
  gainIdx_ = idx;
  sensor_.setGain(GAIN_VALUES[gainIdx_]);
}

float TCS34725Sensor::integrationTimeMs() const {
  return TIME_MS[timeIdx_];
}

const __FlashStringHelper* TCS34725Sensor::timePresetLabel(uint8_t idx) const {
  return Utils::flashStr(TIME_NAMES, idx);
}

void TCS34725Sensor::setTimePreset(uint8_t idx) {
  timeIdx_ = idx;
  sensor_.setIntegrationTime(TIME_VALUES[timeIdx_]);
}

void TCS34725Sensor::setLedOn(bool on) {
  // The Adafruit_TCS34725 library's setInterrupt() only toggles the onboard
  // LED if you've soldered the breakout's LED pin to its INT pin — a fragile,
  // optional hardware mod most builds won't have. Instead we drive the LED
  // pin directly from a dedicated GPIO (wire LED -> pin 7), the same
  // approach already used for the fixed external LEDs on pins 5/6, and
  // toggled by the same generic "enter/leave Reflectance mode" logic in
  // core/Protocol.cpp — no core changes needed for this to work.
  digitalWrite(PIN_LED_INTERNAL, on ? HIGH : LOW);
}
