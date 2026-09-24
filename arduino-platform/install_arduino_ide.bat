@echo off
REM ============================================================
REM  Arduino_dsPIC33CK - install the working copy for development
REM
REM  END USERS DO NOT NEED THIS SCRIPT. The platform installs from the
REM  Boards Manager like any other third-party core; see README.md for the
REM  URL. This script exists for the one thing the Boards Manager cannot do:
REM  install the *uncommitted* tree you are editing, which is what every
REM  hardware iteration needs.
REM
REM  It copies microchip\dspic33ck\ over the installed platform and then
REM  checks the prerequisites. It writes almost nothing: the compiler and
REM  MPLAB X are located at build/upload time by tools\xc-dsc-find.bat, and
REM  the Device Family Packs normally come from the Boards Manager tool
REM  packs. platform.local.txt is only generated when those packs are
REM  absent - i.e. when you have never installed the released core.
REM ============================================================

cd /d "%~dp0"
echo.
echo ============================================
echo  Arduino_dsPIC33CK - development install
echo ============================================
echo.

REM Detect Arduino IDE 2.x data folder
set "ARDUINO_DATA=%LOCALAPPDATA%\Arduino15"
if not exist "%ARDUINO_DATA%" (
    echo [!] Arduino IDE data folder not found at:
    echo     %ARDUINO_DATA%
    echo.
    echo     Please install Arduino IDE 2.x first.
    echo     Download: https://www.arduino.cc/en/software
    pause
    exit /b 1
)

REM Target installation path. Must match the version in platform.txt, so that
REM this overwrites the Boards Manager install rather than sitting beside it.
set "INSTALL_PATH=%ARDUINO_DATA%\packages\microchip\hardware\dspic33ck\1.0.4"
set "TOOLS_PATH=%ARDUINO_DATA%\packages\microchip\tools"

echo [1/4] Creating installation directory...
if exist "%INSTALL_PATH%" (
    echo       Removing existing installation...
    rmdir /s /q "%INSTALL_PATH%"
)
mkdir "%INSTALL_PATH%"

echo [2/4] Copying platform files...
xcopy /E /I /Q "microchip\dspic33ck\*" "%INSTALL_PATH%\"

echo [3/4] Verifying installation...
if exist "%INSTALL_PATH%\boards.txt" (
    echo       boards.txt ......... OK
) else (
    echo       [ERROR] boards.txt missing!
    goto :error
)
if exist "%INSTALL_PATH%\platform.txt" (
    echo       platform.txt ....... OK
) else (
    echo       [ERROR] platform.txt missing!
    goto :error
)
if exist "%INSTALL_PATH%\cores\arduino\Arduino.h" (
    echo       Arduino.h .......... OK
) else (
    echo       [ERROR] Arduino.h missing!
    goto :error
)
if exist "%INSTALL_PATH%\tools\bin\xc-dsc-g++.bat" (
    echo       compiler shims ..... OK
) else (
    echo       [ERROR] tools\bin\ shims missing - the build cannot find the compiler!
    goto :error
)

echo [4/4] Checking prerequisites...
echo.

REM Ask the platform's own resolver rather than re-implementing the search, so
REM this reports exactly what a build would use. It prints its own actionable
REM message on failure, hence no message here.
set "XCDSC_DIR="
call "%INSTALL_PATH%\tools\xc-dsc-find.bat" xcdsc
if errorlevel 1 (
    echo       [WARNING] XC-DSC compiler not found - sketches will not compile.
) else (
    echo       XC-DSC ............. %XCDSC_DIR%
    if not exist "%XCDSC_DIR%\xc-dsc-g++.exe" (
        echo       [ERROR] this XC-DSC has no xc-dsc-g++.exe.
        echo               The core is C++ throughout; v4.00 or newer is required.
        echo               Download: https://www.microchip.com/xc-dsc
    )
)

set "IPE_DIR="
call "%INSTALL_PATH%\tools\xc-dsc-find.bat" ipe
if errorlevel 1 (
    echo       [WARNING] MPLAB X not found - PICkit/SNAP/nEDBG upload will not work.
) else (
    echo       MPLAB X IPE ........ %IPE_DIR%
)

REM ---- Device Family Packs -------------------------------------------------
REM Normally supplied by the Boards Manager tool packs, in which case
REM platform.txt resolves them itself and an override here would only shadow
REM them with a stale path. Fall back to a local pack install only if they are
REM genuinely absent.
set "DFP_MP_TOOL="
set "DFP_MC_TOOL="
if exist "%TOOLS_PATH%\dsPIC33CK-MP_DFP\" set "DFP_MP_TOOL=1"
if exist "%TOOLS_PATH%\dsPIC33CK-MC_DFP\" set "DFP_MC_TOOL=1"

if defined DFP_MP_TOOL if defined DFP_MC_TOOL (
    echo       Device Family Packs  Boards Manager tool packs ^(no override written^)
    goto :dfp_done
)

if exist "%INSTALL_PATH%\platform.local.txt" (
    echo       [INFO] platform.local.txt came from your working tree; leaving it alone.
    goto :dfp_done
)

echo       [INFO] DFP tool packs not installed; falling back to %%USERPROFILE%%\.mchp_packs
call :find_pack dsPIC33CK-MP_DFP DFP_PATH
call :find_pack dsPIC33CK-MC_DFP DFP_MC_PATH

if not defined DFP_PATH (
    echo       [WARNING] dsPIC33CK-MP_DFP not found ^(needed by MP102 / MP508^)
    echo                 Install it via MPLAB X: Tools ^> Packs, or install the
    echo                 released core once from the Boards Manager URL in README.md.
)
if not defined DFP_MC_PATH (
    echo       [WARNING] dsPIC33CK-MC_DFP not found ^(needed by MC002 / MC005^)
)

(
echo # platform.local.txt - written by install_arduino_ide.bat
echo #
echo # Only here because the Boards Manager DFP tool packs were not installed
echo # when this ran. Delete this file after installing the released core once,
echo # otherwise these paths shadow the tool packs and go stale on a pack update.
echo #
echo # Restart Arduino IDE after editing.
echo build.dfp.path=%DFP_PATH:\=/%
echo build.dfp.path.mc=%DFP_MC_PATH:\=/%
) > "%INSTALL_PATH%\platform.local.txt"
echo       platform.local.txt . written ^(DFP paths only^)

:dfp_done

REM A copy of the package index in the data folder makes the IDE offer a
REM Boards Manager install of this same version, which would fight the tree we
REM just copied in. Remove any left by an older installer.
if exist "%ARDUINO_DATA%\package_microchip_dspic33ck_index.json" (
    del /q "%ARDUINO_DATA%\package_microchip_dspic33ck_index.json"
    echo       removed a stale package index from the data folder
)

echo.
echo ============================================
echo  Development install complete
echo ============================================
echo.
echo  Installed to:
echo    %INSTALL_PATH%
echo.
echo  Next steps:
echo    1. Open ^(or restart^) Arduino IDE
echo    2. Tools ^> Board ^> Arduino_dsPIC33CK
echo    3. Click Verify to test compilation
echo.
echo  Re-run this script after every source change: the IDE compiles what is
echo  in the folder above, not what is in this repo.
echo.
pause
exit /b 0

REM ---- :find_pack <pack-name> <out-var> -----------------------------------
REM Newest installed version of a local DFP, as <version>\xc16. Sorted as a
REM version, not as a string, so 1.16.521 beats 1.9.x.
:find_pack
set "%~2="
for /f "delims=" %%P in ('powershell -NoProfile -Command "Get-ChildItem '%USERPROFILE%\.mchp_packs\Microchip\%~1' -Directory 2>$null | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1 | ForEach-Object { $_.FullName + '\xc16' }"') do set "%~2=%%P"
call set "PACKDIR=%%%~2%%"
if defined PACKDIR if not exist "%PACKDIR%" set "%~2="
if defined PACKDIR echo       %~1 %PACKDIR%
exit /b 0

:error
echo.
echo [ERROR] Installation failed! Check file paths.
pause
exit /b 1
