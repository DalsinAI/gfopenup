#!/bin/sh
# Every host C test in tests/: each is built as its own header comment says (the "cc ..." line),
# into a scratch folder, and run from the top of the repository.
#   tests/c_tests.sh
# MIT, Copyright (c) 2026 AmiCradle Team.
set -u
cd "$(dirname "$0")/.."
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
fails=0; n=0
for f in tests/test_*.c; do
    n=$((n + 1))
    flags=$(sed -n '1,8p' "$f" | grep -E '(^|[^a-z])cc ' | head -n 1 | sed -e 's/.*cc //' -e 's| tests/test_.*||' -e 's/ -o [^ ]*//')
    [ -n "$flags" ] || flags="-std=c99 -Wall -Werror -Isrc"
    b=$W/$(basename "$f" .c)
    if ${CC:-cc} $flags "$f" -o "$b" >"$b.log" 2>&1 && "$b" >>"$b.log" 2>&1; then
        echo "PASS $f"
    else
        echo "FAIL $f"; tail -n 20 "$b.log"; fails=$((fails + 1))
    fi
done
echo "C tests: $((n - fails)) of $n pass"
[ "$fails" = 0 ]
