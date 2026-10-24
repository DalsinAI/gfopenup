#!/bin/sh
# OMProbe: the lab prototype behind the OpenMonitor task-manager design (Team 4, 24 October 2026).
# Not for the ROM or a release. With the os32-gcc16 stove: STOVE=.../os32-gcc16 sh build.sh [OUT]
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
: "${STOVE:?set STOVE to the os32-gcc16 stove}"
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-$HERE/OMProbe}
"$STOVE/prefix/bin/m68k-amigaos-gcc" -noixemul -m68020 -Os -Wall -Wno-format -fno-delete-null-pointer-checks \
  -o "$OUT" "$HERE/omprobe.c"
ls -l "$OUT"
