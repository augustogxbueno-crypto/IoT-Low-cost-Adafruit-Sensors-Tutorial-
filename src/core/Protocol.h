// =============================================================================
// core/Protocol.h — the only file that knows about JSON.
// -----------------------------------------------------------------------------
// Builds the "info" descriptor generically from whatever SensorBase is
// active (via SensorManager), and parses/dispatches incoming commands.
// See README.md for the full wire protocol.
// =============================================================================
#pragma once
#include <Arduino.h>

namespace Protocol {

// Drives the external Absorbance/Fluorescence LEDs and the active sensor's
// own internal LED (if it has one) to match the CURRENT Measurement::mode.
// Called reactively whenever "set_mode" arrives over Serial, and ALSO once
// from main.cpp's setup() right after the sensor is initialized — Measurement
// defaults to Mode::REFLECTANCE at boot, and the frontend assumes the same
// default the moment it connects (modes[0] is always "reflectance" for any
// LED-capable sensor), so without this boot-time call the sensor's internal
// LED would be left in the vendor library's own power-on default (off)
// instead of actually being on to match that default.
void syncOutputsForMode();

// Sends the "info" event: full self-description of the active sensor.
// Called once when the browser connects (on "get_info"), and that's it —
// nothing else needs it, the frontend builds its whole UI from this.
void sendInfo();

// Parses one line of incoming JSON and acts on it (updates SensorManager's
// active sensor, or runs a measurement via Measurement::run()).
void handleCommand(const char* line);

} // namespace Protocol
