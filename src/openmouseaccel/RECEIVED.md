# OpenMouseAccel 0.1: received source, kept for analysis and a later build

**Received:** 10 October 2026, from the user, as `OpenMouseAccel_0.1_Source.zip` (25,495 bytes, sha256 `27c7fb817fd0eba10f6e699c53c5be1837691725fca3da1a5e71547440d894d7`).
**State:** source only, kept as received. It is **not** built into the OpenUp pack: `tools/build_pack.py` does not know it, and no part installs it. It is here so the Team can analyse it and build it later.
**Design:** the goal it implements is `Design-Pointer-Acceleration.md` in DalsinAI/openamigainput (OpenInput's design, section 13), from the user's specification of the same day.

## What is here

| Path | What it is |
| --- | --- |
| `include/openup/openmouseaccel.h` | The API: `OMA_Config`, `OMA_Context`, `OMA_Process`, presets, a text prefs format |
| `src/oma_core.c` | The engine: velocity from each report's interval, smoothing up and down, the curve, the caps, direction reset, residuals |
| `src/oma_config.c` | Load and save the prefs as `key=value` text |
| `src/oma_filter.c` | A small shim for OpenInput's filter stage, to be wired to OpenInput's own registration |
| `prefs/OpenMouseAccelPrefs.c` | A preferences program: MUI with `OMA_USE_MUI`, otherwise a host test harness |
| `tests/test_core.c`, `Makefile` | Host checks of the engine (`make test`) |
| `README.md`, `docs/OPENMOUSEACCEL_COMPONENT.md` | The author's notes and the component contract |

Every file is byte for byte as received; the checksums are below. Left out on purpose: the zip's `build/` folder, which held two prebuilt x86-64 Linux programs (`test_core`, `OpenMouseAccelPrefs-host`). Build output is never kept in git; `.gitignore` here keeps `build/` out.

## Checked on 10 October 2026 (x86-64 host, gcc)

- `make test` builds from source and passes.
- `make prefs-host` builds.
- After a fast movement, five one-count movements at a slow speed moved exactly five pixels at 1.0x: the precision zone and the residual handling hold.

## For the analysis

1. **The same movement should give the same result however the mouse reports it** (the specification's section 8). It does not yet: six one-count reports over 6 ms moved 16 pixels, one six-count report over the same 6 ms moved 11. The smoothing advances once per report, not over a time window, so a mouse that reports often accelerates more. The specification asks for velocity over a rolling 8 to 20 ms window.
2. **Floating point.** `powf`, `acosf` and `sqrtf` on every report. Fine on a 68040 with its FPU and on the AC090; slow on a 68020 without one. A fixed-point version, or tables, may be wanted for real hardware.
3. **The speed cap** is not applied to a report with no previous timestamp (the first after a reset).
4. **The prefs file** is `ENV:OpenUp/OpenMouseAccel.prefs` here; OpenInput's design keeps its settings in `ENV:OpenInput/` and `ENVARC:OpenInput/`. One place should be chosen.
5. **The preferences program** is MUI. The Open family's rule is GadTools (or MUI) and settings in OpenPrefs; the design's open question 2 asks whether this becomes a page of OpenPrefs' Input editor.
6. **The OpenInput hook** (`oma_filter.c`) is a shim by design; the real registration waits on OpenInput's relative-pointer sources, which are not built yet.
7. **No licence text** is in the files. They are kept here as the Team's files under this repository's MIT licence (Copyright (c) 2026 Dalsin Limited); to be confirmed when the component is built.
8. **Wording** in the author's notes ("OpenUp/OpenInput", no person's name) already follows the Team's rules.

## Building it later

On a host: `make test` and `make prefs-host` in this folder. On the Amiga: the engine is plain C99 with `<math.h>`; build the core and the filter into OpenInput (the os32 or os32-gcc16 stove, with the Team's 68k flags such as `-fno-delete-null-pointer-checks`), and the prefs program with `OMA_USE_MUI` against OpenMUI. A host test that replays recorded mouse traces through `OMA_Process` (the design's proof, section 11a) should come before any Amiga build.

## Checksums (sha256, as received)

```
89f08c60cd527dfa1464072d56910100792da6ba13004e4afba6e1b433fa7e62  Makefile
2c55e03ca1090ad82db16bfda98fb567cd335392ffb2c14bb7d9a15b32c0c38a  README.md
e2d5b9d198185c3635c753979e3becb593ba65e0d60363f3954691e554afd9b7  docs/OPENMOUSEACCEL_COMPONENT.md
6fe68cd91bc13d021bfa64d0541cfbb5714c4ae2f3d5d4b002b24dc569332f45  include/openup/openmouseaccel.h
52243f8fcfe5ed3aa804a72e607ac998d6519f728f3b56ea0dc704a94ebfa30a  prefs/OpenMouseAccelPrefs.c
e6eaea0ee0816761fd917137f95318739cb7b16ae3ebf7a0c4b85f53ed564067  src/oma_config.c
e529e93e5c95b784ea70b09e2ac579a87b50b1a9d75642e87b980c687823d90f  src/oma_core.c
f16fae3a223984f3958d12d5df41c1803a0775d5270d9afe017a8293f6b5a94e  src/oma_filter.c
5b6940ccd3b4d71b40a13eccd5ed1499501d12adc3239e75662ee8719c6d3317  tests/test_core.c
```
