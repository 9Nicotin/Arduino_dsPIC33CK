# Arduino_dsPIC33CK

Arduino platform for Microchip dsPIC33CK family DSCs. Install it from the Arduino
IDE Boards Manager, select a board, and upload — the usual Arduino workflow, on
dsPIC silicon.

**Windows only.** The build and upload wrappers are `.bat` files; see
[Platform support](#platform-support).

## Installation

1. In Arduino IDE, open **File → Preferences → Additional Boards Manager URLs**
   and add:

   ```
   https://github.com/9Nicotin/Arduino_dsPIC33CK/releases/latest/download/package_microchip_dspic33ck_index.json
   ```

2. Open **Tools → Board → Boards Manager**, search for **dsPIC33CK**, and click
   **Install**.

3. Select your board under **Tools → Board → Arduino_dsPIC33CK**.

You also need two Microchip tools installed, because their licences do not permit
redistribution — they cannot ship inside the platform:

| Prerequisite | Needed for | Notes |
|---|---|---|
| [XC-DSC compiler v4.00+](https://www.microchip.com/xc-dsc) | compiling | v4.00 is the first with C++ (`xc-dsc-g++`), which this core requires. The free edition is enough. |
| [MPLAB X IDE](https://www.microchip.com/mplabx) | uploading via PICkit/SNAP/nEDBG | Not needed if you only compile, or if you upload over the UART bootloader. |

Both are found automatically at the moment they are used, including picking the
newest of several installed versions, so there is nothing to configure. If you
installed either somewhere unusual, see
[Overriding tool paths](arduino-platform/README.md#overriding-tool-paths).

The Device Family Packs *are* redistributable (Apache-2.0), so the Boards Manager
installs them for you, pruned to the supported devices.

## Supported Boards

| Device | Package | Board | Notes |
|--------|---------|-------|-------|
| dsPIC33CK256MC005 | 48-pin TQFP | [EV08P02A Curiosity Nano](https://www.microchip.com/en-us/development-tool/EV08P02A) | Hardware-verified; onboard nEDBG debugger |
| dsPIC33CK256MP508 | 80-pin TQFP | Curiosity DM330030 | |
| dsPIC33CK256MC002 | 28-pin SDIP | Custom | |
| dsPIC33CK32MP102 | 28-pin SDIP | Custom | |

## Features

- Arduino API: `pinMode`, `digitalWrite`, `analogRead`/`analogWrite`, `Serial`,
  `millis`, `micros`, `delay`, `tone`, `attachInterrupt`, `shiftIn`/`shiftOut`,
  `pulseIn`, `map`
- C++ throughout (XC-DSC v4.00): classes, templates, overloading, and
  `Serial.println()` dot notation
- Libraries: SPI, Wire, HRPWM (500 MHz high-resolution PWM, 250 ps edge placement)
- PWM: ~490 Hz via SCCP modules, with automatic prescaler selection
- 11 examples: Blink, AnalogReadSerial, CppDemo, Fade, PWMTest, and a
  `04.CuriosityNano/` set for the EV08P02A (NanoBlink, NanoSelfTest,
  NanoSerialHello, NanoButtonLED, NanoPWMFade, NanoAnalogRead)

## Platform support

Windows only, and deliberately so rather than by omission: every compile and
upload goes through a `.bat` wrapper that locates the XC-DSC compiler and MPLAB X
at the moment of use. XC-DSC itself ships for Linux and macOS, so a port is a
matter of transliterating those wrappers to shell scripts and adding
`.linux`/`.macosx` recipe keys — the TODO markers are in `platform.txt`. Until
then the package index advertises Windows only, so a Linux or macOS user sees a
clear "not available for your OS" instead of an install that half-works.

## Developing this platform

Contributors need one extra step, because the Boards Manager can only install
*released* archives and never your working tree:

```
arduino-platform\install_arduino_ide.bat
```

This copies `arduino-platform/microchip/dspic33ck/` over the installed platform
and checks your prerequisites. Re-run it after every source change — the IDE
compiles what is in `%LOCALAPPDATA%\Arduino15`, not what is in this repo.

To cut a release, see [tools/release/make-release.sh](tools/release/make-release.sh),
which builds the three archives, computes their checksums, and fills them into
`arduino-platform/package_microchip_dspic33ck_index.json`.

## Structure

| Path | Purpose |
|------|---------|
| arduino-platform/ | Arduino IDE platform (cores, variants, libraries, tools) |
| arduino-platform/package_microchip_dspic33ck_index.json | Boards Manager package index (source of truth) |
| arduino-platform/install_arduino_ide.bat | Developer install of the working tree |
| tools/release/ | Release archive + checksum + index generator |
| cmake/ | CMake build files (for MPLAB X compatibility) |
| docs/ | HTML documentation |
| test_led/ | Early hardware test sketches |
