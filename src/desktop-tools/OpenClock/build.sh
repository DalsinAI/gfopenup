#!/bin/sh
# OpenClock with the os32 stove (m68k-amigaos-gcc, NDK 3.2) and OpenGadTools' theme drawing.
#   OpenClock/build.sh [OUT_DIR]      (default build/os3)
# OGT is a checkout of DalsinAI/opengadtools (its lib/).
# 68020, no FPU and no maths library: the hands come from a table of sines.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC=${CC:-"$STOVE/prefix/bin/m68k-amigaos-gcc"}
OGT=${OGT:-$HERE/../../../../opengadtools}
OUT=${1:-$HERE/build/os3}
[ -f "$OGT/lib/ogt_draw.c" ] || { echo "build.sh: set OGT to a checkout of opengadtools (lib/ogt_draw.c)" >&2; exit 1; }
CFLAGS="-noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common -I$OGT/lib"
mkdir -p "$OUT"
"$CC" $CFLAGS -o "$OUT/OpenClock" "$HERE/src/openclock.c" \
    "$OGT/lib/ogt_theme.c" "$OGT/lib/ogt_draw.c" "$OGT/lib/ogt_look.c"
echo "$OUT/OpenClock ($(wc -c < "$OUT/OpenClock") bytes)"
