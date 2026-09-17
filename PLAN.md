# Arduino_dsPIC33CK Platform — Project Plan

> **Last reconciled against the tree: September 17, 2026.** Phases 1–8 complete.
> Phase 9 is IN PROGRESS (test sketches written, hardware measurement pending).
> **Phase 13 (EV08P02A / dsPIC33CK256MC005 board support) added September 8, 2026,
> code-complete the same day, and CLOSED on hardware September 17, 2026** — the
> board is flashed and running: serial, LED and switch all confirmed. This is the
> first time any board in this project has been verified on silicon. See that
> section for the four core bugs it exposed and the nEDBG flashing workflow.
> **Phase 10 closed September 17, 2026** — the debugger reboot is now part of the
> upload recipe, `Serial.println(someInt)` compiles, and the six declared-but-missing
> functions (`tone`, `noTone`, `attachInterrupt`, `detachInterrupt`, `interrupts`,
> `noInterrupts`) are implemented; all builds green, hardware checks still owed.
> Some later-phase items were delivered ahead of the plan — see "Delivered Ahead
> of Plan" below.
>
> **Where the code lives now.** Git history is still not a usable timeline — the whole
> tree landed in one initial commit (`9c14696`), so this file remains the phase record —
> but it is no longer flat. Phase 13 and Phase 10 are committed as `b585acb` and
> `7553e21` on branch **`phase10-platform-cleanup`**, branched from `0a24860`. `main`
> does **not** have them yet; merge when the diffs have been read. The Arduino IDE
> install at `%LOCALAPPDATA%\Arduino15\packages\microchip\hardware\dspic33ck\1.0.0` was
> refreshed from this tree on September 17, 2026, so it is current — but it is a *copy*,
> and `install_arduino_ide.bat` must be re-run after any change to the platform tree or
> the IDE will silently build the old core.
>
> **Known inconsistency, not yet fixed:** the Phase 6 documentation still teaches the
> superseded suffixed `Serial` API and does not mention the MC005 board, `tone()` or
> `attachInterrupt()`. See "Documentation is behind the code" under Phase 12.

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
- [x] `xc-dsc-link.bat` — linker wrapper; `cd`s to `%TEMP%` because the GLD
      preprocessor (elf-cc1.exe) writes temp files to CWD and Arduino IDE's CWD
      may not be writable
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
  - *(Superseded: `Serial` is now a real C++ class with overloaded `print()` /
    `println()`, so `Serial.println(42)` works. The suffixed names are retained.
    See "Serial overloads" below.)*
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
  - NOTE: `DM330030_RGB_POT` lives in `test_led/` (bench build dir, with .elf/.hex/.o),
    NOT in the shipped `examples/` tree. Promote it to `examples/` if it should ship.
- [x] **Platform re-installed** via `install_arduino_ide.bat`
  - Installer uses `xcopy /E /I /Q "microchip\dspic33ck\*"`, so it deploys the whole
    subtree recursively — new libraries/examples are picked up with no installer edit
  - File count is now 40 (was 32 at the time of Phase 7)

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

## DELIVERED AHEAD OF PLAN

Work that exists in the tree but was never recorded in the phase list above.
Found during the August 25, 2026 reconciliation.

### HRPWM Library — belongs to Phase 11 ("higher-level peripheral libraries")
- [x] `libraries/HRPWM/src/HRPWM.h` + `.c` (~468 lines) — PWM Generators PG1–PG8
  - 250 ps edge resolution (high-resolution mode), 16-bit duty and period
  - Output modes: complementary / independent / push-pull (`PGxIOCONH.PMOD`)
  - Edge-aligned and center-aligned (`MODSEL`)
  - Hardware dead-time with independent rise/fall in ns
  - Phase offset, ADC trigger sync (TRIGA/TRIGB/TRIGC), hardware fault protection
  - Same dot-notation struct pattern as SPI/Wire: `HRPWM.begin(ch, freq_hz)`,
    `.duty()`, `.dutyRaw()`, `.setDeadTime()`, `.enableFault()`, `.isFaulted()`, …
  - Pin map documented in the header for MP508 80-pin (PG1: RB14/RB15, PG5–PG8: RC0–RC7,
    PG4 is PPS-routable)
- [x] `libraries/HRPWM/examples/BoostMPPT/BoostMPPT.ino` — solar MPPT example
- [ ] **Hardware-verify HRPWM** — no record that any PG channel was scoped
- [ ] Document HRPWM in `docs/how-to-use/` (Phase 6 docs predate it)

### UART Bootloader Upload — belongs to Phase 11
- [x] `tools/upload_uart.py` — sends .hex over UART, Microchip 16-bit bootloader
      protocol (0x55 sync → erase → chunked hex records)
- [ ] **Hardware-verify** — no record this was ever run against a real bootloader

### C++ Feature Example — belongs to Phase 8
- [x] `examples/02.CppFeatures/CppDemo/CppDemo.ino` — exercises the native C++
      support that XC-DSC v4.00 unlocked

### Orphaned: `tools/pre_build.py`
- Patched Arduino-IDE-generated .cpp back to C-compatible (stripped `extern "C"`,
  duplicate includes, added setup/loop forward decls).
- **Not referenced anywhere in `platform.txt`** — no `recipe.hooks.prebuild` line exists.
  Superseded by Phase 8's native C++ mode. Safe to delete; kept only as history.

---

## IN PROGRESS

### Phase 9: Hardware PWM Verification
- [x] Test sketches written:
  - `examples/03.PWM/PWMTest/PWMTest.ino` — 4 tests on LED2 (RE5 = D58 = RP181 → SCCP5):
    50%; 25/50/75/full; smooth fade; PWM→digital→PWM transition. Serial 115200 via PKOB4 CDC.
    Expected ~490 Hz (at FCY=100 MHz: prescaler 1:64, period=3187 → 490.5 Hz)
  - `examples/03.PWM/Fade/Fade.ino`
- [ ] Run PWMTest on DM330030 and confirm frequency + duty accuracy (scope or LED)
- [ ] Test `analogWrite()` output on D5-D8
- [ ] Test on MP102 board (not just DM330030)

**Blocked on bench access only** — the code side is done; what remains is measurement.

---

## TODO (Next Steps)

### Phase 10: Additional APIs — the Arduino API surface is CLOSED, two extras remain
The three functions this phase was actually about are done and committed; what is left
under this heading are two items that were only ever filed here for convenience. The
implementation write-up is under "The six missing functions" below, and the four
remaining checks are bench-only.

- [x] `tone()` / `noTone()` — SCCP4 as a timer, ISR toggles any pin (Sep 17, 2026)
- [x] `attachInterrupt()` / `detachInterrupt()` — Change Notification, every pin
      (Sep 17, 2026). `interrupts()` / `noInterrupts()` closed at the same time —
      they were declared but unimplemented too. See "The six missing functions" below.
- [ ] `round` macro vs `math.h` — **re-measured Sep 17, 2026; the warning is real but
      appears in only one of the two build modes**, which is why every Arduino-path build
      reports zero warnings and the item looked stale:
      - **plain C** (`xc-dsc-gcc`, i.e. the `cmake/` projects): `math.h` defines `round` as
        a *macro* (`#define round __MPROTO(round)`), and `Arduino.h:72` redefines it →
        `warning: "round" redefined`, once per translation unit.
      - **C++** (`xc-dsc-g++ -x c++`, i.e. every Arduino IDE build): `math.h` does not
        define that macro, so there is no diagnostic in either include order.

      Because `Arduino.h` includes `math.h` at line 19 *before* defining `round` at line 72,
      the macro always wins, in both modes: `round(2.7)` yields `(long)3`, not `3.0`.
      That truncation is stock AVR Arduino behaviour (byte-identical macro upstream), so it
      must **not** be "fixed" by removing the macro — that would break sketch compatibility.
      The fix is `#undef round` immediately before line 72, which silences the plain-C
      redefinition warning while keeping the Arduino semantics exactly. One line.
- [ ] **DAC output** — dsPIC33CK256MP508 has 3x 12-bit DAC channels (DAC1/DAC2/DAC3)
  - Registers: DACxCONL (DACEN, DACOEN), DACxDATH (12-bit value), DACCTRL1L (DACON)
  - Also includes built-in comparator (CMPSTAT, CMPPOL, INSEL)
  - API options: `analogWrite()` on DAC pins (true analog), or dedicated `DAC.write()` library

### Phase 11: Advanced Features (Discussion)
- [~] Higher-level peripheral libraries for users needing more than Arduino APIs
  - PWM side is **already done** — see HRPWM under "Delivered Ahead of Plan"
  - Still open: an ADC equivalent (dsPIC_ADC.h) — multi-channel, triggered, DMA-fed
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
- [~] UART bootloader upload support (alternative to IPE programmer)
  - `tools/upload_uart.py` written — see "Delivered Ahead of Plan"; hardware-untested
- [ ] Motor control library (PWM, QEI) for MC devices

### Phase 12: Testing & Polish
- [ ] **Documentation is behind the code** — the highest-value item left, because it is
      what a new user reads first. Eleven HTML files under `arduino-platform/docs/`
      (the 5-part guide plus `how-to-use/`); **seven of them still teach the suffixed
      `Serial` API** (`print_int`, `println_int`, `print_float`, `println_float`) that
      Phase 10 replaced. Those calls still compile — the suffixed methods were kept
      deliberately — so this is not a broken-docs bug, it is worse: the docs teach the
      dsPIC-specific spelling as *the* API when stock Arduino syntax now works. Also
      missing everywhere: the MC005 / Curiosity Nano board (only `installation_guide.html`
      mentions it at all), `tone()`, `attachInterrupt()`, and the fact that the debugger
      reboot is now automatic. Affected: `arduino_ide_setup.html`,
      `part3_api_reference.html`, `part4_testing_sketches.html`,
      `part5_upload_troubleshooting.html`, `how-to-use/part2_writing_sketches.html`,
      `how-to-use/part5_serial_debugging.html`.
- [ ] No example uses `tone()` or `attachInterrupt()` — the two newest APIs are the two
      with no example. A Nano sketch covering both would double as the bench checklist.
- [ ] Test MC002 board on actual hardware
- [ ] Test all APIs across all 4 board variants (on hardware — all 4 build clean)
- [ ] Add more example sketches (SPI sensor, I2C EEPROM)
- [ ] Package for distribution to colleagues

---

## COMPLETED (continued) — Phases 13 and 10

> Both of these are **done**, and both used to sit under "TODO" above purely because
> they were appended in the order the work happened. Phase 13 is closed on silicon;
> Phase 10 is closed in code with four bench checks outstanding.

### Phase 13: 4th Board — EV08P02A (dsPIC33CK Value Line Curiosity Nano)

**Added September 8, 2026.** Target: `dsPIC33CK256MC005` — 256 KB ECC flash,
16 KB RAM, 12-bit 2 Msps 15-ch ADC, 1 comparator, 12-bit DAC, high-speed PWM,
on-board Curiosity Nano debugger. Same dsPIC33CK family, so this is a variant
addition, not a fork.

> **DFP BLOCKER CLEARED Sep 8 2026.** `dsPIC33CK-MC_DFP` **1.10.386** (installed
> via MPLAB X Pack Manager) is the first local pack carrying MC005; **1.11.412**
> also works and is what the installer's version-sort actually picks. 1.8.299 and
> 1.9.370 do not have it. Build verified against **both** 1.10.386 and 1.11.412.
>
> **Gotcha that cost real time:** `-mdfp=` must point at `<pack>/xc16`, **not** the
> pack root. The pack root has no `bin/c30_device.info`, and the compiler reports
> that as the misleading `Invalid -mcpu option. CPU 33CK256MC005 not recognized.`
> The installer already appends `\xc16`, so this only bites manual invocations.

- [x] **PREREQUISITE: newer `dsPIC33CK-MC_DFP`** — done, 1.10.386. MPLAB X v6.30
      and XC-DSC v4.00 were both fine; no toolchain upgrade was needed.
- [x] `variants/dspic33ck256mc005/pins_arduino.h` — 39 digital pins, 20 analog.
- [x] `variants/dspic33ck256mc005/variant.c` — 39-entry `g_pin_map`.
- [x] `boards.txt` block — `build.dfp.path={build.dfp.path.mc}`,
      `build.mcu=33CK256MC005`, `build.ldscript=p33CK256MC005.gld`,
      `upload.maximum_size=262144`, `upload.maximum_data_size=16384`.
- [x] `programmers.txt` + `platform.txt` tool entry — `nedbg`. The ipecmd short
      name is `nEDBG` (`-TPnEDBG`), taken from MPLAB X `docs/Readme for IPECMD.htm`
      rather than guessed. **Exercised against hardware Sep 17 2026** — flashes
      correctly.
- [x] Fold into Phase 12: "all 3 board variants" → 4.
- [x] **Hardware bring-up — DONE September 17, 2026.** All three acceptance
      criteria confirmed on the physical board with `_build/bringup/bringup.cpp`:
      | Criterion | Evidence |
      |---|---|
      | CDC COM port carries `Serial` at 115200 | Startup banner + continuous `beat=` stream captured on COM63 |
      | LED on D37 lights | Blinking ~2 Hz, inverts while SW0 held (RD10, active low) |
      | SW0 on D38 reads | `sw0=up` released → `sw0=DOWN` pressed |

      Captured output:
      ```
      === Arduino_dsPIC33CK bring-up ===
      board : EV08P02A / dsPIC33CK256MC005
      FCY   : 4000 kHz
      pins  : LED0=D37 SW0=D38
      ready
      beat=0 sw0=up   a0=475
      beat=1 sw0=up   a0=449
      ```
      `FCY : 4000 kHz` confirms the clock, `beat=` restarting at 0 across a target
      power-cycle confirms reset, `beat` incrementing confirms `loop()`/`delay()`,
      `a0=` varying confirms `analogRead()`.

#### Flashing workflow — READ THIS BEFORE DEBUGGING SERIAL SILENCE

**An ipecmd programming session wedges the nEDBG's USB-CDC bridge.** After a
flash the virtual COM port still enumerates, still opens, and delivers **zero
bytes** — forever. This cost most of the bring-up effort and sent three separate
firmwares (the Arduino core, a bare-metal hardware-UART test, and a bare-metal
GPIO bit-bang) to the same false conclusion. **No firmware change can fix it.**

The working sequence is:

```bash
# 1. flash
ipecmd -TPnEDBG -P33CK256MC005 -M -OL -F"out/bringup.hex"
# 2. un-wedge the CDC bridge  (order matters: program FIRST, then reboot)
pymcuprog reboot-debugger
# 3. wait ~10 s for USB to re-enumerate, then open the port
```

**The Arduino IDE now does all three steps itself.** `tools/nedbg-upload.bat`
wraps the upload for the `nedbg` tool: it runs ipecmd, and only on success runs
`pymcuprog reboot-debugger` and sleeps for re-enumeration. `platform.txt` selects
it through `tools.nedbg.upload.pattern.windows`, leaving the plain
`tools.nedbg.upload.pattern` as the non-Windows fallback. A failed program
deliberately does **not** reboot the debugger — that would mask the real error —
and a missing `pymcuprog` prints an actionable note instead of failing the
upload. The manual sequence above is still what you want for bare-metal test
builds flashed outside the IDE.

Related nEDBG facts, all learned the hard way:
- **`pymcuprog` must be run as the CLI**, not `python -m pymcuprog` ("cannot be
  directly executed"). There is **no device file for `dspic33ck256mc005`**, so
  target memory access is unavailable — but the *tool-level* commands work
  regardless of device support and are what make it useful here:
  `getvoltage`, `getsupplyvoltage`, `setsupplyvoltage -l <V>`, `getusbvoltage`,
  `reboot-debugger`.
- **`setsupplyvoltage -l 0` then `-l 3.3` power-cycles the target only**, resetting
  the MCU while leaving the debugger's CDC alive. This is the way to capture a
  startup banner: open the port first, then power-cycle. A `reboot-debugger`
  cannot do this — it drops the port.
- The nEDBG **locks up outright** now and then: `Data transmission failed. Error
  code -121`, `IOError: Unrecoverable error while communicating with the PKOB
  nano`, ipecmd exit 7. `pymcuprog reboot-debugger` recovered it every time.
- ipecmd `-Y` (verify) is **unsupported** for this device/tool. Omitting `-OL`
  leaves the device held in reset; `-OL` alone releases it.
- **`STATUS.TXT` on the kit's mass-storage drive lies.** It read
  `Status: Failed - board voltage is too low` while `pymcuprog getvoltage`
  measured **3.32 V** against a 3.30 V setting and ipecmd printed "Target voltage
  detected". The file is stale; trust the live measurement.

**Authoritative board documentation** is
`dsPIC33CK256MC005-Curiosity-Nano-User-Guide-DS70005656.pdf` (in the repo root).
Table 4-2 gives the debugger connections (`CDC RX → RC10`, `CDC TX → RC11`,
`DBG0 → RB5`, `DBG1 → RB6`, `DBG2 → RD13`, `DBG3 → MCLR`), Table 5-1 the LED
(RD10, active low), Table 5-2 the switch (RD13, **no external pull-up — the
internal one is required**). §5.3 notes the 8 MHz MEMS oscillator is **not
connected by default** (needs a strap cut), which is why FRC is the only clock.
Note that the *figures* in this PDF extract as garbled text — read the tables.

#### Four core bugs the bring-up exposed — all fixed, all affected every board

These were latent because nothing had ever run on silicon before.
`HardwareSerial.c` (1–3) and `wiring_digital.c` (4):
1. **Baud divisor truncated.** `U1BRG = FCY/(16*baud) - 1` with `BRGH=0` yields
   125000 instead of 115200 at FCY=4 MHz — **+8.5%, far outside UART tolerance**,
   so `Serial` could never have worked at this clock. Replaced with the fractional
   baud generator (`BCLKSEL=0b00`, `BCLKMOD=1`, `BRGH=0`,
   `brg = (FCY + baud/2)/baud` rounded, clamped to 0xFFFFF, split across
   `U1BRG`/`U1BRGH`): BRG=35 → 114286 baud, −0.7%.
2. **Enable order inverted.** `UTXEN`/`URXEN` were set *before* `UARTEN`, but they
   are gated by it, so they never stuck. Order is now `UARTEN` → `UTXEN` → `URXEN`.
3. **TX pin not idled high.** The line must be driven HIGH before `TRIS` makes it
   an output, or the first frame is corrupt.
4. **`INPUT_PULLUP` was broken on every port above B.** `pinMode` knew only
   `CNPUA`/`CNPUB` and its `else` branch applied `CNPUA` to *any* other port — so
   `INPUT_PULLUP` on RD13 set the pull-up on RA13 and left RD13 floating. A
   floating input reads LOW, so SW0 reported permanently pressed (`sw0=DOWN`).
   Fixed with a `cnpu_for_port()` helper that selects by port and guards each
   register on its own `#if defined(CNPUx)` — the device headers define a
   self-named macro per SFR (`#define CNPUD CNPUD`), which makes that a reliable
   per-device presence test. All four variants still build.

**dsPIC33CK UART baud modes**, for reference — there are three, and MCC picks the
first: fractional (`BCLKMOD=1`, `BRGH=0`, `BRG = FCY/baud`, 20 bits across
`U1BRG:U1BRGH`); `BRGH=0` (`BRG = FCY/(16*baud) - 1`); `BRGH=1`
(`BRG = FCY/(4*baud) - 1`). Baud clock `BCLKSEL=0b00` = FOSC/2 = FCY.

**Every "unknown" from the original entry is now resolved** — not from the web
(microchip.com 403s the CLI, `search_evk_user_guide` has no EV08P02A entry, and
one search result wrongly claimed dsPIC33CK64MC105), but from two independent
offline sources that agree exactly: the DFP's own `edc/DSPIC33CK256MC005.PIC`
pin database, and Microchip's official *Out-of-Box Demo* MCC project for this
board (unpacked under `_build/oob/`).
- [x] **LED0 = RD10 (D37), active LOW.** `LED_BUILTIN` = 37.
- [x] **SW0 = RD13 (D38), pull-up, active LOW.** `BUTTON_BUILTIN` = 38.
- [x] **CDC virtual COM = UART1**, U1TX = RC10 (RP58, D31), U1RX = RC11 (RP59, D32).
- [x] **48-pin package, 39 bonded I/O.** RP numbering RB(n)=RP(32+n),
      RC(n)=RP(48+n), RD(n)=RP(64+n); PORTA is not remappable.
- [x] Silkscreen device confirmed as `dsPIC33CK256MC005` by the OOB demo project.

**Two board facts that changed earlier assumptions:**
1. The debugger is on **PGC3/PGD3 = RB6/RB5** (`#pragma config ICS = PGD3`), *not*
   PGC1/PGD1 as first assumed. So RB5/RB6 are reserved (marked in the pin table)
   and MC002's SPI MOSI choice of RB6 could **not** carry over.
2. `ALTI2C1 = OFF`, so I2C1 stays on its default **RB8 = SCL1, RB9 = SDA1**.
   Note this is **reversed relative to MC002**, which used SDA=RB9/SCL=RB10.

**SPI pin choice — RC4/RC5/RC12/RC13** (SCK/MOSI/MISO/SS). Picked to avoid every
conflict on this board and to cost zero ADC channels: steers clear of RB5/RB6
(debugger), RB8/RB9 (I2C1), RB0–RB3 (SCCP PWM), RB10–RB15 + RD1 (PWM generator
outputs), and RC10/RC11 (CDC UART).

**PORTD is numbered over bonded pins only** (D35=RD1, D36=RD8, D37=RD10,
D38=RD13) rather than by the sparse `RD(n)→D(35+n)` formula MP508 uses.
Reason: `wiring_digital.c` range-checks `pin >= NUM_DIGITAL_PINS` but does **not**
NULL-check `port_reg`/`tris_reg`, so a gap in the pin table would fault at runtime.
Worth fixing in the core eventually; until then, pin tables must stay dense.

**Three pre-existing, board-independent defects were latent until a non-MP508 part
was compiled** — all fixed, all affecting the other boards too:
1. `wiring_analog.c` referenced `PWM5_PIN`/`CCP5*`/`PMD2bits.CCP5MD` at **4 sites**
   unconditionally, but only MP508 defines PWM5. MC002 and MP102 could never have
   compiled `analogWrite()`. Now `#ifdef PWM5_PIN` guarded.
2. `SPI.h` declared `extern SPIClass_t SPI`, but **every** `p33CK*.h` already has
   `typedef struct tagSPI {...} SPI, *PSPI;` for the SPI1/SPI2 SFR blocks — so the
   SPI library had **never compiled on any board**. A typedef can't be `#undef`'d,
   so the object is now `ArduinoSPI` with `#define SPI ArduinoSPI` keeping sketch
   syntax unchanged.
3. `HRPWM` hard-coded 8 PWM generators. `HRPWM_CH_MAX` is now derived from the
   device header's `PGnCONL` self-defines, with the `_pg[]` table rows guarded.

**HRPWM does not exist on the MC parts, by hardware.** MC002 and MC005 have **no
Auxiliary PLL** (no `ACLKCON1`/`APLLFBD1`/`APLLDIV1` at all) and their `PCLKCON`
is only `{MCLKSEL:2, DIVSEL:2, LOCK:1}` — no `HRERR`, no `HRRDY`. Confirmed in
both the device headers and the `edc` XMLs. The library's whole 500 MHz AFPLLO /
250 ps scheme has nothing to run on. `HRPWM.h` now defines `HRPWM_SUPPORTED`
(keyed on `ACLKCON1`, whose presence tracks `HRRDY` exactly across all four parts)
and `#error`s with a clear message pointing at `analogWrite()`. This is safe
because Arduino only compiles a library when a sketch includes it, so MC boards
still build fine — a sketch that asks for HRPWM gets one honest diagnostic instead
of a wall of undeclared-register errors.

**Build status — all 4 boards compile, link and emit HEX** (core + variant +
eligible libraries + a sketch exercising `pinMode`/`digitalWrite`/`digitalRead`/
`analogRead`/`analogWrite`/`Serial`/`SPI`/`Wire`, harness at `_build/allboards.sh`):

| Board | DFP | Libraries | Flash | RAM |
|---|---|---|---|---|
| 33CK32MP102 | MP 1.16.521 | SPI Wire HRPWM | 11720 B | 504 B |
| 33CK256MP508 | MP 1.16.521 | SPI Wire HRPWM | 12124 B | 504 B |
| 33CK256MC002 | MC 1.10.386 | SPI Wire | 9960 B | 440 B |
| 33CK256MC005 | MC 1.10.386 | SPI Wire | 10092 B | 440 B |

*(Figures as of Phase 13. The harness sketch has since grown to exercise the whole
Phase 10 API — see the two size tables under "The six missing functions" for current
numbers and what drives them.)*

**Installer note:** no `install_arduino_ide.bat` change was needed for the variant
itself — `xcopy /E` picks up new variant folders automatically. Its MC-DFP
detection already version-sorts and appends `\xc16`, so 1.10.386/1.11.412 are
found without change; only its two user-facing "for MC002" messages were widened
to mention MC005.

**Also fixed while here:** `HardwareSerial.h`'s usage comment advertised
`Serial.print(analogRead(A0), DEC)`, which cannot work — C has no overloading and
the struct exposes `print_int`/`println_int`/`print_float`/`println_float`. Comment
corrected to the real API. *(That restriction is now gone — see "Serial overloads"
below; `Serial.print(analogRead(A0), DEC)` compiles.)*

---

### Serial overloads — `Serial.println(someInt)` compiles (Sep 17, 2026)

The suffixed API was the single biggest barrier to copy-pasting a stock Arduino
sketch. Fixed by giving `HardwareSerial` two faces from one `.c` file, which it
has to have: the Arduino build compiles it as **C++** (`platform.txt:23-24` set
both `compiler.c.cmd` and `compiler.cpp.cmd` to `xc-dsc-g++`, and
`compiler.c.extra_flags=-x c++` reaches `recipe.c.o.pattern`), while three MPLAB X
CMake projects still compile the same file as **plain C** with `xc-dsc-gcc`.

- The thirteen `_serial_*` statics became external-linkage `serial_*`; bodies
  unchanged. Two new ones, `serial_print_uint` / `serial_println_uint`, because
  `Serial.println(millis())` is an `unsigned long` and would print negative past
  2^31 (24.8 days) if funnelled through the signed path.
- `#ifndef __cplusplus` → the old `HardwareSerial_t` struct of function pointers.
  `#ifdef __cplusplus` → a stateless class whose methods are all `inline` and
  forward to the same `serial_*` functions. The struct could not simply grow
  methods: it has *data members* named `print`/`println`, and C++ forbids a member
  function sharing a name with a data member.
- The `Serial` object is defined **outside** the `extern "C"` block so its
  language linkage matches its declaration; the `_U1RXInterrupt` vector stays
  inside it.
- No overload for `unsigned char` / `bool` / `short`: integral **promotion** to
  `int` outranks any conversion, so they resolve to `print(int, int)`
  unambiguously and print as numbers — matching stock Arduino. `char` matches
  `print(char)` exactly and prints as a character.

**This made every board smaller, not bigger.** The struct took the address of all
13 functions, pinning every one past `-ffunction-sections` + `--gc-sections`:

| device | flash before | flash after | RAM before | RAM after |
|---|---|---|---|---|
| 33CK32MP102 | 11812 B | **8172 B** | 504 B | **452 B** |
| 33CK256MP508 | 12264 B | **8624 B** | 504 B | **452 B** |
| 33CK256MC002 | 10052 B | **6412 B** | 440 B | **388 B** |
| 33CK256MC005 | 10216 B | **6576 B** | 440 B | **388 B** |

All 45 suffixed call sites in the examples were migrated to the natural spelling
(`Serial.println(v)`, `Serial.print(v, HEX)`, `Serial.print(v, 3)`,
`Serial.println("")` → `Serial.println()`), since examples are what people copy.
The four suffixed methods stay in the API for existing sketches — `test_led/` and
`cpp_support/` outside the platform tree still build untouched.

Two safety clamps added along the way, both newly reachable now that user code
can pass these directly: `base < 2 || base > 36` falls back to 10 (base 0 divided
by zero, base 1 looped forever), and `decimals` is clamped to 0..16 (a large value
ran off the end of a stack buffer).

---

### The six missing functions — Phase 10 closed (Sep 17, 2026)

`tone()`, `noTone()`, `attachInterrupt()`, `detachInterrupt()`, `interrupts()` and
`noInterrupts()` were declared in `Arduino.h` and implemented nowhere. A sketch
calling any of them compiled cleanly and then failed at link with an undefined
reference — the worst failure mode available, because the error names a symbol the
user never wrote. New files: `cores/arduino/wiring_tone.c`,
`cores/arduino/wiring_interrupts.c`, and `cores/arduino/wiring_private.h` (the
core-internal cross-module contract; deliberately **not** reachable from `Arduino.h`).

**`interrupts()` / `noInterrupts()` → `INTCON2bits.GIE`**, a single atomic
`BSET`/`BCLR`. Deliberately **not** `__builtin_disi()`: `DISI #n` expires after *n*
cycles, so a long critical section silently re-enables; it only masks IPL1-6; and it
is a single global with no nesting, so `millis()`'s own `__builtin_disi` pair
(`wiring.c:71-73`) would **re-enable interrupts on exit** from a user's critical
section. `GIE` and `DISI` are orthogonal bits, so the two compose safely and
`wiring.c` was left untouched. (The orthogonality is the one assumption here still
unverified on silicon — see the hardware checklist below.)

**`tone()` claims SCCP4**, the same module `analogWrite(8, …)` uses, so the two are
mutually exclusive — the same caveat stock AVR Arduino carries for pins 3/11. Policy
is *last caller wins*: `tone()` tears down any PWM on D8, and `analogWrite()` calls
`_tone_release()` as its first statement. That ordering is load-bearing: without it
`analogWrite(8, 0)` during a tone finds `_pwm_active & 0x08` false, skips teardown,
and `digitalWrite`s a pin the tone ISR is still toggling. The ISR toggles a cached
`*_tone_lat ^= _tone_mask` rather than calling `digitalWrite` (which bounds-checks and
does three PSV pointer loads), so any pin on any port works, at IPL5 — above `millis()`
(IPL4) and Serial RX (IPL3), both of which latch their flags and tolerate the latency.

Four things worth not rediscovering:
- The period-match vector is **`_CCT4Interrupt`**, not `_CCP4Interrupt`. `CCTxIF` is the
  time-base interrupt; `CCPxIF` is the capture/compare event. `IFS2bits.CCT4IF` (bit 9) /
  `IEC2bits.CCT4IE` / `IPC10bits.CCT4IP`, at **identical register indices on all four
  devices**, so no `#if` guard is needed.
- The prescaler must be chosen **at run time**. `analogWrite` gets away with a fixed
  `PWM_PRESCALER` because it is always 490 Hz; `ticks = FCY/(ps*2*freq)` has to land in
  2..65536 across 31 Hz–20 kHz *and* both FCY values (at FCY=100 MHz, 1:1 bottoms out at
  763 Hz). The loop over `{1,4,16,64}` recomputes the divide per candidate.
- `(uint32_t)duration * frequency / 500` **wraps** past 4.3e9 (10 minutes at 20 kHz) and
  produces a short blip instead of a long tone. Split into
  `whole*freq + (rem*freq)/500` with a saturation guard. No `uint64_t` — `__udivdi3`
  would cost MP102 real flash.
- `PMD2bits.CCP4MD` must be 0 *before* any CCP4 write or the write is silently dropped.

Documented residual race, not fixed: `*_tone_lat ^= _tone_mask` and `digitalWrite`'s
`LATx` update are both read-modify-write, so a tone ISR landing inside `digitalWrite` on
*any pin of the same port* drops one half-cycle. Inaudible; fixing it means putting
`DISI` inside `digitalWrite`.

**`attachInterrupt()` uses Change Notification, not INT1/INT2/INT3.** CN is a strict
superset of what the remappable INTx path could deliver:

| | INT1/2/3 | Change Notification |
|---|---|---|
| Pins reachable | PPS-remappable only → PORTB/C/D. **PORTA and PORTE excluded**, and `D0` is `LED_BUILTIN` on the 28-pin boards | **every pin on every port** |
| Concurrent handlers | 3 (INT0 is welded to D7 = PWM3) | every pin at once, no allocator |
| `CHANGE` mode | **not supported in hardware** — `INTxEP` is one bit | **native**: `CNEN1x:CNEN0x` = 1:1 |
| Can fail | out of slots, or a non-remappable pin → silent no-op on a `void` function | cannot fail for a valid pin |

The `CHANGE` row decided it. Emulating CHANGE by flipping `INTxEP` inside the ISR is not
merely inelegant, it is **incorrect**: if the line returns before the flip, the polarity
is left inverted and the handler fires on the wrong edge sense permanently with no
self-correction — the classic rotary-encoder / IR-receiver failure. `Arduino.h`'s
existing `CHANGE 1` / `FALLING 2` / `RISING 3` map straight onto the `CNEN1x`/`CNEN0x`
truth table with `CNCONxbits.CNSTYLE = 1`.

`digitalPinToInterrupt(p)` is the identity (added to all four variants) — it exists only
so AVR sketches compile unchanged. **`attachInterrupt(0, …)` therefore means D0, not
"INT0"**, as on every non-AVR core.

- Per the datasheet, CN fires only for pins configured as inputs, so `attachInterrupt()`
  **must** set `TRISx` and clear `ANSELx`. It does that **inline** and **never touches
  `CNPUx`** — calling `pinMode(pin, INPUT)` would clear the pull-up at
  `wiring_digital.c:61` and silently break the commonest idiom there is,
  `pinMode(p, INPUT_PULLUP)` then `attachInterrupt(p, …, FALLING)`. That is the exact bug
  Phase 13 just fixed on SW0.
- Ports are resolved by **`port_reg` pointer identity** against `g_pin_map[pin]`, not by
  pin index: `mc005/variant.c`'s PORTD entries are non-contiguous (D35=RD1, D36=RD8,
  D37=RD10, D38=RD13). Same idiom as `cnpu_for_port()`.
- Every port block is guarded on **`#if defined(CNCONx)`**, never on the interrupt-bit
  macros. `p33CK32MP102.h` defines `CNCIF`/`CNDIF`/`CNCIE`/`CNDIE` and its `.gld` has
  `__CNCInterrupt`/`__CNDInterrupt` slots even though MP102 has no PORTC or PORTD, so the
  bit macros would guard nothing. Ports present: A,B on MP102/MC002; A,B,C,D on MC005;
  A,B,C,D,E on MP508.
- **Flag ordering inverts the platform idiom, deliberately, and there is a comment in the
  file asking not to "fix" it back.** The ISR clears the summary `IFSnbits.CNxIF`
  **first**, then snapshots and clears the `CNFx` bits, then dispatches. `CNxIF` is a
  *summary* of the per-pin `CNFx` latches; with the core's usual flag-last ordering an
  edge arriving after `CNFx` is read but before `CNxIF` is cleared is latched in `CNFx`
  while its interrupt request is discarded, and nothing re-asserts it — **that edge is
  lost forever**. Flag-first is safe under either hardware model. Single dispatch pass,
  no `while (*cnf)` loop, or a fast signal livelocks the ISR.
- **IPL2** for user handlers, below `millis()` (IPL4) and Serial RX (IPL3). That is what
  makes handlers usable: `millis()` returns a live value, **`delay()` works rather than
  deadlocking** (it spins on `millis()`, which T1 still advances), and `Serial.print()`
  works because `serial_write` polls `U1STAHbits.UTXBF`. `Serial.print()` from a handler
  is *not reentrant* though — preempting a mainline print interleaves output. All of this
  is in the `Arduino.h` comment block.

**`pinToRP(pin)`** was added to `wiring_digital.c` as a public function: PORTB→32+bit,
C→48+bit, D→64+bit, −1 otherwise. Validity is **not** derived from `_RPnnR` macro
presence — MP102 defines `_RP48R`…`_RP77R` despite having only PORTA/PORTB. Nothing in
Phase 10 needs it; it encodes the rule once for the hardware-PWM tone path below.

**Cost.** Two figures matter, because `platform.txt:97` links `core.a` with
`-Wl,--whole-archive`: the new objects land in every sketch whether called or not, and
while `--gc-sections` still drops unreferenced functions, **the ISRs are reachable from
the interrupt vector table and so are never collected**. The floor is what a sketch that
calls none of this pays anyway:

| device | flash before | floor (unused) | all six called | RAM before | RAM after |
|---|---|---|---|---|---|
| 33CK32MP102 | 8172 B | 8372 B | 13796 B | 452 B | 600 B |
| 33CK256MP508 | 8624 B | 9012 B | 14812 B | 452 B | 792 B |
| 33CK256MC002 | 6412 B | 6612 B | 12048 B | 388 B | 536 B |
| 33CK256MC005 | 6576 B | 6964 B | 12688 B | 388 B | 664 B |

The RAM delta is fully accounted for: **64 bytes per CN port** — a 16-entry handler
table, and *a function pointer is 4 bytes on this architecture* (24-bit program
addresses) — plus 20 bytes of tone statics. So 148 B on the two-port parts, 276 B on
MC005, 340 B on MP508. Measured with `objdump -h`, after a first estimate of 32 B/port
proved wrong by exactly the pointer size. The `_cn_ports` descriptor table is
`static const` and lands in `.const` in **program space** (48 B on MP102, 120 B on
MP508), costing no RAM. The "all six called" column is a synthetic worst case —
`_build/sketch.cpp` calls every function plus `analogWrite(PWM4_PIN, …)` in one
translation unit, which is also what proves the `wiring_private.h` link contract
actually closes; most of that column is the float-print path, not the interrupt code.
MP102's 13796 B is 42% of a 32 KB part, so no `--whole-archive` change is needed.

**Refactoring done first, no behaviour change:** `wiring_analog.c` had **two
byte-identical** 48-line PWM teardown switches, a latent divergence bug in its own
right. Both collapsed into `_pwm_disable_pin(pin)`, and the PMD gate hoisted into
`_sccp_pmd_enable()`, so `wiring_tone.c` could reuse them.

**Verified green:** all four devices compile, link and produce hex with zero warnings
(`_build/allboards.sh`); the six Nano examples with zero warnings
(`_build/examples_mc005.sh`); the five non-Nano examples on the boards that define
their pins, MP508 included, with zero warnings (`_build/examples_all.sh`, new); and all
nine core `.c` files plus `variant.c` still compile as **plain C** with `xc-dsc-gcc
-Wall -Wextra` on all four devices, which is the only gate catching C++-only syntax in
a `.c` file since the Arduino path builds everything as C++. The sole warning anywhere
is the pre-existing `Arduino.h:72: warning: "round" redefined`.

**Still owed by the bench (EV08P02A):** `tone(37, 1000, 500)` on LED0 (a PORTD pin);
`attachInterrupt(digitalPinToInterrupt(38), fn, CHANGE)` on SW0 firing on both press and
release, re-run after `pinMode(38, INPUT_PULLUP)` to confirm **the pull-up survives**;
`attachInterrupt` on a PORTA pin (D0-D4), which INT1/2/3 could never have reached; and
`noInterrupts()` held > 65 ms, then check `millis()` drift and **assert
`INTCON2bits.GIE == 0` after a `millis()` call inside the critical section** — that last
one is the single unverified assumption above, that `DISI` leaves `GIE` alone.

**Natural follow-up, deliberately not done:** a hardware-PWM tone path (SCCP4 in PWM
mode + PPS) would give zero ISR load, an exact frequency and no `LATx` race, falling
back to ISR-toggle for PORTA/PORTE. It needs `pinToRP()`, which is why that landed now.
`wiring_tone.c` keeps its start/stop internals behind two static functions so this can
be slotted in without reshaping the file.

---

### Pre-existing bugs surfaced by this work

Found while designing the above; one fixed, the rest filed here rather than bundled.

1. **`analogWrite()` had no bounds check — FIXED.** The mid-range path dereferenced
   `g_pin_map[pin]` unguarded and then wrote through the resulting garbage
   `ansel_reg`/`tris_reg` pointers, so `analogWrite(200, 128)` corrupted arbitrary SFRs.
   (The `val<=0` and `val>=255` paths were safe only because they end in `digitalWrite`,
   which does check.) One line, in a file already being edited.
2. **MP508 `PWM5_RP 181` is not a pin.** `variants/dspic33ck256mp508/pins_arduino.h`
   maps `PWM5_PIN 58` (RE5) to RP181, but **RP176-181 are the virtual pins RPV0-RPV5** —
   internal nodes with no bond wire. `analogWrite(58, x)` can never reach RE5, so LED2 on
   the DM330030 will not dim. No PPS fix exists; RE5 would need the same ISR-toggle
   mechanism as `tone()`.
3. **`cmake/Arduino_dsPIC33CK/.../file.cmake` lists only three of four variants** —
   `dspic33ck256mc005` is absent, so its `variant.c` gets no plain-C check from that
   project. Left as is because the `dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo`
   project does cover it.
4. **`wiring.c:19`** — `static volatile unsigned long _micros_overflow` declared, never
   used.
5. **MP508 `variant.c`** gives RE0-RE3 `&ANSELE` and channels AN20-23, but `ANSELE` may
   be unimplemented on that part. Needs a pinout cross-check before trusting
   `analogRead()` on those pins.
6. **The `round` macro at `Arduino.h:72` collides with `math.h`'s own `round` macro** —
   one `warning: "round" redefined` per translation unit, **in plain-C builds only**
   (`xc-dsc-gcc`); the C++ path is clean, which is why the Arduino builds all report zero
   warnings. Measured both ways Sep 17, 2026. Fix and full reasoning on the Phase 10 list.

---

## Key Technical Constraints (Don't Forget)
1. **Config bits in ONE .c file only** — archiver crashes on duplicates
2. **ADC power-up order** — ADON first, SHRPWR second, wait SHRRDY
3. **PPS OC codes** — 15-18 (not 8-9, those are SPI2)
4. **XC-DSC v4.00 has C++ support** — g++ with `-D__prog__=""` for DFP compat; all .c MUST be compiled with `g++ -x c++` for linker ABI match (see Phase 8)
5. **`--whole-archive`** — required around core.a for CRT startup
6. **stderr suppression** — Arduino IDE treats any stderr as error
7. **Dot-notation via struct + function pointers** — C trick for Arduino-style
   `Object.method()` syntax. Still how `SPI`, `Wire` and `HRPWM` work. **`Serial` no
   longer does** — a struct member cannot be overloaded, so it became an inline C++
   class in C++ mode with the C struct kept under `#ifndef __cplusplus` (see "Serial
   overloads"). Any future object that needs overloads must go the same way.
8. **MC parts need the MC DFP override** — `boards.txt` must set
   `build.dfp.path={build.dfp.path.mc}`; without it the build reaches for the MP DFP
9. **A board is only addable if its device is in an installed DFP** — the linker
   needs `p{mcu}.gld` from `{build.dfp.path}/support/dsPIC33C/gld/`. Check with
   `ls <DFP>/xc16/support/dsPIC33C/h/` before writing any variant files
   (this blocked Phase 13 / MC005 until MC DFP 1.10.386 was installed)
10. **`-mdfp=` points at `<pack>/xc16`, never the pack root** — the root lacks
   `bin/c30_device.info`, and the compiler misreports that as
   `Invalid -mcpu option. CPU <part> not recognized.`
11. **Pin tables must be dense** — `wiring_digital.c` doesn't NULL-check
   `port_reg`/`tris_reg`, so number over bonded pins only, never leave gaps
12. **Not every dsPIC33CK has every peripheral** — guard on register presence using
   the device header's `#define REG REG` self-defines (`ACLKCON1` for high-res PWM,
   `PGnCONL` for generator count, `PWM5_PIN` for the 5th SCCP). The MC Value Line
   parts have no Auxiliary PLL, so HRPWM cannot work there at all.

---

## API Style Guide (C "OOP" Pattern)

**There are now two patterns, and the choice is forced by one question: does the object
need overloaded methods?** `Serial` does (`print(int)` vs `print(double)` vs
`print(const char *)`), and a struct member name can only ever mean one function, so
`Serial` is the C++-class pattern below. Everything else — `SPI`, `Wire`, `HRPWM` — has
no overloads and stays on the C struct pattern. Use the struct pattern by default; it is
the one that keeps working when the file is compiled as plain C by the `cmake/` projects.

### Pattern A — C struct of function pointers (`SPI`, `Wire`, `HRPWM`)

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

Cost to know about: the initialiser names every function, so `--gc-sections` cannot drop
any of them, and the pointers themselves sit in `.data`. This is exactly why dropping the
pattern for `Serial` made every board *smaller*.

### Pattern B — external C functions + an inline C++ class (`Serial` only)

```c
/* .c file — the implementation stays C, and stays in a .c file, because the
 * cmake/ projects compile it as C. */
size_t serial_write(uint8_t c) { ... }        /* external linkage, not static */
#ifndef __cplusplus
HardwareSerial_t Serial = { .write = serial_write, ... };   /* C mode keeps the struct */
#else
HardwareSerial Serial;                                       /* C++ mode: the object */
#endif
```
```cpp
/* .h file, after the extern "C" block closes */
class HardwareSerial {
public:
    size_t write(uint8_t c) { return serial_write(c); }   /* every method inline */
    size_t print(int v, int base = DEC);                  /* overloads + defaults */
};
```

Rules that make this safe: **every method inline** (no new translation unit, and the link
step runs the C driver `xc-dsc-gcc`, which must never be handed anything needing
libstdc++), **no data members** (the object is 1 byte, versus 26 for the struct), and the
C struct retained under `#ifndef __cplusplus` so the `cmake/` plain-C build still links.
A function needing a C++ default argument — `tone(pin, freq)` — declares that default in
an `#ifdef __cplusplus` branch in `Arduino.h`, never in the definition.

User sketch syntax:
```c
#include <Arduino.h>   // includes Serial automatically
#include <SPI.h>       // opt-in
#include <Wire.h>      // opt-in

Serial.begin(9600);
Serial.println(42);            // stock Arduino spelling, since Sep 17 2026
Serial.print(255, HEX);
Serial.println(3.14159, 3);
SPI.transfer(0x55);
Wire.beginTransmission(0x50);
Wire.endTransmissionStop(1);   // still a suffix workaround -- Wire was NOT migrated
```

---

## File Structure
```
Arduino_dsPIC33CK/
├── arduino-platform/           <- SHAREABLE PACKAGE
│   ├── microchip/dspic33ck/
│   │   ├── cores/arduino/      <- Core API source files
│   │   │   ├── Arduino.h
│   │   │   ├── wiring_private.h        (core-INTERNAL contract; not in Arduino.h)
│   │   │   ├── HardwareSerial.h / .c   (Serial: C struct + C++ class, one file)
│   │   │   ├── wiring.c                (millis/delay)
│   │   │   ├── wiring_digital.c        (GPIO, pinToRP)
│   │   │   ├── wiring_analog.c         (ADC/PWM)
│   │   │   ├── wiring_shift.c          (shiftIn/Out)
│   │   │   ├── wiring_tone.c           (tone/noTone, SCCP4)
│   │   │   ├── wiring_interrupts.c     (attachInterrupt via CN, GIE pair)
│   │   │   ├── system_config.c / .h    (clock/fuses)
│   │   │   └── main.c                  (entry point)
│   │   ├── variants/
│   │   │   ├── dspic33ck32mp102/
│   │   │   ├── dspic33ck256mc002/
│   │   │   ├── dspic33ck256mc005/      (Phase 13 - the only board run on silicon)
│   │   │   └── dspic33ck256mp508/
│   │   ├── libraries/
│   │   │   ├── SPI/src/SPI.h + SPI.c
│   │   │   ├── Wire/src/Wire.h + Wire.c
│   │   │   └── HRPWM/src/HRPWM.h + HRPWM.c   (PG1-PG8, 250ps)
│   │   │       └── examples/BoostMPPT/
│   │   ├── tools/
│   │   │   ├── suppress-stderr.bat       (stderr -> silence for Arduino IDE)
│   │   │   ├── xc-dsc-size-wrapper.bat   (size reporting)
│   │   │   ├── xc-dsc-link.bat           (linker CWD workaround)
│   │   │   ├── nedbg-upload.bat          (flash + reboot the wedged CDC bridge)
│   │   │   ├── upload_uart.py            (UART bootloader upload)
│   │   │   └── pre_build.py              (ORPHANED - not in platform.txt)
│   │   ├── examples/
│   │   │   ├── 01.Basics/        Blink, AnalogReadSerial   (all 4 boards)
│   │   │   ├── 02.CppFeatures/   CppDemo                   (MP508 only: LED1/A22)
│   │   │   ├── 03.PWM/           Fade, PWMTest    (MP508 only: LED2; Phase 9)
│   │   │   └── 04.CuriosityNano/ NanoBlink, NanoSerialHello, NanoButtonLED,
│   │   │                         NanoAnalogRead, NanoPWMFade, NanoSelfTest
│   │   ├── bootloaders/
│   │   ├── boards.txt
│   │   ├── platform.txt
│   │   ├── platform.local.txt.template
│   │   └── programmers.txt
│   ├── docs/                   <- 5-part guide + how-to-use/ (5 more)
│   │                              STALE: teaches the pre-Phase-10 Serial API
│   ├── install_arduino_ide.bat  <- RE-RUN after any platform change
│   └── README.md
├── docs/                       <- Flash CRC integrity check note (standalone)
├── config.mcc/                 <- MCC-generated reference code
├── test_led/                   <- Hardware test builds & objects
│   └── DM330030_RGB_POT/       <- Phase 7 sketch (not in examples/)
├── cpp_support/                <- OBSOLETE (Phase 8); xc16_gcc_source ~500MB
├── cmake/                      <- 3 MPLAB X projects: Arduino_dsPIC33CK,
│                                  SCCP1_dsPIC33CK, ...MC005_Out_of_Box_Demo.
│                                  NOT legacy: they compile the core with
│                                  xc-dsc-gcc in C mode, and are the ONLY gate
│                                  catching C++-only syntax in a .c file, since
│                                  the Arduino path builds everything as C++.
│                                  Add every new core .c to their file.cmake.
├── _build/                     <- gitignored regression harness (see below)
└── PLAN.md                     <- THIS FILE
```

**The build gates live in `_build/` and are gitignored**, so they do not survive a fresh
clone — recreate or copy them before trusting a "builds clean" claim:

| script | what it proves |
|---|---|
| `allboards.sh` | all 4 devices compile + link + emit hex; `sketch.cpp` is a synthetic sketch calling the whole API, including all six Phase 10 functions plus `analogWrite(PWM4_PIN, …)` in one translation unit, which is what proves the `wiring_private.h` link contract closes |
| `examples_mc005.sh` | the six `04.CuriosityNano` sketches, warning count per sketch |
| `examples_all.sh` | `01.Basics` on all 4 devices, `02.CppFeatures` + `03.PWM` on MP508 only (they use `LED1`/`LED2`/`A22`, which only that variant defines) |
| plain-C check | every core `.c` + `variant.c` built with `xc-dsc-gcc -Wall -Wextra` in C mode on all 4 devices — the same guard the `cmake/` projects give, run from the shell |
