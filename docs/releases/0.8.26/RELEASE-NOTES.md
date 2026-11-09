# GFOpenUp 0.8.26 (9 November 2026)

This release is GFOpenUp 0.8.26-rc1 with **SDL2.module** from gfopenrtg #217 added. Against 0.8.25 it brings:
- the blanker's roll call;
- menus drawn off screen;
- the drawer menu drawn off screen;
- smaller printed PDFs;
- the installer fixes: Reinstall, a safe Uninstall, and the Setup wizard's step-2 hang;
- the hardened full-screen exit in SDL 2.

0.8.26 hardens full-screen exit; anyone still carrying the pre-0.8.18 module in LIBS:OpenGPU/ gets the fix by installing this pack.

rc1's notes are in `docs/releases/0.8.26-rc1/`. The candidate didn't have the new SDL2.module; this release does. gfopenrtg #218 (Right Amiga keys in SDL 2) is not in it.

## What's new

From 0.8.26-rc1:
- **GFOpenBlanker's roll call says AmiCradle** (gfopenprefs #186). The first and last cards show the AmiCradle wordmark, and the sign-off reads "The AmiCradle Team" once, not twice. GFOpenBlanker stays 1.4.
- **GFOpenMenus 0.4.8** (gfopenprefs #187). Each menu panel is drawn off screen and then copied to the screen, so menus no longer flicker when they open or when the highlight moves.
- **The drawer window's menu (the three dots) is drawn off screen** (gfopengadtools #100), and redrawn only when something changes. It is in GFOpenBench, which has the drawer windows built in. GFOpenBench stays 0.1.44.
- **Smaller printed pictures** (gfopenprint #48).
  - GFOpenView and OAVWorker scale a picture down to 300 dpi at its printed size, never up, and write it Flate-compressed. A 3840x2228 picture on A4 was a 25 MB PDF; it is now about 4 to 5 MB.
  - When a printer stops taking a job (less than 64 KiB in 30 seconds), the upload ends and says so.
  - Upload progress moves every 64 KiB.
- **Reinstall, and an Uninstall that asks** (gfopenup #214, GFOpenUpTool 0.8.17).
  - When GFOpenUp is installed already, Install-GFOpenUp offers Reinstall, the default when the pack is the version installed. It puts every installed part in again, writes back missing or changed files, and keeps settings and startup lines.
  - Uninstall-GFOpenUp asks first at every level, Novice too, and Abort changes nothing. On the GFOpenKick ROM it warns that the desktop and the programs go too, and points to Reinstall instead.
  - INSTALL records each file's size and CRC-32. UNINSTALL leaves a file that has changed since GFOpenUp wrote it.
- **GFOpenUp Setup 0.1.8: step 2 no longer locks the Amiga** (gfopenprefs #188). Advanced, Next, Back, Next on "Your desktop" made a ring of the window's gadgets, and Intuition followed it forever. Both copies of the wizard have the fix: `SYS:Prefs/GFOpenUp-Setup`, and the copy built into GFOpenBench, which is the one WBStartup runs on GFOS.
- Install-GFOpenUp also moves the old OpenPNG, OpenTGA and OpenGIF datatype descriptors to SYS:Storage when it installs GFOpenImage (gfopenup #202: merged before 0.8.25, but not in its pack). Uninstall puts them back.

New in the final:
- **Full-screen exit, hardened** (gfopenrtg #217). The fault was seen with an older SDL2.module (`6c85c04c...`, from before 0.8.18). When an SDL 2 program left real full screen, or quit while in it, the display never went back to the Workbench:
  - On AmigaOS 3.2.3 the Workbench came back as four squashed copies of itself, because the board stayed at the game's 640x480 under the 800x600 Workbench.
  - On GFOS the screen stayed blank grey.
  - SDL's own screens were left in front of the Workbench, one more each time.

  That older module is still in `LIBS:OpenGPU/` on machines that never had the GFOpenGPU part. 0.8.25's module (`8d6e9570...`, in the pack since 0.8.18, and still rc1's) doesn't show the fault: it passes every check below. The new module adds patch 0019, which makes the way back to the Workbench more robust:
  - When Intuition refuses to close a screen, the screen is sent behind the Workbench and closed later.
  - To get back to the Workbench, the module closes any screens it has left, brings the Workbench to the front, and calls `RemakeDisplay()` so the board takes the Workbench's mode again. It then opens and closes a borderless window over the Workbench, so Intuition repaints it.
  - Leaving full screen or full-screen desktop does that, then reopens the window on the Workbench at its windowed size, in front and active.
  - `SDL_DestroyWindow`, and `SDL_Quit` while in full screen, do the same. When SDL's video stops, any screen still left is released.
  - The window on the Workbench opens with `WA_AutoAdjust`.

  Programs need nothing new: they load SDL2.module when they start. The GFOpenGPU part puts the module in both `LIBS:GFOpenGPU/` and `LIBS:OpenGPU/`, so installing it replaces an older copy in `LIBS:OpenGPU/` too. A program's own workaround (closing and reopening its window) does no harm with it.

## Parts changed against 0.8.25

| Part | Change |
|---|---|
| GFOpenBlanker | still 1.4: `Tools/Commodities/GFOpenBlanker` |
| GFOpenMenus | 0.4.7 to 0.4.8: `C:GFOpenMenus` |
| GFOpenBench | still 0.1.44: `System/GFOpenBench` and `System/OpenBench` (the same file), with gfopengadtools #100 and gfopenprefs #188 |
| GFOpenPrint | still 0.4: `C:GFOpenPrint`, `C:GFOpenView`, `C:OAVWorker`, and the copies of GFOpenView in the drawer and at `SYS:Utilities/MultiView` |
| GFOpenUpSetup | 0.1.7 to 0.1.8: `Prefs/GFOpenUp-Setup` |
| GFOpenGPU | `LIBS:GFOpenGPU/SDL2.module` (and its copy in `LIBS:OpenGPU/`) from gfopenrtg #217. The part is still 0.29, because its version is gfopengpu.library's, and that library hasn't changed. GFOpenUpTool INSTALL replaces the changed file anyway. gfopengpu.library, ACRTG.gpu, GL.module, the SDL 2 satellites (SDL2_image.module, SDL2_mixer.module) and the licences are 0.8.25's. |
| GFOpenUp | 0.8.25 to 0.8.26: `C/GFOpenUpTool` (0.8.17) and its copy in the part, `Install-GFOpenUp` and both copies of `Uninstall-GFOpenUp` (from `pack/` at a844c0f), the readme, and the part |

ogles3.library (OGLES3 1.1) is unchanged. Against 0.8.25, 20 files differ: rc1's 19 and SDL2.module. None was added or removed.

## How it was built

The pack is 0.8.26-rc1's released archive (sha256 `bd1f91b5...`) with only SDL2.module and the version lines changed, as the packs since 0.8.16 were made. Every other member is copied raw from `GFOpenUp-0.8.26-rc1.lha`: 2,911 of its 2,917 members are byte-identical, headers included, and in the same order. The six that changed:
- `Parts/GFOpenGPU/Libs/GFOpenGPU/SDL2.module`: `8d6e9570...` to `4e98af54...`;
- `Parts/GFOpenUp/Part`: `part GFOpenUp 0.8.26-rc1` to `0.8.26`;
- `Install-GFOpenUp`: `$VER` and `#version` from 0.8.26-rc1 to 0.8.26;
- both copies of `Uninstall-GFOpenUp`: `$VER` from 0.8.26-rc1 to 0.8.26;
- `GFOpenUp.readme`: `Version:` from 0.8.26-rc1 to 0.8.26.

The script checks the rc1 archive's sha256, the old and new modules' sha256s, that the GFOpenGPU part (0.29, unchanged) still copies SDL2.module to both drawers, each version line it changes, and that no changed text still says rc1. rc1's own programs are as its notes describe.

SDL2.module was built from gfopenrtg main at **b407b01** (the squash merge of #217) with `library/modules/sdl2/build.sh <OUT> module stub static tests`, as 0.8.18's was from 29d32bf:
- the `os32-gcc16-canvas` stove (OpenAmigaGCC 16.2.0-open4);
- SDL 2.32.10 (UPSTREAM.json's tarball, by its SHA-256);
- libSDL2-amigaos3 at its pinned 1eefa8f3;
- the same CyberGraphX and AHI headers and vasm.

Its `baserel_check` and `fpcr_check` pass. It is byte for byte the module the lab tests below ran as "new", which was built from #217's last commit, b419db1 (the same tree as b407b01). The module keeps the build directory's path in two of SDL's source file names, so it was built in the same directory to compare. gfopenrtg #218 (c1ac7d2) came after b407b01, so it isn't in this module.

| File | From | Bytes | sha256 |
|---|---|---|---|
| SDL2.module | gfopenrtg b407b01 `library/modules/sdl2/build.sh` | 1,003,932 | `4e98af54a7fa0f5a3bfd7bd7e83a3d8c468099e31a552a7278e78d2b1c124e70` |

0.8.25's and rc1's SDL2.module (0.8.18's, from gfopenrtg 29d32bf) was 1,003,264 bytes (`8d6e9570...`). Its SDL 2 source is b407b01's without patch 0019.

| | |
|---|---|
| Archive | `GFOpenUp-0.8.26.lha`, 63,362,403 bytes |
| sha256 | `33e3d67bc2e561908b4990217a3b3bc1b62be3c0a1ae2b39bab0ec3def339688` |

The earlier SDL2-only pack on 0.8.25 (`e693f86c...`, from gfopenup #213) was never published. This one replaces it.

## Tested

**SDL2.module.** gfopenrtg's new test `fsrestore` (`tests/sdl2/fsrestore.c`) goes window, full screen, window, full-screen desktop, window, full screen, then `SDL_Quit`. After each of those 7 steps it checks:
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

The earlier SDL2-only pack was also installed in a lab. This was a fresh lab copy of an AmigaOS 3.2.3 machine that had only the older module in `LIBS:OpenGPU/` and no `LIBS:GFOpenGPU/`:
- `GFOpenUpTool INSTALL DH1:GFOpenUp/Parts/GFOpenGPU` reported "GFOpenGPU 0.29 installed (20 files changed)".
- Both `LIBS:GFOpenGPU/SDL2.module` and `LIBS:OpenGPU/SDL2.module` were then 1,003,932 bytes, `4e98af54...`.
- `fsrestore.m` passed all 7.

That pack's GFOpenGPU part is the same as this one's.

**rc1's parts** were tested on `t5-up0826rc1-20261109`, a copy of Instance-40: blanker, menus, drawer menu, print to PDF, the Setup wizard and Reinstall. The results are in `docs/releases/0.8.26-rc1/RELEASE-NOTES.md`. Those files are byte for byte rc1's in this pack.

**The archive:**
- It tests clean with `lha t` and `lhasa t`.
- It extracts to the same 2,566 files as rc1's, in the same places. Only the six files above differ.
- `Parts/GFOpenGPU/Libs/GFOpenGPU/SDL2.module` is `4e98af54...`, and the GFOpenGPU part copies it to both `LIBS:GFOpenGPU/SDL2.module` and `LIBS:OpenGPU/SDL2.module`.
- No file in it still reads 0.8.26-rc1. `SHA256SUMS` in this folder lists the archive.

Not tested, as in rc1:
- the menu flicker itself;
- the mouse-held right-button menu;
- a printer that stalls;
- Uninstall-GFOpenUp.

## Known issues

- 0.8.25's known issues still stand.
- A smart-refresh Workbench window that was painted over can keep stale pixels in its backing store after full screen. The repaint covers the backdrop and simple-refresh windows.
