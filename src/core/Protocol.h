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

// Sends the "info" event: full self-description of the active sensor.
// Called once when the browser connects (on "get_info"), and that's it —
// nothing else needs it, the frontend builds its whole UI from this.
void sendInfo();

// Parses one line of incoming JSON and acts on it (updates SensorManager's
// active sensor, or runs a measurement via Measurement::run()).
void handleCommand(const char* line);

} // namespace Protocol
