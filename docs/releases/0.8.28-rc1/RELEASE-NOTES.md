# GFOpenUp 0.8.28-rc1 (9 November 2026), release candidate 1

This pack is the released GFOpenUp 0.8.27 with everything gfopenup main merged after 0.8.27-rc1: AmiSans, OpenAL.module, the CPU floor and HostClock. SDL2.module is 0.8.27's, with the standard Right Amiga keys.

## What's new

- **AmiSans** (#218, in the GFOpenFont part, still 1.8). TeX Gyre Heros, the open stand-in for OS 3.2.3's CGTriumvirate:
  - `FONTS:AmiSans.font` with the hinted bitmap sizes `FONTS:AmiSans/12` and `13`;
  - `FONTS:AmiSans.otag` for every other size: engine "bullet", with CGTriumvirate's own `OT_YSizeFactor`, so a size has the same em;
  - `FONTS:_otf/texgyreheros-regular.otf`, unmodified;
  - the GUST Font License in `SYS:Utilities/GFOpenUp/Licences/AmiSans.txt`.

  The lines are `add`, so an AmiSans another program put there stays. GFOpenUp writes no Font prefs.
- **OpenAL.module** (#219, in the GFOpenGPU part). OpenAL 1.1 and ALC 1.1 on AHI, or on ACAHI's voices; no EFX, no capture. It is at `LIBS:GFOpenGPU/OpenAL.module` and copied to `LIBS:OpenGPU/` like the other modules. It is its own file: gfopengpu.library is unchanged, and only programs that use OpenAL load it.

  The GFOpenGPUDev kit (still 0.7) gets OpenAL's developer files, as gfopenrtg #233 puts them there:
  - `include/AL/` (al.h, alc.h, alext.h);
  - `lib/libopenal.a` and `lib/libopenal_static.a`;
  - `lib/pkgconfig/openal.pc`;
  - `lib/cmake/OpenAL/` (`find_package(OpenAL)`, `OpenAL::OpenAL`);
  - their lines in the kit's README and in `Licences/README`.
- **The CPU floor** (#220). GFOpenGPU's modules (SDL2, GL, OpenAL) are built for a 68040 or 68060 with its FPU and MMU:
  - The part says `needs 68040`. **GFOpenUpTool 0.8.18** leaves the part out on anything less and says so. Older GFOpenUpTools ignore the line.
  - Install-GFOpenUp leaves GFOpenGPU out on such an Amiga with a message, as it does Warp3D.
- **HostClock, new part** (#221, 47.101). `C:HostClock` is amicradle-guest's GFOpenBattClock as a command:
  - Early in the boot it puts the host-backed battclock.resource in place of the Kickstart's, and sets the system time from it once.
  - From then on its task keeps the Amiga's time with the host's, across snapshot resumes and when the host's clock is set.
  - The part needs AmiCradle. Install-GFOpenUp installs it first and always, so its line is the first in GFOpenUp's block of `S:User-Startup`.
  - On a runtime without the clock registers it says so, leaves the Kickstart's resource, and returns 5.

## Parts changed against 0.8.27

| Part | Change |
|---|---|
| GFOpenFont | still 1.8: AmiSans (`FONTS:AmiSans.font`, `AmiSans/12`, `AmiSans/13`, `AmiSans.otag`, `_otf/texgyreheros-regular.otf`) and `Licences/AmiSans.txt` |
| GFOpenGPU | still 0.29: `LIBS:GFOpenGPU/OpenAL.module` (new, copied to `LIBS:OpenGPU/` too) and `needs 68040` |
| GFOpenGPUDev | still 0.7: OpenAL's headers, libraries, pkg-config and CMake files; the README and `Licences/README`; and `SHA256SUMS` made again (see below) |
| HostClock | new, 47.101: `C:HostClock` and its startup line |
| GFOpenUp | 0.8.27 to 0.8.28-rc1: `C/GFOpenUpTool` (0.8.18) and its copy in the part, `Install-GFOpenUp` and both copies of `Uninstall-GFOpenUp` (from main's `pack/`), the readme's Version line, and the part |

## How it was built

The pack is 0.8.27's released archive (sha256 `ab5975f6...`) with members replaced and added, as the packs since 0.8.16 are made:
- 13 members changed, and 17 files and 6 drawers were added. All 2,933 other members are byte for byte 0.8.27's.
- The Part lines are written by `tools/build_pack.py`'s own code at main: `Part`, `amisans()` and `binary_version()`. The kit's new files and lines come from gfopenrtg's `tools/make_sdk.py` (`pc_text`, `CMAKE_OPENAL`, its README and notice lines).
- Before it changed anything, the script checked that `build_pack.py` writes 0.8.27's GFOpenGPUDev Part from 0.8.27's kit exactly.
- The kit's `SHA256SUMS` was made again over the whole kit, as make_sdk.py makes it. 0.8.27's was stale for three files replaced in the kit after it was written (`lib/libGL.a`, `include/gla/gla_core.h`, `include/clib/Warp3D_protos.h`); it is right for those now too, and `sha256sum -c` passes on the extracted kit.

| File | From | Bytes | sha256 |
|---|---|---|---|
| `Libs/GFOpenGPU/OpenAL.module` | gfopenrtg 1975197, `library/modules/openal/build.sh`, GCC 16 stove (the same as SDL2.module) | 58,524 | `67050c6745060dca2f80bc8d86ceba21e2bd83b03a33c8961849b0c5deb167c8` |
| `lib/libopenal.a`, `lib/libopenal_static.a` | the same build | 16,300 and 63,904 | `4ae026a8...`, `6b903bca...` |
| `C/GFOpenUpTool` 0.8.18 | gfopenup 02092df (main; `src/` and `pack/` as at 3cf3b6b, #220), Cradle's stove_build recipe with the os32-gcc16 stove | 109,312 | `bc06e0b4b728aa168305ef3108747d251368e2e25116e4fde871e2272c4dded2` |
| `C/HostClock` 47.101 | amicradle-guest 1eb4d7f, `resources/openbattclock.resource/build-amiga.sh`, os32-gcc16 | 7,632 | `943ea1ea88f6cda2a48541037ebdb4f1044636d7df5602dc7ac18635b317e4c6` |
| AmiSans | gfopenfont 6afe55d `fonts/outline` (#37); the .otf is sha256-pinned in build_pack.py | 524, 3,452, 3,756, 80, 133,600 | `.otf` `6ae1a09d...` |

The same recipe built a844c0f's GFOpenUpTool byte for byte as 0.8.27 carries it (`e5f4b477...`). HostClock built on two machines gave the same bytes. OpenAL.module: `baserel_check` says the data is written only through A4, and `fpcr_check` found nothing.

Host checks:
- gfopenup `tests/test_build_pack.py` (with `amisans_checks`) and `tests/test_openup_lines.c` pass.
- gfopenrtg `tests/openal/run_host.sh` passes every core test: API 502 checks, 3D 1316, streams 183, threads 10, the mix against the board. Its last step, `stub_nomodule`, doesn't compile on main: `stub_tell.h` includes `exec/tasks.h`, which the host stand-ins don't have. That is the test's, not the module's.

**Archive:** `GFOpenUp-0.8.28-rc1.lha`, 63,580,421 bytes, sha256 `d5698d4445fa0d8c7d1a787a19d291ea339ea60c4725f744133a6fe76ed5909b`. It tests clean with `lha t` and `lhasa t`, and extracts to 2,597 files (0.8.27's 2,580 and 17 new).

## Tested

The tests ran on fresh copies of Lab-50 (AmigaOS 3.2.3, 800x600) and Lab-51 (GFOS, 1600x900), both with 0.8.26 installed, with the pack as `DH1:GFOpenUp`, and with GFOpenUpTool 0.8.18 from the pack:
- **Install:** `REINSTALL Parts` (GFOpenGPU: "installed (4 files changed)", so `needs 68040` passed on the labs' 68040), then `INSTALL Parts/HostClock`, `Parts/GFOpenFont` (12 files changed) and `Parts/GFOpenUp`. All returned rc 0.
  - Then both `LIBS:` drawers had OpenAL.module `67050c67...` and SDL2.module `8ec87228...`.
  - FONTS: had AmiSans.
  - `GFOpenUpTool STARTUP` wrote `C:HostClock QUIET` as the first line of GFOpenUp's block.
- **OpenAL:**
  - gfopenrtg's ALInfo: ALC 1.1 on "AHI Voices", `AL_VERSION "1.1 OpenAL.module 2"`, "all as expected".
  - ALTone: the tone, the tone round the listener, and a stream rising from 220 to 880 Hz, with 17 buffers streamed and 0 times dry.
- **HostClock:** these labs' runtime has no clock registers. HostClock said "not an AmiCradle machine, or its runtime has no clock registers: the Kickstart's battclock.resource stays" and returned 5, as designed. Its taking over the clock wasn't seen here.
- **SDL 2:** `fsrestore.m` passes all 7 steps, surface and renderer, on both labs. Canvas Skies, on both labs:
  - Right Amiga + F went to full screen and back;
  - F9 went to full screen and back;
  - Right Amiga + P paused;
  - Right Amiga + Q quit to a clean Workbench.

Not tested:
- HostClock on a runtime with the clock registers (an up-to-date instance), and across a reboot;
- AmiSans drawn on the screen (the files were installed; the font wasn't opened);
- the CPU floor on a CPU below a 68040;
- the Installer's own window;
- Uninstall-GFOpenUp.

Reinstall puts back only parts that are already installed. To add HostClock, AkikoPlus or the AmiSans files to a machine that has an older pack, use "Install or upgrade", or the `GFOpenUpTool INSTALL` lines above.
