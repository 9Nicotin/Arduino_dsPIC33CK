# Arduino_dsPIC33CK

A custom Arduino-compatible platform that allows you to program Microchip dsPIC33CK
devices using familiar Arduino APIs (digitalWrite, analogRead, Serial, etc.) while
using the XC-DSC compiler underneath.

> **Note:** the sections below on pin mapping, the API and MPLAB X still describe
> only the dsPIC33CK32MP102, and predate C++ support. Four devices are supported
> today (MP102, MP508, MC002, MC005) and the core is C++ throughout. The
> Prerequisites, Installation and Troubleshooting sections are current; the rest
> is being rewritten.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Folder Structure](#folder-structure)
4. [Pin Mapping](#pin-mapping)
5. [Supported Arduino Functions](#supported-arduino-functions)
6. [How to Write a Sketch](#how-to-write-a-sketch)
7. [Building with MPLAB X / XC16](#building-with-mplab-x--xc16)
8. [Building with Arduino IDE](#building-with-arduino-ide)
9. [Upload Methods](#upload-methods)
10. [Clock Configuration](#clock-configuration)
11. [Limitations & Differences](#limitations--differences)
12. [Troubleshooting](#troubleshooting)

---

## Prerequisites

**Windows only.** Every compile and upload goes through a `.bat` wrapper; a Linux
and macOS port is outstanding (see `platform.txt` TODO markers).

| Requirement | Version | Notes |
|------------|---------|-------|
| XC-DSC compiler | **v4.00+** | [Download](https://www.microchip.com/xc-dsc). v4.00 is the first with C++ (`xc-dsc-g++`), which this core requires. Free edition is fine. |
| Arduino IDE | 2.x | [Download](https://www.arduino.cc/en/software) |
| MPLAB X IDE | v6.20+ | For PICkit / SNAP / PKOB4 / nEDBG upload and debugging. On dsPIC33CK256MC005 it is also needed **once**, to burn the serial bootloader — after that, uploads go over the UART without it. |
| Python 3 | 3.8+ | Only for serial bootloader upload. Standard library only: **no `pyserial`, no `intelhex`**, nothing to `pip install`. |

You do **not** need to install the Device Family Packs yourself: they are
Apache-2.0 licensed, so the Boards Manager ships them (pruned to the supported
devices) and the platform locates them automatically. The compiler and MPLAB X
cannot be shipped — their licences are non-transferable — so they stay
user-installed and are located at build/upload time instead.

### Hardware
- One of: EV08P02A Curiosity Nano (MC005), Curiosity DM330030 (MP508), or a
  custom MC002 / MP102 board
- PICkit 4 / SNAP programmer for ICSP upload — not needed on the Curiosity Nano,
  which has an onboard nEDBG debugger
- OR a USB-to-UART adapter (for bootloader upload)
- LED + 330Ω resistor (for the Blink example)
- Potentiometer (for the AnalogRead example)

---

## Installation

### Option A: Boards Manager (recommended)

1. Open Arduino IDE → **File → Preferences → Additional Boards Manager URLs**
   and add:
   ```
   https://github.com/9Nicotin/Arduino_dsPIC33CK/releases/latest/download/package_microchip_dspic33ck_index.json
   ```
2. Go to **Tools → Board → Boards Manager**, search for "dsPIC33CK", and install.
3. Select **Tools → Board → Arduino_dsPIC33CK**.

No path configuration is needed, and no `platform.local.txt` is involved.

### Option B: Developer install (for working on this platform)

The Boards Manager can only install *released* archives, so contributors run:

```
arduino-platform\install_arduino_ide.bat
```

which copies `microchip\dspic33ck\` over the installed platform and checks the
prerequisites. Re-run it after every source change. If you have never installed
the released core, it also writes a `platform.local.txt` holding just the two DFP
paths, taken from your local `%USERPROFILE%\.mchp_packs` install; delete that file
once you install from the Boards Manager, or it will shadow the tool packs.

### Option C: Use directly with MPLAB X (no Arduino IDE)

You can use this as a library within MPLAB X. See
[Building with MPLAB X](#building-with-mplab-x--xc16).

### Overriding tool paths

Only needed if XC-DSC or MPLAB X is installed outside
`C:\Program Files\Microchip\`. Create a `platform.local.txt` next to the installed
`platform.txt` and set either of:

```
build.compiler.path=D:/tools/xc-dsc/v4.00/bin/
build.tools.mplab.path=D:/tools/MPLABX/v6.35/mplab_platform/mplab_ipe
```

Restart the IDE afterwards. The `XCDSC_PATH` and `MPLABX_IPE_PATH` environment
variables do the same job for one-off overrides. Without either, the newest
version found under the default install roots is used, and a missing compiler
fails the build with a message naming the download page rather than a confusing
"command not found".

---

## Folder Structure

```
arduino-platform/
└── microchip/
    └── dspic33ck/
        ├── boards.txt              # Board definitions (MCU, clock, upload)
        ├── platform.txt            # Compiler toolchain config (XC16)
        ├── cores/
        │   └── arduino/
        │       ├── Arduino.h           # Main API header
        │       ├── main.c              # Entry point (calls setup/loop)
        │       ├── system_config.h     # Config bits & clock setup
        │       ├── system_config.c     # System initialization
        │       ├── wiring.c            # millis, micros, delay
        │       ├── wiring_digital.c    # pinMode, digitalWrite, digitalRead
        │       ├── wiring_analog.c     # analogRead, analogWrite (PWM)
        │       ├── wiring_shift.c      # shiftOut, shiftIn, pulseIn
        │       ├── HardwareSerial.h    # Serial API header
        │       └── HardwareSerial.c    # UART1 implementation
        ├── variants/
        │   └── dspic33ck32mp102/
        │       ├── pins_arduino.h      # Pin mapping definitions
        │       └── variant.c           # Pin-to-register lookup table
        ├── bootloaders/
        │   └── dspic33ck256mc005/      # serial bootloader: C sources + the .hex
        │                               #   that Tools > Burn Bootloader flashes
        ├── ldscripts/                  # generated linker scripts for bootloader
        │                               #   builds (see tools/gld/)
        ├── tools/
        │   └── serial_upload.py        # stdlib-only serial upload tool
        └── libraries/
            ├── Wire/src/               # I2C
            ├── SPI/src/                # SPI
            ├── HRPWM/                  # high-resolution PWM, + BoostMPPT example
            └── Arduino_dsPIC33CK/      # exists to carry the board examples:
                └── examples/           #   Arduino builds File > Examples from
                    ├── 01.Basics/      #   libraries only, never from a
                    ├── 02.CppFeatures/ #   platform-level examples/ directory
                    ├── 03.PWM/
                    └── 04.CuriosityNano/
```

---

## Pin Mapping

### dsPIC33CK32MP102 (28-pin) → Arduino Pin Numbers

| Arduino Pin | dsPIC Pin | Port.Bit | Function | Notes |
|:-----------:|:---------:|:--------:|----------|-------|
| D0 | 2 | RA0 | Digital / LED_BUILTIN | |
| D1 | 3 | RA1 | Digital | |
| D2 | 9 | RA2 | Digital | |
| D3 | 10 | RA3 | Digital | |
| D4 | 12 | RA4 | Digital | |
| **D5** | 4 | **RB0** | Digital / **A0** / PWM (SCCP1) | Analog AN0 |
| **D6** | 5 | **RB1** | Digital / **A1** / PWM (SCCP2) | Analog AN1 |
| **D7** | 6 | **RB2** | Digital / **A2** / PWM (SCCP3) | Analog AN2 |
| **D8** | 7 | **RB3** | Digital / **A3** / PWM (SCCP4) | Analog AN3 |
| **D9** | 11 | **RB4** | Digital / **A4** / **Serial RX** | UART1 RX |
| **D10** | 14 | **RB5** | Digital / **A5** / **Serial TX** | UART1 TX |
| D11 | 15 | RB6 | Digital / **MOSI** (SPI) | |
| D12 | 16 | RB7 | Digital / **MISO** (SPI) | |
| D13 | 17 | RB8 | Digital / **SCK** (SPI) | |
| D14 | 18 | RB9 | Digital / **SDA** (I2C) | |
| D15 | 19 | RB10 | Digital / **SCL** (I2C) | |
| D16 | 21 | RB11 | Digital / **SS** (SPI) | |
| D17 | 22 | RB12 | Digital | |
| D18 | 23 | RB13 | Digital | |
| D19 | 24 | RB14 | Digital | |
| D20 | 25 | RB15 | Digital | |

### Quick Reference
```
Analog Inputs:    A0=D5, A1=D6, A2=D7, A3=D8, A4=D9, A5=D10
PWM Outputs:      D5, D6, D7, D8
Serial (UART1):   TX=D10(RB5), RX=D9(RB4)
SPI:              MOSI=D11, MISO=D12, SCK=D13, SS=D16
I2C:              SDA=D14, SCL=D15
LED_BUILTIN:      D0 (RA0)
```

---

## Supported Arduino Functions

### Digital I/O
| Function | Status | Notes |
|----------|:------:|-------|
| `pinMode(pin, mode)` | ✅ | INPUT, OUTPUT, INPUT_PULLUP |
| `digitalWrite(pin, val)` | ✅ | Uses LATx for glitch-free output |
| `digitalRead(pin)` | ✅ | Reads PORTx register |

### Analog I/O
| Function | Status | Notes |
|----------|:------:|-------|
| `analogRead(pin)` | ✅ | 12-bit ADC, returns 0-1023 (scaled) |
| `analogWrite(pin, val)` | ✅ | 8-bit PWM on D5-D8 via SCCP |
| `analogReference(mode)` | ⚠️ | Only DEFAULT (AVdd) supported |

### Timing
| Function | Status | Notes |
|----------|:------:|-------|
| `millis()` | ✅ | Timer1 interrupt-driven |
| `micros()` | ✅ | Derived from Timer1 + TMR1 count |
| `delay(ms)` | ✅ | Blocking delay |
| `delayMicroseconds(us)` | ✅ | Cycle-counting method |

### Serial (UART)
| Function | Status | Notes |
|----------|:------:|-------|
| `Serial_begin(baud)` | ✅ | Configures UART1 with PPS |
| `Serial_print(str)` | ✅ | Print string |
| `Serial_println(str)` | ✅ | Print with newline |
| `Serial_println_int(val, base)` | ✅ | Print number (DEC/HEX/BIN) |
| `Serial_available()` | ✅ | Interrupt-driven RX buffer |
| `Serial_read()` | ✅ | Read one byte |
| `Serial_write(byte)` | ✅ | Write one byte |

### Advanced I/O
| Function | Status | Notes |
|----------|:------:|-------|
| `shiftOut()` | ✅ | Bit-bang |
| `shiftIn()` | ✅ | Bit-bang |
| `pulseIn()` | ✅ | Timeout-based |
| `tone()` | 🚧 | Future (use SCCP) |
| `noTone()` | 🚧 | Future |

### Since implemented (this table was written before they landed)
| Feature | Notes |
|---------|-------|
| Wire (I2C) | `libraries/Wire/` on I2C1 |
| SPI | `libraries/SPI/` on SPI1 |
| HRPWM | `libraries/HRPWM/` — 500 MHz high-resolution PWM, 250 ps edges (MP parts only) |
| `attachInterrupt()` | `cores/arduino/wiring_interrupts.c` |
| `tone()` / `noTone()` | `cores/arduino/wiring_tone.c` |

### Still not implemented
| Feature | Notes |
|---------|-------|
| EEPROM | dsPIC33CK has no EEPROM; needs a Flash emulation library |
| `String` class | Use C strings |

---

## How to Write a Sketch

Create a `.ino` file (or `.c` file) with `setup()` and `loop()`:

```c
#include <Arduino.h>
#include "HardwareSerial.h"

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial_begin(115200);
    Serial_println("Hello from dsPIC33CK!");
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);

    int adc = analogRead(A0);
    Serial_print("A0 = ");
    Serial_println_int(adc, DEC);
}
```

**Note:** the sketch above uses the older C-style calls. Since XC-DSC v4.00 the core
is compiled as C++ and `Serial` is a class, so ordinary Arduino dot notation is what
you should write:

```cpp
#include <Arduino.h>

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    Serial.println("Hello from dsPIC33CK!");
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);

    Serial.print("A0 = ");
    Serial.println(analogRead(A0));
}
```

The `Serial_begin()` style still works from `.c` files, where `Serial` is a struct of
function pointers instead.

---

## Building with MPLAB X / XC16

If you prefer to use MPLAB X IDE instead of Arduino IDE:

### Step 1: Create MPLAB X Project
1. File → New Project → Standalone Project
2. Select **dsPIC33CK32MP102**
3. Select **XC-DSC** compiler

### Step 2: Add Arduino Core Files
Add all files from `cores/arduino/` and `variants/dspic33ck32mp102/` to your project:
- `Arduino.h`, `main.c`, `system_config.h`, `system_config.c`
- `wiring.c`, `wiring_digital.c`, `wiring_analog.c`, `wiring_shift.c`
- `HardwareSerial.h`, `HardwareSerial.c`
- `pins_arduino.h`, `variant.c`

### Step 3: Configure Include Paths
In Project Properties → XC16 → Include directories:
```
../arduino-platform/microchip/dspic33ck/cores/arduino
../arduino-platform/microchip/dspic33ck/variants/dspic33ck32mp102
```

### Step 4: Add Preprocessor Defines
```
F_CPU=8000000UL
FCY=4000000UL
ARDUINO_DSPIC33CK32MP102
```

### Step 5: Write Your Sketch
Replace `main.c` in the project with your sketch file containing `setup()` and `loop()`. The Arduino `main.c` will handle calling them.

### Step 6: Build & Program
- Click Build (hammer icon)
- Program with PICkit 4 via **Debug → Program**

---

## Building with Arduino IDE

### Prerequisites
- XC16 compiler installed and in PATH
- Platform files installed (see [Installation](#installation))

### Steps
1. Select board: **Tools → Board → dsPIC33CK32MP102 Board**
2. Select clock: **Tools → Clock → 8 MHz Internal FRC** (or 100 MHz PLL)
3. Select upload method: **Tools → Upload → UART Bootloader** or **PICkit**
4. Select port: **Tools → Port → COMx** (for UART)
5. Write sketch in the editor
6. Click **Upload** (→ arrow button)

---

## Upload Methods

### Method 1: PICkit 4 / SNAP (Recommended for Development)

Directly programs flash via ICSP. No bootloader needed.

**Wiring:**
```
PICkit Pin 1 (MCLR)  → MCLR pin
PICkit Pin 2 (VDD)   → VDD (3.3V)
PICkit Pin 3 (GND)   → GND
PICkit Pin 4 (PGD)   → PGD1 (RB0 or configured PGD pin)
PICkit Pin 5 (PGC)   → PGC1 (RB1 or configured PGC pin)
```

**Command:**
```bash
ipecmd -TPPK4 -PdsPIC33CK32MP102 -M -Ffirmware.hex
```

### Method 2: Serial Bootloader (dsPIC33CK256MC005 only)

Uploads over the UART with no debugger. Burn the bootloader once with
**Tools → Burn Bootloader**, select **Tools → Bootloader → Serial (UART, 115200)**,
and press Upload — the IDE does the rest. See
[Uploading over the serial bootloader](../README.md#uploading-over-the-serial-bootloader)
for the workflow, [part5](docs/part5_upload_troubleshooting.html) for what to do when it
fails, **[part6](docs/part6_serial_bootloader.html)** for the full guide — burning it
three different ways, the memory map, interrupt forwarding, recovery, using a plain
USB-serial adapter instead of the debugger, and porting it to your own board — and
**[part7](docs/part7_bench_verification.html)** for the step-by-step procedure to verify
all of it on a real board.

All five programmers can burn it (*PICkit 5*, *PICkit 4*, *MPLAB SNAP*, *PKOB4*,
*nEDBG*); the *Serial Bootloader (UART)* programmer entry cannot, since it would have to
already be installed.

**Wiring** (already wired on the EV08P02A Curiosity Nano, whose nEDBG provides the
CDC bridge):
```
USB-Serial TX  → RC11 (D32, UART1 RX)
USB-Serial RX  → RC10 (D31, UART1 TX)
USB-Serial GND → GND
```

**Invoking the tool directly**, which the IDE does for you:
```bash
python tools/serial_upload.py --port COM3 --hex sketch.hex
```
There is no `--baud`: 115200 is the only rate the bootloader speaks. Add
`--dry-run` to parse and check a HEX without touching the board.

**Note:** the sketch must be *compiled* with the Serial bootloader menu option, not
merely uploaded through it — a sketch linked for the debugger is refused rather than
written to the wrong address. The bootloader occupies 0x000000–0x0017FF and the
application starts at 0x001800. Programming with a debugger erases the bootloader.

**Also note:** entry is negotiated with the *running* sketch, so a sketch that never
calls `Serial.begin()` — `NanoBlink` is the one to watch for — blocks the next serial
upload. **Tools → Burn Bootloader** clears it without touching the hardware; on a board
with no debugger, hold **SW0** through a power cycle.

---

## Clock Configuration

### 8 MHz Internal FRC (Default)
- FOSC = 8 MHz
- FCY = 4 MHz (instruction cycle)
- No external crystal required
- Good for low-power applications

### 100 MHz PLL (High Performance)
- FOSC = 100 MHz (FRC × PLL)
- FCY = 50 MHz
- Maximum performance
- Select in boards.txt menu or `#define F_CPU 100000000UL`

### How PLL is Configured
```
FOSC = FRC × M / (N1 × N2 × N3)
100 MHz = 8 MHz × 100 / (2 × 2 × 1)
```

---

## Limitations & Differences from Standard Arduino

| Area | Arduino (AVR/ARM) | This Platform (dsPIC33CK) |
|------|-------------------|---------------------------|
| Compiler | avr-gcc / arm-gcc (C++) | XC-DSC v4.00+ (C++) |
| Serial API | `Serial.begin()` (C++ class) | `Serial.begin()` (C++ class); `Serial_begin()` from `.c` |
| Voltage | 5V (AVR) / 3.3V (ARM) | **3.3V only** |
| ADC Resolution | 10-bit (AVR) | 12-bit (returned as 10-bit) |
| PWM Resolution | 8-bit | 8-bit (configurable up to 16-bit) |
| Flash Size | varies | 32 KB (app starts at 0x1000 with bootloader) |
| RAM | varies | 4 KB |
| EEPROM | Yes (AVR) | No (use Flash emulation) |
| Libraries | Vast ecosystem | Limited (need porting) |
| `String` class | Yes | No (use C strings) |
| `new`/`delete` | Yes | No (C only, use malloc/free) |

### Important Notes
1. **3.3V ONLY** — Do NOT connect 5V signals to dsPIC33CK pins!
2. **No `String` class, no `new`/`delete`** — C++ is available, but the core does not
   ship a `String` implementation or a heap wrapper. Use C strings and `malloc`/`free`
   if you must.
3. **PPS (Peripheral Pin Select)** — UART/SPI/PWM outputs must be mapped via PPS. The core handles this automatically for default pins.
4. **Analog pins are shared** — A0-A5 share physical pins with D5-D10.

---

## Troubleshooting

### Build Errors

| Error | Solution |
|-------|----------|
| `XC-DSC compiler not found` | Install [XC-DSC v4.00+](https://www.microchip.com/xc-dsc). It does **not** need to be on PATH. If it is installed outside `C:\Program Files\Microchip\`, see [Overriding tool paths](#overriding-tool-paths). |
| `xc-dsc-g++.exe` missing / C++ errors from a C compiler | Your XC-DSC predates v4.00. The core is C++ throughout; upgrade. |
| `Cannot find <xc.h>`, or `CPU not recognized` | The Device Family Pack is not resolving. With a Boards Manager install this should not happen — check for a stale `platform.local.txt` next to `platform.txt` overriding `build.dfp.path` with a path that no longer exists, and delete it. |
| `ipecmd.exe not found` on upload | Install MPLAB X, or see [Overriding tool paths](#overriding-tool-paths). |
| `Undefined _PORTA` | Ensure MCU is set to `dsPIC33CK32MP102` in build flags |
| `Multiple definition of main` | Remove your own `main()` — use `setup()`/`loop()` instead |

### Upload Errors

| Error | Solution |
|-------|----------|
| Cannot sync with bootloader | Reset device while holding BOOT pin; check TX/RX wiring |
| COM port not found | Check Device Manager; install USB-Serial drivers |
| PICkit not detected | Install MPLAB X IPE; check USB connection |
| Erase failed | Device may be code-protected; do bulk erase first |

### Runtime Issues

| Issue | Solution |
|-------|----------|
| `millis()` not counting | Check Timer1 interrupt is enabled (IEC0bits.T1IE) |
| `analogRead()` returns 0 | Ensure pin is in analog mode (ANSEL bit set) |
| Serial garbled output | Verify baud rate matches; check FCY matches your clock config |
| PWM not working | Verify PPS output mapping; check SCCP module is enabled |

---

## Future Roadmap

- [x] Wire (I2C) library using I2C1 peripheral
- [x] SPI library using SPI1 peripheral
- [x] `attachInterrupt()` via Change Notification (CN) pins
- [x] `tone()` / `noTone()` via SCCP timer
- [x] Arduino IDE Board Manager package (install from the URL above)
- [x] Support for dsPIC33CK256MP508, MC002 and MC005
- [ ] Linux / macOS support (shell ports of the `tools\*.bat` wrappers)
- [ ] Flash-based EEPROM emulation library
- [ ] Pre-compiled UART bootloader hex file
- [ ] Rewrite the pin-mapping and API sections above to cover all four devices

---

## License

This Arduino platform core is provided for educational and development purposes.
The underlying Microchip device support requires acceptance of Microchip's license terms.
XC16 compiler is available in a free (unlicensed) mode with limited optimization.

---

## References

- [dsPIC33CK32MP102 Datasheet](https://www.microchip.com/dsPIC33CK32MP102)
- [XC16 Compiler User's Guide](https://www.microchip.com/xc16)
- [Arduino Platform Specification](https://arduino.github.io/arduino-cli/platform-specification/)
- [chipKIT (PIC32 Arduino)](https://chipkit.net/) — Inspiration for this project
