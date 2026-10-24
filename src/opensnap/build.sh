#!/bin/sh
# OpenSnap for OpenUp's SYS:Tools/Commodities/OpenSnap, with the os32 stove:
#   ODT=<opendatatypes checkout> sh src/opensnap/build.sh [OUT]          (default build/os3/OpenSnap)
# Its PNGs go through openpng.datatype's OIM_ENCODEPNG, whose header (datatypes/openimage.h) is opendatatypes'
# Datatypes/include. 68040 code, soft float (specs/m68k-soft-m68040.specs), as OpenUp's other programs. The
# compiler: the os32 stove at STOVE (default ~/AmigaChrome/stoves/os32). OSDEBUG=1 makes a lab build that
# writes each step's time to T:OpenSnap.log.
# MIT, Copyright (c) 2026 AmigaChrome Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
: "${ODT:?set ODT to an opendatatypes checkout}"
STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
OUT=${1:-$HERE/../../build/os3/OpenSnap}
mkdir -p "$(dirname "$OUT")"
"$CC" -noixemul -m68040 -specs="$HERE/../../specs/m68k-soft-m68040.specs" -std=gnu99 -Wall -Werror -O2 \
    -fno-delete-null-pointer-checks -fno-common -I"$ODT/Datatypes/include" -I"$HERE" ${OSDEBUG:+-DOSDEBUG} \
    -o "$OUT" "$HERE/opensnap.c"
echo "$OUT ($(wc -c < "$OUT") bytes)"
