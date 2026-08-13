@echo off
REM =============================================================================
REM install_libraries.bat — Windows
REM -----------------------------------------------------------------------------
REM Downloads EVERY vendor library (and both board toolchains: ESP32-S3 and
REM AVR/Uno) needed to build ANY of this project's sensor environments — so
REM you don't have to know in advance which of the sensors you'll end up
REM using, and you don't have to hunt down each library one at a time.
REM
REM It works by reading every "[env:...]" section straight out of
REM platformio.ini and asking PlatformIO to install that environment's
REM dependencies (lib_deps + platform toolchain). Add an 11th sensor's
REM environments to platformio.ini later and this script picks them up
REM automatically — nothing here is hardcoded to a specific sensor list.
REM
REM Requirements: PlatformIO Core ("pio") on your PATH.
REM   pip install -U platformio
REM (If you use the "PlatformIO IDE" extension in VS Code instead, it installs
REM its own copy of pio — open a terminal from inside VS Code at least once so
REM your PATH picks it up, or just run this script from VS Code's terminal.)
REM
REM Usage:
REM   install_libraries.bat
REM =============================================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

where pio >nul 2>nul
if errorlevel 1 (
  echo ERROR: PlatformIO Core "pio" was not found on your PATH.
  echo Install it first:  pip install -U platformio
  echo ^(or install the "PlatformIO IDE" extension in VS Code, then re-open a terminal^)
  exit /b 1
)

if not exist platformio.ini (
  echo ERROR: platformio.ini not found in %cd% — run this script from the project root.
  exit /b 1
)

set COUNT=0
set ENVLIST=
for /f "usebackq delims=" %%L in (`findstr /r "^\[env:" platformio.ini`) do (
  set "LINE=%%L"
  set "LINE=!LINE:[env:=!"
  set "LINE=!LINE:]=!"
  set /a COUNT+=1
  echo   - !LINE!
  set "ENVLIST=!ENVLIST! -e !LINE!"
)

if "!COUNT!"=="0" (
  echo ERROR: no [env:...] sections found in platformio.ini — nothing to install.
  exit /b 1
)

echo.
echo Found !COUNT! environment(s) in platformio.ini ^(listed above^).
echo Installing platform toolchains + libraries for all of them ^(this can take
echo a few minutes the first time, especially the ESP32-S3 and AVR toolchains^)...
echo.

pio pkg install !ENVLIST!
if errorlevel 1 exit /b 1

echo.
echo Done. Every sensor's library is now cached locally under .pio\libdeps\,
echo and both board toolchains are installed. You can now build/upload any
echo environment, e.g.:
echo    pio run -e as7341_esp32s3   -t upload
echo    pio run -e apds9960_uno     -t upload
