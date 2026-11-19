# Release notes for the next GFOpenUp pack: SDL3.module

For whoever builds the pack after 0.8.30. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **SDL3.module joins the GFOpenGPU part**, in `LIBS:GFOpenGPU/` beside SDL2.module (Dale, 19 November 2026: "add it to the pack"). Programs built with SDL 3 and linked with gfopenrtg's `libSDL3.a` open it there. Oolite is the first.
  - It is SDL 3.4.14 (Zlib), with SDL3_68k's AmigaOS 3 back ends (Zlib) and the Team's GFOpenGPU renderer, from gfopenrtg `library/modules/sdl3`. Like the part's other modules, it needs a 68040 or 68060 with its FPU.
  - It is only in `LIBS:GFOpenGPU/`, not copied to `LIBS:OpenGPU/`: no program was ever built against the old drawer's name for it.
  - SDL 3's and SDL3_68k's licences go in `SYS:Utilities/GFOpenUp/Licences/` (`SDL3.txt`, `SDL3_68k.txt`).

## Tested so far

- `tools/build_pack.py` builds it with the GCC 16 stove; `tests/test_build_pack.py` passes.
- gfopenrtg main (9bd8e26) built it with the open8 stove: 1,428,364 bytes; the baserel and FPCR checks passed.
- Team 1 ran the same module (SDL 3.4.14, gfopenrtg 6e3ae8e3) on AC090 on 18 November: SDL's render comparison test passed 18 scenes.
