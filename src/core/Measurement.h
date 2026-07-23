// =============================================================================
// core/Measurement.h — the 10-second sampling loop, shared by every sensor.
// -----------------------------------------------------------------------------
// This file knows nothing about JSON and nothing about which sensor is
// active beyond calling SensorBase::readChannels() through SensorManager.
// Protocol.cpp is the only thing that talks to this module.
// =============================================================================
#pragma once
#include <Arduino.h>

namespace Measurement {

enum class Mode    { REFLECTANCE, ABSORBANCE, FLUORESCENCE };
enum class Submode { LOG, RAW };

// Sized for the largest sensor in the series (AS7343, 14 channels). Every
// sensor uses a prefix of this array according to its own channelCount().
constexpr uint8_t MAX_CHANNELS = 14;

extern Mode    mode;
extern Submode submode;
extern bool    selectedChannels[MAX_CHANNELS];
extern float   referenceValues[MAX_CHANNELS];
extern bool    referenceValid;

void setMode(Mode m);
void setSubmode(Submode s);
void clearChannelSelection();
void selectChannel(uint8_t idx);
bool anyChannelSelected();

// Invoked once per sample during run(), so Protocol.cpp can stream a
// "progress" event without Measurement knowing anything about JSON.
// `sampleValues` has channelCount() entries; only the selected ones are
// meaningful (already raw or log-transformed, matching the final result).
using ProgressCallback = void (*)(float elapsedSec, int sampleCount, const float* sampleValues);

// Runs the full 10-second sampling loop.
// `avgOut` must have room for at least the active sensor's channelCount();
// only the selected channels' entries are written.
// Returns false (without measuring) if the request is invalid; check
// lastError() for why.
bool run(bool isReference, ProgressCallback onProgress, float* avgOut, bool* usedLogOut);

// Set by the last run() call that returned false; nullptr after a success.
const __FlashStringHelper* lastError();

} // namespace Measurement
