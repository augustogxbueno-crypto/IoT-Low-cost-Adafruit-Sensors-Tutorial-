#include "Measurement.h"
#include "SensorManager.h"
#include <math.h>

namespace Measurement {

Mode    mode          = Mode::REFLECTANCE;
Submode submode       = Submode::RAW;
bool    selectedChannels[MAX_CHANNELS] = { false };
float   referenceValues[MAX_CHANNELS]  = { 0 };
bool    referenceValid = false;

static const __FlashStringHelper* g_error = nullptr;

const __FlashStringHelper* lastError() { return g_error; }

void setMode(Mode m) {
  mode = m;
  referenceValid = false; // switching modes invalidates any previous reference
}

void setSubmode(Submode s) { submode = s; }

void clearChannelSelection() {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) selectedChannels[i] = false;
}

void selectChannel(uint8_t idx) {
  if (idx < MAX_CHANNELS) selectedChannels[idx] = true;
}

bool anyChannelSelected() {
  for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
    if (selectedChannels[i]) return true;
  }
  return false;
}

static constexpr unsigned long DURATION_MS      = 10000;
static constexpr unsigned long SAMPLE_PERIOD_MS = 200;

bool run(bool isReference, ProgressCallback onProgress, float* avgOut, bool* usedLogOut) {
  g_error = nullptr;
  SensorBase& sensor = SensorManager::get();

  if (isReference && mode == Mode::FLUORESCENCE) {
    g_error = F("Fluorescence has no reference measurement.");
    return false;
  }
  if (!anyChannelSelected()) {
    g_error = F("No channel selected.");
    return false;
  }

  bool useLog = (!isReference) && (submode == Submode::LOG) &&
                (mode == Mode::REFLECTANCE || mode == Mode::ABSORBANCE);

  if (useLog && !referenceValid) {
    g_error = F("Measure the reference first (log / -log10 submode).");
    return false;
  }

  const uint8_t n = sensor.channelCount();

  float sum[MAX_CHANNELS]   = { 0 };
  int   count[MAX_CHANNELS] = { 0 };
  float sample[MAX_CHANNELS];

  unsigned long startTime  = millis();
  unsigned long lastSample = 0;
  int sampleCount = 0;

  while (millis() - startTime < DURATION_MS) {
    if (millis() - lastSample < SAMPLE_PERIOD_MS) {
      delay(2); // briefly frees the CPU (avoids ESP32 watchdog, no cost on AVR)
      continue;
    }
    lastSample = millis();

    if (!sensor.readChannels(sample)) continue;
    sampleCount++;

    for (uint8_t i = 0; i < n && i < MAX_CHANNELS; i++) {
      if (!selectedChannels[i]) continue;

      float value;
      if (useLog) {
        float ref = referenceValues[i];
        if (ref <= 0.0f) ref = 1.0f;
        float r = sample[i];
        if (r <= 0.0f) r = 1.0f;
        value = -log10f(r / ref);
      } else {
        value = sample[i]; // raw (reference, explicit raw mode, or fluorescence)
      }

      sum[i]   += value;
      count[i] += 1;
      sample[i] = value; // so the progress callback reports the same value we're averaging
    }

    if (onProgress) onProgress((millis() - startTime) / 1000.0f, sampleCount, sample);
  }

  for (uint8_t i = 0; i < n && i < MAX_CHANNELS; i++) {
    if (!selectedChannels[i]) continue;
    float avg = (count[i] > 0) ? (sum[i] / count[i]) : 0.0f;
    avgOut[i] = avg;
    if (isReference) referenceValues[i] = avg;
  }

  if (isReference) referenceValid = true;
  if (usedLogOut) *usedLogOut = useLog;

  return true;
}

} // namespace Measurement
