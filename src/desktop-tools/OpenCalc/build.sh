#!/bin/sh
# OpenCalc with the os32 stove (m68k-amigaos-gcc, NDK 3.2): the portable engine (src/opencalc.c,
# OpenUp Desktop Tools 0.1) and its Workbench window (src/calc_amiga.c).
#   OpenCalc/build.sh [OUT_DIR]       (default build/os3)
# 68020 with soft float, as every Open app; -lm for the engine's sin, cos, log and pow.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC=${CC:-"$STOVE/prefix/bin/m68k-amigaos-gcc"}
OUT=${1:-$HERE/build/os3}
CFLAGS="-noixemul -m68020 -std=gnu99 -Wall -Werror -O2 -fno-delete-null-pointer-checks -fno-common -I$HERE/include"
mkdir -p "$OUT"
"$CC" $CFLAGS -o "$OUT/OpenCalc" "$HERE/src/calc_amiga.c" "$HERE/src/opencalc.c" -lm
echo "$OUT/OpenCalc ($(wc -c < "$OUT/OpenCalc") bytes)"
