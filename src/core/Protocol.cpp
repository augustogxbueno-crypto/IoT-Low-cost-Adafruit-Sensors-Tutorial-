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

// NOTE: there used to be an INFO_DOC_CAPACITY here sizing a StaticJsonDocument
// for the whole "info" response. sendInfo() now streams that response
// directly to Serial instead (see below) — building it as one in-memory
// document was what overflowed the stack on RAM-tight sensors like AS7262.

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
// Written directly to Serial instead of built up as one big ArduinoJson
// StaticJsonDocument. That document (INFO_DOC_CAPACITY) used to peak at
// 600-850+ stack bytes depending on sensor, alive on the stack at the same
// time as handleCommand()'s own CMD_DOC_CAPACITY document one frame up —
// on a 2KB Uno with a sensor library that already eats 1KB+ of static RAM
// (e.g. AS7262's Adafruit_AS726x), that combination overflowed the stack
// and corrupted the response mid-transmission instead of failing loudly.
// Streaming field-by-field keeps this function's own stack use in the tens
// of bytes regardless of sensor, so it can't be the thing that overflows.
static void printQuoted(const __FlashStringHelper* s) {
  Serial.print('"');
  Serial.print(s);
  Serial.print('"');
}

void sendInfo() {
  SensorBase& sensor = SensorManager::get();

  Serial.print(F("{\"evt\":\"info\",\"sensor\":"));
  printQuoted(sensor.name());
  Serial.print(F(",\"sensorType\":"));
  printQuoted(sensor.sensorType());

  // Channels
  Serial.print(F(",\"channels\":["));
  for (uint8_t i = 0; i < sensor.channelCount(); i++) {
    if (i) Serial.print(',');
    ChannelInfo ci = sensor.channel(i);
    Serial.print(F("{\"id\":"));
    Serial.print(i);
    Serial.print(F(",\"name\":"));
    printQuoted(ci.name);
    Serial.print(F(",\"color\":"));
    printQuoted(ci.color);
    Serial.print('}');
  }
  Serial.print(']');

  // Modes available: Reflectance only if the sensor can drive its own LED
  Serial.print(F(",\"modes\":["));
  bool hasReflectance = sensor.ledType() != LedType::LED_NONE;
  if (hasReflectance) { printQuoted(F("reflectance")); Serial.print(','); }
  Serial.print(F("\"absorbance\",\"fluorescence\"]"));

  // Gain
  Serial.print(F(",\"gain\":{"));
  if (sensor.gainType() == GainType::GAIN_DISCRETE) {
    Serial.print(F("\"type\":\"discrete\",\"options\":["));
    for (uint8_t i = 0; i < sensor.gainOptionCount(); i++) {
      if (i) Serial.print(',');
      printQuoted(sensor.gainOptionLabel(i));
    }
    Serial.print(']');
  } else {
    Serial.print(F("\"type\":\"continuous\",\"min\":"));
    Serial.print(sensor.gainContinuousMin());
    Serial.print(F(",\"max\":"));
    Serial.print(sensor.gainContinuousMax());
  }
  Serial.print('}');

  // LED
  Serial.print(F(",\"led\":{\"internal\":{"));
  switch (sensor.ledType()) {
    case LedType::LED_NONE:
      Serial.print(F("\"type\":\"none\""));
      break;
    case LedType::LED_BINARY:
      Serial.print(F("\"type\":\"binary\""));
      break;
    case LedType::LED_DISCRETE:
      Serial.print(F("\"type\":\"discrete\",\"options\":["));
      for (uint8_t i = 0; i < sensor.ledOptionCount(); i++) {
        if (i) Serial.print(',');
        printQuoted(sensor.ledOptionLabel(i));
      }
      Serial.print(']');
      break;
    case LedType::LED_CONTINUOUS:
      Serial.print(F("\"type\":\"continuous\",\"minMA\":"));
      Serial.print(sensor.ledMinMA());
      Serial.print(F(",\"maxMA\":"));
      Serial.print(sensor.ledMaxMA());
      break;
  }
  Serial.print(F("},\"absorbance\":{\"type\":\"external\",\"pin\":"));
  Serial.print(PIN_LED_ABSORBANCE);
  Serial.print(F(",\"angle\":180},\"fluorescence\":{\"type\":\"external\",\"pin\":"));
  Serial.print(PIN_LED_FLUORESCENCE);
  Serial.print(F(",\"angle\":90}}"));

  // Integration time
  Serial.print(F(",\"time\":{\"currentMs\":"));
  Serial.print(sensor.integrationTimeMs());
  if (sensor.integrationFormula()) {
    Serial.print(F(",\"formula\":"));
    printQuoted(sensor.integrationFormula());
  }
  if (sensor.timeType() == TimeType::TIME_FORMULA) {
    Serial.print(F(",\"type\":\"formula\",\"params\":["));
    for (uint8_t i = 0; i < sensor.timeParamCount(); i++) {
      if (i) Serial.print(',');
      Serial.print(F("{\"key\":\""));
      Serial.print(sensor.timeParamKey(i));
      Serial.print(F("\",\"label\":"));
      printQuoted(sensor.timeParamLabel(i));
      Serial.print(F(",\"min\":"));
      Serial.print(sensor.timeParamMin(i));
      Serial.print(F(",\"max\":"));
      Serial.print(sensor.timeParamMax(i));
      Serial.print('}');
    }
    Serial.print(']');
  } else {
    Serial.print(F(",\"type\":\"presets\",\"presets\":["));
    for (uint8_t i = 0; i < sensor.timePresetCount(); i++) {
      if (i) Serial.print(',');
      printQuoted(sensor.timePresetLabel(i));
    }
    Serial.print(']');
  }
  Serial.print('}');

  // Extra sensor-specific parameters
  if (sensor.extraParamCount() > 0) {
    Serial.print(F(",\"extraParams\":["));
    for (uint8_t i = 0; i < sensor.extraParamCount(); i++) {
      if (i) Serial.print(',');
      ExtraParamInfo ep = sensor.extraParam(i);
      Serial.print(F("{\"key\":\""));
      Serial.print(ep.key);
      Serial.print(F("\",\"label\":"));
      printQuoted(ep.label);
      if (ep.options) {
        Serial.print(F(",\"type\":\"select\",\"options\":["));
        for (uint8_t j = 0; j < ep.optionCount; j++) {
          if (j) Serial.print(',');
          printQuoted(Utils::flashStr((const char* const*)ep.options, j));
        }
        Serial.print(']');
      } else {
        Serial.print(F(",\"type\":\"number\",\"min\":"));
        Serial.print(ep.numMin);
        Serial.print(F(",\"max\":"));
        Serial.print(ep.numMax);
      }
      Serial.print('}');
    }
    Serial.print(']');
  }

  Serial.print(F("}\n"));
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
