#!/bin/sh
# Cradle's stove_build entry for GFOpenExchange (target src/openexchange; STOVE= the os32-gcc16 stove, OUT= where
# it goes): build.sh, with GFOpenLayout and GFOpenGadTools beside this checkout, or fetched (shallow, read only)
# when they aren't.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
OUT=${OUT:-$TOP/build/os3}
DEPS=${DEPS:-$TOP/build/deps}
need() {    # need repo: the checkout beside this one, else a fetched one
    if [ -d "$TOP/../$1" ]; then echo "$TOP/../$1"; return; fi
    if [ ! -d "$DEPS/$1" ]; then
        mkdir -p "$DEPS"
        git clone -q --depth 1 "https://github.com/DalsinAI/$1" "$DEPS/$1" >&2
    fi
    echo "$DEPS/$1"
}
OLAYOUT=${OLAYOUT:-$(need gfopenlayout)}
OGT=${OGT:-$(need gfopengadtools)}
export OLAYOUT OGT
mkdir -p "$OUT"
OS32_GCC16=${OS32_GCC16:-${STOVE:-}} sh "$HERE/build.sh" "$OUT/GFOpenExchange"
