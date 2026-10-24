#!/bin/sh
# OpenMonitor 0.4 for the disk (OpenUp's SYS:Utilities/OpenMonitor), with the os32-gcc16 stove:
#   OGT=<opengadtools checkout> sh src/openmonitor/build.sh [OUT]          (default build/os3/OpenMonitor)
# The compiler: the os32-gcc16 stove at OS32_GCC16 (default ~/AmigaChrome/stoves/os32-gcc16), GCC 16 or
# later. Built resident (libnix -resident: each run its own variables), 68040 code, soft float as
# opengadtools is (no FPU needed), -Os. A cell or a status line cut to its room is meant:
# -Wno-format-truncation. The ROM keeps OpenMonitor 0.3.3 (src/openmonitor.c, built by amigachrome-guest's
# romprefs.sh) until this goes into it (the design's P6).
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
: "${OGT:?set OGT to an opengadtools checkout}"
STOVE=${OS32_GCC16:-$HOME/AmigaChrome/stoves/os32-gcc16}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
GCCV=$("$CC" -dumpversion 2>/dev/null || echo 0)
[ "${GCCV%%.*}" -ge 16 ] 2>/dev/null || { echo "$CC is GCC $GCCV: OpenMonitor needs GCC 16 (the os32-gcc16 stove; set OS32_GCC16)"; exit 2; }
OUT=${1:-$HERE/../../build/os3/OpenMonitor}
mkdir -p "$(dirname "$OUT")"
G="$OGT/lib"
"$CC" -noixemul -m68040 -specs="$OGT/specs/m68k-soft-m68040.specs" -resident -std=gnu99 -Wall -Werror -Wno-format-truncation -Os \
    -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks -fno-common -I"$G" -I"$HERE" \
    -o "$OUT" \
    "$HERE/main.c" "$HERE/ui.c" "$HERE/graphs.c" "$HERE/tasks.c" \
    "$G/ogt_look.c" "$G/ogt_theme.c" "$G/ogt_font.c" "$G/ogt_draw.c" "$G/ogt_list.c" "$G/ogt_tabs.c" "$G/ogt_status.c"
echo "$OUT ($(wc -c < "$OUT") bytes)"
