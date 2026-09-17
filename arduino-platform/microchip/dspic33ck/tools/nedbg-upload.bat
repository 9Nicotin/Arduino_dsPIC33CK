@echo off
REM Upload wrapper for Curiosity Nano boards (nEDBG on-board debugger).
REM
REM Programming through ipecmd leaves the nEDBG's USB-CDC bridge wedged: the
REM virtual COM port still enumerates and opens, but delivers zero bytes, so the
REM Arduino Serial Monitor appears dead after every upload. Rebooting the
REM debugger over USB clears it. This has to happen here because the IDE runs
REM upload.pattern as a single argv, not through a shell, so two commands cannot
REM be chained in platform.txt.
REM
REM   %1 = full path to ipecmd.exe
REM   %2 = device name (e.g. 33CK256MC005)
REM   %3 = path to the .hex file

setlocal

set "IPECMD=%~1"
set "MCU=%~2"
set "HEXFILE=%~3"

REM --- Step 1: program the device (identical to the plain upload.pattern) -----
"%IPECMD%" -TPnEDBG -P%MCU% -M -OL -F"%HEXFILE%"
set "RC=%errorlevel%"

REM Never reboot the debugger after a failed program: it would bury ipecmd's
REM own error message under the reboot output.
if not "%RC%"=="0" (
    echo.
    echo Programming failed ^(ipecmd exit code %RC%^) - debugger not rebooted.
    exit /b %RC%
)

REM --- Step 2: unwedge the CDC bridge ---------------------------------------
REM pymcuprog is an optional extra, and users of the PICkit4/PKOB4 boards have
REM no reason to have installed it. A missing optional tool must not fail an
REM upload that actually succeeded.
where pymcuprog >nul 2>&1
if not "%errorlevel%"=="0" (
    echo.
    echo Note: pymcuprog not found on PATH, so the nEDBG serial bridge was not
    echo       reset. If the Serial Monitor shows nothing, run
    echo       "pip install pymcuprog" once, or unplug/replug the board.
    exit /b 0
)

echo.
echo Rebooting nEDBG to restore the serial bridge...
pymcuprog reboot-debugger
if not "%errorlevel%"=="0" (
    echo Note: reboot-debugger failed. Unplug and replug the board if the
    echo       Serial Monitor stays silent.
    exit /b 0
)

REM Wait for USB re-enumeration so the IDE can reopen the port straight away.
REM "ping -n" rather than "timeout /t": timeout errors out with "input
REM redirection is not supported" when stdin is redirected, which it is here.
ping -n 8 127.0.0.1 >nul 2>&1

echo Serial bridge ready.
exit /b 0
