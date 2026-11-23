#!/bin/sh
# GFOpenApt (C:apt, docs/design/GFOPENAPT.md) with the stove GFOpenUpdate is built with, and its options
# (src/openupdate/build.sh); it has no window, so it needs nothing but this repository:
#   sh src/openapt/build.sh [OUT]
# (default build/os3/GFOpenApt). 68040 code, soft float. The compiler: OS32_GCC16, else STOVE, else
# ~/AmiCradle/stoves/os32-gcc16. It builds with the os32 stove's GCC 6.5 too.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STOVE=${OS32_GCC16:-${STOVE:-$HOME/AmiCradle/stoves/os32-gcc16}}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/../../build/os3/GFOpenApt}
mkdir -p "$(dirname "$OUT")"
FLAGS="-noixemul -m68040 -specs=$HERE/../../specs/m68k-soft-m68040.specs -std=gnu99 -Wall -Werror -O2 \
  -fno-delete-null-pointer-checks -fno-common"
if "$CC" -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then FLAGS="$FLAGS -fno-late-combine-instructions"; fi
# shellcheck disable=SC2086
"$CC" $FLAGS -s -o "$OUT" "$HERE/openapt.c"
echo "$OUT ($(wc -c < "$OUT") bytes)"
