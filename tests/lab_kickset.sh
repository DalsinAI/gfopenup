#!/bin/sh
# Copyright (c) 2026 AmiCradle Team. MIT licence (see LICENSE).
# KICKSET on a lab, ready for the redeploy. It does not start a lab, and the
# lab path is LAB, not an argument: lab_instance.py stop kills every process
# whose command line names that lab.
#
#   LAB=/path/to/lab sh tests/lab_kickset.sh
#
# The pack holds bodies for 62, 183 and 20. Two leftover drawers (183 and 20)
# are already on the disk. A pass:
#   the host is 68k (no board has written it yet):
#     the 68k module is there, no .bodies drawer is, both leftovers are in
#     SYS:Kickstart/.retired/ and the tool says "this host is 68k"
#   the host is 62, 183 or 20:
#     only that .bodies/<cpu>/ is there, and the other leftover is retired
# A stray gfopenfiles.module the Manifest doesn't list is moved to .retired.
# Instance-5, Instance-40 and the card are refused.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
up=$here/..

case "${LAB:-}" in
*"Instance-5"*|*"Instance-40"*|*"instances/5"*|*"instances/40"*)
    echo "refused: Instance-5 and Instance-40 are not labs" >&2; exit 2;;
esac
case "${LAB:-}" in
/dev/sd*|/dev/mmc*) echo "refused: that path is a card" >&2; exit 2;;
esac

echo "=== host: which files KICKSET copies ==="
cc=${CC:-cc}
if command -v "$cc" >/dev/null 2>&1; then
    tmpc=$(mktemp -d)
    "$cc" -std=c99 -Wall -Werror -I"$up/src" "$here/test_openup_kickset.c" -o "$tmpc/t"
    "$tmpc/t"
    rm -rf "$tmpc"
else
    echo "no $cc: the host checks were not run"
fi

if [ -z "${LAB:-}" ]; then
    echo "no LAB: nothing was copied. When a lab is running:"
    echo "  LAB=/path/to/lab sh tests/lab_kickset.sh"
    exit 0
fi

[ -d "$LAB/devices/harddisks/DH0" ] || { echo "not a lab: $LAB" >&2; exit 2; }
DH0=$LAB/devices/harddisks/DH0
P=$DH0/KsTest/Parts/KickstartSet
rm -rf "$DH0/KsTest" "$DH0/Kickstart/default" "$DH0/Kickstart/.retired"
mkdir -p "$P/Kickstart/default/.bodies/62" "$P/Kickstart/default/.bodies/183" "$P/Kickstart/default/.bodies/20"
mkdir -p "$DH0/Kickstart/default/.bodies/183" "$DH0/Kickstart/default/.bodies/20"
printf 'm68k' > "$P/Kickstart/default/icon.module"
printf 'amd64' > "$P/Kickstart/default/.bodies/62/icon.module"
printf 'arm64' > "$P/Kickstart/default/.bodies/183/icon.module"
printf 'power' > "$P/Kickstart/default/.bodies/20/icon.module"
cat > "$P/Kickstart/default/Manifest" << 'EOF'
# GFOpenKickLoad manifest 2
manifest 2
icon.module icon.library 47.5 4 aaaa 4
.bodies/62/icon.module icon.library 47.5 5 bbbb 62
.bodies/183/icon.module icon.library 47.5 5 cccc 183
.bodies/20/icon.module icon.library 47.5 5 dddd 20
EOF
printf 'old183' > "$DH0/Kickstart/default/.bodies/183/old.module"
printf 'old20' > "$DH0/Kickstart/default/.bodies/20/old.module"
# an older set's module the new Manifest doesn't list (Instance-32, 14 Nov 2026: kickload refused the set for it)
printf 'stray' > "$DH0/Kickstart/default/gfopenfiles.module"
cat > "$P/Part" << 'EOF'
part KickstartSet 47.100
; a fake set: bodies for three hosts, so KICKSET can copy one of them
copy "Kickstart/default/icon.module" "SYS:Kickstart/default/icon.module"
copy "Kickstart/default/.bodies/62/icon.module" "SYS:Kickstart/default/.bodies/62/icon.module"
copy "Kickstart/default/.bodies/183/icon.module" "SYS:Kickstart/default/.bodies/183/icon.module"
copy "Kickstart/default/.bodies/20/icon.module" "SYS:Kickstart/default/.bodies/20/icon.module"
copy "Kickstart/default/Manifest" "SYS:Kickstart/default/Manifest"
EOF
echo "copied the fake set, and leftover drawers 183 and 20"

bench=${AMICRADLE:-$up/../amicradle}/tools/labbench/labrun.sh
if [ ! -f "$bench" ]; then
    echo "files are in the lab. labrun.sh was not found (set AMICRADLE). Run, with LAB set:"
    echo "  sh tools/labbench/labrun.sh <this>/tests/kickset.script"
    exit 0
fi

out=$(mktemp)
if ! sh "$bench" "$here/kickset.script" 180 >"$out" 2>&1; then
    cat "$out"; echo "labrun failed"; rm -f "$out"; exit 1
fi
cat "$out"
part=$(awk 'index($0,"--- kickset ---"){p=1;next} index($0,"--- kickset done ---"){exit} p' "$out")
rm -f "$out"

ks=$DH0/Kickstart
fail=0
mod() { if [ -f "$ks/default/icon.module" ]; then echo "ok   68k module"; else echo "FAIL 68k module missing"; fail=1; fi; }
gone() { if [ ! -e "$ks/default/.bodies/$1" ]; then echo "ok   no .bodies/$1"; else echo "FAIL .bodies/$1 is still there"; fail=1; fi; }
kept() { if [ -f "$ks/default/.bodies/$1/icon.module" ]; then echo "ok   .bodies/$1"; else echo "FAIL .bodies/$1 was not installed"; fail=1; fi; }
retired() {
    if find "$ks/.retired" -path "*/bodies-$1/*" -name old.module 2>/dev/null | grep -q .; then
        echo "ok   bodies-$1 retired"; return
    fi
    echo "FAIL bodies-$1 was not moved to .retired"; fail=1
}
mod
if [ ! -e "$ks/default/gfopenfiles.module" ] && [ -f "$ks/.retired/gfopenfiles.module" ]; then
    echo "ok   the stray gfopenfiles.module moved to .retired"
else
    echo "FAIL the stray gfopenfiles.module was not moved to .retired"; fail=1
fi
case "$part" in
*"this host is 68k"*)
    echo "host is 68k: no bodies, both leftovers retired"
    gone 62; gone 183; gone 20
    retired 183; retired 20
    ;;
*"native bodies installed for CPU 62"*)
    echo "host is x86-64"
    kept 62; gone 183; gone 20
    retired 183; retired 20
    ;;
*"native bodies installed for CPU 183"*)
    echo "host is ARM64"
    kept 183; gone 62; gone 20
    retired 20
    ;;
*"native bodies installed for CPU 20"*)
    echo "host is PowerPC"
    kept 20; gone 62; gone 183
    retired 183
    ;;
*)
    echo "FAIL the tool did not say which bodies it installed (C:GFOpenUpTool is older than #229, or KICKSET did not run)"
    printf '%s\n' "$part"
    fail=1
    ;;
esac
[ "$fail" = 0 ] && echo "kickset: the lab agreed"
exit "$fail"
