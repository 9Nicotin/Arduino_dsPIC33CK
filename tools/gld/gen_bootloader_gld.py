#!/usr/bin/env python3
"""Generate the bootloader and application linker scripts for a dsPIC33CK device.

Both scripts are *derived* from the device's stock .gld in the Device Family Pack
rather than written from scratch. The stock script is ~3900 lines of section map,
PSV handling and SFR equates that we have no business reimplementing; all we need
to change is the memory map and the vector table.

Why two scripts
---------------
The IVT is fixed at 0x004-0x1FF and shares erase page 0 with the reset vector. If
the application owned the IVT, updating it would mean erasing the page holding the
bootloader's own reset vector -- a power-loss brick window. So instead:

  * the BOOTLOADER owns page 0 forever. Its IVT slots hold *constant* addresses
    pointing into a fixed-address GOTO trampoline in the application region. Page 0
    is programmed once, when the bootloader is burned, and never erased again.

  * the APPLICATION emits that trampoline, rebuilt on every upload, so sketches and
    cores/ keep using the ordinary vector names (_T1Interrupt, _U1RXInterrupt, ...).
    Nothing in user code changes. This is what rules out the AIVT alternative, whose
    entries can only be filled from _Alt-prefixed names.

Cost: one GOTO of extra interrupt latency (2 instruction cycles) and ~1.2 KB of
application flash.

Memory map produced (addresses are program-space address units; one 24-bit
instruction word occupies two of them, and an erase page is 0x800):

    0x000000  reset vector      GOTO bootloader             ) bootloader-owned,
    0x000004  IVT               constants -> trampoline     ) written ONCE,
    0x000200  bootloader code                               ) never erased
    --------  ---- page boundary at APP_BASE ----
    0x001800  app entry         GOTO application __reset    ) application region,
    0x001804  trampoline        one GOTO per vector         ) erased and rewritten
    0x001Bxx  application code  flows on from the trampoline) on every upload
    0x02B700  signature         magic / length / CRC-32     )
    --------  ---- page boundary; everything above is NEVER erased ----
    0x02B800  unused            shares an erase page with the config words
    0x02BF00  config words      never written by the bootloader

The application region stops at the last page boundary BELOW the config words, not at
the end of the stock program region. The config words sit at 0x02BF00, inside the erase
page that starts at 0x02B800, so a page erase anywhere in 0x02B800-0x02BFFF takes
FOSCSEL/FOSC/FWDT/FICD with it. An erased FWDTEN reads as "watchdog enabled", so an
upload that reached that page would leave the board resetting itself in a loop. The
0x700 address units (896 instruction words, ~1% of flash) between the signature row
and the config page are therefore permanently unused -- the price of being able to
erase the whole application region with plain page erases and no special cases.

The signature lives in the LAST row of the application region, not in a slot carved out
after the trampoline. That is deliberate: `-ffunction-sections` makes every function
its own section, and the stock .text output section matches only .init/.user_init/
.handle/.isr*/.lib*, so ordinary code sections are *orphans*. GNU ld drops orphans
into the first region with room -- which means any gap left inside the application
region gets quietly filled with code. Keeping the region contiguous and parking the
signature at the top, where code only grows away from it, removes that trap. It also
gives the signature a row of its own, which matters because a row can only be
programmed once per erase: the uploader must be able to write the signature at COMMIT
without touching a row that already holds code.

The GOTO encoding is not invented here: it is copied from the stock script's own
.reset section, which spells out that a GOTO is two instruction words, the first
carrying 0x04 in its upper byte and the low 16 bits of the target, the second the
remaining high bits masked to 0x7F.

Usage:
    python tools/gld/gen_bootloader_gld.py                     # MC005, default paths
    python tools/gld/gen_bootloader_gld.py --check             # regenerate and diff only
"""

import argparse
import os
import re
import sys

# --- device table -----------------------------------------------------------
#
# Only MC005 is supported today. The other three devices are deliberately absent:
# each needs its own built bootloader binary and hardware verification, and only
# MC005 is on the bench. Adding one is a table entry plus a build, not a redesign.
#
# app_base must be a page boundary (0x800), because erase granularity is a page and
# the application must never be able to erase a page the bootloader lives in.
#
# The pack version is pinned to the one the platform actually ships as a tool
# (package_microchip_dspic33ck_index.json), not to whatever else is installed: the
# stock .gld is not identical across pack versions, and these scripts are derived
# from it.

DEVICES = {
    "33CK256MC005": {
        "pack": "dsPIC33CK-MC_DFP/1.11.412",
        "app_base": 0x1800,   # bootloader owns pages 0-2; application starts here
    },
}

DEFAULT_DEVICE = "33CK256MC005"
PAGE = 0x800              # erase page, in address units (1024 instruction words)
ROW = 0x100               # write row, in address units (128 instruction words)
GOTO_SIZE = 4             # a GOTO is 2 instruction words = 4 address units


def packs_root():
    return os.path.join(os.path.expanduser("~"), ".mchp_packs", "Microchip")


def stock_gld_path(dev):
    return os.path.join(packs_root(), DEVICES[dev]["pack"],
                        "xc16", "support", "dsPIC33C", "gld", "p%s.gld" % dev)


# --- parsing the stock script ----------------------------------------------

def parse_program_region(text):
    """Return (origin, length) of the program region from the stock MEMORY block."""
    m = re.search(r"^\s*program\s*\(xr\)\s*:\s*ORIGIN\s*=\s*(0x[0-9A-Fa-f]+)\s*,"
                  r"\s*LENGTH\s*=\s*(0x[0-9A-Fa-f]+)", text, re.M)
    if not m:
        sys.exit("FAIL: could not find the program region in the stock linker script")
    return int(m.group(1), 16), int(m.group(2), 16)


def parse_vectors(text):
    """Return the ordered list of vector symbol names from the stock .ivt block.

    The block lives inside `#if __XC16_VERSION < 1026`, i.e. it is dead code for
    XC-DSC v4.00 -- the modern linker builds the table itself. It is still the only
    machine-readable, correctly ordered, device-specific list of vector names there
    is, which is exactly what we need. Reading it beats transcribing 200 names in a
    fixed order by hand, which is how this kind of file goes silently wrong.
    """
    try:
        start = text.index(".ivt __IVT_BASE :")
        end = text.index("} >ivt", start)
    except ValueError:
        sys.exit("FAIL: could not locate the .ivt block in the stock linker script")

    names = re.findall(r"LONG\(\s*DEFINED\((__\w+)\)", text[start:end])
    if not names:
        sys.exit("FAIL: found the .ivt block but no vector entries in it")

    dupes = len(names) - len(set(names))
    if dupes:
        sys.exit("FAIL: %d duplicate vector names extracted -- parse is wrong" % dupes)

    # The ivt region is 0x1FC address units = 254 slots; a device need not define
    # them all (MC005 defines 200). Refuse to overflow the region.
    if len(names) * 2 > 0x1FC:
        sys.exit("FAIL: %d vectors need 0x%X address units, ivt region holds 0x1FC"
                 % (len(names), len(names) * 2))
    return names


# --- emitting -------------------------------------------------------------

def goto_words(expr, indent="    "):
    """Emit the four SHORTs of a GOTO <expr>, per the stock .reset section."""
    return ("{i}SHORT({e} & 0xFFFF);\n"
            "{i}SHORT(0x04);\n"
            "{i}SHORT(({e} >> 16) & 0x7F);\n"
            "{i}SHORT(0);\n").format(i=indent, e=expr)


def patch_memory(text, replacements, drop=()):
    """Rewrite ORIGIN/LENGTH of named MEMORY regions and delete named ones."""
    out = []
    seen = set()
    for line in text.split("\n"):
        m = re.match(r"^(\s*)(\w+)(\s*(?:\([a-z!]+\))?\s*):\s*ORIGIN", line)
        name = m.group(2) if m else None
        if name:
            seen.add(name)

        if name in drop:
            continue
        if name in replacements:
            origin, length = replacements[name]
            decl = m.group(2) + m.group(3)
            line = "%s%s: ORIGIN = 0x%X,%sLENGTH = 0x%X" % (
                m.group(1), decl, origin,
                " " * max(1, 14 - len("0x%X," % origin)), length)
        out.append(line)

    missing = (set(replacements) | set(drop)) - seen
    if missing:
        sys.exit("FAIL: MEMORY regions not found in the stock script: %s"
                 % ", ".join(sorted(missing)))
    return "\n".join(out)


def build_boot_gld(stock, vectors, app_base, tramp_base):
    """Bootloader script: owns reset + IVT, code confined below app_base."""
    prog_origin, _ = parse_program_region(stock)

    text = patch_memory(stock, {"program": (prog_origin, app_base - prog_origin)})

    # Slot n forwards to trampoline entry n, which sits one GOTO past the app entry
    # point. These are constants: the bootloader's IVT never needs rewriting when a
    # new application is uploaded, which is the whole point of the design.
    lines = [
        "",
        "/*",
        "** Bootloader interrupt vector table  (generated -- do not edit)",
        "**",
        "** Every slot holds the CONSTANT address of its GOTO trampoline entry in the",
        "** application region. Nothing here depends on the application, so page 0 is",
        "** programmed once when the bootloader is burned and is never erased again.",
        "** That is what removes the power-loss brick window.",
        "**",
        "** Build with -Wl,--no-ivt so the linker does not also generate a table.",
        "*/",
        "SECTIONS",
        "{",
        "  .ivt __IVT_BASE :",
        "  {",
    ]
    for n, name in enumerate(vectors):
        lines.append("    LONG(0x%06X);  /* %-3d %s */"
                     % (tramp_base + GOTO_SIZE * n, n, name[1:]))

    # Slots this device does not define. They cannot fire; if one somehow does,
    # vectoring to 0x0 re-enters the bootloader through its own reset GOTO, which is
    # the sanest available outcome and costs no application flash.
    used = len(vectors) * 2
    spare = (0x1FC - used) // 2
    if spare:
        lines.append("")
        lines.append("    /* %d slots undefined on this device -> re-enter bootloader */"
                     % spare)
        for n in range(spare):
            lines.append("    LONG(0x000000);")

    lines += ["  } >ivt", "} /* SECTIONS */", ""]
    return text + "\n".join(lines)


def build_app_gld(stock, vectors, app_base, sig_base):
    """Application script: no reset vector, no IVT, GOTO trampoline at a fixed address."""
    # The application must not be able to place anything in page 0, so the reset and
    # ivt regions are removed outright -- if the linker ever tries, it fails loudly
    # instead of silently emitting a second reset vector. The program region is
    # trimmed at the signature row so code can never reach it.
    text = patch_memory(stock,
                        {"program": (app_base, sig_base - app_base)},
                        drop=("reset", "ivt"))

    # Drop the stock .reset section: the bootloader owns the reset vector.
    old = "#if !defined(__CORESIDENT) || defined(__DEFINE_RESET)"
    if old not in text:
        sys.exit("FAIL: could not find the .reset guard to disable in the stock script")
    text = text.replace(
        old,
        "/* Reset vector belongs to the bootloader, not to the application. */\n#if 0",
        1)

    # __CODE_BASE is informational in the stock script, but leaving it at 0x200 in a
    # script whose code starts elsewhere is a trap for whoever reads this next.
    text = re.sub(r"^__CODE_BASE\s*=\s*0x[0-9A-Fa-f]+;",
                  "__CODE_BASE = 0x%X;" % app_base, text, count=1, flags=re.M)

    tramp_end = app_base + GOTO_SIZE * (len(vectors) + 1)
    head = [
        "",
        "/*",
        "** Application entry point and interrupt trampoline  (generated -- do not edit)",
        "**",
        "** The bootloader's IVT slots hold fixed addresses in this table, so its layout",
        "** is a contract with the bootloader binary and must not be reordered:",
        "**",
        "**   0x%06X   GOTO application __reset   <- the bootloader's only jump target"
        % app_base,
        "**   0x%06X   GOTO handler for vector 0" % (app_base + GOTO_SIZE),
        "**   ...        %d vectors, %d address units each, ending at 0x%06X"
        % (len(vectors), GOTO_SIZE, tramp_end),
        "**   0x%06X   application code follows on immediately -- NO gap, see below"
        % tramp_end,
        "**   0x%06X   signature row, written by the uploader at COMMIT" % sig_base,
        "**",
        "** .trampoline is emitted FIRST, inside the stock SECTIONS, so it takes the",
        "** base of the program region and every other section flows on after it. It",
        "** deliberately gets no region of its own and no reserved slack: with",
        "** -ffunction-sections, code sections are orphans that GNU ld drops into the",
        "** first region with room, so any gap here would silently fill with code.",
        "**",
        "** Vectors the application does not handle fall back to the linker's default",
        "** handler, exactly as they do in the stock script.",
        "**",
        "** The per-vector targets are resolved into __tvN symbols HERE, at file scope,",
        "** and not inside .trampoline. GNU ld makes a symbol assigned inside an output",
        "** section relative to that section's base, so the same assignment written in",
        "** there comes out biased by 0x%06X -- a wrong GOTO that still disassembles" % app_base,
        "** as a plausible one. Expressions inside SHORT() are not affected, only",
        "** assignments, which is why only the fallback entries would have been wrong.",
        "**",
        "** Build with -Wl,--no-ivt: there is no ivt region to put a table in.",
        "*/",
        "__tramp_default = DEFINED(__DefaultInterrupt) ? ABSOLUTE(__DefaultInterrupt)",
        "                : (DEFINED(__reset) ? ABSOLUTE(__reset) : 0);",
        "",
    ]
    for n, name in enumerate(vectors):
        head.append("__tv%-3d = DEFINED(%s) ? ABSOLUTE(%s) : __tramp_default;"
                    % (n, name, name))
    head.append("")

    body = [
        "",
        "  /* Generated: see the trampoline note just above SECTIONS. */",
        "  .trampoline :",
        "  {",
        "    /* Application entry point -- the bootloader's only jump target. */",
        goto_words("ABSOLUTE(__reset)").rstrip("\n"),
    ]
    for n, name in enumerate(vectors):
        body.append("")
        body.append("    /* %-3d %s */" % (n, name[1:]))
        body.append(goto_words("__tv%d" % n).rstrip("\n"))
    body += ["  } >program", ""]

    # The note and the __tvN resolution go immediately above SECTIONS; the section
    # itself goes at the very top of the stock SECTIONS body so it is laid out first.
    m = re.search(r"^SECTIONS\s*\n\{\s*$", text, re.M)
    if not m:
        sys.exit("FAIL: could not find the SECTIONS block opening in the stock script")
    return (text[:m.start()] + "\n".join(head)
            + text[m.start():m.end()] + "\n".join(body) + text[m.end():])


# --- main -----------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--device", default=DEFAULT_DEVICE, choices=sorted(DEVICES))
    ap.add_argument("--out-dir", default=os.path.join(
        "arduino-platform", "microchip", "dspic33ck", "ldscripts"))
    ap.add_argument("--check", action="store_true",
                    help="fail if the committed scripts differ from freshly generated ones")
    args = ap.parse_args()

    dev = args.device
    spec = DEVICES[dev]
    src = stock_gld_path(dev)
    if not os.path.isfile(src):
        sys.exit("FAIL: stock linker script not found: %s\n"
                 "      Is the Device Family Pack installed?" % src)

    with open(src, "r", encoding="utf-8", errors="replace") as f:
        stock = f.read()

    app_base = spec["app_base"]
    if app_base % PAGE:
        sys.exit("FAIL: app_base 0x%X is not on a 0x%X erase-page boundary"
                 % (app_base, PAGE))

    vectors = parse_vectors(stock)
    prog_origin, prog_len = parse_program_region(stock)
    prog_end = prog_origin + prog_len

    # The application region must end on an erase-page boundary, and it must not reach
    # into the page that holds the config words. On MC005 the program region ends at
    # 0x2BF00, which is where cfgmem begins -- in the MIDDLE of the page starting at
    # 0x2B800. Erasing that page would clear FOSCSEL/FOSC/FWDT/FICD; an erased FWDTEN
    # reads as "watchdog on", which would reset the application in a loop. So the app
    # stops at the page boundary below the config words and the remainder is unused.
    # (If a device's cfgmem happened to be page-aligned, nothing is lost here.)
    app_end = (prog_end // PAGE) * PAGE
    if app_end <= app_base:
        sys.exit("FAIL: no whole erase page available for the application")

    # The signature occupies the last write row of the application region. It must be a
    # row of its own: a row can only be programmed once per page erase, so the
    # uploader cannot write the signature into a row that already holds code.
    sig_base = app_end - ROW
    if sig_base % ROW:
        sys.exit("FAIL: signature row 0x%X is not on a 0x%X row boundary"
                 % (sig_base, ROW))

    tramp_len = GOTO_SIZE * (len(vectors) + 1)
    if app_base + tramp_len >= sig_base:
        sys.exit("FAIL: trampoline does not fit in the application region")

    print("device      : %s" % dev)
    print("stock gld   : %s" % src)
    print("vectors     : %d (ivt region holds %d slots)" % (len(vectors), 0x1FC // 2))
    print("program     : 0x%06X - 0x%06X (stock)" % (prog_origin, prog_end))
    print("bootloader  : 0x000000 - 0x%06X  (%d pages, 0x%X for code)"
          % (app_base, app_base // PAGE, app_base - prog_origin))
    print("app entry   : 0x%06X" % app_base)
    print("trampoline  : 0x%06X - 0x%06X (0x%X)"
          % (app_base + GOTO_SIZE, app_base + tramp_len, tramp_len - GOTO_SIZE))
    print("app code    : 0x%06X - 0x%06X (0x%X available)"
          % (app_base + tramp_len, sig_base, sig_base - app_base - tramp_len))
    print("signature   : 0x%06X - 0x%06X" % (sig_base, sig_base + ROW))
    print("erase range : 0x%06X - 0x%06X (%d pages)"
          % (app_base, app_end, (app_end - app_base) // PAGE))
    if app_end != prog_end:
        print("unused      : 0x%06X - 0x%06X (0x%X, shares the config words' erase page)"
              % (app_end, prog_end, prog_end - app_end))

    outputs = {
        "p%s-boot.gld" % dev: build_boot_gld(
            stock, vectors, app_base, app_base + GOTO_SIZE),
        "p%s-app.gld" % dev: build_app_gld(stock, vectors, app_base, sig_base),
    }

    rc = 0
    for name, body in outputs.items():
        path = os.path.join(args.out_dir, name)
        if args.check:
            if not os.path.isfile(path):
                print("DIFF missing: %s" % path)
                rc = 1
                continue
            with open(path, "r", encoding="utf-8", newline="") as f:
                have = f.read().replace("\r\n", "\n")
            if have != body:
                print("DIFF %s is not what the generator produces" % path)
                rc = 1
            else:
                print("OK   %s" % path)
        else:
            os.makedirs(args.out_dir, exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(body)
            print("wrote %s (%d lines)" % (path, body.count("\n") + 1))
    return rc


if __name__ == "__main__":
    sys.exit(main())
