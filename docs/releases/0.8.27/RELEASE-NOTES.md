# GFOpenUp 0.8.27 (9 November 2026)

This release is GFOpenUp 0.8.27-rc1 with a new **SDL2.module** from gfopenrtg main, which adds the standard Right Amiga keys (gfopenrtg #218). Against 0.8.26 it brings:
- mitchy.library 1.0 and ServiceTest 0.2, in GFOpenService 1.3;
- AkikoPlus 2.0, a new part;
- the same control keys in every SDL 2 program, and in every SDL 1.2 program through sdl12-compat.

rc1's notes are in `docs/releases/0.8.27-rc1/`. The candidate had 0.8.26's SDL2.module (`4e98af54...`); this release has the new one. AmiSans (#218), OpenAL.module (#219), the CPU floor (#220) and HostClock (#221) were merged after rc1 and are not in this pack. They go in 0.8.28.

## What's new

From 0.8.27-rc1:
- **mitchy.library 1.0** (gfopenservice #34, in the GFOpenService part, now 1.3). Any program can use the services card through it: it opens a service by name, sends requests to it, waits or carries on, and can cancel. When no board or paired Cradle offers a service it says so (`MERR_NOTFITTED`), and the program does the work on the 68k.
- **ServiceTest 0.2** (`C:ServiceTest`), rewritten on mitchy.library.
- **AkikoPlus 2.0, new part** (gfosautoconfigboards #4): `akikoplus.library`, `C:AkikoTest`, `C:APTest` and `SYS:Developer/AkikoPlus`. With AmiCradle's AkikoPlus media board fitted, chunky to planar, byte swaps, fills, scaling, YUV to ARGB, colour reduction and zlib run on the board; without it they run on the 68k with the same results.

New in the final:
- **The standard keys in SDL 2** (gfopenrtg #218). They are in the module, so programs get them without a rebuild:

  | Keys | What they do |
  | --- | --- |
  | Right Amiga + F | Window or full screen; the choice is kept in `ENVARC:GFGames/<program>/Screen` for next time |
  | Right Amiga + Q | Quit, as the close gadget |
  | Right Amiga + P | Pause: picture, sound and the game clock stop, and the title says so |
  | Right Amiga + M | Free the mouse from a relative-mode game, or take it back |
  | Right Amiga + S | A PNG screenshot in `PROGDIR:Screenshots/` |
  | Help | The controls card from `PROGDIR:Controls` when the drawer has one |
  | Right Amiga + F1, F2 | F11, F12 |
  | Right Amiga + cursor keys | Page Up, Page Down, Home, End |

  The `FULLSCREEN` and `WINDOW` tooltypes set how a program starts. `SDL_AMIGA_STANDARD_KEYS=0` turns the keys off.

  0.8.26's full-screen exit fix (gfopenrtg #217) is still in the module.

## Parts changed against 0.8.27-rc1

| Part | Change |
|---|---|
| GFOpenGPU | `LIBS:GFOpenGPU/SDL2.module` (and its copy in `LIBS:OpenGPU/`) from gfopenrtg main at 1975197, which has #217 and #218. The part is still 0.29, because its version is gfopengpu.library's, and that library hasn't changed. GFOpenUpTool INSTALL replaces the changed file anyway. Everything else in the part is rc1's, which is 0.8.26's. |
| GFOpenUp | 0.8.27-rc1 to 0.8.27: `Install-GFOpenUp` (`$VER` and `#version`), both copies of `Uninstall-GFOpenUp` (`$VER`), the readme's Version line, and the part |

## How it was built

**SDL2.module:**
- Built from gfopenrtg main at `19751971dfffaa912e92d9848cbe6e602750ae68` with `library/modules/sdl2/build.sh ... module stub static tests`, the GCC 16 stove and `JOBS=4`. This is the same recipe and stove as 0.8.26's module.
- 1,009,084 bytes, sha256 `8ec87228267f7e4bf329373ca20c67f150679fac728815b501dd38ae64203871`.
- `baserel_check`: the data is written only through A4. `fpcr_check`: none.
- The host checks in `tests/sdl2/` pass: `test_gzz_mouse`, `test_textinput_patch`, `test_threadjoin` and `test_windowresize`.

**The archive:**
- It is 0.8.27-rc1's released archive (`9c24dae6...`), member by member. Of its 2,946 members, 2,940 are raw byte-identical to rc1.
- The other six are the module, the GFOpenUp Part's version, the Installer's `$VER` and `#version`, both Uninstall copies' `$VER`, and the readme's Version line.
- It tests clean with `lha t` and `lhasa t`.
- It extracts to the same 2,580 files as rc1's, and no file in it still reads 0.8.27-rc1.

`GFOpenUp-0.8.27.lha`, 63,423,299 bytes, sha256 `ab5975f6286d17aa84f99a7283b657cfefc834a4f6f0a6c5510a9906ebdb1d7d`. `SHA256SUMS` in this folder lists it.

## Tested

The tests ran on fresh copies of Lab-50 (AmigaOS 3.2.3, 800x600 Workbench) and Lab-51 (GFOS, 1600x900 Workbench). Both had 0.8.26 installed. The pack was put in as `DH1:GFOpenUp`.

- **Install:** `CD DH1:GFOpenUp` and then `C/GFOpenUpTool REINSTALL Parts`. It returned rc 0 with "GFOpenGPU 0.29 installed (2 files changed)", and both `LIBS:GFOpenGPU/SDL2.module` and `LIBS:OpenGPU/SDL2.module` were then `8ec87228...`. These labs had only the GFOpenGPU part recorded, so Reinstall did that part alone.
- **Full screen (`fsrestore.m`, built with the module):** all 7 steps pass on both labs, with the surface and the renderer. After `SDL_Quit` the Workbench is in front at its own mode, with no SDL screens left.
- **Threads (`threadcheck.m`):** every check passes on both labs except `stack-hint`. There, a 64 KB hint gets a bigger stack (128 KB on Lab-51, 1 MB on Lab-50). With 0.8.26's module on the same lab the result is the same, so this comes from the labs' OS, not from this module.
- **The standard keys, with Canvas Skies (SDL 2), on both labs:**
  - Right Amiga + F went to full screen, and pressing it again went back to the window with its border.
  - Canvas Skies' own F9 went to full screen and back.
  - Right Amiga + P paused, and the title read "Canvas Skies (paused - Right Amiga + P)". Pressing it again went on.
  - Right Amiga + M twice did no harm. Canvas Skies doesn't use a relative mouse, so it had nothing to free.
  - Right Amiga + Q quit to a clean Workbench, and the process was gone.

rc1's parts (mitchy.library, ServiceTest, AkikoPlus) are byte for byte rc1's. Their tests are in `docs/releases/0.8.27-rc1/RELEASE-NOTES.md`.

Not tested:
- Right Amiga + S and Help;
- an SDL 1.2 program through sdl12-compat;
- the Installer's own window;
- Uninstall-GFOpenUp.

## Upgrading

Reinstall puts back only parts that are already installed. To add AkikoPlus to a machine that has 0.8.26, use the Installer's "Install or upgrade" (or `GFOpenUpTool INSTALL Parts/AkikoPlus`), as rc1's notes say.

## Known issues

- 0.8.26's known issues still stand.
- Abe ignores `SDL_QUIT` on its title screen, so Right Amiga + Q does nothing there. That is the game's to fix, port by port.
