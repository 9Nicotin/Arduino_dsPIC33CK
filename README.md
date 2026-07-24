# Arduino_dsPIC33CK

Arduino platform for Microchip dsPIC33CK family DSCs.

## Supported Boards

| Device | Package | Board |
|--------|---------|-------|
| dsPIC33CK256MP508 | 80-pin TQFP | Curiosity DM330030 |
| dsPIC33CK256MC002 | 28-pin SDIP | Custom |
| dsPIC33CK32MP102 | 28-pin SDIP | Custom |

## Features

- Full Arduino API: pinMode, digitalWrite, analogRead/Write, Serial, millis, delay
- C++ support (XC-DSC v4.00): classes, templates, function overloading
- Libraries: SPI, Wire, HRPWM (500MHz high-resolution PWM, 250ps edge)
- PWM: ~490 Hz via SCCP modules, auto-prescaler selection
- Examples: Blink, AnalogReadSerial, CppDemo, Fade, PWMTest, BoostMPPT

## Requirements

- [Microchip XC-DSC Compiler v4.00+](https://www.microchip.com/xc-dsc)
- [dsPIC33CK-MP DFP](https://packs.download.microchip.com/) (Device Family Pack)
- [Arduino IDE 1.8.x or 2.x](https://www.arduino.cc/en/software)

## Installation

Run `arduino-platform/install_arduino_ide.bat` — it auto-detects your XC-DSC and DFP paths, then installs the platform into Arduino IDE.

## Structure

| Path | Purpose |
|------|---------|
| arduino-platform/ | Arduino IDE platform (cores, variants, libraries, tools) |
| arduino-platform/install_arduino_ide.bat | One-click installer |
| cmake/ | CMake build files (for MPLAB X compatibility) |
| docs/ | HTML documentation |
| test_led/ | Early hardware test sketches |
