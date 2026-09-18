#!/usr/bin/env bash
# Prune a Microchip dsPIC33CK Device Family Pack down to the devices this
# platform supports, so it can be shipped as an Arduino Board Manager tool.
#
# The full packs are far too big to redistribute (MC_DFP 147 MB, MP_DFP 525 MB)
# and almost all of that is the ~36-83 other devices' headers. The DFPs are
# Apache-2.0 (see LICENSE.txt inside each pack), so pruning and redistributing
# is permitted provided the notices are kept and the change is stated -- hence
# the LICENSE.txt copy and the generated PRUNED.txt.
#
# Usage: prune-dfp.sh <src-pack-version-dir> <dest-dir> <dev> [<dev> ...]
#   <src-pack-version-dir>  e.g. ~/.mchp_packs/Microchip/dsPIC33CK-MC_DFP/1.11.412
#   <dev>                   bare device name, e.g. 33CK256MC005
set -euo pipefail

SRC="${1:?src pack version dir}"; shift
DST="${1:?dest dir}"; shift
DEVS=("$@")
[ "${#DEVS[@]}" -gt 0 ] || { echo "no devices given" >&2; exit 2; }

S="$SRC/xc16"
[ -d "$S" ] || { echo "not a pack version dir (no xc16/): $SRC" >&2; exit 2; }

D="$DST/xc16"
rm -rf "$DST"
mkdir -p "$D/bin/config" "$D/bin/device_files" \
         "$D/support/dsPIC33C/gld" "$D/support/dsPIC33C/h" "$D/support/dsPIC33C/inc"

# --- non-device-specific files: always keep -------------------------------
# c30_device.info and deviceSupport.xml are pack-wide indexes the compiler
# driver reads; GENERIC.info and p33Cxxxx.{h,inc} are the family-generic
# fallbacks that xc.h dispatches through.
cp "$S/bin/c30_device.info" "$S/bin/deviceSupport.xml" "$D/bin/"
cp "$S/bin/device_files/GENERIC.info" "$D/bin/device_files/"
cp "$S/support/dsPIC33C/h/p33Cxxxx.h"     "$D/support/dsPIC33C/h/"
cp "$S/support/dsPIC33C/inc/p33Cxxxx.inc" "$D/support/dsPIC33C/inc/"
cp -r "$S/support/generic" "$D/support/"

# --- per-device slices ----------------------------------------------------
for dev in "${DEVS[@]}"; do
  cp -r "$S/bin/config/$dev"                    "$D/bin/config/"
  cp    "$S/bin/device_files/$dev.info"         "$D/bin/device_files/"
  cp    "$S/support/dsPIC33C/gld/p$dev.gld"     "$D/support/dsPIC33C/gld/"
  cp    "$S/support/dsPIC33C/h/p$dev.h"         "$D/support/dsPIC33C/h/"
  cp    "$S/support/dsPIC33C/inc/p$dev.inc"     "$D/support/dsPIC33C/inc/"
done

# --- Apache-2.0 obligations ----------------------------------------------
cp "$SRC/LICENSE.txt" "$DST/LICENSE.txt"
{
  echo "This is a PRUNED redistribution of a Microchip Device Family Pack."
  echo
  echo "Upstream pack : $(basename "$(dirname "$SRC")") $(basename "$SRC")"
  echo "Pruned for    : Arduino_dsPIC33CK (https://github.com/9Nicotin/Arduino_dsPIC33CK)"
  echo "Devices kept  : ${DEVS[*]}"
  echo
  echo "Modification made: all files specific to devices other than those listed"
  echo "above were deleted, as was xc16/docs/. No file was altered in any other"
  echo "way. The original Apache License 2.0 (LICENSE.txt) and Microchip's"
  echo "copyright notice are retained unchanged."
  echo
  echo "Kept, in full:"
  echo "  xc16/bin/c30_device.info, xc16/bin/deviceSupport.xml"
  echo "  xc16/bin/device_files/GENERIC.info"
  echo "  xc16/support/generic/"
  echo "  xc16/support/dsPIC33C/{h,inc}/p33Cxxxx.*"
  echo "Kept, per device listed above:"
  echo "  xc16/bin/config/<dev>/, xc16/bin/device_files/<dev>.info"
  echo "  xc16/support/dsPIC33C/{gld,h,inc}/p<dev>.*"
} > "$DST/PRUNED.txt"

echo "pruned $(basename "$(dirname "$SRC")") $(basename "$SRC") -> $DST  ($(du -sh "$DST" | cut -f1))"
