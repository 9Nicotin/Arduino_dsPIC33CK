#!/usr/bin/env bash
# ============================================================================
# make-release.sh - build the three Board Manager archives and fill in the
# package index.
#
# Produces, in _build/release/:
#   dspic33ck-arduino-core-<ver>.zip          the platform itself
#   dsPIC33CK-MP_DFP-<ver>-pruned.N.zip       pruned Device Family Pack
#   dsPIC33CK-MC_DFP-<ver>-pruned.N.zip       pruned Device Family Pack
#   package_microchip_dspic33ck_index.json    a copy of the filled-in index
#
# and rewrites the size/checksum fields of the in-repo index in place, so the
# checked-in file is always a working index rather than a template.
#
# WHY THIS IS A SCRIPT
# Six size/checksum fields across three archives is exactly the amount of
# hand-maintenance that goes silently wrong, and a wrong checksum fails the
# user's install with an opaque message rather than a useful one. Everything
# here is derived: the version from platform.txt, the archive names, URLs and
# tool versions from the index, and the pruned pack device lists are checked
# against boards.txt so adding a board cannot quietly skip its DFP slice.
#
# The zips are built with fixed timestamps, sorted entries and forward-slash
# paths, so rebuilding the same tree twice gives byte-identical archives and
# therefore stable checksums.
#
# Requires: python3 (for zipping and JSON), the full DFPs installed locally.
#
#   ./tools/release/make-release.sh
#   MCHP_PACKS=D:/packs/Microchip ./tools/release/make-release.sh
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
SRC="$REPO/arduino-platform/microchip/dspic33ck"
IDX="$REPO/arduino-platform/package_microchip_dspic33ck_index.json"
OUT="$REPO/_build/release"
PACKS="${MCHP_PACKS:-$HOME/.mchp_packs/Microchip}"

PY=python
command -v python >/dev/null 2>&1 || PY=python3

# Which devices go in which pruned pack. Cross-checked against boards.txt below,
# so this list cannot silently fall behind the boards the platform offers.
MP_DEVICES="33CK32MP102 33CK256MP508"
MC_DEVICES="33CK256MC002 33CK256MC005"

# --- read the version, and the archive names/versions out of the index -------
VERSION="$(sed -n 's/^version=\(.*\)$/\1/p' "$SRC/platform.txt" | head -1)"
[ -n "$VERSION" ] || { echo "no version= in platform.txt" >&2; exit 1; }
TAG="v$VERSION"

rm -rf "$OUT"; mkdir -p "$OUT"

"$PY" - "$IDX" "$VERSION" "$TAG" > "$OUT/manifest" <<'PY'
import json, sys
idx, version, tag = sys.argv[1:4]
d = json.load(open(idx, encoding='utf-8'))
pkg = d['packages'][0]
pl = pkg['platforms'][0]
if pl['version'] != version:
    sys.exit('index platform version %r != platform.txt %r' % (pl['version'], version))
def check_url(url, archive):
    want = '/releases/download/%s/%s' % (tag, archive)
    if not url.endswith(want):
        sys.exit('url does not end with %r:\n  %s' % (want, url))
check_url(pl['url'], pl['archiveFileName'])
print('PLATFORM_ARCHIVE=%s' % pl['archiveFileName'])
deps = {t['name']: t['version'] for t in pl['toolsDependencies']}
for t in pkg['tools']:
    if deps.get(t['name']) != t['version']:
        sys.exit('tool %s version %s is not the one the platform depends on (%s)'
                 % (t['name'], t['version'], deps.get(t['name'])))
    if len(t['systems']) != 1 or t['systems'][0]['host'] != 'i686-mingw32':
        sys.exit('tool %s: expected exactly one i686-mingw32 system entry '
                 '(this platform is Windows-only by design)' % t['name'])
    s = t['systems'][0]
    check_url(s['url'], s['archiveFileName'])
    # name | full tool version | upstream pack version | archive file name
    upstream = t['version'].split('-pruned')[0]
    print('TOOL=%s|%s|%s|%s' % (t['name'], t['version'], upstream, s['archiveFileName']))
PY
# shellcheck disable=SC1090
PLATFORM_ARCHIVE=""
TOOLS=()
while IFS= read -r line; do
  # python's stdout is a text stream on Windows, so every line arrives CRLF.
  line="${line%$'\r'}"
  case "$line" in
    PLATFORM_ARCHIVE=*) PLATFORM_ARCHIVE="${line#PLATFORM_ARCHIVE=}" ;;
    TOOL=*)             TOOLS+=("${line#TOOL=}") ;;
  esac
done < "$OUT/manifest"

# --- boards.txt cross-check --------------------------------------------------
# Every device the platform offers must be in exactly one pruned pack, or a user
# would install a board whose headers were never shipped.
while IFS= read -r mcu; do
  case " $MP_DEVICES $MC_DEVICES " in
    *" $mcu "*) ;;
    *) echo "boards.txt offers $mcu but no pruned pack includes it" >&2; exit 1 ;;
  esac
done < <(sed -n 's/^[a-z0-9_]*\.build\.mcu=\(.*\)$/\1/p' "$SRC/boards.txt" | sort -u)

# --- zip helper --------------------------------------------------------------
# Deterministic: sorted entries, fixed 1980-01-01 timestamps, forward slashes,
# and one top-level directory (which the Board Manager strips on install).
zipdir() {  # zipdir <srcdir> <outzip> <root-name-inside-zip>
  "$PY" - "$1" "$2" "$3" <<'PY'
import os, sys, zipfile
src, dst, root = sys.argv[1:4]
n = 0
with zipfile.ZipFile(dst, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for dirpath, dirnames, filenames in os.walk(src):
        dirnames.sort(); filenames.sort()
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, src).replace(os.sep, '/')
            zi = zipfile.ZipInfo(root + '/' + rel, date_time=(1980, 1, 1, 0, 0, 0))
            zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = 0o100644 << 16
            with open(full, 'rb') as f:
                z.writestr(zi, f.read())
            n += 1
print('  %s  (%d files)' % (os.path.basename(dst), n))
PY
}

# --- 1. the platform archive -------------------------------------------------
# Contents are arduino-platform/microchip/dspic33ck/, which is also what
# install_arduino_ide.bat copies -- minus platform.local.txt, which is
# machine-specific and must never ship (the .template stays, as documentation) --
# plus docs/, which is assembled below and is the one thing the archive carries
# that a local install_arduino_ide.bat install does not.
echo "== platform"
STAGE="$OUT/stage/dspic33ck-$VERSION"
mkdir -p "$STAGE"
cp -r "$SRC/." "$STAGE/"
rm -f "$STAGE/platform.local.txt"
find "$STAGE" -name '*.o' -o -name '*.elf' -o -name '.DS_Store' -o -name 'Thumbs.db' \
  -o -name '__pycache__' -prune | while IFS= read -r junk; do rm -rf "$junk"; done
[ ! -e "$STAGE/platform.local.txt" ] || { echo "platform.local.txt leaked into the archive" >&2; exit 1; }

# --- the user guide, copied into the archive rather than moved into the tree ---
# The docs live at arduino-platform/docs/, which is OUTSIDE the directory this
# archive is built from, so up to and including v1.0.4 a Board Manager install
# carried none of them: the guide existed only on GitHub. They ship as docs/ so an
# installed platform is self-documenting with no network.
#
# COPIED, not moved, and that is deliberate. The published v1.0.3 and v1.0.4
# release notes both link to
#   .../blob/main/arduino-platform/docs/part6_serial_bootloader.html
# and /blob/main/ resolves against the branch as it is today, so relocating the
# directory would 404 those links permanently, for readers of releases that have
# already shipped. The source of truth stays where it is and the archive gets a
# copy; the byte-identity check after zipdir is what keeps the copy honest.
#
# The list stays explicit rather than a glob even though docs/ now contains exactly
# these eight files. The superseded docs/how-to-use/ and its zip-inside-a-zip were
# deleted on September 24, 2026, which removed the original reason -- but not the
# durable one: a glob would silently ship whatever scratch file someone leaves in
# the directory, and the per-file tracked-ness assertion below has nothing to
# check against if the list is derived from the directory it is meant to police.
# Adding a page to the package should be a deliberate act, so a new part must be
# added here too.
DOCS=(
  arduino_ide_setup.html
  part1_introduction_setup.html
  part2_pin_mapping_hardware.html
  part3_api_reference.html
  part4_testing_sketches.html
  part5_upload_troubleshooting.html
  part6_serial_bootloader.html
  part7_bench_verification.html
)
DOCSRC="$REPO/arduino-platform/docs"
mkdir -p "$STAGE/docs"
for d in "${DOCS[@]}"; do
  [ -f "$DOCSRC/$d" ] || { echo "doc listed but not present: docs/$d" >&2; exit 1; }
  # Tracked-ness is asserted per file rather than inferred, because the stage
  # check below is about to be told to EXPECT these paths -- without this, an
  # uncommitted doc would ship and the check that exists to catch exactly that
  # would wave it through.
  git -C "$REPO" ls-files --error-unmatch "arduino-platform/docs/$d" >/dev/null 2>&1 || {
    echo "doc is not tracked, so it must not ship: docs/$d" >&2
    echo "commit it, or remove it from the DOCS list in $(basename "$0")" >&2
    exit 1; }
  cp "$DOCSRC/$d" "$STAGE/docs/$d"
done
echo "  docs: ${#DOCS[@]} pages"

# `cp -r` copies the working tree, which means anything gitignored rides along --
# that is how tools/MPLABXLog.xml (a 103-byte skeleton ipecmd drops next to itself)
# ended up in both the 1.0.0 and 1.0.1 archives. Rather than blacklist each stray
# as it appears, require the stage to be exactly the tracked tree: an untracked file
# here is either junk or something the author forgot to commit, and both should stop
# the release. Uncommitted *modifications* are still allowed on purpose -- that is
# what makes a release testable before the commit that carries it.
#
# The expected set is the tracked platform tree PLUS the docs staged above, each
# of which was just asserted to be tracked in its own right.
{
  git -C "$REPO" ls-files "arduino-platform/microchip/dspic33ck" \
    | sed 's|^arduino-platform/microchip/dspic33ck/||'
  printf 'docs/%s\n' "${DOCS[@]}"
} | LC_ALL=C sort >"$OUT/stage.tracked"
( cd "$STAGE" && find . -type f | sed 's|^\./||' | LC_ALL=C sort ) >"$OUT/stage.actual"
if ! extra=$(comm -13 "$OUT/stage.tracked" "$OUT/stage.actual") || [ -n "$extra" ]; then
  echo "untracked files would ship in the archive:" >&2
  echo "$extra" | sed 's/^/  /' >&2
  echo "commit them, or add them to .gitignore AND delete them from $SRC" >&2
  exit 1
fi
if missing=$(comm -23 "$OUT/stage.tracked" "$OUT/stage.actual") && [ -n "$missing" ]; then
  echo "tracked files are missing from the stage:" >&2
  echo "$missing" | sed 's/^/  /' >&2
  exit 1
fi

zipdir "$STAGE" "$OUT/$PLATFORM_ARCHIVE" "dspic33ck-$VERSION"

# Read the docs back OUT of the finished zip and compare them to the repo. The
# stage check above compares file NAMES; this compares bytes, and it reads the
# artifact that will actually be published rather than the directory it was built
# from. A truncated or stale guide in the package is worse than no guide at all,
# because a reader has no way to tell which one they have.
"$PY" - "$OUT/$PLATFORM_ARCHIVE" "dspic33ck-$VERSION" "$DOCSRC" "${DOCS[@]}" <<'PY'
import os, sys, zipfile
zpath, root, docsrc = sys.argv[1:4]
names = sys.argv[4:]
prefix = root + '/docs/'
with zipfile.ZipFile(zpath) as z:
    have = set(z.namelist())
    for n in names:
        entry = prefix + n
        if entry not in have:
            sys.exit('archive is missing %s' % entry)
        got = z.read(entry)
        want = open(os.path.join(docsrc, n), 'rb').read()
        if got != want:
            sys.exit('%s differs from the repo copy (%d vs %d bytes)'
                     % (entry, len(got), len(want)))
    extra = sorted(e for e in have
                   if e.startswith(prefix) and e[len(prefix):] not in names)
    if extra:
        sys.exit('unexpected files under docs/ in the archive: %s' % ', '.join(extra))
print('  docs verified in the archive: %d pages, byte-identical to the repo'
      % len(names))
PY

# --- 2. the pruned DFP archives ---------------------------------------------
for spec in "${TOOLS[@]}"; do
  IFS='|' read -r NAME TOOLVER UPSTREAM ARCHIVE <<<"$spec"
  case "$NAME" in
    *-MP_DFP) DEVS="$MP_DEVICES" ;;
    *-MC_DFP) DEVS="$MC_DEVICES" ;;
    *) echo "don't know which devices belong in tool $NAME" >&2; exit 1 ;;
  esac
  PACKDIR="$PACKS/$NAME/$UPSTREAM"
  [ -d "$PACKDIR/xc16" ] || {
    echo "full pack not installed: $PACKDIR" >&2
    echo "install it through MPLAB X, or set MCHP_PACKS to where your packs live" >&2
    exit 1; }
  echo "== $NAME $TOOLVER  (from $UPSTREAM)"
  # shellcheck disable=SC2086
  "$HERE/prune-dfp.sh" "$PACKDIR" "$OUT/stage/$NAME-$TOOLVER" $DEVS | sed 's/^/  /'
  zipdir "$OUT/stage/$NAME-$TOOLVER" "$OUT/$ARCHIVE" "$NAME-$TOOLVER"
done

# --- 3. sizes and checksums, then fill the index ----------------------------
: > "$OUT/checksums.txt"
for f in "$OUT"/*.zip; do
  printf '%s %s %s\n' "$(basename "$f")" "$(wc -c <"$f" | tr -d ' ')" \
    "$(sha256sum "$f" | cut -d' ' -f1)" >> "$OUT/checksums.txt"
done

"$PY" - "$IDX" "$OUT/checksums.txt" <<'PY'
import json, sys
idx, ckfile = sys.argv[1:3]
vals = {}
for line in open(ckfile):
    name, size, sha = line.split()
    vals[name] = (size, sha)
d = json.load(open(idx, encoding='utf-8'))
pkg = d['packages'][0]
missing = []
def fill(o):
    n = o.get('archiveFileName')
    if n in vals:
        size, sha = vals[n]
        o['size'] = size
        o['checksum'] = 'SHA-256:' + sha
    else:
        missing.append(n)
for pl in pkg['platforms']:
    fill(pl)
for t in pkg['tools']:
    for s in t['systems']:
        fill(s)
if missing:
    sys.exit('no archive was built for: ' + ', '.join(map(str, missing)))
with open(idx, 'w', encoding='utf-8', newline='\n') as f:
    json.dump(d, f, indent=2)
    f.write('\n')
PY
cp "$IDX" "$OUT/"
rm -rf "$OUT/stage" "$OUT/manifest"

echo
echo "== $OUT"
sed 's/^/  /' "$OUT/checksums.txt"
echo
echo "Next: create release $TAG on https://github.com/9Nicotin/Arduino_dsPIC33CK"
echo "and attach every file above plus package_microchip_dspic33ck_index.json, e.g."
echo
echo "  gh release create $TAG --title \"$TAG\" \\"
echo "    $(cd "$OUT" && ls *.zip package_microchip_dspic33ck_index.json | tr '\n' ' ')"
echo
echo "The URL users paste into Additional Boards Manager URLs is then:"
echo "  https://github.com/9Nicotin/Arduino_dsPIC33CK/releases/latest/download/package_microchip_dspic33ck_index.json"
