#!/bin/sh
# OpenMonitor (0.4 on) for the disk (OpenUp's SYS:Utilities/OpenMonitor), with the os32-gcc16 stove:
#   OGT=<opengadtools checkout> GUEST=<amigachrome-guest checkout> sh src/openmonitor/build.sh [OUT]   (default build/os3/OpenMonitor)
# The compiler: the os32-gcc16 stove at OS32_GCC16 (default ~/AmigaChrome/stoves/os32-gcc16), GCC 16 or
# later. Built resident (libnix -resident: each run its own variables), 68040 code, soft float as
# opengadtools is (no FPU needed), -Os. A cell or a status line cut to its room is meant:
# -Wno-format-truncation. The ROM keeps OpenMonitor 0.3.3 (src/openmonitor.c, built by amigachrome-guest's
# romprefs.sh) until this goes into it (the design's P6).
# GUEST: an amigachrome-guest checkout, for sysprobe (common/sysprobe: the Hardware and Speed views read the
# machine as ShowConfig and ACInfo do), accontrol.device's header and taskclock.h (OpenExec's task clock).
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
: "${OGT:?set OGT to an opengadtools checkout}"
: "${GUEST:?set GUEST to an amigachrome-guest checkout (common/sysprobe)}"
SP="$GUEST/common/sysprobe"
[ -f "$SP/sysprobe.c" ] || { echo "$GUEST has no common/sysprobe: amigachrome-guest main from the P3 merge on"; exit 2; }
[ -f "$GUEST/common/include/taskclock.h" ] || { echo "$GUEST has no common/include/taskclock.h: amigachrome-guest main from the P4 merge on"; exit 2; }
STOVE=${OS32_GCC16:-$HOME/AmigaChrome/stoves/os32-gcc16}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
GCCV=$("$CC" -dumpversion 2>/dev/null || echo 0)
[ "${GCCV%%.*}" -ge 16 ] 2>/dev/null || { echo "$CC is GCC $GCCV: OpenMonitor needs GCC 16 (the os32-gcc16 stove; set OS32_GCC16)"; exit 2; }
OUT=${1:-$HERE/../../build/os3/OpenMonitor}
mkdir -p "$(dirname "$OUT")"
G="$OGT/lib"
"$CC" -noixemul -m68040 -specs="$OGT/specs/m68k-soft-m68040.specs" -resident -std=gnu99 -Wall -Werror -Wno-format-truncation -Os \
    -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks -fno-common -I"$G" -I"$HERE" -I"$SP" \
    -I"$GUEST/devices/accontrol.device/include" -I"$GUEST/common/include" \
    -o "$OUT" \
    "$HERE/main.c" "$HERE/ui.c" "$HERE/graphs.c" "$HERE/tasks.c" "$HERE/memory.c" "$HERE/libs.c" "$HERE/ports.c" "$HERE/ints.c" \
    "$HERE/hw.c" "$HERE/speed.c" "$HERE/calls.c" "$SP/sysprobe.c" \
    "$G/ogt_look.c" "$G/ogt_theme.c" "$G/ogt_font.c" "$G/ogt_draw.c" "$G/ogt_list.c" "$G/ogt_tabs.c" "$G/ogt_status.c"
echo "$OUT ($(wc -c < "$OUT") bytes)"
