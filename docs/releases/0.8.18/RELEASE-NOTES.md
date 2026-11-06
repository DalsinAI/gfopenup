# GFOpenUp 0.8.18 (6 November 2026)

This release is GFOpenUp 0.8.17 with a new **SDL2.module**. It can no longer freeze the whole machine while a game plays sound, and it fixes the mouse in relative mode, full screen at the desktop size and games that raise their own priority.

## What's fixed

**The SDL 2 freeze** (gfopenrtg #179). SDL games could stop the whole machine at random while they played sound. Jump 'n Bump and Flare did it in the games lab: the game, the Workbench and the file systems all stopped, and only a reset helped.

SDL's audio thread runs at priority 10. On AmigaOS 3.x SDL has no native atomic operations, so it made them from spin locks. When the audio thread found one of those locks held by the game (priority 0), it spun for ever. Exec never runs a lower priority task while a higher one is ready, so the game never got the chance to let go, and every task at priority 5 or below stopped with it.

- SDL's atomic operations now run inside `Forbid()`, with no spin lock.
- A task waiting on one of SDL's other locks now sleeps instead of spinning.
- `SDL_Delay(0)` now sleeps for a millisecond. It used to call `Delay(0)`, which returns without sleeping.

The "No signal bits available for thread sync" error some games gave after a few runs from one Shell was fixed earlier, by gfopenrtg #173 (2 November), and this module has that fix too: any task can wait for a thread, and a thread no longer takes a signal bit from the task that started it.

**Relative mouse** (gfopenrtg #181). A game that turns on SDL's relative mouse mode (OpenFodder) got no mouse movement at all on AmiCradle, whose mouse sends positions. Each move is now the change in the pointer's position. A move past the screen's edge stops there.

**Full screen at the desktop size** (gfopenrtg #181). `SDL_WINDOW_FULLSCREEN_DESKTOP` and SDL's desktop mode were always 640x480. They are now the Workbench screen's size and format when the Workbench is on an RTG mode.

**Games that raise their priority** (gfopenrtg #181). `SDL_SetThreadPriority` raised a game's task to 5 or 10, and a raised task that busy-waits stops every task below it: EDuke32 froze after its skill screen. Only SDL's own sound threads now go above 0.

## Parts changed against 0.8.17

| Part | Change |
|---|---|
| GFOpenGPU | 0.25 to 0.26: SDL2.module, in `LIBS:GFOpenGPU/` and `LIBS:OpenGPU/`. gfopengpu.library, GL.module, SDL2_image.module and SDL2_mixer.module are unchanged. |
| GFOpenUp | 0.8.17 to 0.8.18: the Installer's and the uninstaller's version lines. GFOpenUpTool is still 0.8.16, because its source hasn't changed. |

Programs need nothing new: they load SDL2.module when they start, so every SDL 2 program gets the fix.

## How it was built

The pack is 0.8.17's archive with only those files changed. Every other member is copied from `GFOpenUp-0.8.17.lha` byte for byte. SDL2.module (1,003,264 bytes, sha256 `8d6e95705ca6f00b37f93f2520609883acebdbada45fa805d48f47f5044eaa31`) was built from gfopenrtg 29d32bf, the last commit of #181, whose module source is the same as main (102f5b6, #179 and #181), with `library/modules/sdl2/build.sh` on the os32-gcc16 stove (OpenAmigaGCC 16.2.0-open4).

## Tested

The #179 module, then #181's, were tested in two labs on a copy of Instance-32 (Instance-45, the games lab).

This was a lab copy of Instance-32 (Instance-45, the games lab), with this SDL2.module in `LIBS:OpenGPU/` and `LIBS:GFOpenGPU/`:

- `threadcheck.m` (gfopenrtg `tests/sdl2`) ran twice from the same Shell, and all 13 checks passed both times. Its `priority` check runs a thread at priority 10 against main() at 0 writing files, and `signals` checks main()'s signal bits are the same after `SDL_Quit` as before `SDL_Init` (0xffff). The 26 October module hung on `priority`.
- Commander Genius started 3 times from one Shell. The old module failed on the third start with "No signal bits available".
- Jump 'n Bump played for 10 minutes with sound, with no freeze (94% idle).
- Flare played for about 12 minutes with sound, and saved.
- Mouse clicks in a window landed where the pointer was. The 26 October module put them about 16 pixels right and 32 down.

With #181's module (6 November): OpenFodder's crosshair follows the mouse and its menus take clicks. Full screen at the desktop size came up at 1920x1080, the Workbench's size. LTris, without its own code to close them, left no empty screens behind. Blob Wars' clicks land on the pointer. Flare's threaded loading gave no "No signal bits" errors (there were 90 before). OpenLieroX ran without its workarounds. threadcheck passed. EDuke32, with no workaround, went from its skill screen into E1L1 with the game at priority 0 and its sound thread at 10, and quit cleanly.

## Known issues

- Blob Wars stops with a memory-corruption alert while it loads 8-bit RGB pictures, so ports keep converting their PNGs to RGBA.
- OpenLieroX's mouse is about 25 pixels off when sdl12-compat scales it to 1920x1080.
- Commander Genius loses about 2.6 MB each time its launcher runs.
