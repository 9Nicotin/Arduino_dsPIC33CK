#!/usr/bin/env python3
"""
check_pinmap.py - prove the pin map still agrees with the code, and that the
documentation does not name a physical pin that is not the one it means.

Why this exists as a separate gate rather than as part of the generator:

  gen_pinmap.py's docstring says the pin data has one source of truth in
  variant.c. That is the intent, but the LEFT/RIGHT tables in the generator are
  Python literals - nothing reads variant.c. So the picture can be internally
  consistent, regenerate byte-identically, and still be wrong, and there is no
  diff anywhere to notice it. README used to claim the diagram "cannot drift
  from the pin table the core actually compiles against"; it could. This script
  is what makes that claim true.

Four checks, each of which has a defect behind it:

  A  every pad on the map matches variant.c's g_pin_map and pins_arduino.h -
     port, bit, ADC channel, An name, LED_BUILTIN, BUTTON_BUILTIN - and no pin
     in g_pin_map is missing from the map.

  B  the committed SVG is what the current generator produces. Editing the SVG
     by hand, or editing the tables and forgetting to re-run, both end here.

  C  the pins compiled into the bootloader HEX (bl_config.h) are labelled with
     those roles on the map. If the bootloader moves its UART, the map is stale
     the moment bl_config.h is saved.

  D  no MC005 page attaches a bare physical pin number to a port name. The
     v1.0.5 shipped text read "RC11 - U1RX, RP59, pin 32". That 32 is the
     Arduino number D32, but in a cell that already says RP59 it scans as a
     package pin, and a reader who counts pins on the package lands somewhere
     else entirely. See PACKAGE PIN NUMBERS below for why the fix is to stop
     printing the number rather than to correct it.

  E  part2's MC005 pin table says what variant.c says, on all 39 rows, and its
     MC005 half is free of the bare pin numbers check D forbids. Up to v1.0.5
     part2 documented only the 28-pin MP102, so a user of the priority device
     read a table whose every row named a different port than their board had.
     part2 is now half one device and half the other, and check D cannot be
     pointed at the whole file: the MP102 sections print that device's package
     numbers throughout and are out of scope here. So E scopes itself to the
     region between the first MC005 heading and the first MP102 heading.

PACKAGE PIN NUMBERS, and why this gate refuses to check them

  The obvious source is the DFP's own device description,
  ~/.mchp_packs/.../edc/DSPIC33CK256MC005.PIC, whose <edc:PinList> holds one
  <edc:Pin> block per package pin. It carries no pin numbers -- the list is
  positional, so pin N is the Nth block. That assumption does not survive
  contact with a second device:

    DSPIC33CK32MP102   edc:desc="28-pin SSOP"   1-4: RA1 RA2 RA3 RA4   25-28: RB14 RB15 MCLR RA0
    DSPIC33CK256MC002  (no desc)                1-4: RB14 RB15 MCLR RA0

  Same 28-pin package, same cyclic order of port names, offset by four. They
  cannot both start at physical pin 1, and nothing inside the pack says which
  one does. DSPIC33CK256MC005.PIC has no edc:desc either, so it is in the
  unlabelled group.

  So this repo has no verifiable source of physical pin numbers, and it does not
  need one: a user wires to a Curiosity Nano edge pad, which is silkscreened
  with the port name, and writes code against the Arduino number. Both of those
  ARE verifiable here. Check D therefore enforces that the docs use them.

Usage:  python tools/pinmap/check_pinmap.py      (exit 0 = in sync)
"""

import importlib.util
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PLAT = os.path.join(ROOT, "arduino-platform", "microchip", "dspic33ck")
VAR = os.path.join(PLAT, "variants", "dspic33ck256mc005")
DOCS = os.path.join(ROOT, "arduino-platform", "docs")
SVG = os.path.join(ROOT, "docs", "img", "pinmap-dspic33ck256mc005.svg")

# Check D scans these pages only - the wholly-MC005 ones. part2 covers both
# devices and prints the MP102's package numbers throughout; those are not this
# gate's business and are not checkable here either (see the module docstring).
# Check E handles part2's MC005 half on its own.
MC005_PAGES = ("part6_serial_bootloader.html", "part7_bench_verification.html")
MIXED_PAGE = "part2_pin_mapping_hardware.html"
# The headings that bound part2's MC005 half. Renaming either one breaks E loudly
# rather than shrinking its scope silently.
MIXED_START = "<h2>2.1 dsPIC33CK256MC005"
MIXED_END = "<h2>2.3 dsPIC33CK32MP102"


def read(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def load_generator():
    spec = importlib.util.spec_from_file_location(
        "gen_pinmap", os.path.join(ROOT, "tools", "pinmap", "gen_pinmap.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main():
    bad = []
    gp = load_generator()
    vc = read(os.path.join(VAR, "variant.c"))
    ph = read(os.path.join(VAR, "pins_arduino.h"))

    # ---- the source of truth -------------------------------------------------
    # /* D31: RC10 */ { &PORTC, &TRISC, &LATC, NULL, 10, -1 },
    truth = {}
    for m in re.finditer(
            r"/\*\s*D(\d+):\s*R([A-D])(\d+)\s*\*/\s*\{[^}]*?,\s*(-?\d+),\s*(-?\d+)\s*\}", vc):
        d, port, bit = int(m.group(1)), m.group(2), m.group(3)
        bitfield, an = int(m.group(4)), int(m.group(5))
        if int(bit) != bitfield:
            bad.append("variant.c D%d: port name says bit %s, struct says %d"
                       % (d, bit, bitfield))
        truth[d] = ("R%s%s" % (port, bit), None if an < 0 else "AN%d" % an)

    # #define A5  5   /* RB0 = AN5 */   -- the trailing comment is why this is
    # not anchored with \s*$; written that way it silently matched nothing.
    anames = {}
    for m in re.finditer(r"^\s*#define\s+(A\d+)\s+\(?(\d+)\)?\s*(?:/\*.*)?$", ph, re.M):
        anames[int(m.group(2))] = m.group(1)
    builtin = dict(re.findall(r"#define\s+(LED_BUILTIN|BUTTON_BUILTIN)\s+\(?(\d+)\)?", ph))

    if not truth or not anames or len(builtin) != 2:
        # A parse that finds nothing must fail, not pass quietly: a check whose
        # pass condition is "nothing was reported" passes when it did not run.
        sys.exit("check_pinmap: parsed %d pins, %d analog names, %d builtins - "
                 "the variant files did not parse, refusing to report OK"
                 % (len(truth), len(anames), len(builtin)))

    # ---- A: every pad matches the code --------------------------------------
    seen = set()
    for side, rows in (("left", gp.LEFT), ("right", gp.RIGHT)):
        for pad, port, d, a, an, cat, tags in rows:
            if port is None:
                continue
            if d not in truth:
                bad.append("%s pad %-11s D%s is not in g_pin_map" % (side, pad, d))
                continue
            seen.add(d)
            t_port, t_an = truth[d]
            labels = [t for t, _ in tags]
            if t_port != port:
                bad.append("%s pad %-11s D%d is %s in variant.c, %s on the map"
                           % (side, pad, d, t_port, port))
            if t_an != an:
                bad.append("%s pad %-11s D%d ADC is %s in variant.c, %s on the map"
                           % (side, pad, d, t_an, an))
            if anames.get(d) != a:
                bad.append("%s pad %-11s D%d analog name is %s in pins_arduino.h, %s on the map"
                           % (side, pad, d, anames.get(d), a))
            if builtin.get("LED_BUILTIN") == str(d) and not any("LED_BUILTIN" in x for x in labels):
                bad.append("%s pad %-11s D%d is LED_BUILTIN, the map does not say so"
                           % (side, pad, d))
            if builtin.get("BUTTON_BUILTIN") == str(d) and not any(
                    "BUTTON_BUILTIN" in x or "SW0" in x for x in labels):
                bad.append("%s pad %-11s D%d is BUTTON_BUILTIN, the map does not say so"
                           % (side, pad, d))

    missing = sorted(set(truth) - seen)
    if missing:
        bad.append("in g_pin_map but on no pad: %s"
                   % ", ".join("D%d (%s)" % (d, truth[d][0]) for d in missing))

    # ---- B: the committed SVG is current ------------------------------------
    # main() writes the file, so run it and compare rather than trying to
    # re-implement the rendering here.
    before = open(SVG, "rb").read() if os.path.exists(SVG) else None
    gp.main()
    after = open(SVG, "rb").read()
    if before is None:
        bad.append("%s did not exist; it has now been generated" % os.path.relpath(SVG, ROOT))
    elif before != after:
        bad.append("the committed SVG was stale - regenerating it changed the file "
                   "(%d bytes -> %d); commit the new file" % (len(before), len(after)))

    # The file carries no <?xml encoding?> declaration, so any multi-byte
    # character in it renders according to the consumer's default rather than
    # anything we stated. Keep it ASCII and the question never arises.
    for i, b in enumerate(after):
        if b > 0x7F:
            line = after[:i].count(b"\n") + 1
            bad.append("SVG line %d holds a non-ASCII byte 0x%02X; the file declares no "
                       "encoding, so write the character as a numeric entity instead"
                       % (line, b))
            break

    # ---- C: bootloader pins are labelled on the map -------------------------
    blc = os.path.join(PLAT, "bootloaders", "dspic33ck256mc005", "bl_config.h")
    if not os.path.exists(blc):
        bad.append("bl_config.h not found at %s" % os.path.relpath(blc, ROOT))
    else:
        bl = read(blc)
        # LATDbits.LATD10 / PORTDbits.RD13 / LATCbits.LATC10 / TRISCbits.TRISC11
        def bl_pin(macro, pat):
            m = re.search(r"#define\s+%s\s+\w+bits\.%s" % (macro, pat), bl)
            return m.group(1) if m else None

        roles = [
            ("BL_LED_LAT", r"LAT([A-D]\d+)", "LED_BUILTIN", "bootloader status LED"),
            ("BL_SW0_PORT", r"R([A-D]\d+)", "SW0", "bootloader entry button"),
            ("BL_TX_LAT", r"LAT([A-D]\d+)", "Serial TX", "bootloader UART TX"),
            ("BL_RX_TRIS", r"TRIS([A-D]\d+)", "Serial RX", "bootloader UART RX"),
        ]
        onmap = {}
        for rows in (gp.LEFT, gp.RIGHT):
            for pad, port, d, a, an, cat, tags in rows:
                if port:
                    onmap.setdefault(port, []).extend(t for t, _ in tags)
        for macro, pat, label, what in roles:
            got = bl_pin(macro, pat)
            if got is None:
                bad.append("could not read %s out of bl_config.h" % macro)
                continue
            port = "R" + got
            if port not in onmap:
                bad.append("%s is %s (%s) and is on no pad of the map" % (macro, port, what))
            elif not any(label in t for t in onmap[port]):
                bad.append("%s is %s (%s) but the map does not label it %r"
                           % (macro, port, what, label))

    # ---- D: no MC005 page prints a bare pin number next to a port name ------
    # "RC11</strong> &mdash; U1RX, RP59, pin&nbsp;32" - a port name and a bare
    # pin number close enough together to read as that port's package pin.
    # "Arduino D32" and "pad RC11" are the two forms this repo can stand behind,
    # and neither matches this pattern.
    near = re.compile(r"\bR([A-D]\d+)\b(?:(?!\bR[A-D]\d+\b).){0,90}?"
                      r"\bpin(?:&nbsp;|\s)+(\d+)\b", re.S)
    for page in MC005_PAGES:
        path = os.path.join(DOCS, page)
        if not os.path.exists(path):
            bad.append("%s is missing" % page)
            continue
        text = read(path)
        for m in near.finditer(text):
            port, num = "R" + m.group(1), int(m.group(2))
            line = text[:m.start()].count("\n") + 1
            bad.append('%s:%d writes %s ... "pin %d". A bare pin number here reads as a '
                       "package pin, and this repo cannot verify package pin numbers "
                       '(see the docstring). Write "Arduino D%d" if that is what %d means, '
                       "or name the pad." % (page, line, port, num, num, num))

    # ---- E: part2's MC005 table agrees with variant.c -----------------------
    mixed = read(os.path.join(DOCS, MIXED_PAGE))
    p2rows = 0
    i, j = mixed.find(MIXED_START), mixed.find(MIXED_END)
    if i < 0 or j < 0 or j <= i:
        bad.append("%s: could not find its MC005 half between %r and %r - the headings "
                   "were renamed or reordered, and check E has nothing to check"
                   % (MIXED_PAGE, MIXED_START, MIXED_END))
    else:
        region = mixed[i:j]
        base = region.count("\n")  # for line numbers, counted from the page start
        base = mixed[:i].count("\n") + 1

        # <td><strong>D37 / A19</strong></td> <td>RD10</td> <td>... AN18 ...</td>
        row = re.compile(r"<td><strong>D(\d+)(?:\s*/\s*(A\d+))?</strong></td>\s*"
                         r"<td>R([A-D]\d+)</td>\s*<td>(.*?)</td>", re.S)
        got = {}
        for m in row.finditer(region):
            d = int(m.group(1))
            if d in got:
                bad.append("%s: D%d appears twice in the MC005 table" % (MIXED_PAGE, d))
            got[d] = (m.group(2), "R" + m.group(3), m.group(4))
        p2rows = len(got)

        for d in sorted(truth):
            if d not in got:
                bad.append("%s: D%d (%s) is in g_pin_map but has no row in the MC005 table"
                           % (MIXED_PAGE, d, truth[d][0]))
                continue
            a, port, funcs = got[d]
            t_port, t_an = truth[d]
            if port != t_port:
                bad.append("%s: D%d is %s in variant.c, the table says %s"
                           % (MIXED_PAGE, d, t_port, port))
            if a != anames.get(d):
                bad.append("%s: D%d analog name is %s in pins_arduino.h, the table says %s"
                           % (MIXED_PAGE, d, anames.get(d), a))
            # An analog pin must name its channel, and a digital-only pin must not
            # claim one: "analogRead() AN18" on a pin whose adc_channel is -1 is
            # how a reader ends up calling analogRead() on RD13 and getting noise.
            claimed = re.search(r"analogRead\(\)</code>\s*(AN\d+)", funcs)
            claimed = claimed.group(1) if claimed else None
            if claimed != t_an:
                bad.append("%s: D%d ADC channel is %s in variant.c, the table's Functions "
                           "cell says %s" % (MIXED_PAGE, d, t_an, claimed))

        extra = sorted(set(got) - set(truth))
        if extra:
            bad.append("%s: MC005 table has rows for pins not in g_pin_map: %s"
                       % (MIXED_PAGE, ", ".join("D%d" % d for d in extra)))

        for m in near.finditer(region):
            port, num = "R" + m.group(1), int(m.group(2))
            bad.append('%s:%d (MC005 half) writes %s ... "pin %d" - see check D; this repo '
                       "cannot verify package pin numbers"
                       % (MIXED_PAGE, base + region[:m.start()].count("\n"), port, num))

    # ---- report -------------------------------------------------------------
    print("g_pin_map entries          : %d" % len(truth))
    print("analog names in header     : %d" % len(anames))
    print("LED_BUILTIN / BUTTON       : D%s / D%s"
          % (builtin.get("LED_BUILTIN"), builtin.get("BUTTON_BUILTIN")))
    print("pads carrying an MCU port  : %d  (RC10/RC11/RD13 appear on two pads each)"
          % sum(1 for rows in (gp.LEFT, gp.RIGHT) for r in rows if r[1]))
    print("SVG                        : %s (%d bytes, ASCII)"
          % (os.path.relpath(SVG, ROOT).replace("\\", "/"), len(after)))
    print("pages scanned for pin nums : %s" % ", ".join(MC005_PAGES))
    print("part2 MC005 table rows     : %d  (of %d in g_pin_map)" % (p2rows, len(truth)))

    if bad:
        print("\nFAIL  %d problem%s" % (len(bad), "" if len(bad) == 1 else "s"))
        for b in bad:
            print("  " + b)
        return 1
    print("\nOK    map, code, bootloader config and MC005 docs all agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
