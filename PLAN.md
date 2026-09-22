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
> **Phase 14 (install from a Boards Manager URL) opened September 17, 2026 and is
> PUBLISHED September 18, 2026.** The repo is public, release `v1.0.0` carries the three
> archives plus the index, and `_build/live_check.sh` installs from the real URL and builds
> all four boards at baseline. **Upload was verified on silicon September 22, 2026** through
> that published install — `Program Succeeded`, the nEDBG bridge reboot intact, and
> `NanoSerialHello` read back live — so the phase has no unverified seam left. Two items are
> open and both want a **1.0.1** release: *Upload Using Programmer* is broken for all six
> programmers (no `program.pattern` recipes exist), and the fresh-install/resolver-glob test
> still wants a machine with a different XC-DSC version. See that section.
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
> **Known inconsistency, not yet fixed:** the Phase 6 documentation still teaches the
> superseded suffixed `Serial` API and does not mention the MC005 board, `tone()` or
> `attachInterrupt()`. See "Documentation is behind the code" under Phase 12. (Its
> *installation and toolchain* half was corrected in Phase 14; the API half was not.)
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
  - `examples/03.PWM/PWMTest/PWMTest.ino` — 4 tests on LED2 (RE5 = D58 = RP181 → SCCP5):
    50%; 25/50/75/full; smooth fade; PWM→digital→PWM transition. Serial 115200 via PKOB4 CDC.
    Expected ~490 Hz (at FCY=100 MHz: prescaler 1:64, period=3187 → 490.5 Hz)
  - `examples/03.PWM/Fade/Fade.ino`
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
      banner is printed into a dead bridge. To see it, press the board's reset button with
      the Serial Monitor already open. Software reset cannot substitute: `pymcuprog reset`
      **does not support this device** (its list has `dspic33ck64mc105`, not `...mc005`),
      which is exactly why the recipe uses `reboot-debugger`, which needs no `-d`.

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

- [ ] **BUG in the published v1.0.0 — *Sketch → Upload Using Programmer* fails for all six
      programmers.** Found by accident during the Sep 22 upload test: invoking
      `arduino-cli upload -P nedbg` dies with `Failed programming: recipe not found
      'program.pattern'`. `programmers.txt` wires every programmer to a `program.tool`
      (`pickit5`, `pickit4`, `snap`, `pkob4`, `nedbg`, `uart_bootloader`), but `platform.txt`
      defines **6 `upload.pattern` keys and 0 `program.pattern` keys**. So the menu entry is
      offered by the IDE and cannot work.

      **Not urgent:** the normal Upload button uses `boards.txt`'s
      `<board>.upload.tool=nedbg` → `tools.nedbg.upload.pattern`, which is the path just
      verified on silicon. Only the *Upload Using Programmer* menu item is affected. The fix
      is mechanical — mirror each `upload.pattern` to a `program.pattern` — but it changes
      the published `platform.txt`, so it needs a **1.0.1 release**, which is a decision for
      the user, not a quiet edit. Bundle it with `-Wl,--gc-sections` if that also goes in.

- [ ] *Offered, not built (awaiting the user's yes):* a short `TESTING.md` in the repo
      carrying the procedure above, so co-workers can be sent a link instead of a relay.

**Flagged, deliberately not changed here:** `compiler.ld.flags` passes
`-ffunction-sections -fdata-sections` at compile time but never `-Wl,--gc-sections` at
link, unlike `_build/allboards.sh:60` — worth ~2.4 KB per sketch. Left alone because it
changes the firmware on silicon and MC005's hardware verification was done through the
un-collected path; it needs its own bench check.

**The sequencing rule that gated this is now satisfied** (Sep 22 2026): the rule was "do
not add `-Wl,--gc-sections` until the hardware upload test passes, or a failure won't be
attributable." Upload now passes on silicon through the published install, so there is a
known-good reference point and the flag can be attempted whenever the user wants. It still
needs its own bench check, and it still means a **1.0.1 release** — so bundle it with the
`program.pattern` fix above rather than cutting two releases.

**Committed September 18, 2026** as five commits on `phase10-platform-cleanup`, working
tree clean: `6486b91` the resolver layer, `435f0b2` the index + release builder,
`bc51179` the developer installer, `059921e` the docs, `6abb2ff` this PLAN.md section.
**Deliberately not pushed** — the release is on hold until the hardware test, so
`origin/main` is still at `0a24860` and has none of Phase 10, 13 or 14.

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
      Under the MC005 priority this item gets *easier and more urgent at once*: the docs
      need to describe one board rather than four, and that board is the one they currently
      barely mention.
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
│   │   │   └── HRPWM/src/HRPWM.h + HRPWM.c   (PG1-PG8, 250ps)
│   │   │       └── examples/BoostMPPT/
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
│   │   │   └── upload_uart.py            (UART bootloader upload)
│   │   │                            (pre_build.py DELETED Sep 18 2026 - a prebuild
│   │   │                             hook cannot work: properties expand first)
│   │   ├── examples/
│   │   │   ├── 01.Basics/        Blink, AnalogReadSerial   (all 4 boards)
│   │   │   ├── 02.CppFeatures/   CppDemo                   (MP508 only: LED1/A22)
│   │   │   ├── 03.PWM/           Fade, PWMTest    (MP508 only: LED2; Phase 9)
│   │   │   └── 04.CuriosityNano/ NanoBlink, NanoSerialHello, NanoButtonLED,
│   │   │                         NanoAnalogRead, NanoPWMFade, NanoSelfTest
│   │   ├── bootloaders/
│   │   ├── boards.txt
│   │   ├── platform.txt
│   │   ├── platform.local.txt.template  <- DEVELOPER path only. A Boards
│   │   │                                   Manager install has no such file.
│   │   └── programmers.txt
│   ├── package_microchip_dspic33ck_index.json  <- Phase 14: the Boards Manager
│   │                                             index; in-repo copy is the
│   │                                             source of truth
│   ├── docs/                   <- 5-part guide + how-to-use/ (5 more)
│   │                              Install/toolchain/DFP instructions were
│   │                              corrected in Phase 14. Still STALE on the
│   │                              pre-Phase-10 Serial API, pin maps and MPLAB X
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
| `package_check.sh` | **Phase 14.** `arduino-cli` compiles one sketch per board against the *installed* platform — the only gate that exercises `platform.txt`, `boards.txt`, the recipes and the wrappers at all. Baselines: **11904 / 13992 / 11916 / 12876 bytes** |
| `install_check.sh` | **Phase 14, the acceptance gate.** Serves the real release archives over local HTTP and runs `arduino-cli core install microchip:dspic33ck`, so checksums, archive roots, tool placement and `toolsDependencies` are all really exercised. Asserts **no `platform.local.txt` anywhere** and the same four sizes |
| `upgrade_check.sh` | **Phase 14.** Installs 1.0.0 then upgrades to a synthesised 1.0.1 off a two-version index: asserts the DFP packs are reused with no second HTTP GET *and* are still on disk afterwards, the old version's directory is gone, and no `platform.local.txt` appears at either version |
| `parallel_check.sh` | **Phase 14.** Five cold-cache `-j16` builds — the only gate that exercises the resolver's cache race, which a serial build cannot reach. Fails on output drift, any warning, or an orphan `.tmp` left by a lost race |
| `live_check.sh` | **Phase 14, post-publication.** Installs from the **real published GitHub URL** into a fresh data directory and compiles all four boards at the baselines. The only gate that covers GitHub itself — a wrong tag in the index's asset URLs, a pre-release flag hiding the release from `/latest`, an asset that failed to upload, or a CDN redirect problem all fail here and nowhere else. Run it after any release |
| plain-C check | every core `.c` + `variant.c` built with `xc-dsc-gcc -Wall -Wextra` in C mode on all 4 devices — the same guard the `cmake/` projects give, run from the shell |
