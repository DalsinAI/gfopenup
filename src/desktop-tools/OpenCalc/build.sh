#!/bin/sh
# OpenCalc with the os32 stove (m68k-amigaos-gcc, NDK 3.2): the portable engine (src/opencalc.c,
# OpenUp Desktop Tools 0.1) and its Workbench window (src/calc_amiga.c).
#   OpenCalc/build.sh [OUT_DIR]       (default build/os3)
# 68040 with soft float, as every Open app (the specs file keeps libm020); -lm for the engine's sin, cos, log and pow.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
# soft float: link libnix's libm020, not libm881 (specs/m68k-soft-m68040.specs)
SPECS="$(cd "$HERE/../../.." && pwd)/specs/m68k-soft-m68040.specs"
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC=${CC:-"$STOVE/prefix/bin/m68k-amigaos-gcc"}
OUT=${1:-$HERE/build/os3}
CFLAGS="-noixemul -m68040 -specs=$SPECS -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common -I$HERE/include"
mkdir -p "$OUT"
"$CC" $CFLAGS -o "$OUT/OpenCalc" "$HERE/src/calc_amiga.c" "$HERE/src/opencalc.c" -lm
echo "$OUT/OpenCalc ($(wc -c < "$OUT/OpenCalc") bytes)"
