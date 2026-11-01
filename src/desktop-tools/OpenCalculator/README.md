# GFOpenCalc 0.1

GFOpenCalc is the GFOpenUp calculator component.

It is deliberately split into a small, portable expression engine and a thin
GFOpenUp GUI adapter. The engine can therefore be unit-tested on the build host
and reused from Shell, scripting, or other GFOpenUp applications.

## UI direction

GFOpenCalc should feel like an Amiga utility, not a phone calculator enlarged
onto Workbench.

The default window has:

- a single editable expression line;
- a large result line;
- calculator keypad;
- scientific controls in a secondary column;
- DEG/RAD mode;
- memory keys;
- scrollable calculation history;
- keyboard-first operation;
- copy/paste of both expressions and results.

Window size and position should be snapshot-capable in the normal Workbench
fashion. At small sizes the scientific column can collapse behind a
"Scientific" disclosure rather than making the keypad unusable.

## Core features

- + - * / and parentheses
- exponentiation with ^
- sin/cos/tan
- sqrt, natural log and base-10 log
- abs
- pi, e, ans and memory constants
- degree/radian mode
- parse/domain/division/range errors
- no silent NaN/Infinity display

## Suggested repository location

    GFOpenUp/
      apps/
        GFOpenCalc/

## Build-host smoke test

    cc -Iinclude src/opencalc.c tests/test_opencalc.c -lm -o test-opencalc
    ./test-opencalc
