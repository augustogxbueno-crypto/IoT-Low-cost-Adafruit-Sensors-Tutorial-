// =============================================================================
// core/Export.h — intentionally minimal.
// -----------------------------------------------------------------------------
// CSV and Excel export already happen entirely in the browser (web/script.js)
// once results arrive over Serial — there was no reason to duplicate that
// logic on-device. This file exists as the designated place to add
// firmware-side export later (e.g. writing measurements to an SD card, or a
// board with its own storage), without having to touch core/Protocol.cpp or
// core/Measurement.cpp to wire it in.
// =============================================================================
#pragma once

namespace Export {
  // Nothing here yet — see the comment above.
}
