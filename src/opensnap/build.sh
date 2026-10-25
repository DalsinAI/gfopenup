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
# OpenAmigaGCC 16.2.0-open3's late-combine pass can store a copy one place late (GCC PR 123853; OpenUpTool's
# manifest came out so): off until open4 (openamigagcc #29), on a compiler that knows the option (GCC 6.5 doesn't).
LATE=""
if "$CC" -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
"$CC" -noixemul -m68040 -specs="$HERE/../../specs/m68k-soft-m68040.specs" -std=gnu99 -Wall -Werror -O2 \
    -fno-delete-null-pointer-checks -fno-common $LATE -I"$ODT/Datatypes/include" -I"$HERE" ${OSDEBUG:+-DOSDEBUG} \
    -o "$OUT" "$HERE/opensnap.c"
echo "$OUT ($(wc -c < "$OUT") bytes)"
