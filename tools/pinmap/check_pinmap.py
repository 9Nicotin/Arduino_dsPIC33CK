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

Six checks, each of which has a defect behind it:

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

  F  part3's per-device table in 3.1 agrees with all four variants'
     pins_arduino.h, and the phrases that made part3 an MP102 document have not
     come back. Up to v1.0.5 part3 stated MP102's numbers as the platform's:
     "pin 0-20", "A0-A5 (or D5-D10)", "A0 is the same physical pin as D5 (RB0)",
     "TX=RB5, RX=RB4", "LED_BUILTIN | 0 | Pin D0 (RA0)". On the priority device
     every one of those is false, and part3 is where a reader goes to find out
     what a function takes. F re-derives every cell from the headers here rather
     than importing tools/docs/gen_device_table.py: a checker that asks the generator
     what it generated cannot catch a generator that is wrong.

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
# The 39 rows are generated, and the marker comments are the only thing telling a
# reader that. E validates the rows either way, so losing the markers costs
# nothing today and costs the next editor everything.
ROWS_START = "<!-- BEGIN mc005-rows"
ROWS_END = "<!-- END mc005-rows -->"

# Check F: part3's generated per-device table, and the wording it replaced.
API_PAGE = "part3_api_reference.html"
TABLE_START = "<!-- BEGIN device-facts"
TABLE_END = "<!-- END device-facts -->"
# Each of these shipped in v1.0.5 and is true only of the 32MP102. v1.0.1's
# examples came back once already through an in-place release overwrite, so the
# retired wording is worth a gate of its own.
RETIRED = (
    "0-20, or D0-D20",
    "A0-A5 (or D5-D10)",
    "same physical pin as D5",
    "TX=RB5, RX=RB4",
    "<td>A0-A5</td>",
    "#define DATA_PIN  11",
    "<tr><td>LED_BUILTIN</td><td>0</td>",
)

# Check G: part5 5.8's generated quick-reference cards.
CARD_PAGE = "part5_upload_troubleshooting.html"
CARDS_START = "<!-- BEGIN device-cards"
CARDS_END = "<!-- END device-cards -->"


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
    ra, rb = mixed.find(ROWS_START), mixed.find(ROWS_END)
    if ra < 0 or rb <= ra:
        bad.append("%s: no generated row block between %r and %r - run "
                   "tools/docs/gen_mc005_rows.py --write, and put the markers back so the "
                   "next editor knows not to hand-edit 39 rows"
                   % (MIXED_PAGE, ROWS_START, ROWS_END))
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

    # ---- F: part3's per-device table agrees with all four variants ----------
    f_rows = f_boards = 0
    api = read(os.path.join(DOCS, API_PAGE))
    for phrase in RETIRED:
        if phrase in api:
            line = api[:api.find(phrase)].count("\n") + 1
            bad.append("%s:%d has the retired v1.0.5 wording %r, which is true only of the "
                       "32MP102" % (API_PAGE, line, phrase))

    i, j = api.find(TABLE_START), api.find(TABLE_END)
    if i < 0 or j <= i:
        bad.append("%s: no generated device-facts table between %r and %r - run "
                   "tools/docs/gen_device_table.py --write"
                   % (API_PAGE, TABLE_START, TABLE_END))
    else:
        region = api[i:j]
        head = re.search(r"<tr><th>Constant</th>(.*?)</tr>", region, re.S)
        cols = re.findall(r"<th>([^<]+)</th>", head.group(1)) if head else []
        f_boards = len(cols)
        variants = {}
        for disp in cols:
            slug = disp.strip().lower()
            vdir = os.path.join(PLAT, "variants", slug)
            if not os.path.isdir(vdir):
                bad.append("%s: table column %r has no variants/%s directory"
                           % (API_PAGE, disp, slug))
                continue
            vph = read(os.path.join(vdir, "pins_arduino.h"))
            vvc = read(os.path.join(vdir, "variant.c"))
            defs = {}
            for m in re.finditer(r"^\s*#define\s+([A-Za-z_]\w*)\s+\(?(\d+)\)?\s*(?:/\*.*)?$",
                                 vph, re.M):
                defs.setdefault(m.group(1), int(m.group(2)))
            flags = set(re.findall(r"#define\s+(\w+_ACTIVE_LOW)\b", vph))
            vports = {int(a): "R" + b for a, b in
                      re.findall(r"/\*\s*D(\d+):\s*R([A-E]\d+)\s*\*/", vvc)}
            variants[disp] = (defs, flags, vports)
            # part3's prose says analogWrite() takes D5-D8 on all four devices.
            pwm = sorted(v for k, v in defs.items() if re.match(r"^PWM\d+_PIN$", k))
            if pwm[:4] != [5, 6, 7, 8]:
                bad.append("%s: part3 says D5-D8 are PWM on every device, but %s has "
                           "PWM pins %s" % (API_PAGE, disp, pwm))

        # Rows are identified by the macro named in the label, so reordering or
        # relabelling the table does not quietly drop a row from the check.
        PIN_KEYS = ("A0", "LED_BUILTIN", "BUTTON_BUILTIN", "PIN_SERIAL_TX",
                    "PIN_SERIAL_RX", "PIN_SPI_SCK", "PIN_SPI_MOSI", "PIN_SPI_MISO",
                    "PIN_SPI_SS", "PIN_WIRE_SDA", "PIN_WIRE_SCL")
        checked = set()
        for m in re.finditer(r"<tr><td>(.*?)</td>((?:<td>.*?</td>)+)</tr>", region, re.S):
            label, cells = m.group(1), re.findall(r"<td>(.*?)</td>", m.group(2), re.S)
            f_rows += 1
            if len(cells) != len(cols):
                bad.append("%s: table row %r has %d cells for %d columns"
                           % (API_PAGE, label[:40], len(cells), len(cols)))
                continue
            names = re.findall(r"<code>(\w+)</code>", label)
            key = next((k for k in ("NUM_DIGITAL_PINS", "NUM_ANALOG_INPUTS") + PIN_KEYS
                        if k in names), None)
            is_pwm = "analogWrite()" in label
            if key is None and not is_pwm:
                bad.append("%s: table row %r names no constant, so nothing checks it"
                           % (API_PAGE, label[:60]))
                continue
            checked.add(key or "PWM")
            for disp, cell in zip(cols, cells):
                if disp not in variants:
                    continue
                defs, flags, vports = variants[disp]
                where = "%s: %s column %s" % (API_PAGE, key or "analogWrite()", disp)
                if is_pwm:
                    want = sorted(v for k, v in defs.items()
                                  if re.match(r"^PWM\d+_PIN$", k))
                    have = []
                    for a, b in re.findall(r"D(\d+)(?:-D(\d+))?", cell):
                        have.extend(range(int(a), int(b) + 1) if b else [int(a)])
                    if sorted(have) != want:
                        bad.append("%s says %s, PWMn_PIN says %s" % (where, have, want))
                elif key.startswith("NUM_"):
                    n = defs.get(key)
                    unit, first = (("pins", "D"), ("channels", "A"))[key.endswith("INPUTS")]
                    want = "%d %s: %s0-%s%d" % (n, unit, first, first, n - 1)
                    if cell.strip() != want:
                        bad.append("%s says %r, the header gives %r"
                                   % (where, cell.strip(), want))
                else:
                    n = defs.get(key)
                    if n is None:
                        if cell.strip() != "not defined":
                            bad.append("%s says %r, but that variant does not define %s"
                                       % (where, cell.strip(), key))
                        continue
                    want = "D%d (%s)" % (n, vports.get(n))
                    if not cell.startswith(want):
                        bad.append("%s says %r, the header and variant.c give %r"
                                   % (where, cell.strip()[:46], want))
                    low = key + "_ACTIVE_LOW" in flags
                    said = "active LOW" in cell
                    if low != said:
                        bad.append("%s: %s_ACTIVE_LOW is %s but the cell %s say so"
                                   % (where, key, "defined" if low else "not defined",
                                      "does not" if low else "does"))
        for k in ("NUM_DIGITAL_PINS", "NUM_ANALOG_INPUTS", "PWM") + PIN_KEYS:
            if k not in checked:
                bad.append("%s: the device table has no row for %s" % (API_PAGE, k))

    # ---- G: part5 5.8's quick-reference cards agree with their variants -----
    #
    # The v1.0.5 card was the 32MP102's, printed as the platform's, on the one
    # page a user is most likely to print out. It now carries one card per
    # documented board, generated -- and this re-derives every D-number on it
    # from pins_arduino.h, the same way F does, rather than asking the generator.
    g_cards = 0
    boards = read(os.path.join(PLAT, "boards.txt"))
    up = read(os.path.join(DOCS, CARD_PAGE))
    i, j = up.find(CARDS_START), up.find(CARDS_END)
    if i < 0 or j <= i:
        bad.append("%s: no generated device-cards block between %r and %r - run "
                   "tools/docs/gen_device_table.py --write"
                   % (CARD_PAGE, CARDS_START, CARDS_END))
    else:
        region = up[i:j]
        # "SDA = D14" / "TX = D31 (RC10)" -> the macro that must hold that pin.
        CARD_KEYS = {"TX": "PIN_SERIAL_TX", "RX": "PIN_SERIAL_RX",
                     "SCK": "PIN_SPI_SCK", "MOSI": "PIN_SPI_MOSI",
                     "MISO": "PIN_SPI_MISO", "SS": "PIN_SPI_SS",
                     "SDA": "PIN_WIRE_SDA", "SCL": "PIN_WIRE_SCL"}
        for card in region.split("<pre><code>")[1:]:
            card = card.split("</code></pre>")[0]
            m = re.search(r"^\|\s*DEVICE:\s*(\S+)\s*\|", card, re.M)
            if not m:
                bad.append("%s: a 5.8 card has no DEVICE: line, so nothing identifies "
                           "which variant it is about" % CARD_PAGE)
                continue
            g_cards += 1
            disp = m.group(1)
            vdir = os.path.join(PLAT, "variants", disp.lower())
            if not os.path.isdir(vdir):
                bad.append("%s: 5.8 card names %r, which has no variants/%s directory"
                           % (CARD_PAGE, disp, disp.lower()))
                continue
            vph = read(os.path.join(vdir, "pins_arduino.h"))
            vvc = read(os.path.join(vdir, "variant.c"))
            defs = {}
            for m in re.finditer(r"^\s*#define\s+([A-Za-z_]\w*)\s+\(?(\d+)\)?\s*(?:/\*.*)?$",
                                 vph, re.M):
                defs.setdefault(m.group(1), int(m.group(2)))
            flags = set(re.findall(r"#define\s+(\w+_ACTIVE_LOW)\b", vph))
            vports = {int(a): "R" + b for a, b in
                      re.findall(r"/\*\s*D(\d+):\s*R([A-E]\d+)\s*\*/", vvc)}
            where = "%s: the %s card in 5.8" % (CARD_PAGE, disp)

            def want(key, seen, w=where, d=defs):
                n = d.get(key)
                if n is None:
                    bad.append("%s claims %s = D%d, but the variant does not define it"
                               % (w, key, seen))
                elif n != seen:
                    bad.append("%s says %s = D%d, pins_arduino.h says D%d"
                               % (w, key, seen, n))

            # SIZE BAR must be the figure the IDE will actually print, i.e.
            # boards.txt's, not a KB figure someone reasoned out.
            m = re.search(r"^\|\s*SIZE BAR:\s*(\d+) B flash / (\d+) B RAM", card, re.M)
            if not m:
                bad.append("%s has no SIZE BAR line" % where)
            else:
                for got, key in ((m.group(1), "upload.maximum_size"),
                                 (m.group(2), "upload.maximum_data_size")):
                    bm = re.search(r"^%s\.%s=(\d+)$" % (disp.lower(), re.escape(key)),
                                   boards, re.M)
                    if not bm:
                        bad.append("%s: boards.txt has no %s.%s" % (where, disp.lower(), key))
                    elif bm.group(1) != got:
                        bad.append("%s says %s = %s, boards.txt says %s"
                                   % (where, key, got, bm.group(1)))

            m = re.search(r"^\|\s*DIGITAL I/O:\s*(\d+) pins \(D0-D(\d+)\)", card, re.M)
            if not m:
                bad.append("%s has no DIGITAL I/O line" % where)
            elif (int(m.group(1)), int(m.group(1)) - 1) != (defs.get("NUM_DIGITAL_PINS"),
                                                            int(m.group(2))):
                bad.append("%s says %s pins, D0-D%s; NUM_DIGITAL_PINS is %s"
                           % (where, m.group(1), m.group(2), defs.get("NUM_DIGITAL_PINS")))

            m = re.search(r"^\|\s*ANALOG IN:\s*(\d+) channels \(A0-A(\d+)\), A0 = D(\d+)",
                          card, re.M)
            if not m:
                bad.append("%s has no ANALOG IN line" % where)
            else:
                if (int(m.group(1)), int(m.group(1)) - 1) != (defs.get("NUM_ANALOG_INPUTS"),
                                                              int(m.group(2))):
                    bad.append("%s says %s channels, A0-A%s; NUM_ANALOG_INPUTS is %s"
                               % (where, m.group(1), m.group(2),
                                  defs.get("NUM_ANALOG_INPUTS")))
                want("A0", int(m.group(3)))

            m = re.search(r"^\|\s*PWM OUT:\s*(.*?)\s*\(SCCP\)", card, re.M)
            if not m:
                bad.append("%s has no PWM OUT line" % where)
            else:
                have = []
                for a, b in re.findall(r"D(\d+)(?:-D(\d+))?", m.group(1)):
                    have.extend(range(int(a), int(b) + 1) if b else [int(a)])
                wpwm = sorted(v for k, v in defs.items() if re.match(r"^PWM\d+_PIN$", k))
                if sorted(have) != wpwm:
                    bad.append("%s says PWM on %s, PWMn_PIN says %s" % (where, have, wpwm))

            for label, n in re.findall(r"\b(TX|RX|SCK|MOSI|MISO|SDA|SCL|SS) = D(\d+)", card):
                want(CARD_KEYS[label], int(n))
            # The port names in brackets are checked too: "TX = D31 (RC10)" with
            # the wrong port is exactly the kind of thing v1.0.5 shipped.
            for n, port in re.findall(r"= D(\d+) \((R[A-E]\d+)\)", card):
                if vports.get(int(n)) != port:
                    bad.append("%s says D%s is %s, variant.c says %s"
                               % (where, n, port, vports.get(int(n))))

            for key, label in (("LED_BUILTIN", "LED_BUILTIN"),
                               ("BUTTON_BUILTIN", "BUTTON_BUILTIN")):
                m = re.search(r"^\|\s*%s:\s*(\S+)(.*?)\|" % label, card, re.M)
                if not m:
                    bad.append("%s has no %s line" % (where, label))
                    continue
                if m.group(1) == "none":
                    if key in defs:
                        bad.append("%s says %s is not defined, but the variant defines it "
                                   "as D%d" % (where, key, defs[key]))
                    continue
                want(key, int(m.group(1).lstrip("D")))
                low, said = key + "_ACTIVE_LOW" in flags, "active LOW" in m.group(2)
                if low != said:
                    bad.append("%s: %s_ACTIVE_LOW is %s but the card %s say so"
                               % (where, key, "defined" if low else "not defined",
                                  "does not" if low else "does"))
        if g_cards and not any(d in region for d in ("dsPIC33CK256MC005",)):
            bad.append("%s: 5.8 has cards but none for the priority device "
                       "dsPIC33CK256MC005" % CARD_PAGE)

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
    print("part3 device table         : %d rows x %d boards" % (f_rows, f_boards))
    print("part5 device cards         : %d card%s" % (g_cards, "" if g_cards == 1 else "s"))

    if bad:
        print("\nFAIL  %d problem%s" % (len(bad), "" if len(bad) == 1 else "s"))
        for b in bad:
            print("  " + b)
        return 1
    print("\nOK    map, code, bootloader config and MC005 docs all agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
