# GFOpenUp Desktop Tools 0.1: received source, kept for analysis and a later build

**Received:** 10 October 2026, from the user, as `OpenUp_Desktop_Tools_0.1_Source.zip` (9,999 bytes, sha256 `1cb96faded284394641c3ab38171ea932ae526aa0cac92a65ac6a0268015e4b5`).
**State:** source only, kept as received. **Not** built into the GFOpenUp pack: `tools/build_pack.py` does not know it and no part installs it. Kept for analysis and a later build. The bundle's `component.json` suggests `GFOpenUp/apps/GFOpenCalc` and `GFOpenUp/apps/GFOpenShell`; this repository keeps its own code in `src/`, so they are here, under `src/desktop-tools/`, until the Team decides where they live.

**Renamed (25 October 2026):** the calculator is **GFOpenCalculator**, in `src/desktop-tools/OpenCalculator/`, as Dale decided: GFOpenCalc is the GFOffice Suite's spreadsheet. The Team's own files there (`build.sh`, `src/calc_amiga.c`: GFOpenCalculator 0.4, its window and `$VER`) say so. The received files keep their own text, and the checksums below, which name them under `GFOpenCalc/` as they came.

## What is here

| Component | What it is | State as received |
| --- | --- | --- |
| **GFOpenCalc** | A calculator: a portable expression engine (`OC_Eval`: + − × ÷, parentheses, `^`, sin, cos, tan, sqrt, ln, log, abs, pi, e, ans, mem, degrees or radians, clear errors) and a console front-end for development | The engine works and is tested; the Workbench window (the keypad, the scientific column, history, memory keys) is a layout in comments, not built |
| **GFOpenShell** | A new Workbench window for the Amiga Shell, not a new Shell language: tabs each with its own Shell process, a path bar, a separate command line with history and completion, a selectable and searchable transcript, a status line | A full interaction design (`GFOpenShell/docs/DESIGN.md`), a data model and the transport interface (`OSH_TransportOps`: create, submit, break, ask the directory, last return code, busy); no implementation |
| `component.json` | The bundle's own manifest | |

Every file is byte for byte as received; the checksums are below. The zip held no prebuilt programs.

## Checked on 10 October 2026 (x86-64 host, gcc, C99)

- GFOpenCalc's test builds from source and passes; the console front-end builds and runs.
- Errors are reported, not shown as NaN or infinity: `sqrt(-1)` is a domain error, `1e308*10` a range error, `sin(30` and `2 3` parse errors.
- GFOpenShell's two sources compile (they are a skeleton).

## For the analysis

1. **GFOpenCalc: `-2^2` gives 4.** The engine applies the minus before the power; most calculators and languages give −4. A decision, then a test.
2. **GFOpenCalc: `%` is on the keypad layout but not in the engine** (`10%3` is a parse error). Percent or remainder: a decision.
3. **GFOpenCalc: memory keys** (MC, MR, M+, M−) have no engine calls; only `mem` can be read in an expression.
4. **GFOpenCalc: doubles throughout.** Fine on a 68040 with its FPU and on the AC090; a 68020 without one goes through the maths libraries.
5. **The windows.** Both components say "the project's MUI/OpenGUI wrapper". The GFOpen family's rule is GadTools (or MUI), styled by GFOpenLook; GFOpenShell's tabs, transcript and completion pop-up are the larger part of its work.
6. **GFOpenShell's transport** is the hard part: driving a real Shell process per tab (`NewShell`-style processes with a console of GFOpenShell's own, `SystemTags` with pipes, or a `CON:`-compatible handler), getting the directory and return code back without parsing commands, and Ctrl-C as a real break. AmigaOS's Shell and the Team's ACKitchen work both bear on it.
7. **Where they belong:** in GFOpenUp as apps (the bundle's suggestion), or each in its own `openamiga<thing>` repository like the other Open apps.
8. **No licence text** is in the files. They are kept as the Team's files under this repository's MIT licence (Copyright (c) 2026 AmigaChrome Team); to be confirmed when they are built.

## Building it later

On a host: `cc -Iinclude src/opencalc.c tests/test_opencalc.c -lm` in `GFOpenCalc/`. On the Amiga: both are plain C99 (the os32 or os32-gcc16 stove, with the Team's 68k flags such as `-fno-delete-null-pointer-checks`), then the window adapters.

## Checksums (sha256, as received)

```
62b6f597b469a7b39c8f4b09d85ebec9db1b9c018e3719b0b9dd53524f7d3c9b  GFOpenCalc/README.md
f9469cfb2cde5c70ee0d7ff464627b3754f2b90bbe02f3e84f9dccab537e5287  GFOpenCalc/include/opencalc.h
f6a5bd217e563415f2a9996f3f6b771990d5af12f5f186d31e8881744d254754  OpenCalc/src/main.c
95f7d8a895e6ae7accd01fd559585f8d8ac8c9a83ffe779de5f35dc28ced9e96  OpenCalc/src/opencalc.c
85f15ab9e4c59edfd2b0b315231eb4474de81ecbe83761321d012c8c0104d535  GFOpenCalc/tests/test_opencalc.c
f55973cc79da4556ae214e51c657b351a3a04109472a2f261bb25e38084ab8ce  GFOpenShell/README.md
d1f8cbf9fa23942d1c0b099875f789935b24eae859f51fe0a01f9e2992b2b797  GFOpenShell/docs/DESIGN.md
91ca5f1d676f84206626f0e609ab5c9a3c423d31e61ff294eba9701e68d8c4cf  GFOpenShell/include/openshell_model.h
96f6b38cf1d803bf0ddb57468d78d84fd6ff04c14dd2d5a90e970698fb008e3f  OpenShell/src/main.c
765a5a3e9449b0e809e133351cd4c57ae9df4a32508dc2e6fc2be54a89b6c58c  OpenShell/src/openshell_transport.c
5503a57b85c34fe515c1fcb1955150b01c8bfe215757b4a19d0bbfcb84623afc  component.json
```
