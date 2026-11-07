# GFOpenUp 0.8.22 (7 November 2026)

This release is GFOpenUp 0.8.21 with the GFOpenGPU part moved to **0.29**, to match the open ROM **full37** (DalsinAI/gfopenkick `rom-full37-rc1`). On an open-ROM machine the ROM's gfopengpu.library and ACRTG.gpu are the ones in memory, so the disk's copies should be the same versions.

## What's new

- **gfopengpu.library 0.29** (gfopenrtg #194, #185). The ACRTG board's fence results: the library keeps the last 256 batches' results, where 0.27 kept 16, so with several GL programs and the compositor a program's wait finds its own result. WriteChunkyPixels and WritePixelArray8 on a planar bitmap go through GFOpenGfx's core, which writes whole bytes 32 pixels at a time (gfopengfx da2cdd4). On the open ROM, graphics.library does that drawing itself, and the ROM's gfopengpu.library has no GFOpenGfx core.
- **ACRTG.gpu 1.6** (amicradle-guest #638): the board's driver keeps 256 results too.
- **GL.module** (gfopenrtg #195): when a program can't have virgl and falls to softpipe (the 68k), GL.module says so on the program's output and in the runtime's log, with the program's name and the reason. `ENV:GFOpenGPU/GLDriver` set to `gpu` refuses softpipe; `cpu` takes it without trying virgl.
- **C:GLQuery** (new): GL's version and renderer, and which driver GL.module took and why. **C:OGLES3Check** prints the driver too.
- **GFOpenGPUDev 0.7**: the SDK's `libGL.a` and `include/gla/gla_core.h` from the same GL build (`gla_last_why`).

## Parts changed against 0.8.21

| Part | Change |
|---|---|
| GFOpenGPU | 0.27 to 0.29: `LIBS:gfopengpu.library` 0.29, `LIBS:GFOpenGPU/ACRTG.gpu` 1.6, `LIBS:GFOpenGPU/GL.module` (and its copy in `LIBS:OpenGPU/`), and `C:GLQuery` (new). The SDL 2 modules, tinygl.library, gfopengfx.library, GFOpenGPUCheck and the licences are 0.8.21's. |
| OGLES3 | still 1.1 (the part's version is ogles3.library's, which is byte for byte 0.8.21's): `C:OGLES3Check`. GFOpenUpTool INSTALL replaces the changed file. |
| GFOpenGPUDev | 0.6 to 0.7: `lib/libGL.a`, `include/gla/gla_core.h`. |
| GFOpenUp | 0.8.21 to 0.8.22: the version lines in the Installer and the uninstaller. |

## How it was built

The pack is 0.8.21's released archive (sha256 `d4b08c3d...`) with only those files changed, as 0.8.16 to 0.8.21 were made. Every other member is copied from `GFOpenUp-0.8.21.lha` byte for byte.

| File | From | Bytes | sha256 |
|---|---|---|---|
| gfopengpu.library 0.29 | gfopenrtg 4b193b22 `library/opengpu/build-amiga.sh` | 262,596 | `ca7fba631615c13de55d070c8e91ab1f0ad9b583d065e508624ce40ac6dbb5f2` |
| ACRTG.gpu 1.6 | amicradle-guest ec212641 `libraries/acrtg.gpu/build.sh` | 5,992 | `685edb319e5c5d1f84bf4a7cc71c2c760f17227ad19ce79bfc81e0ed1665b785` |
| GL.module | gfopenrtg 4b193b22 `library/modules/gl/module/build.sh` (Mesa 26.2.4, meson 1.12.1) | 19,527,572 | `cbd1ddbf41b69776c622f2b4377e6502f1905a7fa2b4f3c267cdd42082098fb8` |
| libGL.a | the same build | 116,600 | `0c5d09eb5753552ecbd316550d95b6ecfd6a50531144bb9a5424ee51dc37a7a2` |
| GLQuery | gfopenrtg 4b193b22 `tests/amiga/glquery/glquery.c`, linked with that libGL.a | 145,016 | `b61182d54a6eb003502b88e5a77e9701ed3181d1cf7e5cee2b8cf04bea9ae165` |
| OGLES3Check | gfopenrtg 4b193b22 `library/ogles3/build-amiga.sh` | 18,328 | `c67bd0a339a8ba267bce2bb9906649d30eef89ebd8d63a00f9851b157f5738cd` |

gfopengpu.library and ACRTG.gpu were built with the ROM's compiler (full33's frozen GFOpenAmigaGCC 16.2.0-open6 stove); GL.module, libGL.a, GLQuery and OGLES3Check with the `os32-gcc16-open6` stove, as 0.8.21's GL.module was.

## Tested

Two labs on the DevDrive: `t4-f37-i32`, a copy of Instance-32 (AA+, OS 3.2's Installer 47.19), and `t4-f37-i40`, a copy of Instance-40 (AGA, OpenInstaller 1.0.0-rc2 put in the lab's `SYS:Utilities/Installer` before the first snapshot). Each had 0.8.21 installed, got the 0.8.22 drawer in DH1:GFOpenUp as Cradle puts it, and was upgraded with `GFOpenUpTool UPDATE YES` (Novice) on the full36 ROM; then the full37 ROM was put in and the lab started cold.

| Check | Instance-32's copy | Instance-40's copy |
|---|---|---|
| Upgrade | "GFOpenGPU: 0.27 to 0.29", "GFOpenGPUDev: 0.6 to 0.7", "GFOpenUp: 0.8.21 to 0.8.22"; 9 files copied, the manifest changed | the same |
| Boot on full37, `Version` | Kickstart 47.2; gfopengpu.library 0.29, ACRTG.gpu 1.6 in memory (the ROM's); the disk's 0.29 too | the same |
| GFOpenGPUCheck | golden, v1.1 and v1.2 scenes right on the GPU and the 68k, "all right" | the same |
| OGLES3Check | OpenGL ES 3.1 Mesa 26.2.4 on virgl (AMD Radeon RX 460), "driver: virgl", compute 256 of 256 right | the same |
| GLQuery | "driver virgl (the host's graphics chip)", GLES 3.1 and GL 4.3 core, compute right, "all right" | the same |
| Three demos together (juggler, boingswarm, boing2026; 768x432, VSYNC, 1500 frames) | all on virgl, no softpipe notice: 44.5, 49.3, 49.2 fps; the runtime: 0 virgl errors | all on virgl, no softpipe notice: 44.5, 49.5, 48.6 fps; 0 virgl errors |
| GFOpenBenchmark GL (medians) | present 755 fps, fill 2,654 Mpix/s, triangles 47.9 Mtris/s, shader 646 fps, compute 1,016 | present 706 fps, fill 2,610 Mpix/s, triangles 45.0 Mtris/s, shader 584 fps, compute 1,024 |
| WriteChunkyPixels / WritePixelArray8 on planar bitmaps, 1 to 8 planes (a lab tool's CRC-32 of the planes) | `ec8ff0bd`, the same as on full36 with gfopengpu.library 0.27 | the same |
| PNG saves (alpha, opaque, STORE, 16 colours) | all four SAME | all four SAME |

The benchmark's baseline (salvor, 7 November): present 677 fps, triangles 48.6 Mtris/s. Other teams' builds were running on daletop during these runs.

## Known issues

- 0.8.21's known issues still stand.
