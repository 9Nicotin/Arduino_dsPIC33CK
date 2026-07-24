# Arduino_dsPIC33CK

A custom Arduino-compatible platform that allows you to program Microchip dsPIC33CK32MP102 using familiar Arduino APIs (digitalWrite, analogRead, Serial, etc.) while using the XC16 (XC-DSC) compiler underneath.

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

| Requirement | Version | Notes |
|------------|---------|-------|
| XC16 Compiler (XC-DSC) | v3.31+ | [Download](https://www.microchip.com/xc16) |
| MPLAB X IDE (optional) | v6.20+ | For debugging / PICkit programming |
| Python 3 | 3.8+ | For UART bootloader upload |
| pyserial | latest | `pip install pyserial` |
| intelhex | latest | `pip install intelhex` |
| Arduino IDE (optional) | 2.x | For full Arduino IDE integration |
| dsPIC33CK-MP DFP | 1.15+ | Device Family Pack (via MPLAB) |

### Hardware
- dsPIC33CK32MP102 development board (or custom board)
- PICkit 4 / SNAP programmer (for ICSP upload)
- OR USB-to-UART adapter (for bootloader upload)
- LED + 330Ω resistor (for Blink example)
- Potentiometer (for AnalogRead example)

---

## Installation

### Option A: Arduino IDE Integration

1. Open Arduino IDE → **File → Preferences**
2. In "Additional Boards Manager URLs", add:
   ```
   file:///C:/Users/A18434/MPLABProjects/SCCP1_dsPIC33CK/arduino-platform/package_microchip_dspic33ck_index.json
   ```
   (Or host the JSON on a local/web server)

3. Go to **Tools → Board → Boards Manager**
4. Search for "dsPIC33CK" and install

5. Select **Tools → Board → Arduino_dsPIC33CK**

### Option B: Manual Installation (Copy to Arduino hardware folder)

```bash
# Windows
xcopy /E /I arduino-platform\microchip "%LOCALAPPDATA%\Arduino15\packages\microchip"

# Linux/Mac
cp -r arduino-platform/microchip ~/.arduino15/packages/
```

### Option C: Use Directly with XC16 (No Arduino IDE)

You can use this as a library within MPLAB X. See [Building with MPLAB X](#building-with-mplab-x--xc16).

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
        │   └── uart/                   # UART bootloader hex (future)
        ├── tools/
        │   └── upload_uart.py          # Python upload script
        ├── libraries/
        │   ├── Wire/src/               # I2C library (future)
        │   └── SPI/src/                # SPI library (future)
        └── examples/
            └── 01.Basics/
                ├── Blink/Blink.ino
                └── AnalogReadSerial/AnalogReadSerial.ino
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

### Not Yet Implemented
| Feature | Notes |
|---------|-------|
| Wire (I2C) | Use I2C1 peripheral - future library |
| SPI | Use SPI1 peripheral - future library |
| `attachInterrupt()` | CN (Change Notification) interrupts |
| EEPROM | dsPIC33CK has no EEPROM; use Flash emulation |

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

**Key difference from standard Arduino:** Since XC16 is a C compiler (not C++), `Serial` is accessed via C function calls (`Serial_begin`, `Serial_print`) rather than C++ methods (`Serial.begin`).

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

### Method 2: UART Bootloader

Requires a bootloader pre-programmed into the device.

**Wiring:**
```
USB-Serial TX → RB4 (D9, UART1 RX)
USB-Serial RX → RB5 (D10, UART1 TX)
USB-Serial GND → GND
```

**Upload command:**
```bash
python tools/upload_uart.py --port COM3 --baud 115200 --hex firmware.hex
```

**Note:** You must first program a UART bootloader into the device using a PICkit. The bootloader occupies address 0x000-0xFFF, and your application starts at 0x1000.

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
| Compiler | avr-gcc / arm-gcc (C++) | XC16 (C only) |
| Serial API | `Serial.begin()` (C++ class) | `Serial_begin()` (C function) |
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
2. **No C++ support** — XC16 is C-only. Use C-style function calls.
3. **PPS (Peripheral Pin Select)** — UART/SPI/PWM outputs must be mapped via PPS. The core handles this automatically for default pins.
4. **Analog pins are shared** — A0-A5 share physical pins with D5-D10.

---

## Troubleshooting

### Build Errors

| Error | Solution |
|-------|----------|
| `xc16-gcc not found` | Add XC16 bin folder to PATH: `C:\Program Files\Microchip\xc16\v3.31\bin` |
| `Cannot find <xc.h>` | Install dsPIC33CK-MP Device Family Pack |
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

- [ ] Wire (I2C) library using I2C1 peripheral
- [ ] SPI library using SPI1 peripheral
- [ ] `attachInterrupt()` via Change Notification (CN) pins
- [ ] `tone()` / `noTone()` via SCCP timer
- [ ] Flash-based EEPROM emulation library
- [ ] Support for dsPIC33CK64MP/128MP/256MP variants
- [ ] Arduino IDE Board Manager JSON package
- [ ] Pre-compiled UART bootloader hex file

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
