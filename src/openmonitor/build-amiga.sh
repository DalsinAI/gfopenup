#!/bin/sh
# GFOpenMonitor for Cradle's stove_build (target src/openmonitor, an os32-gcc16 stove): Cradle checks out this
# repository alone and sets STOVE and OUT. build.sh needs gfopengadtools and amicradle-guest: Cradle's own
# checkouts of them beside this one (cradle-builds/DalsinAI__gfopengadtools, __amicradle-guest), unless OGT and
# GUEST name others. MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILDS=$(cd "$HERE/../../.." && pwd)
OGT=${OGT:-$BUILDS/DalsinAI__gfopengadtools}
GUEST=${GUEST:-$BUILDS/DalsinAI__amicradle-guest}
OUT=${OUT:-$HERE/../../build/os3}
mkdir -p "$OUT"
OGT=$OGT GUEST=$GUEST OS32_GCC16=${STOVE:-$HOME/AmiCradle/stoves/os32-gcc16} sh "$HERE/build.sh" "$OUT/GFOpenMonitor"
