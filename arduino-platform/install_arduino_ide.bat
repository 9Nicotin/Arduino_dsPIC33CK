@echo off
REM ============================================================
REM  Arduino_dsPIC33CK - Arduino IDE Installation Script
REM
REM  This script:
REM   1. Copies platform files to Arduino IDE hardware folder
REM   2. Auto-detects XC-DSC, DFP, and MPLAB X paths
REM   3. Generates platform.local.txt with detected paths
REM ============================================================

cd /d "%~dp0"
echo.
echo ============================================
echo  Arduino_dsPIC33CK Platform Installer
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

REM Target installation path
set "INSTALL_PATH=%ARDUINO_DATA%\packages\microchip\hardware\dspic33ck\1.0.0"

echo [1/5] Creating installation directory...
if exist "%INSTALL_PATH%" (
    echo       Removing existing installation...
    rmdir /s /q "%INSTALL_PATH%"
)
mkdir "%INSTALL_PATH%"

echo [2/5] Copying platform files...
xcopy /E /I /Q "microchip\dspic33ck\*" "%INSTALL_PATH%\"

echo [3/5] Verifying installation...
if exist "%INSTALL_PATH%\boards.txt" (
    echo       boards.txt .... OK
) else (
    echo       [ERROR] boards.txt missing!
    goto :error
)
if exist "%INSTALL_PATH%\platform.txt" (
    echo       platform.txt .. OK
) else (
    echo       [ERROR] platform.txt missing!
    goto :error
)
if exist "%INSTALL_PATH%\cores\arduino\Arduino.h" (
    echo       Arduino.h ..... OK
) else (
    echo       [ERROR] Arduino.h missing!
    goto :error
)

echo [4/5] Auto-detecting tool paths...
echo.

REM ---- Detect XC-DSC Compiler ----
set "XC_DSC_PATH="
for /d %%V in ("C:\Program Files\Microchip\xc-dsc\v*") do (
    if exist "%%V\bin\xc-dsc-gcc.exe" set "XC_DSC_PATH=%%V\bin\"
)
if defined XC_DSC_PATH (
    echo       XC-DSC found: %XC_DSC_PATH%
) else (
    echo       [WARNING] XC-DSC compiler NOT found!
    echo       Expected at: C:\Program Files\Microchip\xc-dsc\v*\bin\
    echo       Download from: https://www.microchip.com/xc-dsc
    set "XC_DSC_PATH=C:\Program Files\Microchip\xc-dsc\v3.31\bin\"
    echo       Using default: %XC_DSC_PATH%
)

REM ---- Detect DFP: MP family (dsPIC33CK-MP_DFP) - pick newest version ----
set "DFP_PATH="
for /f "delims=" %%P in ('powershell -NoProfile -Command "Get-ChildItem '%USERPROFILE%\.mchp_packs\Microchip\dsPIC33CK-MP_DFP' -Directory 2>$null | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1 | ForEach-Object { $_.FullName + '\xc16' }"') do set "DFP_PATH=%%P"
if defined DFP_PATH (
    if exist "%DFP_PATH%" (
        echo       DFP [MP] found: %DFP_PATH%
    ) else (
        set "DFP_PATH="
    )
)
if not defined DFP_PATH (
    echo       [WARNING] MP Device Family Pack NOT found!
    echo       Expected at: %USERPROFILE%\.mchp_packs\Microchip\dsPIC33CK-MP_DFP\*\xc16
    echo       Install via MPLAB X IDE: Tools ^> Packs ^> search "dsPIC33CK-MP_DFP"
    set "DFP_PATH=%USERPROFILE%\.mchp_packs\Microchip\dsPIC33CK-MP_DFP\1.15.423\xc16"
    echo       Using default: %DFP_PATH%
)

REM ---- Detect DFP: MC family (dsPIC33CK-MC_DFP) - pick newest version ----
set "DFP_MC_PATH="
for /f "delims=" %%P in ('powershell -NoProfile -Command "Get-ChildItem '%USERPROFILE%\.mchp_packs\Microchip\dsPIC33CK-MC_DFP' -Directory 2>$null | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1 | ForEach-Object { $_.FullName + '\xc16' }"') do set "DFP_MC_PATH=%%P"
if defined DFP_MC_PATH (
    if exist "%DFP_MC_PATH%" (
        echo       DFP [MC] found: %DFP_MC_PATH%
    ) else (
        set "DFP_MC_PATH="
    )
)
if not defined DFP_MC_PATH (
    echo       [INFO] MC Device Family Pack not found ^(optional, for MC002 board^)
    echo       Install via MPLAB X IDE: Tools ^> Packs ^> search "dsPIC33CK-MC_DFP"
    set "DFP_MC_PATH="
)

REM ---- Detect MPLAB X IPE ----
set "MPLAB_PATH="
for /d %%V in ("C:\Program Files\Microchip\MPLABX\v*") do (
    if exist "%%V\mplab_platform\mplab_ipe\ipecmd.exe" set "MPLAB_PATH=%%V\mplab_platform\mplab_ipe"
)
if defined MPLAB_PATH (
    echo       MPLAB X found: %MPLAB_PATH%
) else (
    echo       [WARNING] MPLAB X IDE NOT found!
    echo       Expected at: C:\Program Files\Microchip\MPLABX\v*\mplab_platform\mplab_ipe\
    echo       Upload via PICkit4/SNAP will not work without it.
    set "MPLAB_PATH=C:\Program Files\Microchip\MPLABX\v6.20\mplab_platform\mplab_ipe"
    echo       Using default: %MPLAB_PATH%
)

echo.
echo [5/5] Generating platform.local.txt...

REM Convert backslashes to forward slashes for Arduino compatibility
set "XC_DSC_FWD=%XC_DSC_PATH:\=/%"
set "DFP_FWD=%DFP_PATH:\=/%"
set "MPLAB_FWD=%MPLAB_PATH:\=/%"
set "DFP_MC_FWD=%DFP_MC_PATH:\=/%"

REM ---- Detect C++ support (xc-dsc-g++.exe) ----
set "CPP_MODE=0"
if exist "%XC_DSC_PATH%xc-dsc-g++.exe" (
    if exist "%XC_DSC_PATH%xc-dsc-cc1plus.exe" (
        set "CPP_MODE=1"
        echo       C++ support: ENABLED ^(xc-dsc-g++.exe found^)
    )
)
if "%CPP_MODE%"=="0" (
    echo       C++ support: not available ^(fallback to C mode^)
    echo       To enable: place xc-dsc-cc1plus.exe + xc-dsc-g++.exe in compiler bin/
)

(
echo # platform.local.txt - Auto-generated by install_arduino_ide.bat
echo # Edit paths below if auto-detection was incorrect.
echo # Restart Arduino IDE after editing.
echo.
echo # XC-DSC compiler bin/ directory
echo build.compiler.path=%XC_DSC_FWD%
echo.
echo # Device Family Pack path ^(MP family - for dsPIC33CK32MP102^)
echo build.dfp.path=%DFP_FWD%
echo.
echo # Device Family Pack path ^(MC family - for dsPIC33CK256MC002^)
echo build.dfp.path.mc=%DFP_MC_FWD%
echo.
echo # MPLAB X IPE directory ^(for PICkit 4 / SNAP programming^)
echo build.tools.mplab.path=%MPLAB_FWD%
) > "%INSTALL_PATH%\platform.local.txt"

REM If no C++ support, fall back to gcc for everything (plain C mode)
if "%CPP_MODE%"=="0" (
    (
    echo.
    echo # C++ fallback: xc-dsc-g++ not found, compile everything as plain C
    echo compiler.c.cmd=xc-dsc-gcc
    echo compiler.c.extra_flags=
    echo compiler.cpp.cmd=xc-dsc-gcc
    echo compiler.cpp.extra_flags=-x c
    ) >> "%INSTALL_PATH%\platform.local.txt"
)

echo       platform.local.txt generated with detected paths

REM Copy package index
if exist "package_microchip_dspic33ck_index.json" (
    copy /Y "package_microchip_dspic33ck_index.json" "%ARDUINO_DATA%\" >nul
)

echo.
echo ============================================
echo  Installation Complete!
echo ============================================
echo.
echo  Installed to:
echo    %INSTALL_PATH%
echo.
echo  Detected paths:
echo    Compiler:  %XC_DSC_PATH%
echo    DFP [MP]:  %DFP_PATH%
echo    DFP [MC]:  %DFP_MC_PATH%
echo    MPLAB X:   %MPLAB_PATH%
echo.
echo  Next steps:
echo    1. Open (or restart) Arduino IDE
echo    2. Go to Tools ^> Board
echo    3. Select "Arduino_dsPIC33CK (dsPIC33CK32MP102)"
echo    4. Click Verify to test compilation
echo.
echo  If paths are wrong, edit:
echo    %INSTALL_PATH%\platform.local.txt
echo.
pause
exit /b 0

:error
echo.
echo [ERROR] Installation failed! Check file paths.
pause
exit /b 1
