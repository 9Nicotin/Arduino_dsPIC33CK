# Emit part2 section 2.2's 39 MC005 pin rows from variant.c + pins_arduino.h.
# Transcribing 39 rows by hand is how a pin table goes wrong. There is no
# physical-pin column and there must never be one: see the PACKAGE PIN NUMBERS
# section of tools/pinmap/check_pinmap.py for why this repo cannot verify a
# package pin number and does not need to.
#
# Usage:
#   python tools/docs/gen_mc005_rows.py            print the rows
#   python tools/docs/gen_mc005_rows.py --write    splice them into part2
#
# --write is the point of keeping this in the repository rather than in the
# gitignored _build/: the region it owns is committed HTML, so the way to change
# it has to be runnable by whoever reads the marker comment. Check E in
# tools/pinmap/check_pinmap.py is the other half -- a generated table still rots
# the moment someone edits the HTML by hand.
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
DOCS = os.path.join(REPO, "arduino-platform", "docs")
V = os.path.join(REPO, "arduino-platform", "microchip", "dspic33ck",
                 "variants", "dspic33ck256mc005")

PAGE = "part2_pin_mapping_hardware.html"
START = "<!-- BEGIN mc005-rows"
END = "<!-- END mc005-rows -->"

# Everything below this line is annotation -- prose a header cannot state. The
# ports, the An names and the ADC channels are all read, never typed.
PWM = {5: "SCCP1", 6: "SCCP2", 7: "SCCP3", 8: "SCCP4"}
MCPWM = {15: "PWM3H", 16: "PWM3L", 17: "PWM2H", 18: "PWM2L",
         19: "PWM1H", 20: "PWM1L", 35: "PWM4H"}
EXTRA = {
    10: "<strong>PGD3 &mdash; nEDBG debugger line. Do not use.</strong>",
    11: "<strong>PGC3 &mdash; nEDBG debugger line. Do not use.</strong>",
    13: "I2C <code>SCL</code> (<code>Wire</code>)",
    14: "I2C <code>SDA</code> (<code>Wire</code>)",
    25: "SPI <code>SCK</code>", 26: "SPI <code>MOSI</code>",
    33: "SPI <code>MISO</code>", 34: "SPI <code>SS</code>",
    29: "ASDA1, SCK2", 30: "ASCL1, SDI2", 36: "SDO2",
    31: "<strong>Serial TX</strong> (U1TX) &mdash; CDC bridge; also the bootloader's TX",
    32: "<strong>Serial RX</strong> (U1RX) &mdash; CDC bridge; also the bootloader's RX",
    37: "<strong><code>LED_BUILTIN</code></strong> &mdash; LED0, <em>active low</em>",
    38: "<strong><code>BUTTON_BUILTIN</code></strong> &mdash; SW0, <em>active low</em>; "
        "hold at power-up to force the bootloader. Carries ANN0 (ADC <em>negative</em> "
        "input), so it has an <code>ANSEL</code> bit but no <code>analogRead()</code> channel",
}

ROW = ('    <tr>\n'
       '        <td><strong>%s</strong></td>\n'
       '        <td>%s</td>\n'
       '        <td>%s</td>\n'
       '        <td class="%s">%s</td>\n'
       '    </tr>')


def read(p):
    return io.open(p, encoding="utf-8", newline="").read()


def rows():
    vc = read(os.path.join(V, "variant.c"))
    ph = read(os.path.join(V, "pins_arduino.h"))

    pins = {}
    for m in re.finditer(
            r"/[*]\s*D(\d+):\s*R([A-D])(\d+)\s*[*]/\s*[{][^}]*?,\s*(-?\d+),\s*(-?\d+)\s*[}]",
            vc):
        d = int(m.group(1))
        pins[d] = ("R%s%s" % (m.group(2), m.group(3)),
                   None if int(m.group(5)) < 0 else "AN%s" % m.group(5))
    anames = {int(m.group(2)): m.group(1) for m in
              re.finditer(r"^\s*#define\s+(A\d+)\s+[(]?(\d+)[)]?\s*(?:/[*].*)?$", ph, re.M)}
    assert len(pins) == 39 and len(anames) == 20, (len(pins), len(anames))

    out = []
    for d in sorted(pins):
        port, an = pins[d]
        a = anames.get(d)
        fn = ["Digital I/O"]
        if an:
            fn.append("<code>analogRead()</code> %s" % an)
        if d in PWM:
            fn.append("<code>analogWrite()</code> PWM (%s)" % PWM[d])
        if d == 8:
            fn.append("<code>tone()</code> &mdash; shares SCCP4 with "
                      "<code>analogWrite(8, ...)</code>")
        if d in MCPWM:
            fn.append("MCPWM %s &mdash; <em>no Arduino API on MC parts</em>" % MCPWM[d])
        if d in EXTRA:
            fn.append(EXTRA[d])

        if d in (10, 11):
            tag, cls = "Do not use", "pin-highlight"
        elif d in (37, 38):
            tag, cls = "On-board", "analog-highlight"
        elif d in (31, 32):
            tag, cls = "Serial", "pwm-highlight"
        elif d in PWM:
            tag, cls = "Analog + PWM", "pwm-highlight"
        elif d in MCPWM:
            tag, cls = "MCPWM", "pin-highlight"
        elif an:
            tag, cls = "Analog", "analog-highlight"
        elif d in EXTRA:
            tag, cls = "Peripheral", "pin-highlight"
        else:
            tag, cls = "Digital", "pin-highlight"

        out.append(ROW % ("D%d%s" % (d, " / %s" % a if a else ""),
                          port, ", ".join(fn), cls, tag))
    return out


def splice(txt):
    """Replace the marked region in part2, keeping the page's own newlines."""
    p = os.path.join(DOCS, PAGE)
    doc = read(p)
    nl = "\r\n" if "\r\n" in doc else "\n"
    i, j = doc.find(START), doc.find(END)
    assert i >= 0 and j > i, "%s: no %s .. %s region to replace" % (PAGE, START, END)
    # The opening marker is kept verbatim -- it runs to the first ">" after the
    # comment text, which is the end of the comment itself, not of a tag.
    head = doc[:doc.index("-->", i) + len("-->")]
    new = head + nl + txt.replace("\n", nl) + nl + doc[j:]
    if new == doc:
        print("  unchanged  %s" % PAGE)
        return
    io.open(p, "w", encoding="utf-8", newline="").write(new)
    print("  rewrote    %s" % PAGE)


def main():
    out = rows()
    txt = "\n".join(out)
    if "--write" in sys.argv[1:]:
        splice(txt)
        print("%d rows -> %s" % (len(out), PAGE))
        print("now run: python tools/pinmap/check_pinmap.py")
    else:
        print(txt)


if __name__ == "__main__":
    main()
