// =============================================================================
// SENSOR TUTORIAL SERIES — main.cpp
// -----------------------------------------------------------------------------
// This file only does three things: bring up Serial/I2C/the active sensor,
// read incoming lines, and hand them to Protocol::handleCommand(). All the
// actual logic lives in core/ and sensors/ — see README.md for the full
// architecture and wire protocol.
//
// Which sensor gets compiled in is decided entirely by the PlatformIO
// environment you build (each one defines a single -D SENSOR_xxx flag and
// restricts build_src_filter to that sensor's .cpp — see platformio.ini).
// =============================================================================
#include <Arduino.h>
#include <Wire.h>
#include "core/Pins.h"
#include "core/SensorManager.h"
#include "core/Measurement.h"
#include "core/Protocol.h"

// Fixed-size line buffer for incoming Serial commands — no String class, so
// nothing here fragments the heap on AVR boards (see core/Utils.h for why
// that matters).
constexpr uint8_t LINE_BUFFER_SIZE = 96;
char    lineBuffer[LINE_BUFFER_SIZE];
uint8_t lineLen = 0;

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) { /* wait for USB port (native USB boards) */ }

  pinMode(PIN_LED_ABSORBANCE, OUTPUT);
  pinMode(PIN_LED_FLUORESCENCE, OUTPUT);
  digitalWrite(PIN_LED_ABSORBANCE, LOW);
  digitalWrite(PIN_LED_FLUORESCENCE, LOW);

  Wire.begin();

  if (!SensorManager::get().begin()) {
    // Keep Serial alive so the browser sees why nothing else works.
    while (true) {
      if (Serial.available()) Serial.read();
      Serial.println(F("{\"evt\":\"error\",\"msg\":\"Sensor not found. Check I2C wiring.\"}"));
      delay(1000);
    }
  }

  // Measurement::mode defaults to REFLECTANCE at compile time, but that's
  // only a valid default for sensors that can actually drive their own LED
  // (LedType != LED_NONE) — the web UI's default mode tab mirrors this same
  // rule (see Protocol.cpp's sendInfo(): "reflectance" is only the first
  // entry in modes[] when the sensor has an LED). For a photodiode sensor
  // with no LED, both sides should default to ABSORBANCE instead, so pick
  // whichever matches what sendInfo() will actually report, then physically
  // apply it — otherwise the sensor's internal LED (or the external
  // Absorbance LED, for LED-less sensors) sits in the vendor library's own
  // power-on-default state, invisibly out of sync with what the UI displays
  // as active, until the user manually re-clicks a mode tab.
  bool hasReflectance = SensorManager::get().ledType() != LedType::LED_NONE;
  Measurement::setMode(hasReflectance ? Measurement::Mode::REFLECTANCE
                                       : Measurement::Mode::ABSORBANCE);
  Protocol::syncOutputsForMode();
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      lineBuffer[lineLen] = '\0';
      if (lineLen > 0) Protocol::handleCommand(lineBuffer);
      lineLen = 0;
    } else if (c != '\r') {
      if (lineLen < LINE_BUFFER_SIZE - 1) {
        lineBuffer[lineLen++] = c;
      } else {
        // Line too long for the buffer: drop it instead of overrunning memory
        lineLen = 0;
      }
    }
  }
}
