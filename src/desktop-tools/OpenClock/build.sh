#!/bin/sh
# GFOpenClock with the os32 stove (m68k-amigaos-gcc, NDK 3.2) and GFOpenGadTools' theme drawing.
#   OpenClock/build.sh [OUT_DIR]      (default build/os3)
# OGT is a checkout of DalsinAI/gfopengadtools (its lib/).
# 68040, no FPU and no maths library: the hands come from a table of sines.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
# soft float: link libnix's libm020, not libm881 (specs/m68k-soft-m68040.specs)
SPECS="$(cd "$HERE/../../.." && pwd)/specs/m68k-soft-m68040.specs"
STOVE=${STOVE:-$HOME/AmiCradle/stoves/os32}
CC=${CC:-"$STOVE/prefix/bin/m68k-amigaos-gcc"}
OGT=${OGT:-$HERE/../../../../gfopengadtools}
OUT=${1:-$HERE/build/os3}
[ -f "$OGT/lib/ogt_draw.c" ] || { echo "build.sh: set OGT to a checkout of gfopengadtools (lib/ogt_draw.c)" >&2; exit 1; }
CFLAGS="-noixemul -m68040 -specs=$SPECS -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common -I$OGT/lib"
# GFOpenAmigaGCC 16.2.0-open3's late-combine pass can store a copy one place late (GCC PR 123853; GFOpenUpTool's
# manifest came out so): off until open4 (gfopengcc #29), on a compiler that knows the option (GCC 6.5 doesn't).
if "$CC" -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then CFLAGS="$CFLAGS -fno-late-combine-instructions"; fi
mkdir -p "$OUT"
"$CC" $CFLAGS -o "$OUT/GFOpenClock" "$HERE/src/openclock.c" \
    "$OGT/lib/ogt_theme.c" "$OGT/lib/ogt_draw.c" "$OGT/lib/ogt_look.c" "$OGT/lib/ogt_font.c"
echo "$OUT/GFOpenClock ($(wc -c < "$OUT/GFOpenClock") bytes)"
