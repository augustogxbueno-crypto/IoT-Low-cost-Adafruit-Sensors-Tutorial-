#!/usr/bin/env bash
# =============================================================================
# install_libraries.sh — macOS / Linux
# -----------------------------------------------------------------------------
# Downloads EVERY vendor library (and both board toolchains: ESP32-S3 and
# AVR/Uno) needed to build ANY of this project's sensor environments — so you
# don't have to know in advance which of the sensors you'll end up using, and
# you don't have to hunt down each library one at a time.
#
# It works by reading every "[env:...]" section straight out of
# platformio.ini and asking PlatformIO to install that environment's
# dependencies (lib_deps + platform toolchain). Add an 11th sensor's
# environments to platformio.ini later and this script picks them up
# automatically — nothing here is hardcoded to a specific sensor list.
#
# Requirements: PlatformIO Core ("pio") on your PATH.
#   pip install -U platformio
# (If you use the "PlatformIO IDE" extension in VS Code instead, it installs
# its own copy of pio — open a terminal from inside VS Code at least once so
# your shell's PATH picks it up, or run this script from VS Code's terminal.)
#
# Usage:
#   ./install_libraries.sh
# =============================================================================
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

if ! command -v pio >/dev/null 2>&1; then
  echo "ERROR: PlatformIO Core ('pio') was not found on your PATH." >&2
  echo "Install it first:  pip install -U platformio" >&2
  echo "(or install the 'PlatformIO IDE' extension in VS Code, then re-open a terminal)" >&2
  exit 1
fi

if [ ! -f platformio.ini ]; then
  echo "ERROR: platformio.ini not found in $(pwd) — run this script from the project root." >&2
  exit 1
fi

mapfile -t envs < <(grep -oE '^\[env:[A-Za-z0-9_]+\]' platformio.ini | sed -E 's/^\[env:(.+)\]$/\1/')

if [ "${#envs[@]}" -eq 0 ]; then
  echo "ERROR: no [env:...] sections found in platformio.ini — nothing to install." >&2
  exit 1
fi

echo "Found ${#envs[@]} environment(s) in platformio.ini:"
printf '  - %s\n' "${envs[@]}"
echo
echo "Installing platform toolchains + libraries for all of them (this can take"
echo "a few minutes the first time, especially the ESP32-S3 and AVR toolchains)..."
echo

env_args=()
for env in "${envs[@]}"; do
  env_args+=(-e "$env")
done

pio pkg install "${env_args[@]}"

echo
echo "Done. Every sensor's library is now cached locally under .pio/libdeps/,"
echo "and both board toolchains are installed. You can now build/upload any"
echo "environment, e.g.:"
echo "   pio run -e as7341_esp32s3   -t upload"
echo "   pio run -e apds9960_uno     -t upload"
