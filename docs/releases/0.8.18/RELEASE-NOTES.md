# GFOpenUp 0.8.18 (6 November 2026)

This release is GFOpenUp 0.8.17 with one change. **SDL2.module** can no longer freeze the whole machine while a game plays sound.

## What's fixed

**The SDL 2 freeze** (gfopenrtg #179). SDL games could stop the whole machine at random while they played sound. Jump 'n Bump and Flare did it in the games lab: the game, the Workbench and the file systems all stopped, and only a reset helped.

SDL's audio thread runs at priority 10. On AmigaOS 3.x SDL has no native atomic operations, so it made them from spin locks. When the audio thread found one of those locks held by the game (priority 0), it spun for ever. Exec never runs a lower priority task while a higher one is ready, so the game never got the chance to let go, and every task at priority 5 or below stopped with it.

- SDL's atomic operations now run inside `Forbid()`, with no spin lock.
- A task waiting on one of SDL's other locks now sleeps instead of spinning.
- `SDL_Delay(0)` now sleeps for a millisecond. It used to call `Delay(0)`, which returns without sleeping.

The "No signal bits available for thread sync" error some games gave after a few runs from one Shell was fixed earlier, by gfopenrtg #173 (2 November), and this module has that fix too: any task can wait for a thread, and a thread no longer takes a signal bit from the task that started it.

## Parts changed against 0.8.17

| Part | Change |
|---|---|
| GFOpenGPU | 0.25 to 0.26: SDL2.module, in `LIBS:GFOpenGPU/` and `LIBS:OpenGPU/`. gfopengpu.library, GL.module, SDL2_image.module and SDL2_mixer.module are unchanged. |
| GFOpenUp | 0.8.17 to 0.8.18: the Installer's and the uninstaller's version lines. GFOpenUpTool is still 0.8.16, because its source hasn't changed. |

Programs need nothing new: they load SDL2.module when they start, so every SDL 2 program gets the fix.

## How it was built

The pack is 0.8.17's archive with only those files changed. Every other member is copied from `GFOpenUp-0.8.17.lha` byte for byte. SDL2.module (1,002,828 bytes, sha256 `095bbef8d860603bbf7ad09edd91f2b26cee0c7b54ab2588200c947649769191`) was built from gfopenrtg main (9e372f4, #179) with `library/modules/sdl2/build.sh` on the os32-gcc16 stove (OpenAmigaGCC 16.2.0-open4).

## Tested

This was a lab copy of Instance-32 (Instance-45, the games lab), with this SDL2.module in `LIBS:OpenGPU/` and `LIBS:GFOpenGPU/`:

- `threadcheck.m` (gfopenrtg `tests/sdl2`) ran twice from the same Shell, and all 13 checks passed both times. Its `priority` check runs a thread at priority 10 against main() at 0 writing files, and `signals` checks main()'s signal bits are the same after `SDL_Quit` as before `SDL_Init` (0xffff). The 26 October module hung on `priority`.
- Commander Genius started 3 times from one Shell. The old module failed on the third start with "No signal bits available".
- Jump 'n Bump played for 10 minutes with sound, with no freeze (94% idle).
- Flare played for about 12 minutes with sound, and saved.
- Mouse clicks in a window landed where the pointer was. The 26 October module put them about 16 pixels right and 32 down.
