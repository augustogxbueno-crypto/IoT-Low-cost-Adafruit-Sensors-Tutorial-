# IoT Color / Photodiode Sensor Tutorial Series

One firmware architecture and one web interface, reused across nine
low-cost spectral/light sensors: **AS7341, AS7343, AS7262, TCS34725,
BH1750, LTR303, LTR329, LTR390, TSL2591**. Pick a sensor, build that one
PlatformIO environment, open the same `web/index.html` — the UI builds
itself from whatever the firmware says it can do.

No Wi-Fi anywhere. The browser talks to the board over USB using the
**Web Serial API**, so the exact same firmware/webpage pair works on an
ESP32-S3 Feather or a plain Arduino Uno.

## How to run it

1. Pick your sensor + board and build/upload that PlatformIO environment, e.g.:
   ```bash
   pio run -e as7341_esp32s3 -t upload
   pio run -e as7262_uno     -t upload
   ```
   (Full list of the 18 environments is in `platformio.ini`.)
2. Open `web/index.html` directly in Chrome or Edge (desktop only — Web
   Serial isn't available on Firefox/Safari or mobile).
3. Click **Connect via USB**, pick the board's port. The page requests
   `get_info`, the board replies with a full self-description, and the UI
   builds itself — modes, gain control, LED control, integration time
   control, and any sensor-specific extra parameters.

## Architecture

```
src/
├── main.cpp                  setup()/loop(), reads Serial lines, hands them to Protocol
├── core/
│   ├── Pins.h                 the two FIXED external LEDs (pin 6 = Absorbance/180°, pin 5 = Fluorescence/90°)
│   ├── Utils.h                PROGMEM string helpers, JSON key builder — no String class anywhere
│   ├── SensorManager.h        picks the ONE compiled-in sensor via a SENSOR_xxx build flag
│   ├── Measurement.h/.cpp     the generic 10-second sampling loop + log/raw math — sensor-agnostic
│   ├── Protocol.h/.cpp        the ONLY file that knows JSON; builds "info", parses commands
│   └── Export.h                placeholder for future on-device export (CSV/Excel already happen in the browser)
└── sensors/
    ├── SensorBase.h            the interface every sensor implements — READ THIS FIRST if adding sensor #10
    ├── AS7341.h/.cpp
    ├── AS7343.h/.cpp
    ├── AS7262.h/.cpp
    ├── TCS34725.h/.cpp
    ├── BH1750.h/.cpp
    ├── LTR303.h/.cpp
    ├── LTR329.h/.cpp           near-duplicate of LTR303 on purpose — see note at the top of LTR303.h
    ├── LTR390.h/.cpp
    └── TSL2591.h/.cpp

web/
├── index.html / style.css      generic shell, zero sensor-specific markup
├── script.js                   builds the ENTIRE UI at runtime from the "info" event — zero sensor-specific logic
└── xlsx.core.min.js            vendored SheetJS build (Excel export works fully offline)
```

**core/ never imports a vendor library and never mentions a sensor by
name.** Everything sensor-specific lives behind `SensorBase`. If you're
adding a 10th sensor, `sensors/SensorBase.h` and any existing `sensors/*.cpp`
is the only reading you need to do — `core/` and `web/` don't change.

## Wire protocol

One JSON object per line (`\n`-terminated), both directions.

**Browser → Board**
```
{"cmd":"get_info"}
{"cmd":"set_mode","mode":"reflectance|absorbance|fluorescence"}
{"cmd":"set_submode","sub":"log|raw"}
{"cmd":"set_led", ...}            // shape depends on led.internal.type — see below
{"cmd":"set_gain", ...}           // shape depends on gain.type — see below
{"cmd":"set_time", ...}           // shape depends on time.type — see below
{"cmd":"set_channels","channels":[0,2,5]}
{"cmd":"set_extra","idx":0,"value":3}   // sensor-specific extra parameter
{"cmd":"measure_ref"}
{"cmd":"measure"}
```

**Board → Browser**
```
{"evt":"info", ...}                        // see schema below
{"evt":"ack","cmd":"...", "tint_ms":...}   // tint_ms only present after set_time
{"evt":"progress","t":1.2,"n":3,"data":{"0":123.4}}
{"evt":"ref_saved","data":{"0":512.0}}
{"evt":"result","mode":"...","sub":"...","data":{...},"gain":"...","tint_ms":...}
{"evt":"error","msg":"..."}
```

### The "info" schema — this is what makes one frontend work for 9 sensors

```jsonc
{
  "evt": "info",
  "sensor": "AS7262",
  "sensorType": "color",                 // or "photodiode"
  "channels": [{"id":0,"name":"450nm","color":"#7F00FF"}, ...],
  "modes": ["absorbance","fluorescence"], // "reflectance" only appears if led.internal.type != "none"

  "gain": {
    "type": "discrete",                   // or "continuous"
    "options": ["1X","3.7X","16X","64X"]  // discrete
    // "min": 31, "max": 254              // continuous (e.g. BH1750's MTreg)
  },

  "led": {
    "internal": {
      "type": "none|binary|discrete|continuous",
      "options": [...]                    // discrete only
      // "minMA": 4, "maxMA": 258         // continuous only
    },
    "absorbance":   {"type":"external","pin":6,"angle":180},
    "fluorescence": {"type":"external","pin":5,"angle":90}
  },

  "time": {
    "type": "formula",                    // or "presets"
    "formula": "value x 2.8ms",
    "currentMs": 140.0,
    "params": [{"key":"itime","label":"Integration Time","min":0,"max":255}]
    // "presets": ["2.4 ms", "24 ms", ...]   // presets mode instead
  },

  "extraParams": [                        // omitted entirely if the sensor has none
    {"key":"measRate","label":"Measurement Rate","type":"select","options":["50 ms", ...]}
    // {"key":"...", "type":"number", "min":0, "max":255}
  ]
}
```

`set_led` / `set_gain` / `set_time` payload shape mirrors whichever variant
`info` reported:

| `type` | Command payload |
|---|---|
| `led.internal: binary` | `{"cmd":"set_led","on":true}` |
| `led.internal: discrete` | `{"cmd":"set_led","idx":2}` |
| `led.internal: continuous` | `{"cmd":"set_led","current":50}` |
| `gain: discrete` | `{"cmd":"set_gain","idx":4}` |
| `gain: continuous` | `{"cmd":"set_gain","value":150}` |
| `time: formula` | `{"cmd":"set_time","params":{"atime":100,"astep":999}}` |
| `time: presets` | `{"cmd":"set_time","preset":2}` |

## Wiring

| Signal | Pin | Notes |
|---|---|---|
| SDA / SCL | board's default I2C | every sensor |
| Absorbance LED (180°) | **6** | fixed, same on every sensor/board |
| Fluorescence LED (90°) | **5** | fixed, same on every sensor/board |
| TCS34725's onboard LED | **7** | only this sensor — see "Particularities" below |
| Every other sensor's internal LED | — | driven through I2C (AS7341/AS7343/AS7262) or doesn't exist (photodiode sensors) |

## Sensor particularities (why `SensorBase` looks the way it does)

| Sensor | Channels | Gain | Integration time | Internal LED |
|---|---|---|---|---|
| AS7341 | 8 (415–680nm) | 11 discrete (0.5X–512X) | ATIME×ASTEP formula | Continuous, 4–258mA (I2C) |
| AS7343 | 13 (405–855nm+Clear) | 13 discrete (0.5X–2048X) | ATIME×ASTEP formula | Continuous, 4–258mA (I2C) |
| AS7262 | 6 (450–650nm) | 4 discrete (1X–64X) | value×2.8ms formula | Discrete, 4 levels (I2C) |
| TCS34725 | 4 raw + 2 derived (Temp, Lux) | 4 discrete (1X–60X) | 6 fixed presets | **Binary**, needs GPIO pin 7 (not I2C — see below) |
| BH1750 | 1 (Lux) | Continuous (MTreg 31–254) | 3 resolution presets | None |
| LTR303 / LTR329 | 2 raw + 1 derived (Visible) | 6 discrete | 8 presets **+ Measurement Rate** (extra param) | None |
| LTR390 | 1 (UV or ALS — **mode** extra param picks which) | 5 discrete, non-linear | 6 resolution presets | None |
| TSL2591 | 2 raw + 2 derived (Visible, Lux) | 4 discrete, huge non-uniform range | 6 presets | None |

Sensors with no internal LED never offer Reflectance mode at all — the
`modes` array in `info` simply won't include it, and the frontend's mode
tabs reflect that automatically.

**TCS34725's LED is a special case.** The Adafruit library's
`setInterrupt()` only toggles the onboard LED if you've soldered the
breakout's LED pin to its INT pin — a fragile, optional hardware mod most
builds won't have done. Instead, `TCS34725Sensor` drives the LED from a
dedicated GPIO (**pin 7**, same on every board) using plain `digitalWrite`,
following the exact same "on entering Reflectance, off leaving it" logic
`core/Protocol.cpp` already applies generically to every LED type — no core
changes were needed to support this.

## Adding a 10th sensor

1. Read `sensors/SensorBase.h` top to bottom (it's short, and documents the
   4 LED types / 2 gain types / 2 time types).
2. Copy the sensor whose particularities are closest to yours as a
   starting point (e.g. a photodiode sensor with no LED → start from
   `BH1750.h/.cpp` or `TSL2591.h/.cpp`).
3. Add a `SENSOR_yourchip` branch to `core/SensorManager.h`.
4. Add the per-sensor `INFO_MAX_*` capacity constants to
   `core/Protocol.cpp` (sized to your sensor's actual channel/gain/preset/
   extra-param counts — see the existing `#if` block; oversizing just wastes
   RAM on AVR, it won't break anything).
5. Add two environments to `platformio.ini` (ESP32-S3 + Uno), copying an
   existing pair and swapping `SENSOR_yourchip`, `build_src_filter`, and
   `lib_deps`.
6. **`web/` needs zero changes.**

## What's verified vs. what's carried over from datasheets/library docs

- **AS7341** end-to-end logic (measurement math, JSON protocol) has been
  exercised in this project before this refactor.
- **This refactor's core/ + AS7341 combination was compiled and linked for
  real** against the actual ArduinoJson v6.21.5 library, targeting
  `atmega328p` (Arduino Uno), using hand-written stand-ins for `Arduino.h`/
  `Wire.h`/`Adafruit_AS7341.h` (real vendor headers weren't reachable from
  this environment's network — see below). Result: links cleanly, 685 bytes
  of permanent RAM, `"info"` packet peaks at 840 bytes on the stack alone
  for AS7341 specifically (tuned per-sensor in `core/Protocol.cpp`).
- **The other 8 sensors' drivers were written against method/enum names
  confirmed via each library's real public source** (AS726x, TCS34725,
  LTR390 API surfaces were specifically looked up rather than guessed), but
  **not compiled** — this environment cannot reach `api.registry.
  platformio.org` or the Arduino package index, so none of the 8 vendor
  libraries could be installed here to compile-test against. Build each
  environment yourself before flashing real hardware.
- **LTR390 resolution→ms table** (400/200/100/50/25/12.5ms for
  20/19/18/17/16/13-bit) comes from the sensor's measurement-rate register
  behavior as documented in other open-source drivers (e.g. ESPHome), not
  stated directly in the short reference sketch this project started from.
- **LTR303/LTR329 code-sharing assumption**: both classes come from the
  same `Adafruit_LTR329_LTR303` library file and are assumed to expose an
  identical method surface. `LTR329Sensor` is a deliberate near-duplicate
  of `LTR303Sensor` rather than a template, specifically because that
  assumption couldn't be compile-verified here — confirm on real hardware
  before treating them as interchangeable in code you build on top of this.

If any of the above turns out to not match a given library version, the
fix is contained entirely to that one `sensors/*.cpp` file.

## Library names in `platformio.ini`

Every library **name** below was checked against the real PlatformIO/Arduino
library registries (via web search, since this environment can't reach
`api.registry.platformio.org` directly) — including catching two mistakes
along the way: the LTR303/LTR329 library's real name has no "Library" suffix
(`Adafruit LTR329 and LTR303`, not `...Library`), and two version pins I'd
guessed (`Adafruit AS726x @ ^1.5.2`, `Adafruit LTR390 Library @ ^1.2.1`) were
both higher than the real latest release (1.2.3 and 1.1.2 respectively) and
would have hard-failed the build with "version not found".

Only **AS7341**'s version pin (`^1.4.2`) and the two universally-common
dependencies (**ArduinoJson** `^6.21.5`, **Adafruit BusIO** `^1.16.1`) are
pinned to a specific version with reasonable confidence. Every other
sensor's library is listed **without** a version pin on purpose — a wrong
guessed pin actively breaks the build (PlatformIO refuses to "downgrade" to
satisfy a caret range), while no pin just resolves to whatever's current.
If you want fully reproducible builds, confirm each real version at
https://registry.platformio.org and add `@ ^x.y.z` yourself.
