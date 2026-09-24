#!/usr/bin/env python3
"""
gen_pinmap.py - generate the EV08P02A Curiosity Nano pin map as an SVG.

Why a generator instead of a hand-drawn image:
  * the pin data has exactly one source of truth in this repo,
    variants/dspic33ck256mc005/variant.c + pins_arduino.h. Hand-editing an
    image lets the two drift silently, and a wrong pinout is worse than none.
  * SVG renders inline on GitHub, diffs as text, and stays sharp when zoomed.
  * no Microchip artwork is redistributed - see NOTES below.

The physical pad order (28 per side, USB at top) was read out of Figure 1-1 of
"dsPIC33CK256MC005 Curiosity Nano User Guide" DS70005656A by extracting the
figure's text with its coordinates, so the left/right columns and their order
match the board. Every AN channel the figure labels agrees with variant.c.

Usage:  python tools/pinmap/gen_pinmap.py
Writes: docs/img/pinmap-dspic33ck256mc005.svg
"""

import os

# --------------------------------------------------------------------------
# palette: (text, fill, stroke)
# --------------------------------------------------------------------------
C = {
    "power":   ("#b71c1c", "#ffebee", "#ef9a9a"),
    "gnd":     ("#37474f", "#eceff1", "#b0bec5"),
    "analog":  ("#1b5e20", "#e8f5e9", "#a5d6a7"),
    "pwm":     ("#e65100", "#fff3e0", "#ffcc80"),
    "mcpwm":   ("#5d4037", "#efebe9", "#bcaaa4"),
    "i2c":     ("#4a148c", "#f3e5f5", "#ce93d8"),
    "spi":     ("#0d47a1", "#e3f2fd", "#90caf9"),
    "uart":    ("#880e4f", "#fce4ec", "#f48fb1"),
    "debug":   ("#102027", "#cfd8dc", "#78909c"),
    "onboard": ("#006064", "#e0f7fa", "#80deea"),
    "nc":      ("#9e9e9e", "#fafafa", "#e0e0e0"),
    "port":    ("#263238", "#ffffff", "#90a4ae"),
    "dnum":    ("#ffffff", "#00695c", "#00695c"),
    "anum":    ("#ffffff", "#2e7d32", "#2e7d32"),
}

# --------------------------------------------------------------------------
# The two edge connectors, top (USB end) to bottom.
#   pad   : what the Curiosity Nano standard calls the pad
#   port  : MCU port pin, or None for power/NC pads
#   d     : Arduino digital pin number (variant.c g_pin_map index)
#   a     : Arduino analog name from pins_arduino.h
#   an    : ADC channel from variant.c
#   cat   : palette key for the pad chip
#   tags  : [(label, palette key)] extra functions
# --------------------------------------------------------------------------
LEFT = [
    ("NC",          None,  None, None, None,  "nc",      []),
    ("ID",          None,  None, None, None,  "nc",      [("board ID", "nc")]),
    ("CDC RX",      "RC10", 31,  None, None,  "uart",    [("Serial TX", "uart"), ("shared", "nc")]),
    ("CDC TX",      "RC11", 32,  None, None,  "uart",    [("Serial RX", "uart"), ("shared", "nc")]),
    ("ICSPCLK",     "RB6",  11,  None, None,  "debug",   [("PGC3", "debug"), ("do not use", "debug")]),
    ("DEBUG GPIO",  "RD13", 38,  None, None,  "debug",   [("SW0", "onboard"), ("shared", "nc")]),
    ("RB4",         "RB4",   9,  "A9", "AN17","analog",  [("RP36", "nc")]),
    ("RB3",         "RB3",   8,  "A8", "AN8", "pwm",     [("analogWrite", "pwm"), ("tone", "pwm")]),
    ("RC8",         "RC8",  29,  None, None,  "i2c",     [("ASDA1", "i2c"), ("SCK2", "spi")]),
    ("RC9",         "RC9",  30,  None, None,  "i2c",     [("ASCL1", "i2c"), ("SDI2", "spi")]),
    ("RC0",         "RC0",  21,  "A13","AN12","analog",  []),
    ("RC1",         "RC1",  22,  "A14","AN13","analog",  []),
    ("RC2",         "RC2",  23,  "A15","AN14","analog",  []),
    ("RC3",         "RC3",  24,  "A16","AN15","analog",  []),
    ("GND",         None,  None, None, None,  "gnd",     []),
    ("RC4",         "RC4",  25,  None, None,  "spi",     [("SPI SCK", "spi")]),
    ("RC5",         "RC5",  26,  None, None,  "spi",     [("SPI MOSI", "spi")]),
    ("RC6",         "RC6",  27,  "A17","AN19","analog",  []),
    ("RC7",         "RC7",  28,  "A18","AN7", "analog",  []),
    ("RC10",        "RC10", 31,  None, None,  "uart",    [("Serial TX", "uart"), ("shared", "nc")]),
    ("RC11",        "RC11", 32,  None, None,  "uart",    [("Serial RX", "uart"), ("shared", "nc")]),
    ("RC12",        "RC12", 33,  None, None,  "spi",     [("SPI MISO", "spi")]),
    ("RC13",        "RC13", 34,  None, None,  "spi",     [("SPI SS", "spi")]),
    ("GND",         None,  None, None, None,  "gnd",     []),
    ("RB1",         "RB1",   6,  "A6", "AN6", "pwm",     [("analogWrite", "pwm")]),
    ("RB0",         "RB0",   5,  "A5", "AN5", "pwm",     [("analogWrite", "pwm")]),
    ("RD10",        "RD10", 37,  "A19","AN18","onboard", [("LED_BUILTIN", "onboard"), ("active low", "nc")]),
    ("RD13",        "RD13", 38,  None, None,  "onboard", [("BUTTON_BUILTIN", "onboard"), ("active low", "nc")]),
]

RIGHT = [
    ("VBUS",        None,  None, None, None,  "power",   [("5 V from USB", "power")]),
    ("VOFF",        None,  None, None, None,  "power",   [("pull low = target off", "power")]),
    ("MCLR",        None,  None, None, None,  "debug",   [("reset", "debug")]),
    ("ICSPDAT",     "RB5",  10,  None, None,  "debug",   [("PGD3", "debug"), ("do not use", "debug")]),
    ("GND",         None,  None, None, None,  "gnd",     []),
    ("VTG",         None,  None, None, None,  "power",   [("3.3 V target", "power")]),
    ("RA0",         "RA0",   0,  "A0", "AN0", "analog",  []),
    ("RB2",         "RB2",   7,  "A7", "AN1", "pwm",     [("analogWrite", "pwm")]),
    ("RB7",         "RB7",  12,  "A10","AN2", "analog",  []),
    ("RB8",         "RB8",  13,  "A11","AN10","i2c",     [("SCL1", "i2c"), ("Wire", "i2c")]),
    ("RB9",         "RB9",  14,  "A12","AN11","i2c",     [("SDA1", "i2c"), ("Wire", "i2c")]),
    ("RA4",         "RA4",   4,  "A4", "AN4", "analog",  []),
    ("RA3",         "RA3",   3,  "A3", "AN3", "analog",  []),
    ("RA2",         "RA2",   2,  "A2", "AN9", "analog",  []),
    ("GND",         None,  None, None, None,  "gnd",     []),
    ("RB15",        "RB15", 20,  None, None,  "mcpwm",   [("PWM1L", "mcpwm"), ("no API", "mcpwm")]),
    ("RB14",        "RB14", 19,  None, None,  "mcpwm",   [("PWM1H", "mcpwm"), ("no API", "mcpwm")]),
    ("RB13",        "RB13", 18,  None, None,  "mcpwm",   [("PWM2L", "mcpwm"), ("no API", "mcpwm")]),
    ("RB12",        "RB12", 17,  None, None,  "mcpwm",   [("PWM2H", "mcpwm"), ("no API", "mcpwm")]),
    ("RB11",        "RB11", 16,  None, None,  "mcpwm",   [("PWM3L", "mcpwm"), ("no API", "mcpwm")]),
    ("RB10",        "RB10", 15,  None, None,  "mcpwm",   [("PWM3H", "mcpwm"), ("no API", "mcpwm")]),
    ("RD8",         "RD8",  36,  None, None,  "spi",     [("SDO2", "spi")]),
    ("RD1",         "RD1",  35,  None, None,  "mcpwm",   [("PWM4H", "mcpwm"), ("no API", "mcpwm")]),
    ("GND",         None,  None, None, None,  "gnd",     []),
    ("RA1",         "RA1",   1,  "A1", "AN16","analog",  []),
    ("NC",          None,  None, None, None,  "nc",      []),
    ("NC",          None,  None, None, None,  "nc",      []),
    ("NC",          None,  None, None, None,  "nc",      []),
]

# --------------------------------------------------------------------------
# geometry
# --------------------------------------------------------------------------
W          = 1240
ROW_H      = 24
CHIP_H     = 19
ROW0       = 196
N          = 28
BOARD_X0   = 512
BOARD_X1   = 728
BOARD_Y0   = 162
BOARD_Y1   = ROW0 + N * ROW_H + 12

# Fixed columns, measured outward from each board edge. PORT_W is sized for the
# longest pad label on the connector ("DEBUG GPIO"); fit_size() shrinks anything
# that still does not fit rather than letting it spill over the board.
PORT_W, D_W, A_W = 64, 44, 84
GAP = 5

MONO = "ui-monospace,'DejaVu Sans Mono',Menlo,Consolas,monospace"
SANS = "-apple-system,'Segoe UI',Roboto,Helvetica,Arial,sans-serif"


def esc(s):
    """Escape for SVG text content.

    Note what this does NOT do: it leaves non-ASCII alone. The output carries no
    <?xml encoding?> declaration, so a literal em dash or smart quote in a note
    string ships as raw UTF-8 and renders at the mercy of whatever the consumer
    defaults to. Every label and note here is therefore plain ASCII, and the
    typographic characters in the title block are written as numeric entities
    (&#183;, &#8212;) *outside* this function on purpose. check_pinmap.py asserts
    the finished file is ASCII, because "it looked fine in my browser" is exactly
    how the raw < in the v1.0.5 guide survived review.
    """
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def fit_size(text, w, mono, size):
    """Shrink the label until it fits its chip, so a long pad name such as
    DEBUG GPIO never spills over the board graphic."""
    f = 0.605 if mono else 0.56
    while size > 7.0 and len(text) * f * size > w - 8:
        size -= 0.5
    return size


def chip(out, x, y, w, cat, text, mono=False, size=11, bold=False):
    tc, fc, sc = C[cat]
    size = fit_size(text, w, mono, size)
    out.append(
        f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{CHIP_H}" rx="3.5" '
        f'fill="{fc}" stroke="{sc}" stroke-width="1"/>'
    )
    out.append(
        f'<text x="{x + w / 2:.1f}" y="{y + CHIP_H / 2 + 3.9:.1f}" fill="{tc}" '
        f'font-family="{MONO if mono else SANS}" font-size="{size:g}" '
        f'font-weight="{"600" if bold else "400"}" text-anchor="middle">{esc(text)}</text>'
    )


def tag_w(text):
    return max(34.0, len(text) * 6.15 + 15.0)


def render_side(out, rows, side):
    """side = -1 for the left connector (labels grow leftward), +1 for right.

    The D and A columns advance even on rows that have no such chip, so the
    columns stay aligned down the whole connector; `far` tracks the outermost
    chip actually drawn, so the lead line stops there instead of running off
    into the empty columns of a GND or NC row.
    """
    edge = BOARD_X0 if side < 0 else BOARD_X1
    for i, (pad, port, d, a, an, cat, tags) in enumerate(rows):
        y = ROW0 + i * ROW_H
        cy = y + CHIP_H / 2

        # solder pad on the board edge
        px = edge - 13 if side < 0 else edge - 5
        out.append(
            f'<rect x="{px:.1f}" y="{y + 3:.1f}" width="18" height="{CHIP_H - 6}" rx="2" '
            f'fill="#f5f5f5" stroke="#90a4ae" stroke-width="1"/>'
        )

        # Chips go into a scratch buffer so the lead line can be emitted first
        # and therefore painted *behind* them - drawn after, it strikes through
        # every label it crosses.
        row = []

        # column 1: pad / port name
        x = edge - 22 - PORT_W if side < 0 else edge + 22
        chip(row, x, y, PORT_W, cat, pad, mono=True, bold=True)
        far = x if side < 0 else x + PORT_W
        cur = x - GAP if side < 0 else x + PORT_W + GAP

        # column 2: Arduino digital pin
        if d is not None:
            x = cur - D_W if side < 0 else cur
            chip(row, x, y, D_W, "dnum", f"D{d}", mono=True, bold=True)
            far = x if side < 0 else x + D_W
        cur = (cur - D_W - GAP) if side < 0 else (cur + D_W + GAP)

        # column 3: analog name + channel
        if a is not None:
            x = cur - A_W if side < 0 else cur
            chip(row, x, y, A_W, "anum", f"{a} / {an}", mono=True)
            far = x if side < 0 else x + A_W
        cur = (cur - A_W - GAP) if side < 0 else (cur + A_W + GAP)

        # column 4: function tags, running further outward
        for label, tcat in tags:
            w = tag_w(label)
            x = cur - w if side < 0 else cur
            chip(row, x, y, w, tcat, label)
            far = x if side < 0 else x + w
            cur = (cur - w - GAP) if side < 0 else (cur + w + GAP)

        # lead line from the outermost chip back to the pad
        ex = edge - 14 if side < 0 else edge + 14
        out.append(
            f'<line x1="{far:.1f}" y1="{cy:.1f}" x2="{ex:.1f}" y2="{cy:.1f}" '
            f'stroke="#cfd8dc" stroke-width="1"/>'
        )
        out.extend(row)


def main():
    # The legend is laid out before anything is emitted, because how many rows
    # it wraps onto decides where the notes start and how tall the canvas is.
    legend = [
        ("power", "power"), ("gnd", "ground"), ("analog", "analog in"),
        ("pwm", "analogWrite PWM"), ("mcpwm", "MCPWM pin, no API"),
        ("i2c", "I2C / Wire"), ("spi", "SPI"), ("uart", "UART / Serial"),
        ("debug", "debugger - avoid"), ("onboard", "on-board LED / button"),
        ("nc", "not connected / note"),
    ]
    items = [(c, t, tag_w(t), False, False, False) for c, t in legend]
    # break=True: the two sample chips read as a pair, so start a fresh row
    # rather than letting the wrap split them.
    items.append(("dnum", "D5", float(D_W), True, True, True))
    items.append(("anum", "A5 / AN5", float(A_W), True, False, False))

    placed, x, row = [], 30.0, 0
    for cat, label, w, mono, bold, brk in items:
        if x > 30.0 and (brk or x + w > W - 30):
            row += 1
            x = 30.0
        placed.append((x, row, w, cat, label, mono, bold))
        x += w + 8

    legend_y = BOARD_Y1 + 46
    notes_y = legend_y + (row + 1) * (CHIP_H + 9) + 34
    H = notes_y + 132

    o = []
    o.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
        f'viewBox="0 0 {W} {H}" font-family="{SANS}">'
    )
    # Explicit light background: GitHub renders README images on light *and*
    # dark pages, and a transparent SVG becomes unreadable on the dark theme.
    o.append(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
    o.append(
        f'<rect x="0.5" y="0.5" width="{W - 1}" height="{H - 1}" fill="none" stroke="#e0e0e0"/>'
    )

    # ---- title ----
    o.append(
        f'<text x="{W/2}" y="46" text-anchor="middle" font-size="25" font-weight="700" '
        f'fill="#102027">Arduino_dsPIC33CK &#183; pin map</text>'
    )
    o.append(
        f'<text x="{W/2}" y="72" text-anchor="middle" font-size="14.5" fill="#455a64">'
        f'dsPIC33CK256MC005 Curiosity Nano (EV08P02A) &#8212; 48-pin TQFP, 39 usable I/O</text>'
    )
    # One plain <text> with no <tspan> children: a centred <text> whose content
    # is split across tspans mis-advances in several SVG renderers (GitHub's
    # included) and the line piles up on itself.
    o.append(
        f'<text x="{W/2}" y="93" text-anchor="middle" font-size="12" fill="#78909c">'
        f'Dn = digitalWrite / digitalRead &#183; An = analogRead &#183; '
        f'ANn = the chip&#8217;s own ADC channel</text>'
    )
    o.append(
        f'<text x="{W/2}" y="113" text-anchor="middle" font-size="11.5" fill="#90a4ae">'
        f'Generated from variants/dspic33ck256mc005/variant.c by tools/pinmap/gen_pinmap.py</text>'
    )

    # ---- board ----
    o.append(
        f'<rect x="{BOARD_X0}" y="{BOARD_Y0}" width="{BOARD_X1 - BOARD_X0}" '
        f'height="{BOARD_Y1 - BOARD_Y0}" rx="9" fill="#0f5132" stroke="#0a3d26" stroke-width="1.5"/>'
    )
    # USB connector
    o.append(
        f'<rect x="{(BOARD_X0 + BOARD_X1)/2 - 31:.0f}" y="{BOARD_Y0 - 21}" width="62" height="22" '
        f'rx="3" fill="#b0bec5" stroke="#78909c"/>'
    )
    o.append(
        f'<text x="{(BOARD_X0 + BOARD_X1)/2:.0f}" y="{BOARD_Y0 - 6}" text-anchor="middle" '
        f'font-size="10.5" font-weight="600" fill="#37474f">USB</text>'
    )
    # debugger block
    o.append(
        f'<rect x="{BOARD_X0 + 30}" y="{BOARD_Y0 + 28}" width="{BOARD_X1 - BOARD_X0 - 60}" '
        f'height="42" rx="4" fill="#123f2c" stroke="#2e7d5b"/>'
    )
    o.append(
        f'<text x="{(BOARD_X0+BOARD_X1)/2:.0f}" y="{BOARD_Y0 + 47}" text-anchor="middle" '
        f'font-size="11" font-weight="600" fill="#a5d6a7">nEDBG debugger</text>'
    )
    o.append(
        f'<text x="{(BOARD_X0+BOARD_X1)/2:.0f}" y="{BOARD_Y0 + 62}" text-anchor="middle" '
        f'font-size="10" fill="#7fb894">programmer + USB serial bridge</text>'
    )
    # MCU
    mcx, mcy = (BOARD_X0 + BOARD_X1) / 2, (BOARD_Y0 + BOARD_Y1) / 2
    o.append(
        f'<rect x="{mcx - 66:.0f}" y="{mcy - 52:.0f}" width="132" height="104" rx="5" '
        f'fill="#1c1c1c" stroke="#000"/>'
    )
    o.append(
        f'<circle cx="{mcx - 50:.0f}" cy="{mcy - 36:.0f}" r="5" fill="#424242"/>'
    )
    for i, line in enumerate(("dsPIC33CK", "256MC005", "", "48-pin TQFP")):
        if not line:
            continue
        o.append(
            f'<text x="{mcx:.0f}" y="{mcy - 10 + i * 17:.0f}" text-anchor="middle" '
            f'font-family="{MONO}" font-size="{12 if i < 2 else 10}" '
            f'fill="{"#eceff1" if i < 2 else "#90a4ae"}">{line}</text>'
        )
    # on-board LED0 / SW0
    o.append(
        f'<circle cx="{BOARD_X0 + 42}" cy="{BOARD_Y1 - 52}" r="8" fill="#ffee58" stroke="#f9a825"/>'
    )
    o.append(
        f'<text x="{BOARD_X0 + 42}" y="{BOARD_Y1 - 30}" text-anchor="middle" font-size="9.5" '
        f'font-weight="600" fill="#e0f7fa">LED0</text>'
    )
    o.append(
        f'<rect x="{BOARD_X1 - 60}" y="{BOARD_Y1 - 60}" width="17" height="17" rx="2.5" '
        f'fill="#cfd8dc" stroke="#78909c"/>'
    )
    o.append(
        f'<text x="{BOARD_X1 - 51}" y="{BOARD_Y1 - 30}" text-anchor="middle" font-size="9.5" '
        f'font-weight="600" fill="#e0f7fa">SW0</text>'
    )

    # ---- pads ----
    render_side(o, LEFT, -1)
    render_side(o, RIGHT, +1)

    # ---- legend ----
    o.append(
        f'<text x="30" y="{legend_y - 12}" font-size="13" font-weight="700" fill="#263238">Legend</text>'
    )
    for lx, lrow, w, cat, label, mono, bold in placed:
        chip(o, lx, legend_y + lrow * (CHIP_H + 9), w, cat, label, mono=mono, bold=bold)

    # ---- notes ----
    o.append(
        f'<text x="30" y="{notes_y}" font-size="13" font-weight="700" fill="#263238">'
        f'Things this board will not tell you</text>'
    )
    notes = [
        "D10 (RB5/PGD3) and D11 (RB6/PGC3) are the nEDBG debugger lines. Driving them kills programming and the "
        "serial bridge until you power-cycle. Treat them as unavailable.",
        "analogWrite() reaches four pins only: D5, D6, D7, D8 (SCCP1-4, ~490 Hz). RB10-RB15 and RD1 are motor-control "
        "PWM outputs with no Arduino API on this device - HRPWM.h is a hard error on MC parts.",
        "tone() borrows SCCP4, which is D8's PWM channel. Calling tone() stops PWM on D8, and analogWrite(8, x) stops "
        "the tone. Neither warns. D5-D7 are unaffected.",
        "D31/D32 (RC10/RC11) are wired to the debugger's USB CDC, which is what Serial talks to. The pads are labelled "
        "from the debugger's side, so the CDC RX pad is the MCU's TX.",
        "Serial is 63 bytes of usable RX buffer and the ISR drops the NEWEST byte on overflow, so a long burst loses "
        "its terminating newline.",
        "Peripheral Pin Select can move most digital peripherals to other RPn pins; PORTA has no RPn on any device in "
        "this family, so D0-D4 cannot host a remappable function.",
    ]
    for i, n in enumerate(notes):
        o.append(
            f'<text x="30" y="{notes_y + 22 + i * 17.5:.0f}" font-size="11.3" fill="#455a64">'
            f'<tspan fill="#90a4ae">&#8226; </tspan>{esc(n)}</text>'
        )

    o.append("</svg>")

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    dst = os.path.join(root, "docs", "img", "pinmap-dspic33ck256mc005.svg")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(o) + "\n")
    print(f"wrote {dst}  ({os.path.getsize(dst)} bytes)")


if __name__ == "__main__":
    main()
