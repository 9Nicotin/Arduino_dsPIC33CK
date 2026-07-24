# Arduino_dsPIC33CK Platform — Project Plan

## Project Goal
Build a portable Arduino-compatible platform for Microchip dsPIC33CK family using XC-DSC compiler, enabling familiar Arduino APIs (digitalWrite, analogRead, Serial, analogWrite) while leveraging the XC-DSC toolchain underneath.

---

## COMPLETED

### Phase 1: Platform Foundation
- [x] Platform structure created (`arduino-platform/microchip/dspic33ck/`)
- [x] `boards.txt` with 3 board definitions
- [x] `platform.txt` with generic build recipes (no hardcoded paths)
- [x] `platform.local.txt.template` for machine-specific paths
- [x] `programmers.txt` for MPLAB IPE upload
- [x] `suppress-stderr.bat` — suppresses harmless XC-DSC stderr warnings
- [x] `xc-dsc-size-wrapper.bat` — size reporting via PowerShell + xc-dsc-readelf
- [x] `--whole-archive core.a` in linker recipe (CRT startup fix)
- [x] XC-DSC C-only mode (`-x c` flag for .ino/.cpp files)

### Phase 2: Core Arduino APIs
- [x] `main.c` — setup()/loop() entry point
- [x] `Arduino.h` — type definitions, macros, function prototypes
- [x] `wiring.c` — millis(), micros(), delay() using Timer1
- [x] `wiring_digital.c` — pinMode(), digitalWrite(), digitalRead()
- [x] `wiring_analog.c` — analogRead(), analogWrite()
- [x] `HardwareSerial.c` — Serial.begin(), print(), println(), available(), read()
- [x] `wiring_shift.c` — shiftIn(), shiftOut()
- [x] `system_config.c` — config bits (ONE file only, archiver constraint)
- [x] `system_config.h` — clock/pin initialization

### Phase 3: Board Variants
- [x] `variants/dspic33ck32mp102/variant.c` — pin map for MP102
- [x] `variants/dspic33ck256mc002/variant.c` — pin map for MC002
- [x] `variants/dspic33ck256mp508/variant.c` — pin map for DM330030

### Phase 4: Installer & Portability
- [x] `install_arduino_ide.bat` — auto-detects:
  - XC-DSC compiler (newest in `C:\Program Files\Microchip\xc-dsc\v*\`)
  - DFP MP (newest via PowerShell version sort)
  - DFP MC (newest via PowerShell version sort)
  - MPLAB X IPE (newest in `C:\Program Files\Microchip\MPLABX\v*\`)
- [x] Generates `platform.local.txt` with detected paths
- [x] Copies platform to Arduino15 packages folder

### Phase 5: Peripheral Debugging & Fixes (June 2026)
- [x] **analogRead() fix:**
  - PMD1bits.ADC1MD = 0 (ungate ADC clock)
  - ADCON3Hbits.SHREN = 1 (shared core enable)
  - ADCON5Hbits.WARMTIME = 0b1111
  - Power-up order: ADON=1 first, then SHRPWR=1, wait SHRRDY
  - Poll ANxRDY (ADSTATL/ADSTATH) not CNVRTCH
- [x] **analogWrite() fix:**
  - PPS codes corrected: OCM1=15, OCM2=16, OCM3=17, OCM4=18 (not 0x08/0x09)
  - All 4 SCCP modules implemented
  - PMD2 enable for CCP1-CCP4
  - `_pps_out_set()` helper with RPOR base address arithmetic
- [x] **Serial fix:**
  - Variant-defined macros (SERIAL_TX_RP_REG, SERIAL_RX_RP_NUM, etc.)
  - DM330030: RD4(TX)/RD3(RX) via PKOB4 CDC
  - MP102/MC002: RB5(TX)/RB4(RX)
- [x] **Hardware verification on DM330030 (dsPIC33CK256MP508):**
  - ADC: AN23/RE3 (potentiometer) ✓
  - Software PWM: SCCP3 ISR driving RGB LED (RE13/RE14/RE15) ✓
  - GPIO: Port E direct register manipulation ✓
  - UART: 9600 baud via PKOB4 CDC ✓

### Phase 6: Documentation
- [x] `arduino-platform/docs/how-to-use/` — 5 HTML guide files:
  - Part 1: Installation
  - Part 2: Writing Sketches
  - Part 3: Build & Verify
  - Part 4: Upload to Hardware
  - Part 5: Serial Debugging
- [x] `arduino-platform/README.md`
- [x] Example sketches (Blink, AnalogReadSerial)

### Phase 7: C++ Dot-Notation Refactor (June 23, 2026)
- [x] **HardwareSerial refactored to struct + function pointers:**
  - Global `Serial` object with `.begin()`, `.print()`, `.println()`, etc.
  - Users write `Serial.begin(9600)` — identical to real Arduino syntax
  - Added `print_float()` / `println_float()` for double output
  - `Arduino.h` auto-includes `HardwareSerial.h` (no separate include needed)
  - Old `Serial_begin()` / `Serial_print()` style removed
- [x] **SPI library implemented (full, not skeleton):**
  - `SPI.begin()`, `SPI.transfer()`, `SPI.beginTransaction(SPISettings(...))`
  - All 4 SPI modes (MODE0-MODE3) supported
  - PPS auto-mapped from variant macros (SPI_SCK_RP_REG, SPI_MOSI_RP_REG, etc.)
  - SPI PPS macros added to all 3 variant `pins_arduino.h` files
  - Master mode, configurable clock via BRG
- [x] **Wire/I2C library implemented (full, not skeleton):**
  - `Wire.begin()`, `Wire.beginTransmission()`, `Wire.write()`, `Wire.endTransmission()`
  - `Wire.requestFrom()`, `Wire.read()`, `Wire.available()`, `Wire.peek()`
  - `Wire.setClock()` for 100kHz / 400kHz
  - Dedicated I2C1 pins (no PPS needed), polled master mode
  - Arduino-standard return codes (0=success, 2=NACK addr, 3=NACK data)
  - WIRE_SDA_TRIS / WIRE_SCL_TRIS macros added to all 3 variants
- [x] **Example sketch updated:**
  - `DM330030_RGB_POT.ino` rewritten to use `Serial.*` and `analogRead()`
  - Removed ~80 lines of manual UART/ADC code
  - `AnalogReadSerial.ino` updated to dot-notation
- [x] **Platform re-installed** via `install_arduino_ide.bat` (all 32 files deployed)

---

## COMPLETED (continued)

### Phase 8: C++ Compiler Support (July 9, 2026)

**Goal:** Build `xc-dsc-cc1plus.exe` and `xc-dsc-g++.exe` so XC-DSC can compile real C++ (classes, templates, virtual functions, overloading). This eliminates the struct+function-pointer workaround and enables true Arduino C++ compatibility.

**Background:**
- XC-DSC v3.31 = GCC 8.3.1 targeting `pic30-elf`, built with `--enable-languages=c` ONLY
- No `cc1plus.exe` or `g++.exe` shipped — C++ frontend is disabled, not missing from source
- The `xc16plusplus` project (github.com/fabio-d/xc16plusplus) proved C++ works for pic30-elf
- GPL v3 requires Microchip to provide source; their download server blocks CLI (403)

**What's Done:**
- [x] Confirmed XC-DSC v3.31 = GCC 8.3.1 pic30-elf (`xc-dsc-gcc -v` output captured)
- [x] Confirmed inner binaries in `bin/bin/` use `elf-` prefix (elf-cc1.exe, elf-gcc.exe)
- [x] Confirmed `elf-gcc.exe` rejects `-xc++` with "language c++ not recognized"
- [x] Cloned xc16plusplus-source (branch v2.10) — GCC 4.5.1 + pic30 backend with C++ patches
- [x] Source at: `cpp_support/xc16_gcc_source/src/XC_GCC/gcc/` (94,714 files)
- [x] MSYS2 available at `C:\msys64` with MinGW64 GCC 15.2.0, make, bison, flex
- [x] Build prerequisites installed via pacman (base-devel, mingw-w64-x86_64-gcc, bison, flex, gmp-devel, mpfr-devel, mpc-devel)
- [x] Build script written: `cpp_support/build_in_msys2.sh`
- [x] `platform.txt` already configured for C++ (compiler.cpp.cmd=xc-dsc-g++, fno-exceptions/fno-rtti flags)
- [x] `install_arduino_ide.bat` already detects cc1plus/g++ and auto-enables C++ mode
- [x] Fallback to C mode (`-x c`) if C++ binaries not present (graceful degradation)
- [x] `cpp_support/minimal_cxx.cpp` — minimal C++ runtime (new/delete, pure_virtual, guards)
- [x] `cpp_support/test_cpp.cpp` — test file (classes, inheritance, templates, overloading)

**RESOLVED — XC-DSC v4.00 Released with Native C++ (July 9, 2026)**

Microchip released XC-DSC v4.00 (build date June 24, 2026) which includes:
- `xc-dsc-g++.exe` — C++ compiler driver
- `xc-dsc-cc1plus.exe` — C++ frontend
- `xc-dsc-c++filt.exe` — symbol demangler
- Full C++ standard library headers (`<algorithm>`, `<array>`, `<bitset>`, etc.)
- Same GCC 8.3.1 pic30-elf backend, now with `--enable-languages=c,c++`

**What Was Done:**
- [x] Installed XC-DSC v4.00
- [x] Verified `xc-dsc-g++ -v` shows `--enable-languages=c,c++`
- [x] Discovered DFP compatibility fix: `-D__prog__=""` needed for older DFP headers in C++ mode
- [x] Updated `platform.txt`: added `-D__prog__=""` to cpp.flags, updated default path to v4.00
- [x] Re-ran `install_arduino_ide.bat` — auto-detects v4.00 with C++ support ENABLED
- [x] Tested `test_cpp.cpp` — classes, templates, virtual functions, overloading all compile
- [x] Tested `test_arduino_cpp.cpp` — full Arduino sketch with C++ features + Arduino APIs compiles clean
- [x] Verified C++ name mangling in object file (confirms real C++ compilation)
- [x] No more `-x c` fallback needed in platform.local.txt

**DFP Compatibility Note:**
Older DFP headers (dsPIC33CK-MP_DFP v1.15.423) use `__prog__` type qualifier which
is C-only. Adding `-D__prog__=""` to C++ flags makes it transparent. This is harmless
and will be unnecessary once Microchip releases a C++-aware DFP.

**Linker ABI Fix (July 9, 2026):**
- XC-DSC v4.00 linker rejects mixing gcc-compiled (.c) and g++-compiled (.cpp) objects
- Root cause: `__c30_signature` section has flag `0x00` (gcc) vs `0x04` (g++)
- Fix: compile all .c files with `xc-dsc-g++ -x c++` so they get the `0x04` signature
- platform.txt: `compiler.c.cmd=xc-dsc-g++`, `compiler.c.extra_flags=-x c++ -fno-exceptions ...`
- Preprocessor recipes still use `xc-dsc-gcc` directly (dependency scanning only)
- Installer fallback for v3.x: override c.cmd back to gcc, clear extra_flags

**Build-from-source effort (cpp_support/) is OBSOLETE:**
- `build_in_msys2.sh` — no longer needed
- `xc16_gcc_source/` — can be deleted to save ~500MB
- `BUILD_CPP.md` — historical reference only

---

## TODO (Next Steps)

### Phase 9: Hardware PWM Verification
- [ ] Test `analogWrite()` output on D5-D8 with oscilloscope or LED
- [ ] Verify PWM frequency and duty cycle accuracy
- [ ] Test on MP102 board (not just DM330030)

### Phase 10: Additional APIs
- [ ] `tone()` / `noTone()` — use a spare SCCP module
- [ ] Fix `round` macro warning (Arduino.h vs math.h conflict)
- [ ] `attachInterrupt()` / `detachInterrupt()` — external interrupts
- [ ] **DAC output** — dsPIC33CK256MP508 has 3x 12-bit DAC channels (DAC1/DAC2/DAC3)
  - Registers: DACxCONL (DACEN, DACOEN), DACxDATH (12-bit value), DACCTRL1L (DACON)
  - Also includes built-in comparator (CMPSTAT, CMPPOL, INSEL)
  - API options: `analogWrite()` on DAC pins (true analog), or dedicated `DAC.write()` library

### Phase 11: Advanced Features (Discussion)
- [ ] Higher-level peripheral libraries (dsPIC_ADC.h, dsPIC_PWM.h) for users needing more than Arduino APIs
- [ ] **DMA library** — 4 DMA channels (DMACH0–DMACH3) on MP508
  - No standard Arduino DMA API exists; create dsPIC-specific `dsPIC_DMA.h`
  - Use cases: ADC→buffer, memory→SPI, memory→UART (zero-CPU-overhead transfers)
  - API concept: `DMA.begin(ch, src, dst, count, trigger)`
- [ ] **Software USB (V-USB style) — Virtual COM port via GPIO**
  - dsPIC33CK at 200 MHz / 100 MIPS gives ~67 cycles per USB bit (6x more than AVR V-USB)
  - USB Low-Speed (1.5 Mbps): 2 GPIO pins for D+/D-, 1.5k pull-up on D-
  - Implementation: cycle-counted pic30 assembly for NRZI/bit-stuff/CRC + C for descriptors
  - CDC-ACM class = appears as virtual COM port on PC (no driver needed)
  - Eliminates need for external UART-to-USB bridge chip (MCP2221A/FT232/CP2102)
  - Effort: significant (~2-4 weeks), but hardware math is very favorable
- [ ] Code generator / MCC-like configurator for Arduino sketches
- [ ] UART bootloader upload support (alternative to IPE programmer)
- [ ] Motor control library (PWM, QEI) for MC devices

### Phase 12: Testing & Polish
- [ ] Test MC002 board on actual hardware
- [ ] Test all APIs across all 3 board variants
- [ ] Add more example sketches (SPI sensor, I2C EEPROM)
- [ ] Package for distribution to colleagues

---

## Key Technical Constraints (Don't Forget)
1. **Config bits in ONE .c file only** — archiver crashes on duplicates
2. **ADC power-up order** — ADON first, SHRPWR second, wait SHRRDY
3. **PPS OC codes** — 15-18 (not 8-9, those are SPI2)
4. **XC-DSC v4.00 has C++ support** — g++ with `-D__prog__=""` for DFP compat; all .c MUST be compiled with `g++ -x c++` for linker ABI match (see Phase 8)
5. **`--whole-archive`** — required around core.a for CRT startup
6. **stderr suppression** — Arduino IDE treats any stderr as error
7. **Dot-notation via struct + function pointers** — C trick for Arduino-style `Object.method()` syntax

---

## API Style Guide (C "OOP" Pattern)

All peripheral libraries use the same pattern:

```c
// Header: typedef struct with function pointers, extern global object
typedef struct {
    void (*begin)(void);
    void (*end)(void);
    // ... methods ...
} ClassName_t;
extern ClassName_t ObjectName;

// Source: static implementation functions, then global struct initializer
static void _impl_begin(void) { ... }
ClassName_t ObjectName = { .begin = _impl_begin, ... };
```

User sketch syntax:
```c
#include <Arduino.h>   // includes Serial automatically
#include <SPI.h>       // opt-in
#include <Wire.h>      // opt-in

Serial.begin(9600);
SPI.transfer(0x55);
Wire.beginTransmission(0x50);
```

---

## File Structure
```
Arduino_dsPIC33CK/
├── arduino-platform/           <- SHAREABLE PACKAGE
│   ├── microchip/dspic33ck/
│   │   ├── cores/arduino/      <- Core API source files
│   │   │   ├── Arduino.h
│   │   │   ├── HardwareSerial.h / .c  (Serial object)
│   │   │   ├── wiring.c               (millis/delay)
│   │   │   ├── wiring_digital.c       (GPIO)
│   │   │   ├── wiring_analog.c        (ADC/PWM)
│   │   │   ├── wiring_shift.c         (shiftIn/Out)
│   │   │   ├── system_config.c / .h   (clock/fuses)
│   │   │   └── main.c                 (entry point)
│   │   ├── variants/
│   │   │   ├── dspic33ck32mp102/
│   │   │   ├── dspic33ck256mc002/
│   │   │   └── dspic33ck256mp508/
│   │   ├── libraries/
│   │   │   ├── SPI/src/SPI.h + SPI.c
│   │   │   └── Wire/src/Wire.h + Wire.c
│   │   ├── tools/
│   │   ├── examples/
│   │   ├── boards.txt
│   │   ├── platform.txt
│   │   └── programmers.txt
│   ├── docs/how-to-use/
│   ├── install_arduino_ide.bat
│   └── README.md
├── config.mcc/                 <- MCC-generated reference code
├── test_led/                   <- Hardware test builds & objects
├── cmake/                      <- MPLAB X project files (legacy)
└── PLAN.md                     <- THIS FILE
```
