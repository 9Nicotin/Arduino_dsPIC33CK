# Emit part3 section 3.1's per-device facts table from the four variants'
# pins_arduino.h. Nothing in here is typed twice: every number and port name is
# read out of the header, and only the prose annotations are hand-written.
#
# v1.0.5 shipped part3 as an MP102 document wearing a platform title -- "pin
# 0-20", "A0 is the same physical pin as D5", "LED_BUILTIN | 0 | Pin D0 (RA0)".
# On the priority device every one of those is wrong. Generating the table is
# half the fix; tools/pinmap/check_pinmap.py check F is the other half, because
# a generated table still rots the moment someone edits the HTML by hand.
#
# Usage:
#   python tools/docs/gen_device_table.py            print both regions
#   python tools/docs/gen_device_table.py --write    splice them into the pages
#
# --write is the point of keeping this in the repository rather than in a
# scratch directory: the two regions it owns are committed HTML, so the way to
# change them has to be runnable by whoever reads the marker comment.
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
DOCS = os.path.join(REPO, "arduino-platform", "docs")
VAR = os.path.join(REPO, "arduino-platform", "microchip", "dspic33ck", "variants")

# MC005 first: it is the priority device, so it is the column a reader's eye
# lands on. The rest are in the order boards.txt lists them. No package sizes
# here -- the board name is only printed where the variant header names one.
BOARDS = [
    ("dspic33ck256mc005", "dsPIC33CK256MC005"),
    ("dspic33ck256mc002", "dsPIC33CK256MC002"),
    ("dspic33ck32mp102",  "dsPIC33CK32MP102"),
    ("dspic33ck256mp508", "dsPIC33CK256MP508"),
]


def read(p):
    return io.open(p, encoding="utf-8", newline="").read()


def load(slug):
    ph = read(os.path.join(VAR, slug, "pins_arduino.h"))
    vc = read(os.path.join(VAR, slug, "variant.c"))
    d = {}
    for m in re.finditer(r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+\(?(\d+)\)?\s*(?:/\*.*)?$",
                         ph, re.M):
        d.setdefault(m.group(1), m.group(2))
    # D-number -> port name, straight from the variant.c row comments that
    # check A already proves line up with g_pin_map.
    ports = {int(a): "R" + b for a, b in
             re.findall(r"/\*\s*D(\d+):\s*R([A-E]\d+)\s*\*/", vc)}
    pwm = sorted(int(v) for k, v in d.items() if re.match(r"^PWM\d+_PIN$", k))
    # Whether LED_BUILTIN is a real board LED is a fact the header states in the
    # comment above it -- MC005 "LED0 on EV08P02A Curiosity Nano", MP508 "LED1 on
    # DM330030 Curiosity Board", and the other two just "default to RA0". Keying
    # this off LED_BUILTIN_ACTIVE_LOW instead got MP508 wrong: it has a board LED
    # and no active-low flag.
    m = re.search(r"/\*[^*]*?(LED\d+)\s+on\s+([^-,.*()\n]+?)\s*[-,.*)\n]", ph)
    d["_led_board"] = "%s on the %s" % (m.group(1), m.group(2).strip()) if m else None
    m = re.search(r"/\*[^*]*?(SW\d+)\s+on\s+([^-,.*()\n]+?)\s*[-,.*)\n]", ph)
    d["_btn_board"] = "%s on the %s" % (m.group(1), m.group(2).strip()) if m else None
    return d, ports, pwm


def dport(d, ports, key):
    """'D37 (RD10)' for a #define naming a pin, or None if the macro is absent."""
    if key not in d:
        return None
    n = int(d[key])
    p = ports.get(n)
    return "D%d (%s)" % (n, p) if p else "D%d" % n


def runs(nums):
    """[5,6,7,8,58] -> 'D5-D8, D58'."""
    out, i = [], 0
    while i < len(nums):
        j = i
        while j + 1 < len(nums) and nums[j + 1] == nums[j] + 1:
            j += 1
        out.append("D%d" % nums[i] if j == i else "D%d-D%d" % (nums[i], nums[j]))
        i = j + 1
    return ", ".join(out)


def cells():
    rows = []           # (label, {slug: cell})
    data = {slug: load(slug) for slug, _ in BOARDS}

    def row(label, fn):
        rows.append((label, {slug: fn(*data[slug]) for slug, _ in BOARDS}))

    row("<code>NUM_DIGITAL_PINS</code>",
        lambda d, p, w: "%s pins: D0-D%d" % (d["NUM_DIGITAL_PINS"],
                                             int(d["NUM_DIGITAL_PINS"]) - 1))
    row("<code>NUM_ANALOG_INPUTS</code>",
        lambda d, p, w: "%s channels: A0-A%d" % (d["NUM_ANALOG_INPUTS"],
                                                 int(d["NUM_ANALOG_INPUTS"]) - 1))
    row("<code>A0</code> is which digital pin",
        lambda d, p, w: dport(d, p, "A0"))

    def led(d, p, w):
        c = dport(d, p, "LED_BUILTIN")
        if d["_led_board"]:
            c += " &mdash; %s" % d["_led_board"]
        else:
            c += " &mdash; a default, not a board LED: wire your own"
        if "LED_BUILTIN_ACTIVE_LOW" in d:
            c += ", <strong>active LOW</strong>"
        return c
    row("<code>LED_BUILTIN</code>", led)

    def btn(d, p, w):
        c = dport(d, p, "BUTTON_BUILTIN")
        if c is None:
            return "not defined"
        if d["_btn_board"]:
            c += " &mdash; %s" % d["_btn_board"]
        if "BUTTON_BUILTIN_ACTIVE_LOW" in d:
            c += ", <strong>active LOW</strong>"
        return c
    row("<code>BUTTON_BUILTIN</code>", btn)

    row("<code>Serial</code> TX &mdash; <code>PIN_SERIAL_TX</code>",
        lambda d, p, w: dport(d, p, "PIN_SERIAL_TX"))
    row("<code>Serial</code> RX &mdash; <code>PIN_SERIAL_RX</code>",
        lambda d, p, w: dport(d, p, "PIN_SERIAL_RX"))
    row("<code>analogWrite()</code> pins", lambda d, p, w: runs(w))
    for k, lbl in (("PIN_SPI_SCK", "SPI SCK"), ("PIN_SPI_MOSI", "SPI MOSI"),
                   ("PIN_SPI_MISO", "SPI MISO"), ("PIN_SPI_SS", "SPI SS"),
                   ("PIN_WIRE_SDA", "Wire SDA"), ("PIN_WIRE_SCL", "Wire SCL")):
        row("%s &mdash; <code>%s</code>" % (lbl, k),
            lambda d, p, w, k=k: dport(d, p, k))
    return rows


BOARDS_TXT = os.path.join(REPO, "arduino-platform", "microchip", "dspic33ck", "boards.txt")

W = 71          # card inner width; borders make the line 73 columns


def boards_txt(slug, key):
    m = re.search(r"^%s\.%s=(.*)$" % (re.escape(slug), re.escape(key)),
                  read(BOARDS_TXT), re.M)
    assert m, "%s.%s missing from boards.txt" % (slug, key)
    return m.group(1).strip()


def card(slug, disp):
    """part5 5.8's quick-reference card for one device, as fixed-width lines.

    The numbers are generated for the same reason the table is: the v1.0.5 card
    printed "DIGITAL I/O: 21 pins (D0-D20)" and "LED_BUILTIN: D0 (RA0)" as the
    platform's, on the only card in the guide. check F re-derives every D-number
    on these lines from pins_arduino.h.
    """
    d, ports, pwm = load(slug)
    nd, na = int(d["NUM_DIGITAL_PINS"]), int(d["NUM_ANALOG_INPUTS"])
    led = "D%s" % d["LED_BUILTIN"]
    if "LED_BUILTIN_ACTIVE_LOW" in d:
        led += " (active LOW)"
    btn = ("D%s" % d["BUTTON_BUILTIN"]) if "BUTTON_BUILTIN" in d else "none"
    if "BUTTON_BUILTIN" in d and "BUTTON_BUILTIN_ACTIVE_LOW" in d:
        btn += " (active LOW)"
    lines = [
        ("DEVICE", disp),
        ("SIZE BAR", "%s B flash / %s B RAM (as the IDE reports)"
         % (boards_txt(slug, "upload.maximum_size"),
            boards_txt(slug, "upload.maximum_data_size"))),
        None,
        ("DIGITAL I/O", "%d pins (D0-D%d)" % (nd, nd - 1)),
        ("ANALOG IN", "%d channels (A0-A%d), A0 = D%s" % (na, na - 1, d["A0"])),
        ("PWM OUT", "%s (SCCP)" % runs(pwm)),
        ("SERIAL", "TX = D%s (%s), RX = D%s (%s)"
         % (d["PIN_SERIAL_TX"], ports[int(d["PIN_SERIAL_TX"])],
            d["PIN_SERIAL_RX"], ports[int(d["PIN_SERIAL_RX"])])),
        ("SPI", "SCK = D%s, MOSI = D%s, MISO = D%s, SS = D%s"
         % (d["PIN_SPI_SCK"], d["PIN_SPI_MOSI"], d["PIN_SPI_MISO"], d["PIN_SPI_SS"])),
        ("I2C", "SDA = D%s, SCL = D%s" % (d["PIN_WIRE_SDA"], d["PIN_WIRE_SCL"])),
        ("LED_BUILTIN", led),
        ("BUTTON_BUILTIN", btn),
    ]
    out = ["+" + "=" * W + "+"]
    for ln in lines:
        if ln is None:
            out.append("+" + "-" * W + "+")
            continue
        body = "%-15s %s" % (ln[0] + ":", ln[1])
        assert len(body) <= W - 2, "card line too wide: %r" % body
        out.append("| %-*s|" % (W - 1, body))
    out.append("+" + "=" * W + "+")
    return out


def splice(page, start, end, txt):
    """Replace one marked region in a doc page, keeping the page's own newlines."""
    p = os.path.join(DOCS, page)
    doc = io.open(p, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in doc else "\n"
    i, j = doc.find(start), doc.find(end)
    assert i >= 0 and j > i, "%s: no %s .. %s region to replace" % (page, start, end)
    new = doc[:i] + txt.replace("\n", nl).rstrip(nl) + doc[j + len(end):]
    if new == doc:
        print("  unchanged  %s" % page)
        return
    io.open(p, "w", encoding="utf-8", newline="").write(new)
    print("  rewrote    %s" % page)


def main():
    write = "--write" in sys.argv[1:]
    rows = cells()
    out = ['<!-- BEGIN device-facts: generated by tools/docs/gen_device_table.py,',
           '     checked by tools/pinmap/check_pinmap.py check F. Do not hand-edit. -->',
           '<table>',
           '    <tr><th>Constant</th>%s</tr>'
           % "".join("<th>%s</th>" % disp for _, disp in BOARDS)]
    for label, cs in rows:
        out.append("    <tr><td>%s</td>%s</tr>"
                   % (label, "".join("<td>%s</td>" % cs[slug] for slug, _ in BOARDS)))
    out += ['</table>', '<!-- END device-facts -->']
    txt = "\n".join(out) + "\n"
    assert all(ord(c) < 128 for c in txt), "non-ASCII leaked into the table"
    if write:
        splice("part3_api_reference.html",
               "<!-- BEGIN device-facts", "<!-- END device-facts -->", txt)
    else:
        print(txt)

    # part5 5.8. One card per board, in BOARDS order, so the priority device is
    # first and the MP102 -- the device the single v1.0.5 card was silently
    # written for -- is no longer the only alternative a reader is offered. An
    # earlier pass emitted two and left 5.8 saying "one card per board", which is
    # the same class of defect as the v1.0.5 pages themselves: prose describing a
    # device set the artefact does not cover. Cards are generated and gated, so
    # the two held boards cost nothing to carry.
    # HTML comments inside <pre> are stripped by the parser, so the markers do
    # not show up on the page.
    c = ['<!-- BEGIN device-cards: generated by tools/docs/gen_device_table.py,',
         '     checked by tools/pinmap/check_pinmap.py check G. Do not hand-edit. -->']
    for slug, disp in BOARDS:
        c.append('<pre><code>')
        c += card(slug, disp)
        c.append('</code></pre>')
    c.append('<!-- END device-cards -->')
    txt = "\n".join(c) + "\n"
    assert all(ord(ch) < 128 for ch in txt), "non-ASCII leaked into the cards"
    if write:
        splice("part5_upload_troubleshooting.html",
               "<!-- BEGIN device-cards", "<!-- END device-cards -->", txt)
        print("now run: python tools/pinmap/check_pinmap.py")
    else:
        print(txt)


if __name__ == "__main__":
    main()
