# IoT Color / Photodiode Sensor Tutorial Series

One firmware architecture and one web interface, reused across ten
low-cost spectral/light sensors: **AS7341, AS7343, AS7262, TCS34725,
APDS9960, BH1750, LTR303, LTR329, LTR390, TSL2591**. Pick a sensor, build
that one PlatformIO environment, open the same `web/index.html` — the UI
builds itself from whatever the firmware says it can do.

> **TCS34725 is discontinued by Adafruit.** [`APDS9960`](https://www.adafruit.com/product/3595)
> (Proximity, Light, RGB, and Gesture Sensor — STEMMA QT/Qwiic) is the
> recommended replacement for new builds: same 4-channel RGBC + derived
> Color Temp/Lux shape, same "color" sensor type, same wiring. The one
> functional difference is that APDS9960's breakout has **no controllable
> illumination LED usable for this project** (its onboard IR LED only feeds
> the separate proximity/gesture engine), so **APDS9960 only ever offers
> Absorbance and Fluorescence mode** — Reflectance never appears for it, the
> same way it never appears for BH1750/LTR3xx/TSL2591. TCS34725's own
> firmware/library entry is kept below for anyone maintaining existing
> TCS34725 hardware.

No Wi-Fi anywhere. The browser talks to the board over USB using the
**Web Serial API**, so the exact same firmware/webpage pair works on an
ESP32-S3 Feather or a plain Arduino Uno.

## How to run it

1. **First time only — install every sensor's libraries in one go:**
   ```bash
   ./install_libraries.sh      # macOS / Linux
   install_libraries.bat       # Windows
   ```
   Requires PlatformIO Core (`pip install -U platformio`) on your PATH — or
   just open this folder in VS Code with the "PlatformIO IDE" extension and
   run the script from its integrated terminal. The script reads every
   `[env:...]` section straight out of `platformio.ini` and runs
   `pio pkg install` for each one, so it downloads all 10 sensors' vendor
   libraries *and* both board toolchains (ESP32-S3 + AVR/Uno) up front —
   you don't need to know yet which sensor you'll end up using, and you
   never have to manually search/install a library by hand. It's safe to
   re-run any time (e.g. after adding an 11th sensor to `platformio.ini`);
   already-installed packages are skipped.
2. Pick your sensor + board and build/upload that PlatformIO environment, e.g.:
   ```bash
   pio run -e as7341_esp32s3 -t upload
   pio run -e apds9960_uno   -t upload
   ```
   (Full list of the 20 environments is in `platformio.ini`.) Since step 1
   already downloaded everything, this step works offline.
3. Open `web/index.html` directly in Chrome or Edge (desktop only — Web
   Serial isn't available on Firefox/Safari or mobile).
4. Click **Connect via USB**, pick the board's port. The page requests
   `get_info`, the board replies with a full self-description, and the UI
   builds itself — modes, gain control, LED control, integration time
   control, and any sensor-specific extra parameters.

## Architecture

```
install_libraries.sh / .bat   run once to fetch all 10 sensors' libraries + both board toolchains (see "How to run it")
platformio.ini                 the 20 build environments (10 sensors × 2 boards)

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
    ├── SensorBase.h            the interface every sensor implements — READ THIS FIRST if adding sensor #11
    ├── AS7341.h/.cpp
    ├── AS7343.h/.cpp
    ├── AS7262.h/.cpp
    ├── TCS34725.h/.cpp        discontinued sensor — kept for existing hardware, see note above
    ├── APDS9960.h/.cpp        recommended TCS34725 replacement
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
adding an 11th sensor, `sensors/SensorBase.h` and any existing `sensors/*.cpp`
is the only reading you need to do — `core/` and `web/` don't change.

## Wire protocol

One JSON object per line (`\n`-terminated), both directions.

At boot, `main.cpp` picks the same default mode `sendInfo()` reports
(Reflectance for LED-capable sensors, Absorbance otherwise) and calls
`Protocol::syncOutputsForMode()` right away, so the physical LED state
always matches what the UI shows as active from the very first frame —
see `src/main.cpp` and `src/core/Protocol.h`.

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

### The "info" schema — this is what makes one frontend work for 10 sensors

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
| Every other sensor's internal LED | — | driven through I2C (AS7341/AS7343/AS7262) or doesn't exist (photodiode sensors, **including APDS9960**) |

## Sensor particularities (why `SensorBase` looks the way it does)

| Sensor | Channels | Gain | Integration time | Internal LED |
|---|---|---|---|---|
| AS7341 | 8 (415–680nm) | 11 discrete (0.5X–512X) | ATIME×ASTEP formula | Continuous, 4–258mA (I2C) |
| AS7343 | 13 (405–855nm+Clear) | 13 discrete (0.5X–2048X) | ATIME×ASTEP formula | Continuous, 4–258mA (I2C) |
| AS7262 | 6 (450–650nm) | 4 discrete (1X–64X) | value×2.8ms formula | Discrete, 4 levels (I2C) |
| TCS34725 | 4 raw + 2 derived (Temp, Lux) | 4 discrete (1X–60X) | 6 fixed presets | **Binary**, needs GPIO pin 7 (not I2C — see below) — **discontinued, see note at top** |
| APDS9960 | 4 raw + 2 derived (Temp, Lux) | 4 discrete (1X–64X) | Direct-ms formula (~3–700ms) | **None** — onboard IR LED only serves proximity/gesture, unusable here |
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
`core/Protocol.cpp` already applies generically to every LED type.

**APDS9960 has no equivalent workaround.** Its onboard IR LED (and driver)
exist purely to support the proximity/gesture engine — the Adafruit library
gives no way to fire it during a color/ALS reading, and IR isn't a useful
wavelength for reflectance anyway. `APDS9960Sensor::ledType()` simply
returns `LedType::LED_NONE`, the same as every other photodiode-only sensor
in this project, so `sendInfo()`'s existing "only list Reflectance if
`ledType() != LED_NONE`" logic hides Reflectance for it automatically —
again, no core changes needed. Absorbance and Fluorescence both still work
normally, driven entirely by the two fixed external LEDs on pins 6/5.

## Adding an 11th sensor

1. Read `sensors/SensorBase.h` top to bottom (it's short, and documents the
   4 LED types / 2 gain types / 2 time types).
2. Copy the sensor whose particularities are closest to yours as a
   starting point (e.g. a photodiode sensor with no LED → start from
   `BH1750.h/.cpp` or `TSL2591.h/.cpp`; a 4-channel RGBC color sensor with
   no LED → start from `APDS9960.h/.cpp` instead of `TCS34725.h/.cpp`, since
   the latter's binary-LED-on-GPIO-7 logic is extra complexity you probably
   don't need).
3. Add a `SENSOR_yourchip` branch to `core/SensorManager.h`.
4. Add the per-sensor `INFO_MAX_*` capacity constants to
   `core/Protocol.cpp` (sized to your sensor's actual channel/gain/preset/
   extra-param counts — see the existing `#if` block; oversizing just wastes
   RAM on AVR, it won't break anything).
5. Add two environments to `platformio.ini` (ESP32-S3 + Uno), copying an
   existing pair and swapping `SENSOR_yourchip`, `build_src_filter`, and
   `lib_deps`.
6. **`web/` needs zero changes.**

## Library names in `platformio.ini`

Most libraries are listed **without** a version pin on purpose, so
PlatformIO always resolves to whatever release is current. **AS7341** and
the two universally-common dependencies (**ArduinoJson** `^6.21.5`,
**Adafruit BusIO** `^1.16.1`) are the exceptions, pinned to a specific
version. ArduinoJson in particular must stay on the 6.x line, since
`core/Protocol.cpp` uses the `StaticJsonDocument<N>` API that ArduinoJson
v7 removed.

If you want fully reproducible builds for every sensor, look up each
library's current version at https://registry.platformio.org and add
`@ ^x.y.z` to its `lib_deps` entry yourself.

## Authors

This project was developed by the **Smart research group — Laboratório de
Análise Instrumental**, at the **Universidade Federal de Uberlândia (UFU)**,
Brazil.
