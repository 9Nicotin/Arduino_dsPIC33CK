@echo off
REM Wrapper for xc-dsc-gcc linker stage.
REM The GLD preprocessor (elf-cc1.exe) writes temp files to CWD.
REM Arduino IDE's CWD may not be writable, so we cd to TEMP first.
cd /d "%TEMP%"
%* 2>nul
exit /b %errorlevel%
