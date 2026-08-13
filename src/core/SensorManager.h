// =============================================================================
// core/SensorManager.h — picks the ONE active sensor at compile time.
// -----------------------------------------------------------------------------
// Each PlatformIO environment in platformio.ini defines exactly one
// SENSOR_xxx build flag (e.g. -D SENSOR_AS7341) and restricts
// build_src_filter to that sensor's .cpp file only. That's what lets nine
// completely different vendor libraries coexist in sensors/ without ever
// being compiled together — only one is ever built into a given firmware.
//
// core/ and main.cpp only ever call SensorManager::get(), and never see
// which sensor class is actually behind it.
// =============================================================================
#pragma once
#include "../sensors/SensorBase.h"

#if defined(SENSOR_AS7341)
  #include "../sensors/AS7341.h"
  using ActiveSensor = AS7341Sensor;
#elif defined(SENSOR_AS7343)
  #include "../sensors/AS7343.h"
  using ActiveSensor = AS7343Sensor;
#elif defined(SENSOR_AS7262)
  #include "../sensors/AS7262.h"
  using ActiveSensor = AS7262Sensor;
#elif defined(SENSOR_TCS34725)
  #include "../sensors/TCS34725.h"
  using ActiveSensor = TCS34725Sensor;
#elif defined(SENSOR_APDS9960)
  #include "../sensors/APDS9960.h"
  using ActiveSensor = APDS9960Sensor;
#elif defined(SENSOR_BH1750)
  #include "../sensors/BH1750.h"
  using ActiveSensor = BH1750Sensor;
#elif defined(SENSOR_LTR303)
  #include "../sensors/LTR303.h"
  using ActiveSensor = LTR303Sensor;
#elif defined(SENSOR_LTR329)
  #include "../sensors/LTR329.h"
  using ActiveSensor = LTR329Sensor;
#elif defined(SENSOR_LTR390)
  #include "../sensors/LTR390.h"
  using ActiveSensor = LTR390Sensor;
#elif defined(SENSOR_TSL2591)
  #include "../sensors/TSL2591.h"
  using ActiveSensor = TSL2591Sensor;
#else
  #error "No SENSOR_xxx build flag defined. Pick a PlatformIO environment for a specific sensor (see platformio.ini)."
#endif

namespace SensorManager {

inline SensorBase& get() {
  static ActiveSensor instance;
  return instance;
}

} // namespace SensorManager
