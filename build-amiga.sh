#!/bin/sh
# GFOpenUpTool and GFOpenUpMenu for Cradle's stove_build (target .): Cradle checks out this repository alone and sets
# STOVE and OUT. These two need nothing but this repository and are built as tools/build_pack.py builds them
# (68040 code, soft float, the same options); the pack itself, with every part, is build_pack.py's.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT=${OUT:-$HERE/build/os3}
STOVE=${STOVE:-$HOME/AmiCradle/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
mkdir -p "$OUT"
# as build_pack.py's late_combine_off: off on a compiler that knows the option (GCC PR 123853)
LATE=""
if "$CC" -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
for prog in openuptool:GFOpenUpTool openupmenu:GFOpenUpMenu; do
    src=${prog%%:*}
    name=${prog#*:}
    "$CC" -noixemul -m68040 -specs="$HERE/specs/m68k-soft-m68040.specs" -std=gnu99 -Wall -Werror -O2 \
        -fno-delete-null-pointer-checks $LATE -fno-common -o "$OUT/$name" "$HERE/src/$src.c"
    echo "$OUT/$name ($(wc -c < "$OUT/$name") bytes)"
done
