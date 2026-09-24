# Arduino_dsPIC33CK Platform — Project Plan

> **Last reconciled against the tree: September 24, 2026.** Phases 1–8 complete.
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
> **Phase 14 (install from a Boards Manager URL) opened September 17, 2026, PUBLISHED
> September 18, 2026 as `v1.0.0`, and released **`v1.0.1` on September 22, 2026**.** The
> repo is public, each release carries the three archives plus the index, and
> `_build/live_check.sh` installs from the real URL and builds all four boards at baseline.
> **Upload was verified on silicon September 22, 2026** through the published install —
> `Program Succeeded`, the nEDBG bridge reboot intact, and `NanoSerialHello` read back live.
> `v1.0.1` then fixed the two items that release opened: *Upload Using Programmer* (no
> `program.pattern` recipes existed, so all six programmers failed) and the missing
> `-Wl,--gc-sections`, worth ~9 KB per sketch. Both are verified on silicon, `install_check`
> and `live_check` are green at the new baselines, and the 1.0.0→1.0.1 upgrade was tested.
> `v1.0.1` was then **re-published in place on September 22, 2026**: `File > Examples` had
> never shown a single board sketch, because Arduino builds that menu from libraries only
> and all eleven lived in a platform-level `examples/`. They now ship inside
> `libraries/Arduino_dsPIC33CK/`, and `install_check.sh` asserts the menu offers every one.
> **`v1.0.2` is PUBLISHED (September 22, 2026)** — a content release: 15 new
> `04.CuriosityNano` sketches (27 examples shipped in total), a generated pin-map diagram on
> the GitHub page, and the `-Wpedantic` cleanup in `SPI.c`/`Wire.c`. No core API changed.
> Commit `97c3718`, tag `v1.0.2`, all four assets attached, and `/releases/latest` resolves
> to it. **All nine gates green**, including `live_check.sh` against the published URL and
> `upgrade_check.sh` — and a live install proves `File > Examples` offers all 27. Unlike the
> 1.0.1 re-publish, Boards Manager will offer this one unprompted, because the version
> string changed. See that section — it also records that `package_check.sh` had been
> reporting false drift against baselines two releases stale, and that `upgrade_check.sh`
> was stale and is now version-agnostic.
> **Phase 15 (serial bootloader for dsPIC33CK256MC005) opened and became CODE-COMPLETE on
> September 23, 2026.** A sketch can now be uploaded from the IDE over the CDC COM port with
> no debugger and no MPLAB X — which is what finally makes the platform usable on a
> dsPIC33CK board that has nothing but a UART. The bootloader owns page 0 permanently and
> forwards all 200 interrupt vectors through a GOTO trampoline in the application, so **no
> sketch and no file in `cores/` changes the way it names an ISR**; the one core change is a
> 33-word soft-entry sniffer in the UART RX interrupt. Two gates
> (`_build/build_bootloader.sh`, `_build/bootloader_check.sh` sections 1–10) hold the
> linker scripts, the firmware and the host tool to the same memory map, and the whole
> plumbing chain is verified through `arduino-cli` against an install — including that
> `-Wl,--gc-sections` does **not** collect the ISRs, which was the single most likely thing
> to be wrong. **Verified on silicon the same day:** six of the seven hardware checks pass,
> and the seventh — surviving a power cut part-way through a flash row write — is documented
> as a known limit rather than claimed. Getting there cost **five defects the offline gates
> could not see** — a `boards.txt` key that silently routed serial uploads through the debugger
> and erased the bootloader, an uncleared ANSEL bit that stopped the bootloader ever
> auto-jumping to a valid sketch, a `bin2hex` call missing `-mdfp=` that scattered config words
> into the application region, a host knock timeout longer than the firmware's own entry
> window, and `Burn Bootloader` working with the nEDBG only — i.e. with every programmer except
> the ones the feature exists for. All five are fixed, each with a comment recording the
> symptom, and the gate went from 84 checks to 112. `Bootloader: "none"` is the default.
> **It is not bit-for-bit unchanged, and v1.0.3 was published claiming it was** - see the
> v1.0.4 section, which is what that claim cost.
> **Phase 15 SHIPPED on September 23, 2026 — as `v1.0.3`, corrected within hours by
> `v1.0.4`, which is the version to install** — with
> `docs/part6_serial_bootloader.html` as its user guide. Writing that guide found a sixth
> defect the bench session had missed, because it only shows up across two consecutive uploads:
> **a sketch with no `Serial.begin()` — `NanoBlink`, the first example anyone opens — blocks the
> next serial upload**, since soft entry asks the *running* sketch to reset itself. Recovery is
> verified (`Burn Bootloader`, no hands on the board) and documented; the real fix, a host-side
> replug catch, is the top candidate for the next version.
> **`v1.0.5` is PUBLISHED (September 24, 2026) and is the version to install** — a
> documentation release in which the **platform code is unchanged from v1.0.4**, measured
> rather than claimed: flash and RAM sit exactly on the 2556 / 3920 / 2556 / 3188 baselines in
> five separate gates. A pre-release audit compiled the shipped user guide for the first time
> and found two defects in it: **133 calls to `Serial_*` functions that do not exist**, and
> **6 raw `<` operators inside `<pre>` blocks**, which HTML parses as start tags — so browsers
> had been eating spans of code and serving readers 12 invisible lines of `part4` plus a `for`
> loop that compiles and never runs. Both fixed, and both now gated by
> `_build/docs_code_check.sh` (14 sketches, 28 builds, 0 warnings), which exists because every
> other gate compiled files in the repo and nothing compiled the code in the docs. This release
> also **ships the 8 guide pages inside the archive**, adds **Part 6 §6.9** (uploading with a
> plain 3.3 V USB-serial adapter and no debugger at all — *a programmer once per board, a
> serial adapter forever after*) and **Part 7**, the bench-verification procedure. See that
> section; it also records why `package_check.sh`'s hex column was stale since v1.0.4 and what
> was checked before re-baselining it.
> **Committed after v1.0.5 and HELD, unreleased (September 24, 2026):** a pin-map audit that
> found the picture was right and the prose around it was not. The SVG regenerates
> byte-identically and matches `variant.c` on all 39 pins — but nothing *checked* that, and
> the README claimed it "cannot drift". `tools/pinmap/check_pinmap.py` is now that check
> (five parts, all seven mutations caught), and `docs/part2` finally documents the priority
> device: up to v1.0.5 it covered only the 28-pin MP102, so an MC005 user read a table whose
> every row named a different port than their board had. **`part2` and `part6` are inside the
> archive, so this work reaches nobody until a `v1.0.6` is cut** — the release is deliberately
> held. Nothing else in the archive changed. See the section below for the retracted claim
> that came out of this: the DFP's `<edc:PinList>` is **not** a usable source of physical pin
> numbers.
> **One item remains open:** the fresh-install/resolver-glob test still wants a machine with
> a different XC-DSC version. See that section.
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
> the IDE will silently build the old core. That installer is now the **developer** path
> only (Phase 14): end users install from the Boards Manager URL and never get a
> `platform.local.txt`, so a bug that only a `platform.local.txt` hides will not show up
> on this bench. **Branch state, checked September 17, 2026:** HEAD is
> `phase10-platform-cleanup` at `37f0b52`, which carries all four commits
> (`b585acb`, `7553e21`, `78fe2fd`, `37f0b52`). Local `main` and `origin/main` are both
> still at `0a24860` and have **none** of them. So a release cut from `latest` on GitHub
> today would ship the pre-Phase-10 tree — push before tagging.
>
> **Known inconsistency — the API half was fixed in v1.0.5, and this note had it wrong.**
> It said the Phase 6 documentation taught the "superseded suffixed `Serial` API", which
> implied code that worked but was old-fashioned. The docs in fact called `Serial_begin`
> and `Serial_println`, which do not exist and do not compile. Fixed, and now gated by
> `_build/docs_code_check.sh`. What remains is coverage: the MC005 board, `tone()` and
> `attachInterrupt()` are still unmentioned. See Phase 12. (The *installation and
> toolchain* half was corrected in Phase 14.)
>
> ### PRIORITY, set September 17, 2026: dsPIC33CK256MC005 only
>
> **From now on the dsPIC33CK256MC005 (EV08P02A Curiosity Nano) is the priority device.
> The other three — 33CK32MP102, 33CK256MC002, 33CK256MP508 / DM330030 — are ON HOLD.**
> Items below that can only be done on a held board are marked `ON HOLD`; do not propose
> them as next steps. This is a change of *priority*, not of support: all four boards stay
> in `boards.txt` and all four must keep building.
>
> Three consequences worth knowing before planning anything:
>
> - **`_build/allboards.sh` keeps building all four devices, deliberately.** A compile +
>   link check on a held board costs seconds and is the only thing stopping the held
>   variants from rotting into a broken state that is expensive to diagnose later. Narrowing
>   the gate to MC005 would be the wrong economy. The same goes for the plain-C check and
>   for `01.Basics` in `examples_all.sh`.
> - **The priority is already aligned with the outstanding work.** All four bench checks
>   still owed from Phase 10 are on EV08P02A: `tone()` on LED0, `attachInterrupt()` on SW0
>   and on a PORTA pin, the `INPUT_PULLUP`-survives test, and the `GIE` assertion. MC005 is
>   also the only board ever verified on silicon (Phase 13), so it is the only one where a
>   hardware result means anything today.
> - **What MC005 can and cannot do — checked against `p33CK256MC005.h`, not assumed:**
>   it has **no `ACLKCON1`, i.e. no Auxiliary PLL, so HRPWM cannot work on it at all** —
>   that one is silicon and is moot rather than deferred. It **does** have a DAC, one
>   channel (`DAC1CONL`/`DACCTRL1L` present; MP508 has three), so the Phase 10 DAC item is
>   *reachable* here after all.
> - **Clock: FCY is a menu choice, not a device property, and the default is slow.** All
>   four boards default to `build.f_cpu=8000000UL`, i.e. **FCY = 4 MHz**, and all four —
>   MC005 included — offer a `200mhz_pll` entry on the Tools > Clock menu. So the 100 MHz
>   figures in the Phase 9 notes are not "the MP508's speed": they assume the PLL menu
>   option is selected. Check which clock entry is active before reusing any prescaler or
>   period figure, on any board.

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
- [x] `arduino-platform/docs/how-to-use/` — 5 HTML guide files. **These files no longer
      exist**: superseded by the eight-page guide and deleted September 24, 2026. Kept as a
      record of what Phase 6 delivered; recover with `git checkout <sha> -- <path>` if the
      original wording is ever needed.
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
    NOT in the shipped examples tree. Promote it to
    `libraries/Arduino_dsPIC33CK/examples/` if it should ship.
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
- [ ] Document HRPWM in `docs/part3_api_reference.html` (it predates the HRPWM library;
      the original target, `docs/how-to-use/`, was deleted September 24, 2026)

### UART Bootloader Upload — SUPERSEDED by Phase 15 (Sep 23, 2026)
- [x] `tools/upload_uart.py` — sends .hex over UART, Microchip 16-bit bootloader
      protocol (0x55 sync → erase → chunked hex records)
- [x] **DELETED Sep 23, 2026.** It was never hardware-verified because it never could
      be: it implemented a protocol no firmware in this project spoke, hardcoded
      32 KB-device flash constants wrong for MC005, and indexed Intel HEX bytewise,
      ignoring phantom-byte encoding. Replaced by `tools/serial_upload.py` plus real
      bootloader firmware — see Phase 15.

### C++ Feature Example — belongs to Phase 8
- [x] `libraries/Arduino_dsPIC33CK/examples/02.CppFeatures/CppDemo/CppDemo.ino` — exercises the native C++
      support that XC-DSC v4.00 unlocked

### Orphaned: `tools/pre_build.py`
- Patched Arduino-IDE-generated .cpp back to C-compatible (stripped `extern "C"`,
  duplicate includes, added setup/loop forward decls).
- **Not referenced anywhere in `platform.txt`** — no `recipe.hooks.prebuild` line exists.
  Superseded by Phase 8's native C++ mode. Safe to delete; kept only as history.

---

## ON HOLD — held-device work

### Phase 9: Hardware PWM Verification — ON HOLD (DM330030 / MP102 only)
**Held September 17, 2026 by the MC005-only priority.** This phase was the long-standing
"IN PROGRESS" item and is now parked: every remaining task needs a DM330030 or an MP102 on
the bench. Nothing here is blocked on code.

Note before resuming: pre-existing bug 2 below means **`analogWrite(58, x)` can never reach
LED2** — `PWM5_RP 181` is a virtual pin RPV5, not a bond wire — so the headline PWMTest
target does not work as written and the sketch needs re-pointing at a real PWM pin first.
Resuming this phase without reading that will waste a bench session.

- [x] Test sketches written:
  - `libraries/Arduino_dsPIC33CK/examples/03.PWM/PWMTest/PWMTest.ino` — 4 tests on LED2 (RE5 = D58 = RP181 → SCCP5):
    50%; 25/50/75/full; smooth fade; PWM→digital→PWM transition. Serial 115200 via PKOB4 CDC.
    Expected ~490 Hz (at FCY=100 MHz: prescaler 1:64, period=3187 → 490.5 Hz)
  - `libraries/Arduino_dsPIC33CK/examples/03.PWM/Fade/Fade.ino`
- [ ] ON HOLD — Run PWMTest on DM330030 and confirm frequency + duty accuracy (scope or LED)
- [ ] ON HOLD — Test `analogWrite()` output on D5-D8
- [ ] ON HOLD — Test on MP102 board (not just DM330030)

**Was blocked on bench access; now held by priority.** The code side is done.

**The MC005 equivalent is NOT held** — `analogWrite()` on the Curiosity Nano is worth
verifying on its own terms, and does not need this phase. It is a different measurement: the
~490 Hz figure and the 1:64 prescaler quoted above assume the **200 MHz PLL clock menu entry
(FCY = 100 MHz)**, whereas the default on every board is `f_cpu=8000000UL` → **FCY = 4 MHz**,
where the same prescaler gives ~19.6 Hz. Re-derive against whichever clock entry is actually
selected. Filed under Phase 12.

---

## Phase 14: Install from a Boards Manager URL (IN PROGRESS — Sep 17, 2026)

Goal, in the user's words: *"the way just to add the github link to the arduino when click
add board and let it download our platform into their arduino ide, as same as STM mcu
(Instead of using \*.bat to install)"*. Plan approved from
`~/.claude/plans/expressive-wishing-lynx.md`; three design questions were answered
Recommended-option: resolve paths at build time in wrappers, Windows-only stated plainly,
keep the old installer as the developer path.

**The constraint that shaped everything:** Board Manager only downloads and unzips — it
runs *no* post-install script, by design. And a prebuild hook cannot substitute, because
`platform.txt` properties expand *before* `recipe.hooks.*` run, so a hook can never supply
`{build.dfp.path}` to the same build. So the four machine-specific paths had to split by
*why* they are machine-specific:

| path | why | where it went |
|---|---|---|
| `build.dfp.path`, `build.dfp.path.mc` | per-user profile + upstream pack version — **undefaultable** | Board Manager tool packs, reached via `{runtime.tools.<pack>.path}`. The DFPs are **Apache-2.0**, so they can be pruned and redistributed (notices kept, modification stated in `PRUNED.txt`). |
| `build.compiler.path`, `build.tools.mplab.path` | licence-gated, drift only by *version* | resolved at build/upload time by `tools/xc-dsc-find.bat`, called from the `tools/bin/*.bat` shims |

XC-DSC's licence §2 is "non-transferable" and §6(a) forbids distribution, so the STM32
model (ST ships its own GCC as a tool) is closed to us; same for MPLAB X IPE. Both stay
user-installed prerequisites. **This is why Windows-only is a published claim** — the
`systems[]` list names `i686-mingw32` only, so a Linux/macOS user gets a clear "not
available" instead of a half-working install.

**Done and verified:**
- [x] Two pruned DFP tool packs (`1.16.521-pruned.1`, `1.11.412-pruned.1`) — 792 KB and
      637 KB, down from 672 MB of upstream packs
- [x] `tools/xc-dsc-find.bat` resolver (override key → env var → cache → version-sorted
      glob → actionable error), plus seven byte-identical `tools/bin/*.bat` shims that
      derive the tool name from `%~n0`, so `%*` forwards compiler arguments untouched
- [x] `tools/ipecmd-upload.bat` for all five ipecmd programmers
- [x] `package_microchip_dspic33ck_index.json` rewritten: real urls, sizes, SHA-256s,
      all four boards, both tool dependencies
- [x] `tools/release/make-release.sh` — deterministic zips (sorted entries, fixed
      `date_time`), so a rebuild is byte-identical and checksums are stable
- [x] `install_arduino_ide.bat` rewritten (196 lines) as the **developer** path: it now
      calls the platform's own resolver instead of duplicating detection, checks the
      `tools\bin\` shims, writes only two DFP keys and only when the tool packs are
      absent, and *removes* the stale data-folder index copy it used to create. The old
      C++-fallback block was deleted — it wrote `compiler.c.cmd=xc-dsc-gcc` with no
      `.bat` suffix, which is broken against the shim layer.
- [x] **`_build/install_check.sh` — the gate this phase needed.** Serves the real release
      archives over local HTTP and runs `arduino-cli core install microchip:dspic33ck`, so
      checksums, archive roots, tool placement and `toolsDependencies` are all actually
      exercised. Result: both packs + platform installed, **no `platform.local.txt`
      anywhere**, and all four boards compiled at the exact `_build` baselines —
      **11904 / 13992 / 11916 / 12876 bytes**. Compressed release total under 1.5 MB.
- [x] Developer path re-verified as a genuinely different configuration (no tool packs,
      DFPs via a 2-line override): MC005 12876 B and MP102 11904 B, zero warnings —
      byte-identical to the Boards Manager path, which independently confirms the pruned
      packs match the full ones.
- [x] Docs: `README.md` and `arduino-platform/README.md` rewritten to lead with the URL;
      six HTML docs bannered; the whole `v3.31` / `xc16-gcc` / "no C++ support" /
      "install the DFP from MPLAB X" family corrected across nine files. Two doc
      instructions were **actively harmful**, not just stale, and are now fixed:
      `part4_upload_hardware.html:164` told users to set `tools.pickit4.path`, a key that
      no longer exists; `part3_build_verify.html:242` told them to put
      `compiler.c.extra_flags=-O2` in `platform.local.txt`, which would replace
      `platform.txt`'s `-x c++ -fno-exceptions -fno-rtti -fno-threadsafe-statics
      -fno-use-cxa-atexit` and silently compile the C++ core as C.

- [x] **`platform.txt`'s own comments corrected September 18, 2026** — lines 6, 82 and
      111 promised a v3.x C-mode fallback that the shim layer makes impossible
      (`compiler.c.cmd` and `compiler.cpp.cmd` are both `xc-dsc-g++.bat`; the core is C++
      throughout). All three now say v4.00+ is required and name the real failure mode.
      The `c.extra_flags` one was the same harmful class as the two doc lines above: it
      told the reader to blank a key that carries `-x c++`. `package_check.sh` re-run
      after the edit — four boards, baseline sizes, zero warnings.
- [x] **`tools/pre_build.py` deleted September 18, 2026** (`git rm`) — nothing in the
      tree referenced it, and a prebuild hook could not have worked anyway for the
      expansion-order reason above.
- [x] **Parallel-build race cleared — `_build/parallel_check.sh`, new.** The resolver
      caches its glob hit at `%LOCALAPPDATA%\Microchip\Arduino_dsPIC33CK\xcdsc.path`,
      written to a temp name and `move`d into place; a serial build never exercises that.
      Five cold-cache builds at `-j16` (cache deleted before each) all produced a
      byte-identical hex, zero warnings, a single-line cache file and **no orphan `.tmp`**
      — the signature a lost race would leave. `-j16` 7.1–7.5 s vs `-j1` 11.8 s.
- [x] **Shim spawn cost measured, as the plan required rather than assumed: ≈44 ms per
      invocation.** 30 calls, bare `.exe` 1420 ms vs 2740 ms through
      `tools/bin/xc-dsc-g++.bat`, timed from inside `cmd.exe` (`_build/spawn_cost.bat`) so
      the measurement does not include a bash→cmd hop the real build never pays. Blink
      links ~11 objects, so this is well under a second per sketch and it parallelises.
- [x] **Upgrade path 1.0.0 → 1.0.1 — `_build/upgrade_check.sh`, new.** Serves an index
      carrying both versions (1.0.1 synthesised from the 1.0.0 zip with `version=` bumped,
      repacked with make-release's deterministic settings), installs 1.0.0, clears the
      download cache, upgrades. Both DFP packs reused with **no second HTTP GET** (counted
      from the server's own access log, not arduino-cli's console text), 1.0.0's directory
      removed, no `platform.local.txt` at either version, four boards at baseline sizes.
      **Worth knowing:** arduino-cli 1.5.1 prints `Uninstalling <pack>, tool is no more
      required...` mid-upgrade, in the window after 1.0.0 is dropped and before 1.0.1 is
      installed. It does not act on it — the packs stay on disk with `xc16/` intact, which
      the gate now asserts directly rather than inferring from the absent re-download,
      since that ordering is arduino-cli's internal business and not a contract.

**PUBLISHED September 18, 2026 — the URL in the docs now works for anyone:**
- [x] **Branch pushed, then `main` fast-forwarded to it** — `origin/main` is at `9fd77f7`
      (was `0a24860`). This had to happen *before* the repo went public: `main`'s README
      still said *"Run `install_arduino_ide.bat` — One-click installer"*, so a visitor
      would have landed on the retired path with no Boards Manager URL anywhere.
- [x] **Repository made public.** It was **private**, which blocked the entire design and
      was not in the plan: Arduino IDE fetches the Additional Boards Manager URL
      unauthenticated with no way to pass a credential, so cutting a release would not
      have been enough on its own — unauthenticated GETs of the repo, of
      `raw.githubusercontent.com` and of the releases API all returned 404 (GitHub answers
      404, not 403, for private repos). Publishing the artifacts while keeping the source
      private is not an available option either: an Arduino platform archive *is* source.
- [x] **Release `v1.0.0` cut** at `9fd77f7`, with **four** assets — the three zips *and*
      the index, since the index is what the pasted URL resolves to. Two traps avoided:
      the tag must be exactly `v1.0.0` because the index pins the archive URLs to
      `releases/download/v1.0.0/`, and the release must **not** be marked pre-release
      because GitHub's `/releases/latest/` skips pre-releases — which would have left the
      documented URL 404ing exactly as before. `gh` is not installed; the release was
      created through the REST API with the credential-manager token.
- [x] **`_build/live_check.sh` — PASS, and it closes the one gap `install_check.sh`
      documents in its own header ("the only thing it cannot test is GitHub itself").**
      A fresh Arduino data directory pointed at the real
      `releases/latest/download/package_microchip_dspic33ck_index.json`, a real
      `core install` pulling all three archives off GitHub's CDN, then all four boards
      compiled at the exact baselines **11904 / 13992 / 11916 / 12876 bytes** with zero
      warnings and no `platform.local.txt`. Also verified independently of arduino-cli:
      the three published archives' SHA-256s match the published index.

- [x] **UPLOAD VERIFIED ON SILICON, September 22 2026 — the untested seam is closed.** Done
      from the **published install** (`_build/cli-live`, installed off the real GitHub URL),
      not the developer tree, so it exercises exactly what a user gets. EV08P02A, serial
      `MC020162801RYN000004`, CDC on COM63:
      - `NanoBlink` compiled at **12876 bytes** (the MC005 baseline) and programmed:
        `Target device dsPIC33CK256MC005 found` → `Program Succeeded` → `Operation Succeeded`.
      - **The nEDBG program-then-reboot sequence survived the refactor** out of the deleted
        `nedbg-upload.bat` into `ipecmd-upload.bat`: `Rebooting nEDBG to restore the serial
        bridge... / Serial bridge ready. / New upload port: COM63`.
      - `NanoSerialHello` (13372 B) uploaded and its output read back live: ticks ~1002 ms
        apart, `millis()` tracking, and the board ran **200 s continuously** without reset.

      **This did not need a fresh machine, and the item used to imply it did.** The two
      things were independent and are now separated:
      - *Upload on hardware* needed **the board**, which is on this bench. **Done.**
      - *A genuinely fresh install* is the only part wanting another machine — see the
        remaining item below.

      **Correction to the note that used to live here:** the "Serial Monitor silent after
      upload" symptom has **two** causes, and only one is the nEDBG wedge. The second bit
      during this test: a terminal that does **not assert DTR** reads nothing at all, while
      the board is running perfectly. 12 s of silence, then the same port with
      `DtrEnable=$true` produced output immediately. Arduino IDE's Serial Monitor and PuTTY
      assert DTR by default; hand-rolled `System.IO.Ports` / pyserial readers do not. Check
      DTR before believing a silent port.

      **Also learned:** `setup()` output is **always lost on upload** and that is inherent —
      the target is released from reset *before* the CDC bridge finishes rebooting, so the
      banner is printed into a dead bridge. **To capture it, open the port first and then
      power-cycle the target only**, which leaves the bridge alive:
      `pymcuprog setsupplyvoltage -l 0` then `-l 3.3`. Verified Sep 22 2026 — the full
      banner arrived followed by `tick=0  uptime_ms=111`, a genuine reset:
      `=== dsPIC33CK256MC005 Curiosity Nano === / FCY : 4000 kHz / LED_BUILTIN: D37 / ready`.
      (`FCY = 4000 kHz` is the expected default — `f_cpu=8000000UL`, with `200mhz_pll` on the
      Tools → Clock menu.) The two alternatives do **not** work: `pymcuprog reset` does not
      support this device (its list has `dspic33ck64mc105`, not `...mc005`), and
      `reboot-debugger` drops the port — which is also why the upload recipe uses
      `reboot-debugger`, since it needs no `-d`.

**Still open — this is the part that wants a machine that is not this one:**
- [ ] Fresh-install acceptance test. Everything *functional* is now verified on this bench;
      what remains is the **no-prior-install state** and the resolver glob against a
      different toolchain version. **Does not need a new computer** — a second Windows user
      account gives a clean `%LOCALAPPDATA%\Arduino15`, a clean resolver cache and no
      `.mchp_packs`, and Windows Sandbox (available on this Enterprise SKU) gives a clean OS
      where a *newer* XC-DSC can be installed, which is the case the version-sorted glob was
      written for and has still never exercised. Neither proxy covers USB upload well, but
      upload is no longer the unknown.

      **The procedure, so it does not have to be re-derived:**

      *Do not clone the repo or run `install_arduino_ide.bat` on the test machine* — that
      is the developer path and would contaminate the test with the overrides this phase
      exists to remove.

      1. Install Arduino IDE 2.x and **XC-DSC v4.00+** (free, microchip.com/mplab/compilers).
         **MPLAB X** only if uploading — it is a ~2 GB install and compile-only tests do
         not need it.
      2. Preferences → Additional Boards Manager URLs →
         `https://github.com/9Nicotin/Arduino_dsPIC33CK/releases/latest/download/package_microchip_dspic33ck_index.json`
      3. Boards Manager → search `dsPIC33CK` → expect `Arduino_dsPIC33CK (Windows only)`
         1.0.0 → Install (~1.5 MB including both DFP packs).
      4. Tools → Board → **Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)**.
      5. Examples → `04.CuriosityNano` → `NanoBlink` → Verify, then Upload to the EV08P02A.
      6. `NanoSerialHello` for the Serial Monitor; `NanoSelfTest` is the broadest single
         sketch if only one run is possible.

      **Three expected behaviours that look like bugs — do not chase any of them:**
      - The Serial Monitor is **silent for ~10 s after an upload**. `ipecmd` wedges the
        nEDBG CDC bridge, the recipe reboots the debugger, USB re-enumerates. See
        `nedbg-flash-then-reboot-debugger`.
      - A **terminal that does not assert DTR reads nothing, ever**, from a perfectly
        healthy board — confirmed on this bench Sep 22 2026. The IDE and PuTTY assert it;
        scripted readers usually do not.
      - A macOS/Linux co-worker **sees the platform but cannot install it** — `systems[]`
        names `i686-mingw32` only, deliberately, so they get a clean "not available".

      If XC-DSC is missing or installed outside `%ProgramFiles%\Microchip\xc-dsc\v*\bin`,
      the resolver prints an explicit error naming the download page and the two escape
      hatches (`%XCDSC_PATH%`, or a `platform.local.txt` override). Seeing that error with
      a *normal* XC-DSC install is a real finding — capture the version and install path.

      Before reporting any failure, turn on Preferences → **Show verbose output during:
      compile + upload**; that is what reveals which paths the resolver actually picked.

- [x] **BUG in the published v1.0.0 — *Sketch → Upload Using Programmer* fails for all six
      programmers. FIXED in v1.0.1, verified on silicon Sep 22 2026.** Found by accident
      during the Sep 22 upload test: invoking `arduino-cli upload -P nedbg` died with
      `Failed programming: recipe not found 'program.pattern'`. `programmers.txt` wires every
      programmer to a `program.tool` (`pickit5`, `pickit4`, `snap`, `pkob4`, `nedbg`,
      `uart_bootloader`), but `platform.txt` defined **6 `upload.pattern` keys and 0
      `program.pattern` keys**. So the menu entry was offered by the IDE and could not work.

      The normal Upload button was never affected — it uses `boards.txt`'s
      `<board>.upload.tool=nedbg` → `tools.nedbg.upload.pattern`.

      **Fix:** each of the six tools now defines `program.pattern` (plus
      `program.params.verbose`/`.quiet`) mirroring its `upload.pattern` verbatim, with a
      header comment in `platform.txt` explaining that the two are reached by different menu
      items and must be edited in pairs. They are deliberately identical: ipecmd does a full
      erase-and-program either way, so there is no distinct "program" operation to express.

      **Verified** through the installed 1.0.1 platform: 6 `upload.pattern` / 6
      `program.pattern` / 6 programmers declaring `program.tool`, and
      `arduino-cli upload -P nedbg` reached the recipe and reported `Program Succeeded` /
      `Operation Succeeded`, after which the board was read back live on COM63. The run also
      hit the known intermittent nEDBG `Error code -121` USB lockup and recovered from it by
      itself — unrelated to this fix, see the Phase 13 nEDBG notes.

- [ ] *Offered, not built (awaiting the user's yes):* a short `TESTING.md` in the repo
      carrying the procedure above, so co-workers can be sent a link instead of a relay.

**`-Wl,--gc-sections` — added in v1.0.1, bench check PASSED Sep 22 2026.** `compiler.ld.flags`
passed `-ffunction-sections -fdata-sections` at compile time but never `-Wl,--gc-sections`
at link, unlike `_build/allboards.sh:60` — so the two split flags created sections that
nothing ever collected. The sequencing rule that gated the change ("do not add it until the
hardware upload test passes, or a failure won't be attributable") was satisfied once upload
was verified on silicon, giving a known-good reference point.

**It is worth far more than the ~2.4 KB previously estimated — about 9 KB on Blink:**

| Board | 1.0.0 | 1.0.1 | saved |
|---|---|---|---|
| `dspic33ck32mp102` | 11904 B | 2564 B | 9340 B |
| `dspic33ck256mp508` | 13992 B | 3928 B | 10064 B |
| `dspic33ck256mc002` | 11916 B | 2564 B | 9352 B |
| `dspic33ck256mc005` | 12876 B | 3196 B | 9680 B |

The size of the drop is not a red flag, it is the expected consequence of
`recipe.c.combine.pattern` already using `-Wl,--whole-archive`: the entire core is pulled
into the link and collection then discards everything Blink never calls. Nothing about
`--whole-archive` changed.

**The bench check, because a 9 KB drop is exactly when you stop trusting the analysis.**
The risk was the Timer1 `millis()` ISR being collected, which would deadlock `delay()`.
PLAN.md's existing analysis (see "the ISRs are reachable from the interrupt vector table"
below) says it survives as a GC root, and `allboards.sh` has always linked this way — but
that is why it was *expected* to work, not why it is *known* to. On an EV08P02A, flashed
from the installed 1.0.1 platform:

- `NanoSerialHello` boots and its `setup()` banner prints (captured by opening the port
  first, then power-cycling the target only: `pymcuprog setsupplyvoltage -l 0` then `-l 3.3`)
- `tick=0  uptime_ms=111`, then ticks advance at **1001–1002 ms** per 1000 ms `delay()`

So the T1 ISR fires, `millis()` advances and `delay()` does not deadlock. The flag is safe.

Guard against regression: both `_build/install_check.sh` and `_build/live_check.sh` now
*assert* the four sizes rather than printing them, so a recipe that silently stops
collecting fails the gate instead of passing quietly.

**Committed September 18, 2026** as five commits on `phase10-platform-cleanup`, working
tree clean: `6486b91` the resolver layer, `435f0b2` the index + release builder,
`bc51179` the developer installer, `059921e` the docs, `6abb2ff` this PLAN.md section.
**Deliberately not pushed** — the release is on hold until the hardware test, so
`origin/main` is still at `0a24860` and has none of Phase 10, 13 or 14.

### Release v1.0.1 — September 22, 2026

Two commits on `main`, pushed, tagged `v1.0.1`:
`496d238` the two fixes, `159f2d1` the version bump + index + installer path.

| | |
|---|---|
| `dspic33ck-arduino-core-1.0.1.zip` | 94717 B · `48d0806f9e9a693811997a806c17efb419e0db25db8225f0449adcbbaceb75ce` (re-published — see below; was 93094 B · `8d4db23a…`) |
| `dsPIC33CK-MP_DFP-1.16.521-pruned.1.zip` | 810615 B · `6cee9c30b3027b2dd14ccd64483440a5432613e3945a7d4a820795cfd0476899` |
| `dsPIC33CK-MC_DFP-1.11.412-pruned.1.zip` | 652421 B · `5533d058359bd7482401c8c2011a5dbd05fd4396271cad93bd4c8cc54a076651` |

**Both DFP packs are byte-identical to v1.0.0** — same sizes, same SHA-256s. That is
`make-release.sh`'s deterministic zipping (sorted entries, fixed 1980-01-01 timestamps,
forward slashes) working as designed, and it is why upgrading re-downloads only the 93 KB
core archive. Their **URLs** still had to move to the new tag, because `make-release.sh`
asserts every archive URL ends with `/releases/download/v<VERSION>/<archiveFileName>`, so
the unchanged bytes are attached to both releases.

Gates, all green: `allboards.sh`; `install_check.sh` (archives downloaded, checksums
verified, both tool packs unzipped, four boards at the new baselines); `live_check.sh`
against the real `/releases/latest/download/...` URL; and the **1.0.0 → 1.0.1 upgrade**
against the two live releases — the DFP packs were reused with no second download (only
`dspic33ck-arduino-core-1.0.1.zip` came down), 1.0.0's versioned directory was removed,
and no `platform.local.txt` appeared.

**Both gate scripts now derive the version from `platform.txt`** instead of pinning it.
The pinned `1.0.0` in `install_check.sh` reported a phantom `FAIL no platform.txt` the
moment the version was bumped, which is a gate lying about the thing it exists to check.

`_build/publish_release.py` derives its `TAG` and core-archive name the same way, for the
same reason: a stale constant there would attach the right bytes under a tag the index
does not point at.

**Known, pre-existing, not fixed here:** 17 of the 59 files in the shipped platform tree
are LF on disk where a fresh clone would check them out as CRLF (no `.gitattributes`,
`core.autocrlf=true`). `make-release.sh` zips the working tree, so **the archive is not
byte-reproducible from a clean clone** — a release cut elsewhere would produce a different
core checksum. Harmless to users (both endings parse), and v1.0.0 shipped the same way.
Fixing it means normalising those 17 files or adding a `.gitattributes`, which changes the
archive, so it belongs in its own change rather than inside a bug-fix release. Watch for it
if releases ever move to CI. Beware `sed -i` on `platform.txt` for the same reason: it
rewrote all 255 line endings to LF as a side effect of a one-line version bump.

#### Re-published under the same tag — the examples were invisible in the IDE

`File > Examples` offered nothing but `HRPWM > BoostMPPT`. All eleven board sketches
were in the archive, all eleven compiled, and the menu was still empty, because
**Arduino assembles that menu from installed *libraries* only — a platform-level
`examples/` directory is never scanned.** They had been there from the first commit, so
v1.0.0 and v1.0.1 both shipped them unreachable.

Fix: the tree moved to `libraries/Arduino_dsPIC33CK/examples/`, with a
`library.properties` and a header that just re-includes `Arduino.h` (a 1.5-format
library needs a header to be valid; the sketches do not include it). This is the
arrangement arduino-esp32 uses for its `libraries/ESP32` examples. The `01.`–`04.`
grouping survives as submenus — the bundled TFT library's `examples/Arduino/` proves
nesting works — so the menu now reads
`File > Examples > Arduino_dsPIC33CK > 04.CuriosityNano > NanoButtonLED`.

**Why no gate caught it:** every gate compiles sketches by absolute path, which works
whether or not the IDE can find them. `install_check.sh` now asks `arduino-cli` for the
menu it would hand the IDE (`lib examples --fqbn … --format json`) and requires every
shipped `.ino` to come back under `container_platform microchip:dspic33ck@<version>`.
12/12 offered — the eleven board sketches plus `HRPWM`'s `BoostMPPT`, which was always
visible and is now covered too. A gate that proves code compiles cannot prove it is
reachable.

**`make-release.sh` also stopped shipping untracked files.** It stages with `cp -r`, so
anything gitignored rode along: `tools/MPLABXLog.xml`, a 103-byte skeleton `ipecmd` drops
beside itself, was in both the 1.0.0 and 1.0.1 archives. Rather than blacklist strays one
at a time, the stage must now equal `git ls-files` exactly — an untracked file there is
either junk or something the author forgot to commit, and both should stop a release.
Uncommitted *modifications* are still allowed, which is what makes a release testable
before the commit that carries it.

Published **over the existing `v1.0.1` release rather than as 1.0.2**, per the request to
keep the version and overwrite it. Two consequences worth remembering:

- **Boards Manager keys on the version string**, so it will not re-fetch an archive whose
  version it already has. Anyone already on 1.0.1 must delete
  `%LOCALAPPDATA%\Arduino15\packages\microchip\hardware\dspic33ck\1.0.1\` and
  reinstall. A version bump is the mechanism that exists for this; overwriting is a
  deliberate exception, not a pattern to repeat once other people are installing.
- **The `v1.0.1` tag still points at `ddd6035`**, which does not contain this content.
  Moving it means force-pushing a tag on a public repo; leaving it means the tag names the
  release's provenance loosely rather than exactly.
- **`/releases/latest/download/…` served the *old* bytes for a few minutes** after the
  assets were replaced. The first `live_check.sh` run right after publishing passed — it
  installed 1.0.1, found the four boards, and hit every size baseline — while quietly
  installing the **previous** archive: 93094 B, platform-root `examples/`, no
  `Arduino_dsPIC33CK` library. The checksum did not catch it because the index it had
  fetched was stale in the same way, so stale index and stale archive agreed. A second run
  a few minutes later got 94717 B and the full menu. **A version bump cannot do this** —
  its URLs are new, so there is nothing cached to serve. Two lessons: never trust a
  post-publish gate that ran immediately after an in-place asset replacement, and prefer a
  bump whenever anyone else might be installing.

Two things needed fixing in `_build/publish_release.py` (which is gitignored, so this is
its only record). It skipped re-uploading an asset whose **size** matched — and
`package_microchip_dspic33ck_index.json` is exactly 2850 B carrying either checksum, since
SHA-256 hex is fixed-width and both sizes are five digits, so the stale index would have
stayed up and every install would have failed verification. It now compares content. It
also only wrote release notes when *creating* a release, leaving notes that described the
superseded assets; it now `PATCH`es them, which matters here because the notes are the only
place a user is told to delete the installed directory and reinstall.

### Release v1.0.2 — September 22, 2026 — PUBLISHED

Content release: **15 new `04.CuriosityNano` example sketches** (11 → 21 there, 27 shipped
in total), a **generated pin-map diagram** on the GitHub page, and the `-Wpedantic`
cleanup in `SPI.c` / `Wire.c`. No core API changed, so nothing here can regress a sketch
that worked on 1.0.1.

Archives built locally by `make-release.sh`; the index carries their real checksums:

| | |
|---|---|
| `dspic33ck-arduino-core-1.0.2.zip` | 173203 B · `218eab3e1d07a2bbfc20016c60af1a0ea1a1eb4c2bc5023cdff55d72fabe7189` (94717 B in 1.0.1 — the examples nearly doubled it) |
| `dsPIC33CK-MP_DFP-1.16.521-pruned.1.zip` | 810615 B · `6cee9c30…` — **byte-identical to v1.0.0 and v1.0.1** |
| `dsPIC33CK-MC_DFP-1.11.412-pruned.1.zip` | 652421 B · `5533d058…` — **byte-identical to v1.0.0 and v1.0.1** |

Both DFP URLs were moved to the `v1.0.2` tag, as in 1.0.1 and for the same reason
(`make-release.sh` asserts every URL ends with `/releases/download/v<VERSION>/…`). The
1.4 MB of duplicated bytes is deliberate: **this project has already re-published a
release in place once**, so a fresh 1.0.2 install must not depend on v1.0.1's assets
still being what they were. Upgraders re-download nothing — arduino-cli keys tools on
name+version, both unchanged.

**The version lives in six places** and `make-release.sh` refuses to build if the index
disagrees with `platform.txt`, which is what caught the two DFP URLs. The set:
`platform.txt:33`, the index's `version` / `url` / `archiveFileName`, both DFP `url`s,
`install_arduino_ide.bat:40`, and the four `library.properties`. The libraries were still
on `1.0.0` two releases after the platform left it; they now track the platform version,
which is what arduino-esp32 does with its bundled libraries.

Gates, all green: `allboards.sh` (4 devices); `examples_all.sh` (9); `examples_mc005.sh`
(21); `examples_new_mc005.sh` **21 × 2 clock options = 42 builds** through the real
arduino-cli path at `--warnings all`, zero warnings, and the two independent gates agree
byte-for-byte on all 21 hex sizes; `package_check.sh` (4 boards); and `install_check.sh`,
which served the archives over local HTTP, ran a real `core install`, verified the
checksums, placed both tool packs, found **no `platform.local.txt`**, compiled all four
boards at baseline, and confirmed **all 27 shipped examples are offered** by
`File > Examples`.

**`package_check.sh`'s baselines were two releases stale and the gate was reporting false
drift.** It still held the 1.0.0 flash column (11904 / 13992 / 11916 / 12876) that
`--gc-sections` retired in 1.0.1; the table in this file at "Release v1.0.1" had the
current numbers all along. Updated to the observed 2564 / 3928 / 2564 / 3196. Worth noting
*why* it was safe to update rather than investigate: the comment above the table explained
the baselines were deliberately larger than the core-source gates' because `platform.txt`
lacked `-Wl,--gc-sections` — 1.0.1 added that flag, which closed the divergence, and
`examples_all.sh` and `package_check.sh` now agree exactly (Blink/32MP102 = 15353 B hex on
both). That agreement is the invariant the gate exists to protect, and it holds. **A gate
whose baseline outlives the change it was measuring reports drift that is not there**,
which is how a real regression gets waved through next time.

**`upgrade_check.sh` was stale and is now version-agnostic.** It selected the index entry
with `version == "1.0.0"` to synthesise an upgrade from, and the index no longer carried
one, so it died on an `IndexError` that looked nothing like *"your gate is stale"*. It now
**derives** `BASE` from the built index (highest version) and synthesises `NEXT` as
`BASE` with the patch incremented, so it cannot rot at 1.0.3. Every hardcoded version in
the shell half went with it. **A gate that names a specific version outlives its usefulness
by exactly one release** — that is now two gates in this release alone.

De-staling it immediately exposed a second bug the old pin had been hiding: the synthesiser
asserted `(?m)^version=<base>$` against `platform.txt` **inside the archive**, which ships
CRLF, so `$` sits before the `\n` and cannot match after `version=1.0.2\r`. The pattern now
captures `(\r?)` and puts it back — normalising that one line would make the synthetic
archive differ from the released one in a way unrelated to what the gate measures — and the
assertion is on `re.subn`'s count rather than *"is the new string present"*, which would
also pass if the string happened to occur elsewhere. **The assertion is why this was a
two-minute fix and not a mystery**: without it the gate would have served an archive whose
`platform.txt` still said `1.0.2` under a directory named `1.0.3`, and the failure would
have surfaced three steps later as an incomprehensible version mismatch.

#### Published — September 22, 2026

Commit `97c3718` pushed to `main` **before** creating the release, which matters: the
GitHub API creates the tag at the default branch's HEAD, so publishing first would have
tagged `3d67430` and shipped assets built from a tree the tag did not describe. Verified
after the fact — `git/ref/tags/v1.0.2` → `97c3718`, and `/releases/latest` → `v1.0.2`,
`draft=false`, `prerelease=false`.

`publish_release.py` needed its `BODY` and release title rewritten: both still described
1.0.1's bug fixes (*"Bug-fix release"*, the `--gc-sections` savings table, the re-publish
notice telling users to delete their installed directory). **The script reads `VERSION`
from `platform.txt`, so the tag and asset names were right automatically while the notes
were wrong** — exactly the kind of half-correct release that reads as deliberate. The notes
now describe the 15 sketches, the pin map, where the examples live in the menu, and are
explicit that Boards Manager *will* offer this version unprompted, unlike 1.0.1.

Two gates that could not run before publication, both green:

- **`live_check.sh`** — a fresh Arduino data directory, the real
  `/releases/latest/download/…` URL, `core install` over the wire: index resolved at
  1.0.2, both tool packs downloaded and placed, no `platform.local.txt`, all four boards
  at baseline (2564 / 3928 / 2564 / 3196).
- **`upgrade_check.sh`** — 1.0.2 → synthetic 1.0.3: DFP archive GETs stayed at **1 each**
  across the upgrade with the download cache cleared, both packs still on disk with
  `xc16/` afterwards, the old version's directory removed, all four boards recompiled at
  baseline. This is what makes the release notes' *"click Update, ~173 KB"* claim true
  rather than assumed.

The example menu was re-verified **from the published bytes**, not the working tree: 27
`.ino` in the downloaded archive, 27 offered, split 26 `Arduino_dsPIC33CK` + 1 `HRPWM`.
`live_check.sh` does not assert this itself, which is worth fixing — the invisible-examples
bug shipped **twice** in 1.0.1, and the gate that catches it (`install_check.sh`) runs
against locally served archives, so nothing in the published path would catch a regression.

Left alone deliberately: the **`v1.0.1` tag still points at `ddd6035`**, not at the tree
that was re-published in place under it. Moving a published tag is worse than leaving it
wrong, and 1.0.2 supersedes it.

---

### Release v1.0.3 — September 23, 2026 — PUBLISHED

**The serial bootloader release.** Phase 15 in full (below), plus
`docs/part6_serial_bootloader.html`. This is the first release in which
**MPLAB X stops being a day-to-day prerequisite** on dsPIC33CK256MC005: burn the bootloader
once with any debugger, and every upload after that goes over the CDC COM port. It is also the
first release that can program a dsPIC33CK board that has **no debugger at all**.

What a 1.0.2 user gets:

- `Tools > Bootloader` menu on MC005 — `None (upload with the debugger)` stays the default.
  This release claimed **nothing changes unless the menu is touched**, and that was wrong:
  1.0.3 charged all four boards 132 bytes for a sniffer three of them cannot use. Fixed in
  v1.0.4, below — read that section before trusting any size number in this one.
- `Tools > Burn Bootloader`, working with all five programmers (this was broken for four of
  them until defect 5 — see Phase 15).
- `tools/serial_upload.py`, stdlib-only: no `pyserial`, no `intelhex`, nothing to `pip install`.
- The bootloader's own C sources shipped alongside its `.hex`, so nothing that runs on the
  board is opaque.
- The dead `tools/upload_uart.py` and the unusable `UART Bootloader` programmer entry are
  **gone** — 1.0.0–1.0.2 let a user select an upload path that could not possibly work.

Archives built locally by `make-release.sh`; the index carries their real checksums:

| | |
|---|---|
| `dspic33ck-arduino-core-1.0.3.zip` | 255032 B · `51b3e8519fd4ffbd1e0d20b24b3bdc7d3e6d8b741a26d4094e8eec68fcc8d91c` (173203 B in 1.0.2 — the bootloader sources, the `.hex`, two gld scripts and the host tool) |
| `dsPIC33CK-MP_DFP-1.16.521-pruned.1.zip` | 810615 B · `6cee9c30…` — **byte-identical to v1.0.0 through v1.0.2** |
| `dsPIC33CK-MC_DFP-1.11.412-pruned.1.zip` | 652421 B · `5533d058…` — **byte-identical to v1.0.0 through v1.0.2** |

Both DFP URLs moved to the `v1.0.3` tag again, for the reason recorded under 1.0.2: a fresh
install must not depend on an older tag's assets still being what they were, and this project
has re-published a release in place once already. Upgraders re-download nothing —
arduino-cli keys tools on name+version, both unchanged.

All six version sites were bumped together, and `make-release.sh` refuses to build when the
index disagrees with `platform.txt`: `platform.txt:33`, the index's `version` / `url` /
`archiveFileName`, both DFP `url`s, `install_arduino_ide.bat:40`, and the four
`library.properties`.

Gates before shipping: `bootloader_check.sh` **105 OK / 0 FAIL**, `allboards.sh` **4/4**,
`examples_all.sh` **11/11**, and the hardware session recorded in Phase 15 — six of seven
checks on silicon. **`install_check.sh` was not re-run for this release**, unlike 1.0.2: the
archive-serving and fresh-`core install` path is unchanged from 1.0.2 and the new files are
plain additions to the same tree, but this is a real gap and is stated rather than glossed.


### Release v1.0.4 — September 23, 2026 — PUBLISHED

**A correction to v1.0.3, published the same day.** 1.0.3 is not withdrawn — it works, the
bootloader works, and every hardware result under Phase 15 stands. What it got wrong is
narrower than that, and worth writing down exactly, because the way it was found is the
useful part.

**The defect.** The soft-entry sniffer in `HardwareSerial.c`'s RX interrupt was guarded by
`#ifndef SERIAL_NO_BOOTLOADER_ENTRY`, and **nothing anywhere defines that macro**. So the
guard was decoration: the sniffer compiled into every build of every board. That is 132 bytes
and a reset path on `33CK32MP102`, `33CK256MP508` and `33CK256MC002`, none of which have a
bootloader and none of which can ever use it — a sketch on those boards can only be
replaced with a debugger, so an RX byte sequence that resets the board is pure liability with
no upside. It also went into MC005 on `Bootloader: none`.

**What found it, and what did not.** `bootloader_check.sh` passed at 105 checks: it asserted
the macro appeared three times, which it did. `allboards.sh` passed — it invokes the
compiler directly and never reads a `platform.txt` recipe at all. `examples_all.sh` passed.
The release was built, tagged, published, and verified end-to-end from outside: the index's
checksum matched the downloaded archive exactly. Then `live_check.sh`, run **after**
publishing because this file admitted the install path had not been re-verified, installed
1.0.3 from the real Boards Manager URL and failed all four boards at once:

```
  FAIL dspic33ck32mp102       2696 B, expected 2564
  FAIL dspic33ck256mp508      4060 B, expected 3928
  FAIL dspic33ck256mc002      2696 B, expected 2564
  FAIL dspic33ck256mc005      3328 B, expected 3196
```

Uniformly `+132`. Four boards moving by the same amount is not four bugs, it is one thing
added to the core — and 132 bytes was already a measured number in this file, the cost of
the sniffer. **The only gate that could see it was the only gate that runs after publishing.**

**The fix.** The sniffer is now opt-in on `-DSERIAL_BOOTLOADER_ENTRY`, threaded through a new
standard Arduino hook:

- `platform.txt` declares `build.extra_flags=` (empty) and passes `{build.extra_flags}` in
  `recipe.c.o.pattern`, `recipe.cpp.o.pattern` and `recipe.S.o.pattern`.
- `boards.txt` sets it on **one** key:
  `dspic33ck256mc005.menu.bootloader.serial.build.extra_flags=-DSERIAL_BOOTLOADER_ENTRY`.
- `HardwareSerial.c` guards the helper and the ISR call with `#ifdef`, not `#ifndef`.

`compiler.c.extra_flags` could **not** be reused for this, and the reason is a trap worth
recording: it already carries the mandatory `-x c++` set, so a per-menu override of it would
silently drop that flag and the whole core would compile as C.

**A second, smaller finding, and the claim it retires.** With the sniffer removed from the
default path the four boards did not return to 2564 / 3928 / 2564 / 3196 — they came out
**8 bytes below** it. Attributed rather than assumed: dropping 1.0.2's `HardwareSerial.c`
into the 1.0.4 tree returns MC005 to exactly 3196, so nothing else in the release touches the
default path. The cause is the RX interrupt now reading `U1RXREG` **once into a local**
instead of once per branch, which it has to do — the sniffer must see bytes the ring buffer
drops, since a sketch that has stopped calling `read()` is exactly the one you need to
replace.

That is smaller and better code, so it stays, and **the claim goes instead**. The baselines
are now **2556 / 3920 / 2556 / 3188**, updated in `install_check.sh`, `live_check.sh`,
`package_check.sh` and `menu_size_check.sh`. `Bootloader: none` in 1.0.4 is **8 bytes
smaller** than 1.0.2, not identical to it. "Bit-for-bit unchanged" was a proxy for "nothing
broke"; the proxy failed in both directions in a single release, which is why it has been
replaced with numbers a gate can check.

**The new gate: `_build/menu_size_check.sh`.** The hole was structural, not an oversight —
no gate compiled a *menu option* through `arduino-cli` against the *working tree*.
`allboards.sh` never sees a recipe; `install_check.sh` and `live_check.sh` build the default
option only; and `live_check.sh` needs a published release to exist. `menu_size_check.sh`
stages the working tree as a `<version>-dev` install and asserts two things that pull against
each other:

1. `Bootloader: none` on all four boards sits **exactly** on the baselines. Anything added to
   the core that the user did not opt into shows up here as a delta.
2. `Bootloader: Serial` on MC005 is **larger** than its own baseline. This is the one with
   teeth: it is the only automated proof that the `build.extra_flags` chain actually reaches
   the compiler. A sniffer that is opt-in and never opted in does not exist, and soft entry
   would then fail on hardware with **no diagnosis at all** — the upload just reports that
   no bootloader answered. Asserted as a relation, not a constant, so it cannot fail for
   reasons that are not defects.

Measured: `+1740` = 1608 (the 200-GOTO trampoline) + 132 (the sniffer). Both halves of the
matrix are now covered before a tag exists.

`bootloader_check.sh` also grew the assertion that failed to catch this. The old check counted
occurrences of a macro name; the five that replace it assert the whole chain — `#ifdef`
twice in the core, the inverted 1.0.3 macro **absent** so the polarity cannot be
half-changed, `build.extra_flags` declared, all three recipes passing it, the one `boards.txt`
key setting it, and **no** `Bootloader: none` option setting it. Gate is now
**112 OK / 0 FAIL**.

Gates for 1.0.4: `bootloader_check.sh` **112 / 0**, `allboards.sh` **4/4**,
`examples_all.sh` **11/11**, `menu_size_check.sh` **PASS**, and `live_check.sh` against the
published v1.0.4 — the gate that is only meaningful after publishing, run this time as the
last step rather than as an afterthought.

**Published and measured, not asserted.** Commit `d8d45e7`, tag `v1.0.4`, core archive
**256003 B** sha256 `257ab974a5e9d8cccaf1b2c7e3354143692cf807fa98c87f26edd4e3c41629b4`,
reproducible across two independent `make-release.sh` runs. Both DFP archives went up
byte-identical to 1.0.0 through 1.0.3, so the upgrade re-downloads only the platform.
`/releases/latest/download/` resolves to `v1.0.4`. Then, against **what actually installs
from that URL** rather than against the tree:

| | |
|---|---|
| `live_check.sh` | **PASS** — 2556 / 3920 / 2556 / 3188, all four on the new baselines |
| `bootloader=serial` on MC005 | **4928 B**, `+1740` over its own 3188 baseline |
| the size bar on that option | `Maximum is 246784 bytes` |

The second row is the one that could not be checked before the tag existed, and it is the
whole point: the `build.extra_flags` → `-DSERIAL_BOOTLOADER_ENTRY` chain reaches the
compiler in the **published** artifact, not merely in the working tree that built it.

The v1.0.3 release notes on GitHub now carry a superseded banner, and the two false
statements in them — the bit-for-bit claim and the cost table's understated per-sketch
figure — are struck through and corrected in place rather than quietly edited away.
`_build/fix_1_0_3_notes.py` did it and refuses to stack a second banner on a re-run.

**The lesson, stated plainly so the next release inherits it:** a claim about the shipped
artifact that no gate can check is not a fact, it is an intention. Three separate places
(commit `7070e4c`, the v1.0.3 release notes, and this file) asserted a byte-identity that
nothing had measured, and all three were wrong.


### Release v1.0.5 — September 24, 2026 — PUBLISHED

**A documentation release, and the first one whose documentation is compiled.** The
platform **code is unchanged from v1.0.4** — not as a claim, as a measurement: flash and RAM
land **exactly** on the 2556 / 3920 / 2556 / 3188 baselines in five independent gates
(`allboards`, `menu_size_check`, `package_check`, `install_check`, `upgrade_check`), and
MC005 with `Bootloader: Serial` is still 4928 B, `+1740`, with the size bar still at 246784.
What changed is the docs, the version strings, four comments, and two new gates.

**Two defects in the shipped user guide, both found by compiling it.** v1.0.4 exists because
a claim nothing could check turned out false; this release found the same failure mode in the
one place no gate had ever looked.

*Defect 1 — 133 calls to functions that do not exist.* The guide taught
`Serial_begin(115200)`, `Serial_println("x")`, `Serial_print_int(v, DEC)` and four more
`Serial_*` names. **None of them exist** anywhere in `cores/`, `variants/` or `libraries/`.
Compiled, not grepped: `error: 'Serial_begin' was not declared in this scope`. Counts:
`arduino_ide_setup` 7, `part1` 1, `part3` 29, `part4` 82, `part5` 14. Rewritten to the
object API the `HardwareSerial.h` header's own examples show — `Serial.begin`,
`Serial.print`, `Serial.println` with the real overload set. A *lowercase* C API
(`serial_print`, `serial_print_int`) does genuinely exist and would have compiled; it was
rejected deliberately, because an Arduino guide should teach the Arduino API. `DEC` is
dropped as the default; `HEX`/`BIN`/`OCT` are kept.

*Defect 2 — 6 raw `<` operators inside `<pre>` blocks, and this one is worse.* HTML does
not exempt `<pre>`. `if ((voltage_mv % 1000) < 100)` is parsed as a start tag that runs to
the next `>`, and the browser **swallows everything in between**. Readers were served
truncated code. In `part4` the eaten span crossed a statement boundary and left behind code
that looks plausible:

```
for (brightness = 0; brightness = 0; brightness -= 5) {     <- what the reader saw
for (brightness = 0; brightness <= 255; brightness += 5) {  <- what was written
```

That loop compiles, warns only `suggest parentheses around assignment used as truth value`,
and never executes. **12 lines of part4 were invisible** — the extracted sketch grew from
83 to 95 lines once the escaping was fixed. Distribution: part3 (1), part4 (4), part5 (1).
A bare `>` is valid HTML, which is why only `<` was damaged, and every `#include` in these
blocks uses quotes rather than brackets, so no real markup was at risk.

Defect 1 fails loudly. Defect 2 produces code that looks right, compiles, and silently does
nothing — the strictly worse of the two, and the reason this release ships a gate rather than
just a fix.

**The new gate: `_build/docs_code_check.sh`.** Extracts every `<pre>` block containing both
`void setup` and `void loop`, HTML-unescapes it (`&amp;` decoded **last**, or `&amp;lt;`
double-decodes), and compiles each one for MP102 and MC005 with `-include Arduino.h` — a doc
snippet never prints its own `#include`. No link step; these are illustrative sketches.
**14 sketches, 28 builds, 0 warnings.** Every other gate compiles files in the repo. Nothing
compiled the code in the docs, which mattered less when the docs were a web page and matters
now that they ship inside the archive.

**Docs now ship in the package.** All 8 guide pages are **copied** into
`dspic33ck-1.0.5.zip` under `docs/`, and `make-release.sh` asserts each one is tracked and
byte-identical inside the archive: `docs verified in the archive: 8 pages, byte-identical to
the repo`. Copied, not moved, on purpose — the published v1.0.3 and v1.0.4 release notes link
`/blob/main/arduino-platform/docs/part6_serial_bootloader.html`, which resolves against the
branch as it is today, so relocating would permanently 404 both.

`install_check.sh` gained the matching assertions, because make-release checks the docs
against the *repo* before upload and this is the only gate that sees a real install on a real
disk: all 8 pages present and non-empty, `docs/how-to-use/` **absent** (it is superseded and
must not ship), and every `href="*.html"` in the installed guide resolving to a file that
exists. That last one is the class of bug that let v1.0.1 ship 11 invisible sketches.
`install_arduino_ide.bat` also copies `docs\*.html` now — deliberately without `/E`, which
would recurse into `how-to-use`.

**New: Part 6 §6.9, "Using a plain USB-serial adapter instead of the debugger."** The user's
question — *after I remove the nEDBG, can a generic USB-serial adapter upload sketches?* —
answered from the sources: **yes, and nothing in the platform has to change.**
`platform.txt:229` runs `serial_upload.py` against a COM port; there is no `ipecmd`, no
`pymcuprog`, no debugger anywhere in that path. All five programmers carry
`bootloader.pattern` (platform.txt 270/284/298/312/349), so Burn Bootloader works from any of
them; `bootloader.tool=nedbg` in `boards.txt` is only the board default. The one thing a
serial adapter cannot do is the **initial** burn — there is no ICSP over UART. Hence:
**a programmer once per board, a serial adapter forever after.** Wiring: adapter TXD → RC11
(U1RX, RP59, pin 32), adapter RXD ← RC10 (U1TX, RP58, pin 31), common GND, VCC left off;
115200 8N1, no flow control. **3.3 V only** — a 5 V FTDI damages the input, and that is the
one mistake on the page that costs a chip. No reset wire is needed (soft entry is in-band
magic bytes). An SW0-equivalent button is strongly recommended: without it, a sketch that
never calls `Serial.begin()` can only be recovered with a programmer. Pins are compiled into
`bootloaders/dspic33ck256mc005/bl_config.h` and changed by rebuilding with
`_build/build_bootloader.sh`. **Stated as unbenched, and untestable on a Nano** — the nEDBG
CDC bridge already occupies RC10/RC11, so an external adapter would contend for the same two
pins. Part 7 §7.11 carries the same caveat as its fifth limitation.

**New: `docs/part7_bench_verification.html`**, sections 7.1–7.12 — the step-by-step bench
procedure for the serial bootloader, written so the one hardware check still owed can be run
by following a page instead of a chat transcript.

**`package_check.sh`'s hex column was stale and is re-baselined.** All four boards failed with
flash and RAM **byte-exact** and only the hex file size moved. Established before changing
anything, because "re-baseline the failing gate" is the wrong reflex:

- Not caused by v1.0.5 — stashing the entire platform tree back to HEAD reproduced the
  identical four failures on a pristine tree.
- Not the DFP version — `examples_all.sh` uses 1.10.386 from `~/.mchp_packs`,
  `package_check.sh` uses the pruned 1.11.412 in `_build/dfp`, and **both produce 17189**.
- The cause is in this file: PLAN.md records flash 2564 → 2556 **in v1.0.4**, and v1.0.4 was
  released without re-running this gate. Two of its three columns were updated; one was not.
- Rebuilding Blink from the v1.0.3 tree in a `git worktree` gave **15738**, not 15353 — so
  15353 is reproducible from neither tree. It is not arithmetic anyone can re-derive; it is
  stale.

Baselines are now 15337 / 19445 / 15405 / 17189, and the two gates agree on 15337 for
Blink/32MP102. **If this column drifts again while flash and RAM hold, suspect the gate
before the platform — but check that `examples_all.sh` moved with it. The two agreeing is
the signal; either alone is not.** `_build/` is gitignored, so this reasoning lives in the
gate's own comment block, where it has no git history to fall back on.

**The PLAN.md entry that was wrong about all of this.** Phase 12 used to read: *"those calls
still compile — the suffixed methods were kept deliberately — so this is not a broken-docs
bug."* That was a guess from grepping for `print_int`, and it was false. It is now an `[x]`
that owns the error explicitly, plus a narrower `[ ]` for the coverage debt that remains
(MC005 barely mentioned, part5's card is still an MP102 card, no `tone()` or
`attachInterrupt()`). The v1.0.5 record asked for `docs/how-to-use/` — six superseded files
carrying **19 more `Serial_*` calls** — to be deleted or fixed rather than left ambiguous.
**Deleted September 24, 2026**, along with the gitignored `docs/how-to-use.zip` beside it.
Nothing shipped it, nothing linked to it, and no published release note mentioned it, so no
version bump was needed; the files remain in git history if the old wording is ever wanted.

Gates for 1.0.5, all green: `allboards` 4/4, `examples_all` 11/11 with 0 warnings,
`bootloader_check` PASS, `menu_size_check` PASS (4 baselines plus the `+1740` and 246784),
`package_check` PASS after the re-baseline, `install_check` PASS including the new docs
assertions (27 examples offered, 4 boards at baseline), `upgrade_check` PASS,
`examples_mc005` OK, `examples_new_mc005` OK (`NanoSerialHello 4532 420 21144`, which
confirms Part 7's own figures), `parallel_check` PASS, `docs_code_check` 28/28.

| | |
|---|---|
| `dspic33ck-arduino-core-1.0.5.zip` | 311937 B · `79148b335b47e42d42298f9ddfe50d667714c16a3d4d8caa98b9e0d678d5cd6d` (256003 B in 1.0.4 — the `+55934` is the docs) |
| `dsPIC33CK-MP_DFP-1.16.521-pruned.1.zip` | 810615 B · `6cee9c30…` — byte-identical since v1.0.0 |
| `dsPIC33CK-MC_DFP-1.11.412-pruned.1.zip` | 652421 B · `5533d058…` — byte-identical since v1.0.0 |

Archive root `dspic33ck-1.0.5`, 97 files. Commit `b61433b`, tag `v1.0.5`, release id
`395379753`, `/releases/latest` resolves to it.

**Then verified against what GitHub actually serves**, which is the only check that counts:

| | |
|---|---|
| `live_check.sh` | **PASS** — 2556 / 3920 / 2556 / 3188, all four exactly on the v1.0.4 baselines |
| `bootloader=serial` on MC005 | **4928 B**, `+1740`, size bar `246784` |
| the 8 guide pages in the install | present, `how-to-use` absent, every cross-link resolving |
| those pages vs the repo | **byte-identical**, and **0** `Serial_*` calls and **0** raw `<` in any `<pre>` |

The first row is the evidence for "the code is unchanged from v1.0.4" — not the intention,
the measurement, taken from the downloaded artifact rather than the tree that built it.

**Two things went wrong during the publish, both recorded rather than tidied away.**

*The tag was created on the wrong commit.* `publish_release.py` creates the release with
`tag_name`, which GitHub resolves against the **default branch as it currently stands** — and
`main` had not been pushed. I had run `git rev-parse HEAD origin/main`, seen two hashes, and
read it as agreement when it was the opposite. So `v1.0.5` was cut at `5e1f0a5`: the right
assets under a tag whose tree had none of the doc fixes. Caught within minutes by reading the
tag back, and fixed by pushing `main` and force-moving the tag to `b61433b`. Moving a
published tag is normally worse than leaving it wrong — the v1.0.1 tag is still deliberately
wrong for exactly that reason — but that judgement is about a tag people may already have
fetched, and this one was minutes old with nothing pointing at it. **The lasting fix belongs
in the script:** it should refuse to create a release when `HEAD` is not pushed, since
nothing else in the pipeline looks at the remote.

*A new gate reported OK while its checker had crashed.* The docs link check captured
python's stdout and tested whether it was empty. A traceback prints no findings, so a check
that never ran was indistinguishable from a check that passed — visible in the first
`live_check` run, where the same block printed a `FileNotFoundError` **and** `OK  every
cross-link in the installed guide resolves` two lines apart. The exit status is now tested in
both `live_check.sh` and `install_check.sh`, and the reason is in the comment. Same family as
the v1.0.3 macro that nothing defined: **a check whose pass condition is "nothing was
reported" passes when it did not run.**

For the record, the first `live_check` run also failed on a CDN connection reset
(`wsarecv: An existing connection was forcibly closed`) during the DFP download. Transient,
not a release defect — the index had already resolved `microchip:dspic33ck 1.0.5` — and the
re-run downloaded all three archives cleanly.

**For anyone mid-bench on 1.0.4: your results carry over.** The compiled image is the same
one. Upgrading gets you the guide that compiles, §6.9 and Part 7 — not a different binary.


---

## Phase 15: Serial bootloader for dsPIC33CK256MC005 (SHIPPED in v1.0.3, corrected in v1.0.4 — Sep 23, 2026; 1 check owed)

**Goal:** upload a sketch from the Arduino IDE over the CDC COM port, with no debugger and
no MPLAB X. Until now every upload on every board routed through `ipecmd`, so **MPLAB X IPE
was a hard prerequisite just to blink an LED**, and a custom dsPIC33CK board with nothing
but a UART could not be programmed from the IDE at all. On the four Curiosity boards the
debugger path is better and stays the default; the point is the boards that have no
debugger.

Scope is **MC005 only**, per the standing priority. The generator and both gld templates are
device-portable, so the other three are a mechanical follow-up — but they get no bootloader
menu option and no claim of support.

Status: everything below is written and **verified on the attached board on Sep 23, 2026** —
six of the seven checks pass, and **one remains that needs a hand on the USB cable**. Five
defects were found in the process, every one of them invisible to a gate that had 84 passing
checks at the time. See "Hardware verification" at the end of this section, which is the part
of this document worth reading if you are about to trust the offline gates.

**Committed and published as v1.0.3 on Sep 23, 2026** — see "Release v1.0.3" below. The one
owed check does not block the release: it is about surviving a power cut mid-row-write, which
is documented as a known limit rather than claimed as passing.

### The memory map, and why it is shaped like this

```
0x000000  reset vector   GOTO bootloader                  ] bootloader-owned,
0x000004  IVT, 254 slots (MC005 defines 200)              ] pages 0-2, written
0x000200  bootloader code, capped at 0x1600               ] ONCE, never erased
--------------------------------------------------------- page boundary
0x001800  app entry GOTO + 200-GOTO trampoline (0x324)    ] app region,
0x001B24  application code, contiguous, no gap            ] 0x1800-0x2B800,
0x02B700  signature row: length / CRC-32 / magic          ] erased and rewritten
0x02B800  UNUSED, 0x700 - shares an erase page with cfg   ] never erased
0x02BF00  config words FOSCSEL/FOSC/FWDT/FICD             ] never written
```

**The IVT is at a fixed 0x004–0x1FF and cannot be moved.** The reset vector, the whole IVT
*and* program 0x200–0x7FF all sit in erase page 0. So any design where the application owns
the IVT must erase the page holding the bootloader's own reset vector — a power-loss brick
window. That single constraint shapes everything else.

The answer is **vector forwarding**: each IVT slot is programmed once, at burn time, with
the *constant* address `0x1800 + 4n`, pointing at entry `n` of a `goto` trampoline the
application rebuilds on every upload. This buys three things:

- **No brick window** — nothing the bootloader writes can destroy the reset vector.
- **No source changes** — sketches and `cores/` keep writing `_T1Interrupt`,
  `_U1RXInterrupt`, `attachInterrupt()`. This is exactly what ruled out the AIVT, which
  would have required renaming every ISR in `cores/` *and* in every user sketch
  (`_Alt`-prefixed names, compiler guide §15.3).
- **Device-portable** — no config-word games, no `BSEN`/`FBSLIM`, no dual partition.
  (Dual partition is **MP508-only**: `FBOOT` appears in no other device's EDC, which is also
  why `p33CK256MP508.gld` uniquely carries two `MEMORY` blocks. It could never have been the
  uniform answer, and MC005 cannot use it at all.)

Cost: 0x324 words (1608 bytes on the size bar) of application flash, and **two instruction
cycles of extra interrupt latency** — 20 ns at FCY 100 MHz.

**Why the app stops at 0x2B800 and not at the config words.** The config words at 0x2BF00
sit *inside* the erase page starting at 0x2B800. Erasing that page would clear `FWDTEN`, and
an erased `FWDTEN` reads as watchdog-**on**, which resets the application in a loop. So
0x2B800–0x2BEFF — 0x700, 1792 bytes — is permanently unused. That is the price of never
touching the config page, and it is worth paying.

**The signature row is at 0x2B700, inside the erase range, deliberately.** `ERASE_APP`
therefore destroys it, so an interrupted upload cannot leave a stale signature describing an
image that is no longer there. `COMMIT` writes it last, after `READ_CRC` has verified every
row. Lose power mid-transfer and the signature is simply absent, the bootloader refuses to
jump, and the board waits for the host again — **recoverable over serial alone, with no
debugger**. The gate asserts the erase sweep covers the row.

### The erase-page-size ambiguity — unresolvable offline, designed around

The EDC says `erasepagesize="1024" sizeunits="words"`, which reads as **either 0x800 or
0x400 address units**. Both readings are defensible from the file alone. Two independent
sources point at 0x400 on this family: the bench-validated `lu_config.h` in the local
live-update demo, and Microchip's own EZBL demos.

Rather than guess, the design is correct under **both** readings:

- **align and reserve with 0x800** — safe under either, because 0x800 is a whole number of
  pages in both readings;
- **erase with a 0x400 step across the whole region** — correct under either, because if the
  true page is 0x800 then every second erase is a redundant re-erase of a page already
  erased, and that is harmless **because all erases precede all programming**.

84 pages at 0x800, i.e. 168 steps at 0x400. The gate asserts the step divides the
reservation granularity. **If the true page size is ever established, nothing needs to
change** — only the step count would be halved, as an optimisation.

Programming uses **double-word writes, not row writes**: the row-write op code is not
sourceable offline and guessing an NVM op code is not a risk worth taking for a speed
improvement on a one-off operation. `WRITE_ROW` survives as a 128-word *wire* chunk only.

### Two generated linker scripts, one generator

`tools/gld/gen_bootloader_gld.py` reads the DFP's stock `p33CK256MC005.gld` and emits both
scripts. It extracts the **ordered vector symbol list** from the stock gld's `.ivt` block —
which is dead code under XC-DSC v4.00 (`#if __XC16_VERSION < 1026`; the modern linker builds
the table itself) but is still the authoritative, device-correct, correctly *ordered* list of
names. Hand-transcribing that list is precisely how this kind of file goes silently wrong.

**The count is 200, not 254.** The plan said 254 from the stock gld's slot count; MC005
actually *defines* 200. The trampoline is 201 entries: one entry point plus 200 vectors.

Both scripts are built with `-Wl,--no-ivt` (documented by `xc-dsc-ld --help`, alongside
`--ivt ADDR`, `--civt`, `--boot LIST`, `--partition`).

**Four linker traps worth carrying forward** — every one of these cost real time:

1. **Orphan sections.** With `-ffunction-sections`, a stock `.text` output section that
   matches only `.init/.user_init/.handle/.isr*/.lib*` leaves ordinary code as *orphans*,
   and `ld` drops orphans into the first region with room. They silently filled the reserved
   signature gap. Match `.text*` explicitly.
2. **A symbol assigned *inside* an output section is relative to that section's base**, not
   absolute. This biased every fallback `GOTO` by 0x1800.
3. **VMA is not where a section lands.** PSV `.const` has VMA 0x00A3BC and LMA 0x0023BC.
4. **Data and program addresses are the same numbers**, so a section filter must go by
   `CONTENTS+ALLOC+LOAD` and not-`NEVER_LOAD`, never by address range.

### Protocol

Request `SOH(0x01) CMD LEN16 payload… CRC16`, response `ACK(0x06)|NAK(0x15) payload… CRC16`.
CRC-16/CCITT-FALSE per frame, CRC-32 for the image. Three retries, 2 s timeout. **A NAK ends
the exchange rather than triggering a retry** — a NAK is the board's considered answer, not a
lost byte.

| CMD | | |
|---|---|---|
| `0x10` | `SYNC` | returns `"33CK"`, protocol version, DEVID, page/row size, app base and length — **the host never assumes geometry, it asks, and refuses outright on a mismatch** |
| `0x20` | `ERASE_APP` | 0x1800 upward, 0x400 step |
| `0x30` | `WRITE_ROW` | 3-byte address + up to 128 words; refuses anything below 0x1800 or at/above the signature row |
| `0x40` | `READ_CRC` | CRC-32 over a range — verify without reading flash back over the wire |
| `0x50` | `COMMIT` | write length + CRC-32 + magic into the signature row |
| `0x60` | `JUMP` | validate the signature, then jump |

**The bootloader needs no PLL.** The fractional baud generator (`BCLKMOD=1`, `BRG=35`)
reaches 115200 at **−0.7 %** from the bare 8 MHz FRC (FCY 4 MHz). The classic prescaled
divisor would have been 8.5 % off and forced PLL init into the bootloader. Running on the
reset-default clock keeps it small and removes a whole failure mode. 115200 is the *only*
rate the bootloader speaks, which is why the upload tool has no `--baud`.

### Entry — three ways in, and the one that was redesigned

1. **Bootloader-first, ~300 ms window at every reset.** The only path that works when no
   valid application exists.
2. **Soft entry from a running sketch**, so an ordinary upload needs no button press.
3. **Fallback: hold SW0 through a power cycle.** Documented because the Nano has **no reset
   button** and the only power cycle available is unplugging USB.

**SW0 is RD13, not RB5.** The plan said RB5; RB5 is PGD3. Corrected against the board facts.

**No host-triggered reset exists.** User guide Table 4-2: `DBG3` → `MCLR`, driven by the
debugger only. No DTR reset, no 1200-baud touch. Entry can never depend on the host resetting
the target — which is the whole reason soft entry has to exist.

**The `persistent` shared-RAM flag from the plan was dropped.** Two independently linked
images cannot agree on the address of a `persistent` variable without reserving the same RAM
in both linker scripts, and that reservation would have to be maintained in two places
forever. Instead the sketch's RX interrupt simply executes `reset`, and the host floods SYNC
frames into the reset window until one is answered. Equivalent effect, one fewer cross-image
invariant.

**Soft entry does not require the sketch to use 115200** — an addition beyond the plan, and a
real bug avoided. The magic has to be understood by the **running sketch**, which is at
whatever rate its own `Serial.begin()` chose. So the host sweeps
`115200, 9600, 57600, 38400, 19200, 250000`, then returns to 115200 to talk to the
bootloader.

The magic is **ten bytes: eight arbitrary plus the CRC-16 of those eight.** The CRC adds
nothing an attacker would care about, but it does mean a garbled or truncated prefix cannot
reset a running sketch — which is the accident that actually happens. The RX sniffer
re-tests a mismatched byte against position 0, so a repeated prefix can resynchronise.

**Measured cost of the sniffer: 33 instruction words, 132 bytes** — `NanoBlink` went 3196 →
3328 bytes and `appcore.hex` 3881 → 3914 words, two independent measurements that agree. Per
received byte it is one compare and one branch. It can be compiled out with
`SERIAL_NO_BOOTLOADER_ENTRY`, at the cost of leaving SW0-through-a-power-cycle as the only
way in.

Two deliberate orderings inside the ISR, both worth keeping: the byte reaches the ring buffer
**before** the sniffer sees it, so a sketch that has stopped calling `read()` — exactly the
sketch you most need to replace — can still be reset; and `U1RXREG` is read **once**, which
the gate asserts, because reading it twice would drop every byte the sniffer inspected.

### Board menu, not a second board entry

`menu.bootloader` on MC005 only: `none` (default, unchanged) and `serial`. The `serial`
option overrides `build.ldscript`, `build.ldscript.dir`, `compiler.ld.extra_flags`,
`upload.tool`, `upload.protocol` and `upload.maximum_size`.

**`compiler.ld.flags` used to ignore `build.ldscript` entirely** — it hardcoded
`-T {build.dfp.path}/support/dsPIC33C/gld/p{build.mcu}.gld`, which means the four
`*.build.ldscript=` keys in `boards.txt` had been **decorative since the day they were
written**. They now mean something. The default `build.ldscript.dir` is set **per board in
`boards.txt`**, not as a `platform.txt` default: defining `build.*` defaults in
`platform.txt` has ambiguous merge order against `boards.txt`, and this way the
`Bootloader: none` path links against exactly the linker script, at exactly the path, that
it always did. (That is a statement about the linker invocation only - v1.0.4's RX
interrupt does change the default path's *size*, by -8 bytes. See the v1.0.4 section.)

**Four things must change together** for a fifth board to get a bootloader: the generated
gld pair, the committed HEX, the `bootloader.file` key, and the `menu.bootloader.serial.*`
block. The comment in `boards.txt` names all four.

**`upload.maximum_size = 246784` is measured, not derived.** The size bar's unit is **4 bytes
per instruction word** — ELF PROGBITS sizes are 2× address units, which
`xc-dsc-size-wrapper.bat` totals. Proved by `.trampoline` measuring 0x648 for 0x324 address
units, and confirmed end to end by a real `arduino-cli compile`. 246784 = 262144 − 15360,
where 15360 = (0x1600 + 0x800) × 2.

**A pre-existing understatement, left alone deliberately.** The existing `262144` is too
*small* for a 256 KB dsPIC in this accounting — the real program region is nearer 358912. It
is not corrected here, so that selecting the bootloader can only ever *lower* the limit,
never raise it. Filed as debt, not fixed inside a bootloader change. Note also that the
linker script is the real backstop: `program` is `ORIGIN 0x1800, LENGTH 0x29F00`, so an
oversize sketch is a **link error**, which is a stronger guarantee than a size-bar check.

**`xc-dsc-size-wrapper.bat` had to be taught about `.trampoline`.** Its allowlist was
`.text* .dinit .reset .const*`, so 1608 bytes of real, sketch-unusable flash was invisible
and `bootloader=serial` reported *less* than `bootloader=none`. Fixed; `.config_*` stays
excluded on purpose, being outside the program region.

A **Tools > Burn Bootloader** recipe on the `nedbg` tool flashes the committed HEX through
the existing `ipecmd` path. Users cannot build the bootloader themselves, so the binary
ships in the package — along with its C sources, so it is auditable.

### Host tool

**`tools/serial_upload.py`, pure standard library**, ~700 lines. COM access through Win32
`CreateFileW`/`SetCommState`/`SetCommTimeouts`/`ReadFile`/`WriteFile` via `ctypes`. The
platform is Windows-only already, so nothing is lost and the first-run experience stops
depending on `pip install pyserial`.

Reads are sliced at 40 ms rather than pushing the whole timeout into `COMMTIMEOUTS`, because
`ReadIntervalTimeout` would make a read of N bytes wait the full time for the **last** byte.

Two things this file gets right that its predecessor got wrong, spelled out in its docstring
because both are silent failures:

- **HEX encoding.** 4 bytes per 24-bit instruction word, record address = **2 ×** program
  address, byte 3 is the phantom byte and is *asserted* to be 0x00. A tool that walks the
  records bytewise — as the deleted `tools/upload_uart.py` did — silently programs a quarter
  of the image as padding.
- **The CRC-32 definition.** 3 bytes per word, little-endian, phantom byte *excluded*, gaps
  filled with 0xFFFFFF. Host and device must agree exactly or every upload fails
  verification.

It refuses a sketch linked below 0x1800 with an actionable message naming the menu option —
**verified**: a `bootloader=none` HEX is rejected, exit 1.

### Scaffolding removed

- **`tools/upload_uart.py` (227 lines) deleted.** It invented a protocol no firmware
  implemented, hardcoded 32 KB-device constants (`FLASH_START 0x1000`,
  `FLASH_PAGE_SIZE 1024`, `FLASH_ROW_SIZE 128`) wrong for MC005, and indexed Intel HEX
  bytewise, ignoring phantom-byte encoding. It was a second `pre_build.py`.
- **`programmers.txt`'s `uart_bootloader` entry** was user-selectable in 1.0.0–1.0.2 and
  could not possibly work. It now points at the real tool.
- The upload tool is named **`dspic33ck_serial`, deliberately not a reuse of
  `dspic33ck_upload`**, so an old `boards.txt` cannot reach the old behaviour.

### What the two gates assert — `_build/` is gitignored, so this is the only record

**`_build/build_bootloader.sh`** builds the firmware against the boot gld and asserts: 5
translation units link; footprint 0x000000–0x000A9E (**1359 instruction words, 0xD62 / 1713
words of room left** under the 0x1600 cap); nothing at or above 0x1800; the 4 config words
are identical to the core's; and — because the jump is inline asm and therefore not
type-checked — that the disassembly **contains `goto 0x001800` and that no GOTO reaches past
it**. Publishes `bootloader-dspic33ck256mc005-115200-v1.hex`.

**`_build/bootloader_check.sh`**, sections 1–10, `RESULT: PASS`:

- **1–8** (pre-existing): the generator's vector list against the stock gld; the boot and app
  scripts' geometry; the trampoline decoded from a real ELF — **201 GOTOs, entry 0 is
  `GOTO __reset`, every defined handler reached from its own slot, no handler reachable from
  a slot that is not its own, 193 falling back to `__DefaultInterrupt`, no GOTO outside the
  application**; both app HEXes writing only where allowed with every phantom byte 0x00; and
  that overflowing the bootloader region is a **link error, not a silent overlap**.
- **9 — the firmware's own map against the generated linker script.** `BL_APP_BASE`,
  `BL_SIG_BASE` and `BL_APP_END` are parsed out of `bl_config.h` and compared to the app
  gld's own numbers; `BL_APP_END` is on a page boundary; the erase sweep covers the signature
  row; the erase step divides the reservation granularity. This closes a drift gap that
  `bl_config.h`'s header comment *promised* and nothing enforced.
- **10 — the host tool against the firmware, offline.** Both checksums against their
  published check values (0x29B1, 0xCBF43926); **the 16-entry nibble table parsed out of
  `bl_crc.c` produces the host's CRC-32 over 6 seeded-random vectors** — a single mistyped
  entry there would produce a checksum self-consistent on the device and wrong everywhere
  else; 14 protocol constants match `bl_config.h`; every `BL_ERR_*` the firmware can send is
  named by the host; the soft-entry magic matches and its trailing CRC-16 checks out; **the
  core's third copy of the magic is byte-identical** and its array bound matches its
  initialiser, a full match executes `reset`, a mismatched byte is re-tested as a first byte,
  `SERIAL_NO_BOOTLOADER_ENTRY` guards all three sites, matched bytes still reach the ring
  buffer, and `U1RXREG` is read exactly once; and for two real HEX files,
  `parse_hex` → `build_image` → `blocks` yields only 4-aligned addresses ≥ 0x1800, none
  reaching the signature row, none over 128 words, with everything skipped at or above the
  config words.

Nothing in the three-way agreement — **linker scripts ↔ firmware ↔ host** — is held together
by a comment any more. The gate sets `sys.dont_write_bytecode = True` before importing the
host tool, because the import would otherwise drop `__pycache__/` into a git-tracked
directory and `make-release.sh` refuses untracked files.

### Verified through `arduino-cli`, on the real installed platform

Installed as `1.0.3-dev` **alongside** 1.0.2 rather than over it, so the working install
stays intact and rollback is deleting one directory.

- The `Bootloader` menu appears with `None (upload with the debugger)` as the default.
- `bootloader=serial` links with `-T <platform>/ldscripts/p33CK256MC005-app.gld` **and**
  `-Wl,--no-ivt` — the `build.ldscript.dir` indirection resolves correctly through
  `{runtime.platform.path}`.
- Size bar: `none` → 3328 bytes / max 262144 (**unchanged**); `serial` → 4928 bytes / max
  **246784**.
- **`--gc-sections` does not collect the ISRs.** This was flagged as *the most likely thing
  to be wrong*, because nothing in the application references an ISR except the trampoline's
  `DEFINED()` test, and `DEFINED()` may well not count as a reference. On the real IDE-built
  `NanoBlink`: vector 9 → `__T1Interrupt`, **vector 19 → `__U1RXInterrupt`**, 10/11/27/83 →
  the four `_CN?Interrupt`, 49 → `__CCT4Interrupt`, 193 → `__DefaultInterrupt`. **No `KEEP()`
  needed.**
- **`__DefaultInterrupt` is still synthesised under `--no-ivt`** (0x001D8E), so the core does
  not have to provide one.
- The app ELF has **no `.reset` and no `.ivt`** — it structurally cannot overwrite page 0.
- `serial_upload.py --dry-run` accepts the IDE-built HEX (1232 words = 4928/4, 10 blocks) and
  **rejects** the `bootloader=none` HEX with the menu-option hint, exit 1.
- `_build/allboards.sh` and `_build/examples_all.sh` **green on all four boards, zero
  warnings**, after the core change.

**Baseline change to record: `NanoBlink` on MC005 is now 3328, not 3196.** The other three
baselines are untouched by the sniffer only insofar as they do not use `Serial`; the
`examples_all.sh` numbers all moved and are green at the new values.

### Hardware verification — Sep 23, 2026, on the attached board (COM63)

The prediction in the previous revision of this section was **"nothing here is a code gap;
these are the checks that need the board."** That was wrong, and expensively so: the board
found **four defects**, three of them in shipped files, and every one of them was invisible
to a gate that had 84 passing checks. The gate is now at 94, and the four additions exist
because these four bugs got past it.

| # | Check | Result |
|---|---|---|
| 1 | Burn Bootloader via nEDBG | **PASS** — `program memory 0x0-0xfff` + `configuration memory` as its own region |
| 2 | Serial upload, Serial Monitor afterwards | **PASS** — 18 blocks, verified, committed, running; CDC prints and echoes |
| 3 | Soft entry from a running sketch | **PASS** — magic accepted at 115200, no debugger, no button, no unplug |
| 4 | Interrupted transfer does not brick the board | **PASS** — see below |
| 5 | `attachInterrupt()` + Serial RX + `tone()` | **PASS** — all four vectors forward: T1, U1RX, the tone timer, and CN on SW0 (`sw0_presses` increments on a real press, confirmed by the user) |
| 6 | `RCON` tells POR from software reset | **PASS** — `RCON=0x40`, i.e. `SWR` alone: `POR=0 BOR=0 WDTO=0` |
| 7 | The debugger path wipes the bootloader | **PASS** — confirmed deliberately, not inferred |

**Check 4, precisely.** A true power-loss test needs a hand on the USB cable, so what was
run is the property that makes the unplug survivable: `ERASE_APP`, three of eighteen rows
written, then stop dead with no `COMMIT`. `JUMP` was then **refused** —
*"no valid application: signature missing or CRC mismatch (error 7)"* — and `SYNC` still
answered, so the board was recoverable, and a full serial upload recovered it. The refusal
is meaningful because the `JUMP` handler and the reset path call the **same** `app_valid()`
(`bl_main.c:436` and `bl_main.c:497`), so a board that refuses here is a board that stays in
the bootloader after a power cut. The remaining gap is the physical unplug alone.

**Check 7, and the recovery message.** A debugger upload with `Bootloader: none` left the
sketch running and no bootloader answering `SYNC` at all. A serial upload attempted against
that state prints the right first step — *"use Tools > Burn Bootloader with the debugger
attached first"*. Both halves matter: the wipe is real, and the tool says what to do.

### The five defects the board found

1. **`boards.txt` sent "Bootloader: Serial" through the debugger — silent and destructive.**
   The board declares both `upload.tool` and `upload.tool.default`; arduino-cli 1.5.x prefers
   the `.default` form, and the menu overrode only the legacy key. So selecting the serial
   bootloader uploaded via `ipecmd`, which **erased the bootloader it was supposed to use**.
   The upload reported success, at the correct address, and the board could not be uploaded
   to again. Fixed by shadowing both keys; the gate now asserts that *every* board-level
   `upload.tool*` key is shadowed, not just the two that exist today.

2. **`sw0_init()` never cleared ANSEL, so the bootloader never auto-jumped.** SW0 is RD13,
   which is **ANN0** — it has an ANSEL bit and that bit resets to analog. An analog-enabled
   pin has its digital input buffer switched off and reads 0 whatever the voltage, so SW0
   read as **permanently held**, `stay` was always 1, and the bootloader waited forever
   instead of running a perfectly good sketch. **A board on a USB charger would never run.**
   `cores/arduino/system_config.c` clears all of `ANSELD` for exactly this reason; the
   bootloader runs before any of it and has to do its own. Fixed with `BL_SW0_ANSEL`, cleared
   in `sw0_init()` and restored in `sw0_release()`.

3. **`build_bootloader.sh` called `bin2hex` without `-mdfp=`, and threw away the error.**
   Without the pack, `bin2hex` does not know where this device's config words live and emits
   their records at the *program* address instead of doubling it: `FOSCSEL` landed at HEX
   `0x2BF18` rather than `0x57E30`, so `ipecmd` programmed **0x15F8C — the middle of the
   application region** — and left the real config words erased. An erased `FWDTEN` reads as
   watchdog-on, so the board reset in a loop and the bootloader never answered. The only
   visible symptom was a silent COM port. `>/dev/null 2>&1 || true` is what let it through.
   Fixed, made fatal, and the build now asserts placement **in the HEX** — the ELF is not
   what gets programmed. This is the project's known `-mdfp` trap biting a manual invocation
   again.

4. **Soft entry was a coin flip: a 500 ms knock against a 300 ms window.** `sync()` waited
   500 ms for silence, so one `SYNC` frame lost to the target still being in reset threw the
   *entire* entry window away, and the host's next attempt met the sketch that had already
   restarted. There was exactly one chance per reset. This was hidden by defect 2 — while
   SW0 read as held, the window was effectively infinite, so check 3 passed for the wrong
   reason and the real timing was never exercised. Fixed with `KNOCK_MS = 60` and a genuine
   flood; the gate now cross-checks that Python constant against the C `BL_WINDOW_MS`.

5. **`Tools > Burn Bootloader` worked with the nEDBG only — i.e. with every programmer except
   the ones the feature exists for.** Only `tools.nedbg` carried `erase.pattern` and
   `bootloader.pattern`, and arduino-cli refuses to start the operation at all if either key
   is missing, so selecting a PICkit or a SNAP failed with `recipe not found 'erase.pattern'`.
   The bootloader's entire reason to exist is a board with **no on-board debugger**, and that
   board is burned once over ICSP with a PICkit — so the one audience that needs Burn
   Bootloader was the one audience it did not serve. Found while answering "where is the
   bootloader hex for ICSP", not by any gate. Fixed by giving pickit4, pickit5, snap and pkob4
   both recipes; the gate now derives the programmer list from `programmers.txt` and asserts
   both keys for each, so adding a programmer without its recipes fails offline.

Two smaller things came out of watching real output: `--quiet`, which is what the IDE passes
when Verbose is unticked, silenced **every** line, so a normal upload printed nothing at all
and could not be told from a recipe that never ran; and the block counter used `\r`, which
the IDE console and any pipe both render as concatenation (`1/18 blocksserial_upload: 17/18
blocks`). Both replaced with whole lines, at most ten of them regardless of image size.

### An erase guard was built, then deliberately removed — do not re-add it

Fixing defect 5 opened what looked like a sharper hazard. arduino-cli runs `erase.pattern`
*before* `bootloader.pattern`, and only MC005 defines `bootloader.file`, so on the other three
boards Burn Bootloader appeared able to erase the chip and only then discover it had nothing
to program — handing the user a blank device. So `--erase` was given a fourth "this file must
exist" argument and `ipecmd-upload.bat` refused before touching the board.

Testing it on MC002 showed the premise was wrong: **arduino-cli stops at
`Property 'bootloader.tool.serial' is undefined` without running either recipe**, so nothing
can reach the chip in the first place. The guard duplicated a check that already existed
upstream, and in exchange it made Burn Bootloader conditional on a path expansion matching —
a new way to fail on a board that is in fact perfectly burnable. Removed at the user's
direction on Sep 23; all five programmers now erase and program unconditionally. The reason
is recorded in the wrapper's own header so it does not get reinvented.

### The defect the *documentation* found: `NanoBlink` locks out the next serial upload

Writing the user guide turned up a usability defect no gate and no bench check had caught,
because it only appears in a sequence of two uploads. **Soft entry asks the *running sketch*
to reset itself**, so it needs that sketch to have a live UART RX interrupt. `NanoBlink`
contains no `Serial` usage at all (`grep -c Serial` → 0), and it is the first example almost
anyone opens. Upload it over serial and it runs perfectly — and then the *next* serial upload
fails:

```
serial_upload: error: no bootloader answered on COM63.
Failed uploading: uploading error: exit status 1
```

Reproduced deliberately on Sep 23, not inferred. Every other `04.CuriosityNano` example calls
`Serial.begin()` and is unaffected, which is exactly why the whole bench session missed it.

**Recovery, verified with no hands on the board:** `Tools > Burn Bootloader` erases the sketch
and leaves the board waiting in the bootloader, and the serial upload that follows succeeds
(`the bootloader is already listening`, `1568 instruction words written and verified on
COM63`, `running the sketch`). The SW0-held-through-a-power-cycle fallback takes the same code
path as the auto-jump decision that *is* verified, but the held-button case has not itself
been reproduced, and the docs say so rather than implying it was.

**Deliberately not fixed at release time.** The real fix is an always-on RX sniffer in the
core (entry works regardless of what the sketch does with `Serial`) or a host-side
"replug catch" that waits for the port to reappear and grabs the reset window. Both need bench
verification, and neither was going to get it on release day. **The host-side replug catch is
the top candidate for the next version** — it changes nothing on the target, so it cannot
regress a board that already works. For now the lockout is documented prominently in three
places (Part 6 §6.3 and §6.6, Part 5 §5.2, both READMEs) and the recoveries are distinguished
by whether they were actually observed.

### Documentation — `part6_serial_bootloader.html`

The bootloader gets its own guide rather than a subsection, because the reader who needs it is
not the reader of Parts 1–5: they have a board with no debugger. Eight sections — what it
costs (§6.1), three ways to burn it, IDE / command line / MPLAB IPE (§6.2), everyday use
(§6.3), internals with the memory map and the vector trampoline (§6.4), recovery (§6.5),
troubleshooting (§6.6), limits (§6.7), and what porting it to your own hardware requires
(§6.8). Parts 1–5 all gained a Part 6 nav link, and Part 5 §5.2 was corrected: it claimed
Burn Bootloader needed the nEDBG, which stopped being true when defect 5 was fixed.

Two things in it were wrong when written and were caught by checking against the hardware
rather than by rereading the prose: the verbose-output sample was **paraphrased from memory
and contained lines the tool never prints**, replaced with output captured from a real upload;
and the recovery advice did not distinguish what had been observed from what was merely
designed. Both are worth remembering as the failure mode of writing docs for code you wrote
yourself.

### What is still owed — one check, and it needs a hand on the cable

**Unplug USB mid-transfer, replug, and upload again over serial alone.** Check 4's reasoning
covers the decision the board makes when it reboots — `JUMP` is refused and `SYNC` still
answers — but only a real power cut covers the NVM controller being interrupted part-way
through a row write. Everything else in this phase is confirmed on silicon.

The trampoline is now proven for **four unrelated vectors at once** — T1, U1RX, the tone
timer and change-notification on SW0 — which was the whole point of check 5: a trampoline
uniformly off by one slot would still have landed somewhere plausible for a single vector.

### Known limits, stated rather than hidden

- **The bootloader cannot update itself.** Owning the reset vector permanently is exactly
  what makes it unerasable. Re-burning needs a debugger. This is the correct trade.
- **Soft entry depends on a cooperating sketch.** A sketch that never calls `Serial.begin()`,
  or that sits with interrupts disabled, cannot be asked to reset. On a Curiosity Nano
  `Burn Bootloader` recovers it with no hands on the board; on a board with no debugger the
  only way back is SW0 held through a power cycle, which on a board with no reset button means
  a USB unplug. See "The defect the documentation found" above — this is a real usability
  problem, not a footnote, and the host-side replug catch is the fix candidate.
- **Python remains a prerequisite** for serial upload — pyserial no longer is. If that proves
  to be a real barrier, the fallback is shipping a PyInstaller binary as a tool pack through
  the index's `tools[]` array, exactly as the two DFP packs already are.
- **Not doing:** the other three devices; dual partition on MP508; the AIVT; a `.sh` port of
  the host tool.

### Worth raising: a validated protocol already exists locally

The live-update demo on the user's OneDrive (`lu_protocol.c/h`, `lu_uart.c`, `lu_image.c`,
`lu_commit.c`, `tools/lu_host.py`) is a **bench-validated dsPIC33CK UART bootloader**, and it
is where the 0x400 erase-step evidence came from. **Only public datasheet facts were taken
from it — no code was copied into this public repo**, since it is Microchip-internal.
Adopting its MCC-compatible protocol instead of this custom one is a real option and worth a
deliberate decision rather than drift.

### Where to pick up — state as of Sep 23, 2026, end of the release (v1.0.4, not v1.0.3)

**The board is running `NanoSerialHello`, uploaded over serial, with the bootloader intact.**
It was deliberately left on a sketch that *does* call `Serial.begin()` so that soft entry
works and the next upload needs no intervention — see the `NanoBlink` lockout above for why
that matters. `SYNC` on COM63 answers `{version 1, devid 0xA272, erase_step 0x400,
block_words 128, app_base 0x1800, sig_base 0x2B700, app_end 0x2B800}`.

Gates at this moment: `_build/bootloader_check.sh` **112 OK / 0 FAIL** (105 through 1.0.3; 110
before the 5 guard assertions were dropped; 84 before the bench session), `_build/allboards.sh`
**4/4**, `_build/examples_all.sh` **11/11** with no warnings, and `_build/menu_size_check.sh`
**PASS** — the gate added in 1.0.4, which is the one that would have caught the 1.0.3
defect before it was published rather than after.

**Housekeeping: no longer owed, and now automatic.** The `1.0.3-dev` hand-made install under
`%LOCALAPPDATA%\Arduino15\packages\microchip\hardware\dspic33ck\` is gone. Two installs of
the same platform differ only by a version string in `platform.txt`, which makes a stale one a
real trap — so `menu_size_check.sh` now owns that directory: it deletes any leftover
`*-dev` before staging (a leftover would outrank the staged tree on semver and silently compile
the wrong sources) and removes its own on exit, pass or fail. **`1.0.2` is still installed
alongside**, deliberately: it is the reference the 8-byte delta was attributed against.

**The test that is owed, in IDE terms.** Install **1.0.4** from the Boards Manager first, and
restart the IDE — it caches `platform.txt` at startup, and 1.0.3 is the release that charges
every board for the sniffer. Then
Tools > Board `Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)`, Port `COM63`,
**Bootloader `Serial (UART, 115200)`**, Programmer `nEDBG (Curiosity Nano On-Board)`. Open
`File > Examples > Arduino_dsPIC33CK > 04.CuriosityNANO > NanoSerialHello` and press Upload,
with
**the Serial Monitor closed** — the upload tool opens the port exclusively. Tick
`File > Preferences > "Show verbose output during: upload"` to see the block counter. Then
unplug USB part-way through the transfer, replug, and upload again: the board must still
answer `SYNC` and must refuse to `JUMP` into the half-written image.

**Use `NanoSerialHello`, not `NanoBlink`, and the reason is not cosmetic.** The interrupted
upload leaves no valid app, so the board sits in the bootloader and the retry needs no soft
entry either way. But if the retry *succeeds*, the sketch it just installed is the one that
has to accept the upload after that — and `NanoBlink` never calls
`Serial.begin()`, so it would lock the board out and turn a passing test into a recovery
job. `NanoSerialHello` exercises the same path and leaves the board uploadable.

**One decision still owed by the user**, recorded above and not blocking: whether to adopt the
bench-validated Microchip-internal protocol in place of this custom one. The version question
is settled — this shipped as **v1.0.3**, corrected by **v1.0.4** the same day. The erase-page
ambiguity no longer needs a decision
either; the board reports `erase step 0x400` itself.

---

## Pin-map audit — committed September 24, 2026, release HELD

Asked whether the pin-map diagram still agreed with v1.0.5, the answer turned out to be yes,
three independent ways: regenerating `docs/img/pinmap-dspic33ck256mc005.svg` from
`tools/pinmap/gen_pinmap.py` produces a byte-identical file, the generator's tables match
`g_pin_map` and `pins_arduino.h` on all 39 pins and 20 analog names, and v1.0.5's platform
code is unchanged from v1.0.4 so nothing could have moved. **What was wrong was everything
written around the picture.**

### The generator only *claims* to be generated from `variant.c`

Its docstring says the pin data has one source of truth in `variant.c` + `pins_arduino.h`.
That is the intent, but `LEFT`/`RIGHT` are Python literals — **nothing reads `variant.c`**. So
the diagram can be internally consistent, regenerate byte-identically, and still be wrong,
with no diff anywhere to notice. `README.md` asserted it "cannot drift from the pin table the
core actually compiles against"; it could. `tools/pinmap/check_pinmap.py` is what makes that
sentence true, and the README now says what is actually enforced instead.

### `tools/pinmap/check_pinmap.py` — five checks, one defect behind each

Tracked, unlike every other gate (those live in the gitignored `_build/`), because it is the
thing that keeps a *committed artefact* honest.

| | |
|---|---|
| **A** | every pad matches `g_pin_map` and `pins_arduino.h` — port, bit, ADC channel, `An` name, `LED_BUILTIN`, `BUTTON_BUILTIN` — and no pin in `g_pin_map` is missing from the map |
| **B** | the committed SVG is what the current generator produces, *and is pure ASCII* — the file declares no `<?xml encoding?>`, so a literal em dash renders at the consumer's mercy. One was found and removed (69623 → 69621 bytes) |
| **C** | the four pins compiled into the bootloader HEX (`bl_config.h`) carry those roles on the map, so moving the bootloader's UART makes the map stale the moment the header is saved |
| **D** | no wholly-MC005 page attaches a bare physical pin number to a port name |
| **E** | `part2`'s MC005 table matches `variant.c` on port, `An` name and ADC channel, has no missing or extra rows, and its MC005 half obeys D |

Wired into `_build/docs_code_check.sh` ahead of the compiles. **Not** as `python … | sed`: in a
pipeline the exit status is `sed`'s, which is always 0, so `set -e` would have sailed straight
past a FAIL. `PIPESTATUS[0]` is checked instead. That is the third instance in this project of
*a check whose pass condition is "nothing was reported" passing because it did not run* — after
the v1.0.3 `#ifndef` macro and the v1.0.5 docs link checker. Every check here was therefore
mutation-tested: seven mutations, seven caught, including one that renames a heading so check E
would lose its scope silently.

### The §6.9 defect, and the claim that was retracted

`part6_serial_bootloader.html` §6.9 wrote `RC11 — U1RX, RP59, pin 32`. That `32` is the
Arduino number `D32`, but in a cell that already says `RP59` it reads as a package pin. Fixed
to `Arduino D32` plus *"solder to the edge pad silkscreened `RC11`"*, with a warning box
saying so.

**Retracted: the DFP's `<edc:PinList>` is not a usable source of physical pin numbers.** The
investigation first concluded that RC10/RC11 are package pins 40/41 and that physical 31/32
are VSS/VDD — i.e. that a reader counting pins would wire TXD into VDD. That rested entirely
on the list being in physical pin order (it carries no numbers; pin *N* = the *N*th
`<edc:Pin>` block). Two devices kill the assumption:

```
DSPIC33CK32MP102   edc:desc="28-pin SSOP"   1-4: RA1 RA2 RA3 RA4   25-28: RB14 RB15 MCLR RA0
DSPIC33CK256MC002  (no desc)                1-4: RB14 RB15 MCLR RA0
```

Same 28-pin package, same cyclic order of port names, **offset by four**. At most one starts at
pin 1 and nothing inside the pack says which; `DSPIC33CK256MC005.PIC` has no `edc:desc` either,
so it is in the unlabelled group. **This repo has no verifiable source of package pin numbers,
and does not need one** — a user wires to a Curiosity Nano edge pad, which is silkscreened with
the port name, and writes code against the Arduino number. Both of those *are* verifiable here,
so check D forbids the bare number rather than correcting it.

Consequence for future work: the MP102 ASCII diagram in `part2` §2.3 **was left alone**. Its
numbering disagrees with the EDC by two and it lists `RB5` at two different pins, but only the
second of those is provable from inside the repo, so only that one was fixed (pin 15 is now
`VCAP`). *Do not "correct" package pin numbers in this tree from the EDC.*

### `part2` now documents the priority device

Up to v1.0.5, `part2_pin_mapping_hardware.html` covered only the 28-pin MP102 — so a user of
the priority device read a complete pin-mapping table whose every row named a different port
than their board had, with no warning that it was the wrong device. Now:

- **§2.1** MC005 intro, a link to the diagram, and the warning that package numbers are
  deliberately absent
- **§2.2** all 39 MC005 pins, **generated** from `variant.c` + `pins_arduino.h` by
  `_build/gen_mc005_rows.py` — transcribing 39 rows by hand is how a pin table goes wrong —
  and gated by check E. Includes the `D10`/`D11` = `PGD3`/`PGC3` danger box, the "MCPWM has no
  Arduino API on MC parts" note, and the `A19` = `D37` = channel `AN18` mismatch spelled out
- **§2.3–§2.7** the existing MP102 sections, renumbered and retitled to name their device,
  behind a lead box warning that the same Arduino number is a different port on each

Renumbering was safe because `part2` has no `id=` anchors and nothing in any doc, `README.md`
or this file cites its section numbers — checked before editing, and the reason the change is
this cheap.

### Release status

`README.md`, the SVG and the gates are repo-only and need no version bump. **`part2` and
`part6` ship inside the archive, so those two changes reach users only in a `v1.0.6`, which is
held at the user's instruction.** When it is cut, `part6`'s §6.9 correction and `part2`'s MC005
half go out together; nothing else in the archive has changed, so the four boards' 2556 / 3920 /
2556 / 3188 baselines must come back identical.

---

## TODO (Next Steps)

### Phase 10: Additional APIs — the Arduino API surface is CLOSED, two extras remain
The three functions this phase was actually about are done and committed; what is left
under this heading are two items that were only ever filed here for convenience. The
implementation write-up is under "The six missing functions" below, and the four
remaining checks are bench-only.

Under the MC005-only priority both remaining items stay live: the `round` fix is
device-independent, and the DAC turns out to be present on MC005 (one channel — see below),
so it is developable and bench-testable on the priority board. The four outstanding bench
checks are all MC005 checks, so they are the live end of this phase.

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
- [ ] **DAC output — NOT held; MC005 has one DAC channel.** Checked in
      `p33CK256MC005.h`: `DAC1CONL` and `DACCTRL1L` are present, `DAC2CONL`/`DAC3CONL` are
      not, so the priority board gives **1 channel** against the MP508's 3. That is enough
      to develop and bench-verify the whole API on MC005; guard the 2nd and 3rd channels on
      `#if defined(DAC2CONL)` the way the platform already guards PWM5 and HRPWM
      (constraint 12). Do not size the API around 3 channels.
  - Registers: DACxCONL (DACEN, DACOEN), DACxDATH (12-bit value), DACCTRL1L (DACON)
  - Also includes built-in comparator (CMPSTAT, CMPPOL, INSEL)
  - API options: `analogWrite()` on DAC pins (true analog), or dedicated `DAC.write()` library

### Phase 11: Advanced Features (Discussion)

Re-read under the MC005-only priority: **HRPWM is unreachable** (no Auxiliary PLL on Value
Line MC parts — this is silicon, not a porting gap), and the DMA and software-USB items were
both scoped against MP508 silicon and its 100 MIPS. What *is* newly interesting is the motor
control item at the bottom, because MC005 is an MC part — that is the one line of Phase 11
work the priority makes more relevant rather than less.

- [~] Higher-level peripheral libraries for users needing more than Arduino APIs
  - PWM side is **already done** — see HRPWM under "Delivered Ahead of Plan", but note it
    **cannot run on MC005 at all**, so it is effectively held with the MP508
  - Still open: an ADC equivalent (dsPIC_ADC.h) — multi-channel, triggered, DMA-fed.
    Portable in principle; would need re-scoping against MC005's ADC and 4 MHz FCY
- [ ] **DMA library — NOT held.** Checked: MC005 has the **same four channels** as MP508
      (`DMACH0`–`DMACH3` all present in `p33CK256MC005.h`), so this is fully developable and
      testable on the priority board. The "on MP508" in the line below is just where it was
      first scoped. — 4 DMA channels (DMACH0–DMACH3)
  - No standard Arduino DMA API exists; create dsPIC-specific `dsPIC_DMA.h`
  - Use cases: ADC→buffer, memory→SPI, memory→UART (zero-CPU-overhead transfers)
  - API concept: `DMA.begin(ch, src, dst, count, trigger)`
- [ ] **Software USB (V-USB style) — Virtual COM port via GPIO. ON HOLD, and largely
      pointless on the priority device:** MC005 already has a working CDC through the
      on-board nEDBG (proven on silicon in Phase 13), which was the entire motivation —
      eliminating an external USB bridge. The cycle budget below is *reachable* on MC005 via
      the `200mhz_pll` clock entry, so this is not a silicon limit; it is simply the item
      whose payoff the priority board already has for free.
  - dsPIC33CK at 200 MHz / 100 MIPS gives ~67 cycles per USB bit (6x more than AVR V-USB)
  - USB Low-Speed (1.5 Mbps): 2 GPIO pins for D+/D-, 1.5k pull-up on D-
  - Implementation: cycle-counted pic30 assembly for NRZI/bit-stuff/CRC + C for descriptors
  - CDC-ACM class = appears as virtual COM port on PC (no driver needed)
  - Eliminates need for external UART-to-USB bridge chip (MCP2221A/FT232/CP2102)
  - Effort: significant (~2-4 weeks), but hardware math is very favorable
- [ ] Code generator / MCC-like configurator for Arduino sketches
- [~] UART bootloader upload support (alternative to IPE programmer)
  - `tools/upload_uart.py` written — see "Delivered Ahead of Plan"; hardware-untested
- [ ] Motor control library (PWM, QEI) for MC devices — **inventory checked against
      `p33CK256MC005.h`, and it splits: the PWM half is available, the QEI half is not.**
      MC005 has **no `QEI1CON`/`QEI2CON` at all** (MP508 has two QEI modules), and **4 PWM
      generators** `PG1`–`PG4` against MP508's 8. So on the priority board this reduces to a
      PWM-only motor library with no hardware quadrature decode — encoder feedback would
      have to be done in software off `attachInterrupt()`, which the platform now has.
      Worth knowing before scoping: the "MC" in the part name does **not** imply the full
      motor-control peripheral set. Also note HRPWM cannot run here (no APLL), so this would
      build on plain `analogWrite()`-class PWM, not the 250 ps path.

### Phase 12: Testing & Polish
- [x] **The guide taught an API that does not exist** — *fixed in v1.0.5, and this entry
      was wrong about it.* It used to read "those calls still compile — the suffixed
      methods were kept deliberately — so this is not a broken-docs bug." That was a guess
      from grepping for `print_int`, and it was false. What the files actually contained
      was **`Serial_begin`, `Serial_println`, `Serial_print_int`** — 133 calls across five
      pages, to identifiers that appear **nowhere** in `cores/`, `variants/` or
      `libraries/`. Not a style problem: `error: 'Serial_begin' was not declared in this
      scope`. Every serial example in the shipped user guide was uncompilable, through
      four releases.
      Two separate defects, both found by *compiling* the docs rather than reading them:
      - 133 `Serial_*` calls rewritten to the stock object API (`_build/fix_docs_serial_api.py`).
      - **Six raw `<` comparison operators inside `<pre>` blocks**
        (`_build/fix_docs_angle_brackets.py`). A browser reads `if (x < 100) {` as a start
        tag and swallows everything to the next `>`, so readers were shown truncated code.
        In one sketch the eaten span crossed a statement and left behind
        `for (brightness = 0; brightness = 0; brightness -= 5)` — a loop that compiles,
        warns only about parentheses, and never executes. Twelve lines of
        `part4_testing_sketches.html` were invisible on the page.
      New gate **`_build/docs_code_check.sh`** extracts every complete sketch from the
      guide and compiles it for MP102 and MC005: 14 sketches, 28 builds, no warnings. It
      exists because every other gate compiled files in the repo and nothing compiled the
      code in the docs — which mattered more from v1.0.5 on, since the docs now ship
      inside the archive.
- [ ] **Documentation is still behind the code** in the ways that are about *coverage*
      rather than correctness, now that the API names are right. Missing everywhere: the
      MC005 / Curiosity Nano board (only `installation_guide.html` mentions it at all, and
      `part5`'s quick-reference card is still an MP102 card), `tone()`,
      `attachInterrupt()`, and the fact that the debugger reboot is now automatic.
      Under the MC005 priority this item gets *easier and more urgent at once*: the docs
      need to describe one board rather than four, and that board is the one they currently
      barely mention.
      `docs/how-to-use/` — six superseded files with **19 more `Serial_*` calls** — was
      **deleted September 24, 2026** rather than fixed, since fixing it would have implied it
      was maintained. It shipped in no archive and nothing linked to it, so it needed no
      version bump. `docs/` is now exactly the eight pages that ship.
- [ ] No example uses `tone()` or `attachInterrupt()` — the two newest APIs are the two
      with no example. **A `04.CuriosityNano` sketch covering both is now the highest-value
      code task**: it doubles as the Phase 10 bench checklist (all four outstanding checks
      are MC005 checks), and it is on the priority board, so it can actually be run.
- [ ] Verify `analogWrite()` on MC005 — the Phase 9 measurement retargeted at the priority
      board. Re-derive the expected frequency from the clock menu entry in use (default
      `f_cpu=8000000UL` → FCY = 4 MHz → ~19.6 Hz at prescaler 1:64, *not* the ~490 Hz the
      Phase 9 notes quote for the 200 MHz PLL entry). `NanoPWMFade` already exists as the
      sketch. Worth testing under both clock entries, since that is a one-menu-click change
      a user will make and nothing has ever verified the PLL path on silicon.
- [ ] ON HOLD — Test MC002 board on actual hardware
- [ ] ON HOLD — Test all APIs across all 4 board variants (all 4 still build clean; the
      MC005 column of this matrix is the part that is not held)
- [ ] Add more example sketches (SPI sensor, I2C EEPROM) — target `04.CuriosityNano`.
      Note both `SPI` and `Wire` are **untested on any silicon**: the SPI library had never
      compiled on any board until the September 8 fix, so "it builds" is the entire extent
      of what is known about either. An I2C/SPI example on MC005 would be the first real
      exercise of them.
- [ ] Package for distribution to colleagues — with one board prioritised, the honest
      framing is "supports MC005, builds for three others" rather than "supports 4 boards".

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
the interrupt vector table and so are never collected**. (That claim stopped being an
argument and became a measurement on Sep 22 2026, when `--gc-sections` shipped in v1.0.1:
the Timer1 `millis()` ISR survives collection on silicon — see the Phase 14 bench check.)
The floor is what a sketch that calls none of this pays anyway:

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
**Under the MC005-only priority: 2 and 5 are MP508-only and go on hold with that board;
3 becomes more relevant, not less, because the file it names is the priority device's;
4 and 6 are device-independent and stay actionable.**

1. **`analogWrite()` had no bounds check — FIXED.** The mid-range path dereferenced
   `g_pin_map[pin]` unguarded and then wrote through the resulting garbage
   `ansel_reg`/`tris_reg` pointers, so `analogWrite(200, 128)` corrupted arbitrary SFRs.
   (The `val<=0` and `val>=255` paths were safe only because they end in `digitalWrite`,
   which does check.) One line, in a file already being edited.
2. **MP508 `PWM5_RP 181` is not a pin — ON HOLD with the MP508.** `variants/dspic33ck256mp508/pins_arduino.h`
   maps `PWM5_PIN 58` (RE5) to RP181, but **RP176-181 are the virtual pins RPV0-RPV5** —
   internal nodes with no bond wire. `analogWrite(58, x)` can never reach RE5, so LED2 on
   the DM330030 will not dim. No PPS fix exists; RE5 would need the same ISR-toggle
   mechanism as `tone()`.
3. **`cmake/Arduino_dsPIC33CK/.../file.cmake` lists only three of four variants** —
   `dspic33ck256mc005` is absent, so its `variant.c` gets no plain-C check from that
   project. Previously left as is because the
   `dsPIC33CK256MC005_Curiosity_Nano_Out_of_Box_Demo` project does cover it. **Now worth
   fixing:** the one variant missing from the main project is the one variant that
   matters, and the coverage currently depends on a demo project nobody would think to
   open. One line in a generated file list.
4. **`wiring.c:19`** — `static volatile unsigned long _micros_overflow` declared, never
   used.
5. **MP508 `variant.c` — ON HOLD with the MP508.** It gives RE0-RE3 `&ANSELE` and channels AN20-23, but `ANSELE` may
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
│   │   │   ├── HRPWM/src/HRPWM.h + HRPWM.c   (PG1-PG8, 250ps)
│   │   │   │   └── examples/BoostMPPT/
│   │   │   └── Arduino_dsPIC33CK/            (carries the board examples: File >
│   │   │       └── examples/                 Examples is built from libraries ONLY,
│   │   │           ├── 01.Basics/        never from a platform-level examples/)
│   │   │           │                     Blink, AnalogReadSerial   (all 4 boards)
│   │   │           ├── 02.CppFeatures/   CppDemo         (MP508 only: LED1/A22)
│   │   │           ├── 03.PWM/           Fade, PWMTest   (MP508 only: LED2)
│   │   │           └── 04.CuriosityNano/ NanoBlink, NanoSerialHello,
│   │   │                                 NanoButtonLED, NanoAnalogRead,
│   │   │                                 NanoPWMFade, NanoSelfTest
│   │   ├── tools/
│   │   │   ├── bin/                      (Phase 14: 7 byte-identical shims;
│   │   │   │                            each derives its tool from %~n0)
│   │   │   ├── xc-dsc-find.bat           (Phase 14: resolves XC-DSC + MPLAB X
│   │   │   │                            at build time; override > env >
│   │   │   │                            cache > newest-version glob > error)
│   │   │   ├── ipecmd-upload.bat         (Phase 14: all 5 ipecmd programmers)
│   │   │   ├── suppress-stderr.bat       (stderr -> silence; STILL USED by the
│   │   │   │                            ar and objcopy recipes)
│   │   │   ├── xc-dsc-size-wrapper.bat   (size reporting)
│   │   │   ├── xc-dsc-link.bat           (linker CWD workaround)
│   │   │   │                            (nedbg-upload.bat is GONE - its flash-then-
│   │   │   │                             reboot logic moved into ipecmd-upload.bat)
│   │   │   └── serial_upload.py          (serial bootloader upload, stdlib only)
│   │   │                            (pre_build.py DELETED Sep 18 2026 - a prebuild
│   │   │                             hook cannot work: properties expand first)
│   │   │                            (upload_uart.py DELETED Sep 23 2026 - invented
│   │   │                             a protocol no firmware spoke; see Phase 15)
│   │   ├── ldscripts/              (generated: p33CK256MC005-{boot,app}.gld)
│   │   ├── bootloaders/            (dspic33ck256mc005/: bl_*.c/h + the .hex)
│   │   ├── boards.txt
│   │   ├── platform.txt
│   │   ├── platform.local.txt.template  <- DEVELOPER path only. A Boards
│   │   │                                   Manager install has no such file.
│   │   └── programmers.txt
│   ├── package_microchip_dspic33ck_index.json  <- Phase 14: the Boards Manager
│   │                                             index; in-repo copy is the
│   │                                             source of truth
│   ├── docs/                   <- the 8 pages that ship inside the archive
│   │                              (setup + parts 1-7). Install/toolchain/DFP
│   │                              instructions corrected in Phase 14; the Serial
│   │                              API fixed and gated in v1.0.5. Still thin on
│   │                              MC005, tone() and attachInterrupt().
│   │                              how-to-use/ was deleted Sep 24, 2026.
│   ├── install_arduino_ide.bat  <- DEVELOPER install (uncommitted working tree).
│   │                              RE-RUN after any platform change. End users
│   │                              install from the Boards Manager URL instead.
│   └── README.md
├── tools/release/              <- Phase 14: make-release.sh builds the three
│                                  deterministic zips (sorted entries, fixed
│                                  date_time, so checksums are reproducible) and
│                                  rewrites the 9 url/size/checksum fields in the
│                                  index. Never hand-edit those nine fields.
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
| `package_check.sh` | **Phase 14.** `arduino-cli` compiles one sketch per board against the *installed* platform — the only gate that exercises `platform.txt`, `boards.txt`, the recipes and the wrappers at all. Baselines: **2556 / 3920 / 2556 / 3188 bytes** (2564 / 3928 / 2564 / 3196 from 1.0.1 through 1.0.3; 8 bytes lower in 1.0.4, when the RX interrupt stopped reading `U1RXREG` once per branch — see Release v1.0.4. Was 11904 / 13992 / 11916 / 12876 before `--gc-sections` landed in 1.0.1) |
| `install_check.sh` | **Phase 14, the acceptance gate.** Serves the real release archives over local HTTP and runs `arduino-cli core install microchip:dspic33ck`, so checksums, archive roots, tool placement and `toolsDependencies` are all really exercised. Asserts **no `platform.local.txt` anywhere** and the same four sizes (the default menu option only — `menu_size_check.sh` covers the other one). Takes its expected version from `platform.txt` rather than a pinned constant -- the pinned `1.0.0` made it report a phantom `FAIL no platform.txt` the moment 1.0.1 was cut. Also asserts that **`File > Examples` offers every shipped `.ino`** (`lib examples --format json`, filtered to this platform's `container_platform`) — the gap that let eleven invisible examples ship twice, since compiling by absolute path works whether or not the IDE can find them |
| `menu_size_check.sh` | **Phase 15, added in 1.0.4 as the gate that would have caught its predecessor's defect.** Stages the **working tree** as a `<version>-dev` install and compiles through `arduino-cli` at **both** ends of the `Tools > Bootloader` menu — the only gate that compiles a *menu option* at all. Asserts `Bootloader: none` sits exactly on the four baselines (so nothing reaches the core that the user did not opt into) **and** that `Bootloader: Serial` on MC005 is *larger* than its own baseline (so the `build.extra_flags` → `-DSERIAL_BOOTLOADER_ENTRY` chain really reaches the compiler; a sniffer that is opt-in and never opted in fails on hardware with no diagnosis). Measured `+1740` = 1608 trampoline + 132 sniffer. Sizes asserted as a relation, not a constant |
| `upgrade_check.sh` | **Phase 14.** Installs 1.0.0 then upgrades to a synthesised 1.0.1 off a two-version index: asserts the DFP packs are reused with no second HTTP GET *and* are still on disk afterwards, the old version's directory is gone, and no `platform.local.txt` appears at either version. Re-run for real against the two **live** releases on Sep 22 2026, not a synthesised index: DFP packs reused with no second download, only the 93 KB core archive fetched |
| `parallel_check.sh` | **Phase 14.** Five cold-cache `-j16` builds — the only gate that exercises the resolver's cache race, which a serial build cannot reach. Fails on output drift, any warning, or an orphan `.tmp` left by a lost race |
| `live_check.sh` | **Phase 14, post-publication.** Installs from the **real published GitHub URL** into a fresh data directory and compiles all four boards at the baselines. The only gate that covers GitHub itself — a wrong tag in the index's asset URLs, a pre-release flag hiding the release from `/latest`, an asset that failed to upload, or a CDN redirect problem all fail here and nowhere else. Run it after any release |
| plain-C check | every core `.c` + `variant.c` built with `xc-dsc-gcc -Wall -Wextra` in C mode on all 4 devices — the same guard the `cmake/` projects give, run from the shell |
