#!/bin/sh
# GFOpenExchange for GFOpenUp's SYS:Tools/Commodities/Exchange, with the os32-gcc16 stove (GCC 16, as
# GFOpenLayout UI wants) and GFOpenLayout's and GFOpenGadTools' sources (its window is GFOpenLayout UI, in the
# theme GFOpenPrefs Look chose, read by GFOpenGadTools' lib/ogt_theme.c):
#   OLAYOUT=<gfopenlayout checkout> OGT=<gfopengadtools checkout> sh src/openexchange/build.sh [OUT]
# (default build/os3/GFOpenExchange). 68040 code, soft float (specs/m68k-soft-m68040.specs), as GFOpenUp's other
# programs. The compiler: OS32_GCC16, else STOVE, else ~/AmiCradle/stoves/os32-gcc16.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
: "${OLAYOUT:?set OLAYOUT to a gfopenlayout checkout}"
: "${OGT:?set OGT to a gfopengadtools checkout}"
STOVE=${OS32_GCC16:-${STOVE:-$HOME/AmiCradle/stoves/os32-gcc16}}
CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
GCCV=$("$CC" -dumpversion 2>/dev/null || echo 0)
[ "${GCCV%%.*}" -ge 16 ] 2>/dev/null || { echo "$CC is GCC $GCCV: this needs GCC 16 (the os32-gcc16 stove)"; exit 2; }
OUT=${1:-$HERE/../../build/os3/GFOpenExchange}
mkdir -p "$(dirname "$OUT")"
FLAGS="-noixemul -m68040 -specs=$HERE/../../specs/m68k-soft-m68040.specs -std=gnu99 -Wall -Werror -O2 \
  -fno-delete-null-pointer-checks -fno-common -I$OLAYOUT/include -I$OLAYOUT/ui -I$OGT/lib"
# GCC 16.2.0-open3's late-combine pass has a wrong-code bug (GCC PR 123853): off on a compiler that knows the option
if "$CC" -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then FLAGS="$FLAGS -fno-late-combine-instructions"; fi
# GFOpenLayout and its CSS engine (src/*.c), the UI and its Intuition glue, as gfopenlayout's build-ui-amiga.sh
OL="$(echo "$OLAYOUT"/src/*.c) $OLAYOUT/ui/olui.c $OLAYOUT/ui/olui_css.c $OLAYOUT/ui/olui_markup.c $OLAYOUT/ui/olui_theme.c $OLAYOUT/amiga/olui_amiga.c"
# shellcheck disable=SC2086
"$CC" $FLAGS -s -o "$OUT" "$HERE/exchange.c" $OL "$OGT/lib/ogt_theme.c"
echo "$OUT ($(wc -c < "$OUT") bytes)"
