# GFOpenUp 0.8.26 (9 November 2026)

This release is GFOpenUp 0.8.25 with one file changed: **SDL2.module**, GFOpenGPU's SDL 2, from gfopenrtg #217.

0.8.26 hardens full-screen exit; anyone still carrying the pre-0.8.18 module in LIBS:OpenGPU/ gets the fix by installing this pack.

## What's changed

**Full-screen exit, hardened** (gfopenrtg #217). The fault was seen with an older SDL2.module (`6c85c04c...`, from before 0.8.18). When an SDL 2 program left real full screen, or quit while in it, the display never went back to the Workbench:

- On AmigaOS 3.2.3 the Workbench came back as four squashed copies of itself, because the board stayed at the game's 640x480 under the 800x600 Workbench.
- On GFOS the screen stayed blank grey.
- SDL's own screens were left in front of the Workbench, one more each time.

That older module is still in `LIBS:OpenGPU/` on machines that never had the GFOpenGPU part. 0.8.25's module (`8d6e9570...`, in the pack since 0.8.18) doesn't show the fault: it passes every check below.

The new module adds patch 0019, which makes the way back to the Workbench more robust:

- When Intuition refuses to close a screen, the screen is sent behind the Workbench and closed later.
- To get back to the Workbench, the module closes any screens it has left, brings the Workbench to the front, and calls `RemakeDisplay()` so the board takes the Workbench's mode again. It then opens and closes a borderless window over the Workbench, so Intuition repaints it.
- Leaving full screen or full-screen desktop does that, then reopens the window on the Workbench at its windowed size, in front and active.
- `SDL_DestroyWindow`, and `SDL_Quit` while in full screen, do the same. When SDL's video stops, any screen still left is released.
- The window on the Workbench opens with `WA_AutoAdjust`.

Programs need nothing new: they load SDL2.module when they start. The GFOpenGPU part puts the module in both `LIBS:GFOpenGPU/` and `LIBS:OpenGPU/`, so installing it replaces an older copy in `LIBS:OpenGPU/` too. A program's own workaround (closing and reopening its window) does no harm with it.

## Parts changed against 0.8.25

| Part | Change |
|---|---|
| GFOpenGPU | `LIBS:GFOpenGPU/SDL2.module` (and its copy in `LIBS:OpenGPU/`) from gfopenrtg #217. The part is still 0.29, because its version is gfopengpu.library's, and that library hasn't changed. GFOpenUpTool INSTALL replaces the changed file anyway. gfopengpu.library, ACRTG.gpu, GL.module, the SDL 2 satellites (SDL2_image.module, SDL2_mixer.module) and the licences are 0.8.25's. |
| GFOpenUp | 0.8.25 to 0.8.26: the version lines in the Installer, both uninstaller copies, the readme and the part. |

ogles3.library (OGLES3 1.1) is unchanged.

## How it was built

The pack is 0.8.25's released archive (sha256 `341278af...`) with only those files changed, as the packs since 0.8.16 were made. Every other member is copied from `GFOpenUp-0.8.25.lha` byte for byte: 2,911 of its 2,917 members are byte-identical, headers included, and in the same order. The six that changed are SDL2.module, `Parts/GFOpenUp/Part`, `Install-GFOpenUp`, both copies of `Uninstall-GFOpenUp`, and `GFOpenUp.readme`. In the last five only the version lines changed.

SDL2.module was built from gfopenrtg main at **b407b01** (the squash merge of #217) with `library/modules/sdl2/build.sh <OUT> module stub static tests`, as 0.8.18's was from 29d32bf:
- the `os32-gcc16-canvas` stove (OpenAmigaGCC 16.2.0-open4);
- SDL 2.32.10 (UPSTREAM.json's tarball, by its SHA-256);
- libSDL2-amigaos3 at its pinned 1eefa8f3;
- the same CyberGraphX and AHI headers and vasm.

Its `baserel_check` and `fpcr_check` pass. It is byte for byte the module the lab tests below ran as "new", which was built from #217's last commit, b419db1 (the same tree as b407b01). The module keeps the build directory's path in two of SDL's source file names, so it was built in the same directory to compare.

| File | From | Bytes | sha256 |
|---|---|---|---|
| SDL2.module | gfopenrtg b407b01 `library/modules/sdl2/build.sh` | 1,003,932 | `4e98af54a7fa0f5a3bfd7bd7e83a3d8c468099e31a552a7278e78d2b1c124e70` |

0.8.25's SDL2.module (0.8.18's, from gfopenrtg 29d32bf) was 1,003,264 bytes (`8d6e9570...`). Its SDL 2 source is b407b01's without patch 0019.

| | |
|---|---|
| Archive | `GFOpenUp-0.8.26.lha`, 63,326,714 bytes |
| sha256 | `e693f86c2795477e8f0cbadbc5401a9ea45112bfea1d511c6a13b3fcf5ffb070` |

## Tested

gfopenrtg's new test `fsrestore` (`tests/sdl2/fsrestore.c`) goes window, full screen, window, full-screen desktop, window, full screen, then `SDL_Quit`. After each of those 7 steps it checks:
- which screen is in front, and its mode;
- that no SDL screens are left;
- the screen count;
- that the window is on the Workbench with its border, at its size.

It ran on lab copies only, with each module in `LIBS:GFOpenGPU/`:

| Lab | SDL2.module | fsrestore |
|---|---|---|
| Lab-50, AmigaOS 3.2.3, 800x600 Workbench | `6c85c04c...` (older, from before 0.8.18) | **6 of 7 fail**: 640x480 SDL screens left in front, 4 of them after quit |
| Lab-50 | `8d6e9570...` (0.8.25) | **all 7 pass** (module build `fsrestore.m`) |
| Lab-50 | `4e98af54...` (0.8.26) | **all 7 pass**, static and module builds |
| Lab-51, GFOS, 1600x900 Workbench | `6c85c04c...` (older, from before 0.8.18) | **6 of 7 fail**, the same way |
| Lab-51 | `8d6e9570...` (0.8.25) | **all 7 pass** (module build `fsrestore.m`) |
| Lab-51 | `4e98af54...` (0.8.26) | **all 7 pass**, surface and renderer |

Canvas Skies, which has its own workaround, with the new module on Lab-51: F9 into full screen and back, twice, then Esc from full screen, gave a clean Workbench every time.

Installed from the archive: on a fresh lab copy of an AmigaOS 3.2.3 machine that had only the older module in `LIBS:OpenGPU/` and no `LIBS:GFOpenGPU/`, `GFOpenUpTool INSTALL DH1:GFOpenUp/Parts/GFOpenGPU` reported "GFOpenGPU 0.29 installed (20 files changed)". Both `LIBS:GFOpenGPU/SDL2.module` and `LIBS:OpenGPU/SDL2.module` were then 1,003,932 bytes, `4e98af54...`, and `fsrestore.m` passed all 7.

The archive:
- It tests clean with `lha t` and `lhasa t`.
- It extracts to the same 2,566 files as 0.8.25's, in the same places. Only the six files above differ.
- `Parts/GFOpenGPU/Libs/GFOpenGPU/SDL2.module` is `4e98af54...`, and the GFOpenGPU part (unchanged, 0.29) copies it to both `LIBS:GFOpenGPU/SDL2.module` and `LIBS:OpenGPU/SDL2.module`.
- No version line still reads 0.8.25. `SHA256SUMS` in this folder lists the archive.

## Known issues

- 0.8.25's known issues still stand.
- A smart-refresh Workbench window that was painted over can keep stale pixels in its backing store after full screen. The repaint covers the backdrop and simple-refresh windows.
