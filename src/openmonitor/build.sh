#!/bin/sh
# GFOpenMonitor 1.1, the disk's (GFOpenUp's SYS:Utilities/GFOpenMonitor) and the open ROM's (amicradle-guest's
# trial-1m romprefs.sh builds the ROM's C/GFOpenMonitor with this script), with the os32-gcc16 stove:
#   OGT=<gfopengadtools checkout> GUEST=<amicradle-guest checkout> sh src/openmonitor/build.sh [OUT]   (default build/os3/GFOpenMonitor)
# The compiler: the os32-gcc16 stove at OS32_GCC16 (default ~/AmiCradle/stoves/os32-gcc16), GCC 16 or
# later. Built resident (libnix -resident: each run its own variables), 68040 code, soft float as
# gfopengadtools is (no FPU needed), -Os. A cell or a status line cut to its room is meant:
# -Wno-format-truncation.
# GUEST: an amicradle-guest checkout, for sysprobe (common/sysprobe: the Hardware and Speed views read the
# machine as ShowConfig and ACInfo do), accontrol.device's header and taskclock.h (GFOpenExec's task clock).
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
: "${OGT:?set OGT to a gfopengadtools checkout}"
: "${GUEST:?set GUEST to an amicradle-guest checkout (common/sysprobe)}"
SP="$GUEST/common/sysprobe"
[ -f "$SP/sysprobe.c" ] || { echo "$GUEST has no common/sysprobe: amicradle-guest main from the P3 merge on"; exit 2; }
[ -f "$GUEST/common/include/taskclock.h" ] || { echo "$GUEST has no common/include/taskclock.h: amicradle-guest main from the P4 merge on"; exit 2; }
STOVE=${OS32_GCC16:-$HOME/AmiCradle/stoves/os32-gcc16}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
GCCV=$("$CC" -dumpversion 2>/dev/null || echo 0)
[ "${GCCV%%.*}" -ge 16 ] 2>/dev/null || { echo "$CC is GCC $GCCV: GFOpenMonitor needs GCC 16 (the os32-gcc16 stove; set OS32_GCC16)"; exit 2; }
OUT=${1:-$HERE/../../build/os3/GFOpenMonitor}
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
