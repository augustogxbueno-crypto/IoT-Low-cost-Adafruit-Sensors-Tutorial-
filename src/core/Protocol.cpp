#include "Protocol.h"
#include "Pins.h"
#include "Utils.h"
#include "SensorManager.h"
#include "Measurement.h"
#include <ArduinoJson.h>

// -----------------------------------------------------------------------------
// Per-sensor worst-case sizes for the "info" document.
// ArduinoJson's StaticJsonDocument allocates a FIXED buffer on the stack, so
// oversizing it just wastes RAM instead of causing a crash — but on a 2KB
// AVR board that waste matters, so each sensor gets a number that actually
// matches it instead of one giant number shared by all nine.
// (AS7341/AS7343 remain the tightest fits on a bare Uno; see README.md.)
// -----------------------------------------------------------------------------
#if defined(SENSOR_AS7343)
  constexpr uint8_t INFO_MAX_CHANNELS = 13;
  constexpr uint8_t INFO_MAX_GAINS    = 13;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 2;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 0;
#elif defined(SENSOR_AS7341)
  constexpr uint8_t INFO_MAX_CHANNELS = 8;
  constexpr uint8_t INFO_MAX_GAINS    = 11;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 2;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 0;
#elif defined(SENSOR_AS7262)
  constexpr uint8_t INFO_MAX_CHANNELS = 6;
  constexpr uint8_t INFO_MAX_GAINS    = 4;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 1;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 0;
#elif defined(SENSOR_TCS34725)
  constexpr uint8_t INFO_MAX_CHANNELS = 6; // 4 raw (R,G,B,C) + 2 derived (colorTemp, lux)
  constexpr uint8_t INFO_MAX_GAINS    = 4;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 0;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 6;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 0;
#elif defined(SENSOR_BH1750)
  constexpr uint8_t INFO_MAX_CHANNELS = 1;
  constexpr uint8_t INFO_MAX_GAINS    = 1; // continuous gain, array unused
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 0;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 3;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 0;
#elif defined(SENSOR_LTR303) || defined(SENSOR_LTR329)
  constexpr uint8_t INFO_MAX_CHANNELS = 3; // CH0 (Vis+IR), CH1 (IR), derived Visible
  constexpr uint8_t INFO_MAX_GAINS    = 6;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 0;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 8;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 1; // Measurement Rate
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 6;
#elif defined(SENSOR_LTR390)
  constexpr uint8_t INFO_MAX_CHANNELS = 1; // UV or ALS, depending on the "mode" extra param
  constexpr uint8_t INFO_MAX_GAINS    = 5;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 0;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 6;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 1; // ALS/UVS mode
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 2;
#elif defined(SENSOR_TSL2591)
  constexpr uint8_t INFO_MAX_CHANNELS = 4; // Full, IR, derived Visible, derived Lux
  constexpr uint8_t INFO_MAX_GAINS    = 4;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 0;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 6;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 0;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 0;
#else
  constexpr uint8_t INFO_MAX_CHANNELS = 14;
  constexpr uint8_t INFO_MAX_GAINS    = 16;
  constexpr uint8_t INFO_MAX_TIME_PARAMS  = 2;
  constexpr uint8_t INFO_MAX_TIME_PRESETS = 8;
  constexpr uint8_t INFO_MAX_EXTRA_PARAMS = 3;
  constexpr uint8_t INFO_MAX_EXTRA_OPTS   = 6;
#endif

constexpr size_t INFO_DOC_CAPACITY =
    JSON_OBJECT_SIZE(9) +                                                  // top-level fields
    JSON_ARRAY_SIZE(INFO_MAX_CHANNELS) + INFO_MAX_CHANNELS * JSON_OBJECT_SIZE(3) + // channels[]
    JSON_ARRAY_SIZE(INFO_MAX_GAINS) +                                       // gain.options[] (discrete)
    JSON_OBJECT_SIZE(4) +                                                   // gain{}
    JSON_OBJECT_SIZE(3) +                                                   // led{}
    JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(INFO_MAX_GAINS) +                 // led.internal{} (+ options[])
    2 * JSON_OBJECT_SIZE(3) +                                               // led.absorbance / led.fluorescence
    JSON_OBJECT_SIZE(3) +                                                   // time{}
    JSON_ARRAY_SIZE(INFO_MAX_TIME_PARAMS) + INFO_MAX_TIME_PARAMS * JSON_OBJECT_SIZE(4) +  // time.params[]
    JSON_ARRAY_SIZE(INFO_MAX_TIME_PRESETS) +                                // time.presets[]
    JSON_ARRAY_SIZE(INFO_MAX_EXTRA_PARAMS) +
    INFO_MAX_EXTRA_PARAMS * (JSON_OBJECT_SIZE(5) + JSON_ARRAY_SIZE(INFO_MAX_EXTRA_OPTS)) + // extraParams[]
    96; // margin for short literal values

constexpr size_t CMD_DOC_CAPACITY =
    JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(INFO_MAX_CHANNELS) + JSON_OBJECT_SIZE(INFO_MAX_TIME_PARAMS) + 64;

constexpr size_t PROGRESS_DOC_CAPACITY =
    JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(INFO_MAX_CHANNELS) + 24;

constexpr size_t RESULT_DOC_CAPACITY =
    JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(INFO_MAX_CHANNELS) + 24;

constexpr size_t SMALL_DOC_CAPACITY = JSON_OBJECT_SIZE(3) + 32;

namespace Protocol {

// -----------------------------------------------------------------------------
// Small send helpers
// -----------------------------------------------------------------------------
static void sendJson(JsonDocument& doc) {
  serializeJson(doc, Serial);
  Serial.print('\n');
}

static void sendError(const __FlashStringHelper* msg) {
  StaticJsonDocument<SMALL_DOC_CAPACITY> doc;
  doc["evt"] = F("error");
  doc["msg"] = msg;
  sendJson(doc);
}

static void sendAck(const char* cmd, float tintMs = -1) {
  StaticJsonDocument<SMALL_DOC_CAPACITY> doc;
  doc["evt"] = F("ack");
  doc["cmd"] = cmd;
  if (tintMs >= 0) doc["tint_ms"] = tintMs;
  sendJson(doc);
}

// -----------------------------------------------------------------------------
// LED application: decides, from the current mode + the active sensor's
// ledType(), what the sensor's own LED and the two fixed external LEDs
// should be doing right now.
// -----------------------------------------------------------------------------
static int     g_ledMA  = 20;
static uint8_t g_ledIdx = 0;

static void applyLedForMode() {
  SensorBase& sensor = SensorManager::get();

  if (Measurement::mode == Measurement::Mode::REFLECTANCE) {
    digitalWrite(PIN_LED_ABSORBANCE, LOW);
    digitalWrite(PIN_LED_FLUORESCENCE, LOW);
    switch (sensor.ledType()) {
      case LedType::LED_CONTINUOUS: sensor.setLedContinuous(g_ledMA); sensor.setLedOn(true); break;
      case LedType::LED_DISCRETE:   sensor.setLedDiscrete(g_ledIdx);  sensor.setLedOn(true); break;
      case LedType::LED_BINARY:     sensor.setLedOn(true);                                    break;
      case LedType::LED_NONE:       break; // shouldn't be reachable from the UI
    }
  } else if (Measurement::mode == Measurement::Mode::ABSORBANCE) {
    if (sensor.ledType() != LedType::LED_NONE) sensor.setLedOn(false);
    digitalWrite(PIN_LED_FLUORESCENCE, LOW);
    digitalWrite(PIN_LED_ABSORBANCE, HIGH);
  } else { // FLUORESCENCE
    if (sensor.ledType() != LedType::LED_NONE) sensor.setLedOn(false);
    digitalWrite(PIN_LED_ABSORBANCE, LOW);
    digitalWrite(PIN_LED_FLUORESCENCE, HIGH);
  }
}

// -----------------------------------------------------------------------------
// "info" — full self-description of the active sensor
// -----------------------------------------------------------------------------
void sendInfo() {
  SensorBase& sensor = SensorManager::get();
  StaticJsonDocument<INFO_DOC_CAPACITY> doc;

  doc["evt"]        = F("info");
  doc["sensor"]     = sensor.name();
  doc["sensorType"] = sensor.sensorType();

  // Channels
  JsonArray channels = doc.createNestedArray("channels");
  for (uint8_t i = 0; i < sensor.channelCount(); i++) {
    ChannelInfo ci = sensor.channel(i);
    JsonObject ch = channels.createNestedObject();
    ch["id"]    = i;
    ch["name"]  = ci.name;
    ch["color"] = ci.color;
  }

  // Modes available: Reflectance only if the sensor can drive its own LED
  JsonArray modes = doc.createNestedArray("modes");
  if (sensor.ledType() != LedType::LED_NONE) modes.add(F("reflectance"));
  modes.add(F("absorbance"));
  modes.add(F("fluorescence"));

  // Gain
  JsonObject gain = doc.createNestedObject("gain");
  if (sensor.gainType() == GainType::GAIN_DISCRETE) {
    gain["type"] = F("discrete");
    JsonArray opts = gain.createNestedArray("options");
    for (uint8_t i = 0; i < sensor.gainOptionCount(); i++) opts.add(sensor.gainOptionLabel(i));
  } else {
    gain["type"] = F("continuous");
    gain["min"]  = sensor.gainContinuousMin();
    gain["max"]  = sensor.gainContinuousMax();
  }

  // LED
  JsonObject led = doc.createNestedObject("led");
  JsonObject ledInternal = led.createNestedObject("internal");
  switch (sensor.ledType()) {
    case LedType::LED_NONE:
      ledInternal["type"] = F("none");
      break;
    case LedType::LED_BINARY:
      ledInternal["type"] = F("binary");
      break;
    case LedType::LED_DISCRETE: {
      ledInternal["type"] = F("discrete");
      JsonArray opts = ledInternal.createNestedArray("options");
      for (uint8_t i = 0; i < sensor.ledOptionCount(); i++) opts.add(sensor.ledOptionLabel(i));
      break;
    }
    case LedType::LED_CONTINUOUS:
      ledInternal["type"] = F("continuous");
      ledInternal["minMA"] = sensor.ledMinMA();
      ledInternal["maxMA"] = sensor.ledMaxMA();
      break;
  }
  JsonObject ledAbs = led.createNestedObject("absorbance");
  ledAbs["type"]  = F("external");
  ledAbs["pin"]   = PIN_LED_ABSORBANCE;
  ledAbs["angle"] = 180;
  JsonObject ledFlu = led.createNestedObject("fluorescence");
  ledFlu["type"]  = F("external");
  ledFlu["pin"]   = PIN_LED_FLUORESCENCE;
  ledFlu["angle"] = 90;

  // Integration time
  JsonObject time = doc.createNestedObject("time");
  time["currentMs"] = sensor.integrationTimeMs();
  if (sensor.integrationFormula()) time["formula"] = sensor.integrationFormula();

  if (sensor.timeType() == TimeType::TIME_FORMULA) {
    time["type"] = F("formula");
    JsonArray params = time.createNestedArray("params");
    for (uint8_t i = 0; i < sensor.timeParamCount(); i++) {
      JsonObject p = params.createNestedObject();
      p["key"]   = sensor.timeParamKey(i);
      p["label"] = sensor.timeParamLabel(i);
      p["min"]   = sensor.timeParamMin(i);
      p["max"]   = sensor.timeParamMax(i);
    }
  } else {
    time["type"] = F("presets");
    JsonArray presets = time.createNestedArray("presets");
    for (uint8_t i = 0; i < sensor.timePresetCount(); i++) presets.add(sensor.timePresetLabel(i));
  }

  // Extra sensor-specific parameters
  if (sensor.extraParamCount() > 0) {
    JsonArray extras = doc.createNestedArray("extraParams");
    for (uint8_t i = 0; i < sensor.extraParamCount(); i++) {
      ExtraParamInfo ep = sensor.extraParam(i);
      JsonObject e = extras.createNestedObject();
      e["key"]   = ep.key;
      e["label"] = ep.label;
      if (ep.options) {
        e["type"] = F("select");
        JsonArray opts = e.createNestedArray("options");
        for (uint8_t j = 0; j < ep.optionCount; j++) opts.add(Utils::flashStr((const char* const*)ep.options, j));
      } else {
        e["type"] = F("number");
        e["min"]  = ep.numMin;
        e["max"]  = ep.numMax;
      }
    }
  }

  sendJson(doc);
}

// -----------------------------------------------------------------------------
// Measurement progress -> "progress" event
// -----------------------------------------------------------------------------
static void onProgress(float elapsedSec, int sampleCount, const float* sampleValues) {
  StaticJsonDocument<PROGRESS_DOC_CAPACITY> doc;
  doc["evt"] = F("progress");
  doc["t"]   = elapsedSec;
  doc["n"]   = sampleCount;
  JsonObject data = doc.createNestedObject("data");

  char key[3];
  for (uint8_t i = 0; i < Measurement::MAX_CHANNELS; i++) {
    if (!Measurement::selectedChannels[i]) continue;
    Utils::smallIndexKey(i, key);
    data[key] = sampleValues[i];
  }
  sendJson(doc);
}

static void runAndSendMeasurement(bool isReference) {
  float avg[Measurement::MAX_CHANNELS];
  bool usedLog = false;

  bool ok = Measurement::run(isReference, onProgress, avg, &usedLog);
  if (!ok) {
    sendError(Measurement::lastError());
    return;
  }

  SensorBase& sensor = SensorManager::get();
  StaticJsonDocument<RESULT_DOC_CAPACITY> doc;
  doc["evt"] = isReference ? F("ref_saved") : F("result");
  if (!isReference) {
    switch (Measurement::mode) {
      case Measurement::Mode::REFLECTANCE:  doc["mode"] = F("reflectance");  break;
      case Measurement::Mode::ABSORBANCE:   doc["mode"] = F("absorbance");   break;
      case Measurement::Mode::FLUORESCENCE: doc["mode"] = F("fluorescence"); break;
    }
    doc["sub"] = usedLog ? F("log") : F("raw");
  }

  JsonObject data = doc.createNestedObject("data");
  char key[3];
  for (uint8_t i = 0; i < Measurement::MAX_CHANNELS; i++) {
    if (!Measurement::selectedChannels[i]) continue;
    Utils::smallIndexKey(i, key);
    data[key] = avg[i];
  }

  if (!isReference) {
    doc["tint_ms"] = sensor.integrationTimeMs();
  }

  sendJson(doc);
}

// -----------------------------------------------------------------------------
// Command dispatch
// -----------------------------------------------------------------------------
void handleCommand(const char* line) {
  StaticJsonDocument<CMD_DOC_CAPACITY> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    sendError(F("Invalid JSON"));
    return;
  }

  const char* cmd = doc["cmd"] | "";
  SensorBase& sensor = SensorManager::get();

  if (strcmp(cmd, "get_info") == 0) {
    sendInfo();

  } else if (strcmp(cmd, "set_mode") == 0) {
    const char* m = doc["mode"] | "reflectance";
    if      (strcmp(m, "reflectance")  == 0) Measurement::setMode(Measurement::Mode::REFLECTANCE);
    else if (strcmp(m, "absorbance")   == 0) Measurement::setMode(Measurement::Mode::ABSORBANCE);
    else if (strcmp(m, "fluorescence") == 0) Measurement::setMode(Measurement::Mode::FLUORESCENCE);
    else { sendError(F("Unknown mode")); return; }
    applyLedForMode();
    sendAck("set_mode");

  } else if (strcmp(cmd, "set_submode") == 0) {
    const char* s = doc["sub"] | "raw";
    Measurement::setSubmode(strcmp(s, "log") == 0 ? Measurement::Submode::LOG : Measurement::Submode::RAW);
    sendAck("set_submode");

  } else if (strcmp(cmd, "set_led") == 0) {
    switch (sensor.ledType()) {
      case LedType::LED_BINARY: {
        bool on = doc["on"] | false;
        if (Measurement::mode == Measurement::Mode::REFLECTANCE) sensor.setLedOn(on);
        break;
      }
      case LedType::LED_DISCRETE: {
        int idx = doc["idx"] | 0;
        if (idx >= 0 && idx < sensor.ledOptionCount()) {
          g_ledIdx = (uint8_t)idx;
          if (Measurement::mode == Measurement::Mode::REFLECTANCE) {
            sensor.setLedDiscrete(g_ledIdx);
            sensor.setLedOn(true);
          }
        }
        break;
      }
      case LedType::LED_CONTINUOUS: {
        int mA = doc["current"] | sensor.ledMinMA();
        mA = constrain(mA, sensor.ledMinMA(), sensor.ledMaxMA());
        g_ledMA = mA;
        if (Measurement::mode == Measurement::Mode::REFLECTANCE) {
          sensor.setLedContinuous(g_ledMA);
          sensor.setLedOn(true);
        }
        break;
      }
      case LedType::LED_NONE:
        sendError(F("This sensor has no controllable LED"));
        return;
    }
    sendAck("set_led");

  } else if (strcmp(cmd, "set_gain") == 0) {
    if (sensor.gainType() == GainType::GAIN_DISCRETE) {
      int idx = doc["idx"] | 0;
      if (idx < 0 || idx >= sensor.gainOptionCount()) { sendError(F("Gain out of range")); return; }
      sensor.setGainIndex((uint8_t)idx);
    } else {
      float v = doc["value"] | sensor.gainContinuousMin();
      v = constrain(v, sensor.gainContinuousMin(), sensor.gainContinuousMax());
      sensor.setGainContinuous(v);
    }
    sendAck("set_gain");

  } else if (strcmp(cmd, "set_time") == 0) {
    if (sensor.timeType() == TimeType::TIME_FORMULA) {
      JsonObject params = doc["params"];
      for (uint8_t i = 0; i < sensor.timeParamCount(); i++) {
        const char* key = sensor.timeParamKey(i);
        if (params.containsKey(key)) {
          int32_t v = params[key].as<int32_t>();
          v = constrain(v, sensor.timeParamMin(i), sensor.timeParamMax(i));
          sensor.setTimeParam(i, v);
        }
      }
    } else {
      int idx = doc["preset"] | 0;
      if (idx < 0 || idx >= sensor.timePresetCount()) { sendError(F("Preset out of range")); return; }
      sensor.setTimePreset((uint8_t)idx);
    }
    sendAck("set_time", sensor.integrationTimeMs());

  } else if (strcmp(cmd, "set_channels") == 0) {
    Measurement::clearChannelSelection();
    JsonArray arr = doc["channels"];
    for (JsonVariant v : arr) {
      int idx = v.as<int>();
      if (idx >= 0 && idx < sensor.channelCount()) Measurement::selectChannel((uint8_t)idx);
    }
    sendAck("set_channels");

  } else if (strcmp(cmd, "set_extra") == 0) {
    int idx = doc["idx"] | -1;
    int value = doc["value"] | 0;
    if (idx < 0 || idx >= sensor.extraParamCount()) { sendError(F("Unknown extra param")); return; }
    sensor.setExtraParam((uint8_t)idx, value);
    sendAck("set_extra");

  } else if (strcmp(cmd, "measure_ref") == 0) {
    runAndSendMeasurement(true);

  } else if (strcmp(cmd, "measure") == 0) {
    runAndSendMeasurement(false);

  } else {
    sendError(F("Unknown command"));
  }
}

} // namespace Protocol
