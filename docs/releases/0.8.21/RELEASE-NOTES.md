# GFOpenUp 0.8.21 (7 November 2026)

This release is GFOpenUp 0.8.20 with one file changed: **GL.module**, GFOpenGPU's GL (Mesa), from gfopenrtg #192. Several GL or ogles3 programs at once now all stay on the PC's GPU.

## What's fixed

**An expired fence is done, not failed** (gfopenrtg #192). GL.module waits for each batch it sends to the GPU through the ACRTG board's fence. The board remembers the result of the last 16 batches. When a program asked about an older one, the board said "expired" (`OGPU_ERR_EXPIRED`: done, but its result forgotten), and GL.module took that as a failure. With several GL programs running, one program then dropped to softpipe (the 68k) or later lost its virgl context ("Illegal command buffer"). GL.module now takes an expired fence as done.

In gfopenrtg's lab test (4 trials, three ogles3 demos started together each time), 1 of the 12 programs exited and 2 virgl contexts died without the fix. With it, all 12 stayed on virgl and there were 0 virgl errors.

## Parts changed against 0.8.20

| Part | Change |
|---|---|
| GFOpenGPU | `LIBS:GFOpenGPU/GL.module` (and its copy in `LIBS:OpenGPU/`) from gfopenrtg #192. The part is still 0.27, because its version is gfopengpu.library's, and that library hasn't changed. GFOpenUpTool INSTALL replaces the changed file anyway. SDL2.module, the SDL 2 satellites, gfopengpu.library, the SDK's libGL.a and the Mesa licence text are 0.8.20's. |
| GFOpenUp | 0.8.20 to 0.8.21: the version lines in the Installer and the uninstaller. |

ogles3.library (OGLES3 1.1) is unchanged: `library/ogles3` hasn't changed on gfopenrtg main since e0fdf789. gfopenrtg #191 (deferred presents in acvirgl) is a change to Cradle's runtime, not to the Amiga's files, so it isn't in this pack.

## How it was built

The pack is 0.8.20's released archive (sha256 `5ea97f3e...`) with only those files changed, as 0.8.16 to 0.8.20 were made. Every other member is copied from `GFOpenUp-0.8.20.lha` byte for byte.

GL.module was built from gfopenrtg main at **233dbb81** (the merge of #192) with `library/modules/gl/module/build.sh`, on the `os32-gcc16-open6` stove (GFOpenAmigaGCC 16.2.0-open6), from a fresh Mesa 26.2.4 build (UPSTREAM.json's tarball, by its SHA-256; meson 1.12.1). Its `fpcr_check` and `baserel_check` pass. It is byte for byte the GL.module gfopenrtg's lab test ran as "fixed".

| File | From | Bytes | sha256 |
|---|---|---|---|
| GL.module | gfopenrtg 233dbb81 `library/modules/gl/module/build.sh` | 19,523,528 | `50dc2a4f6ed6cc1011452445a4f6de6298cbcec69303aad2fa74817f9f21723d` |

0.8.20's GL.module was 19,541,652 bytes (`55c556dd...`).

## Tested

Two labs on the DevDrive: `t4-up0821-i32`, a copy of Instance-32 (0.8.20, OS 3.2's Installer 47.19), and `t4-up0821-i40`, a copy of Instance-40 (0.8.20, OpenInstaller 1.0.0-rc2, put in the lab's `SYS:Utilities/Installer` from `t4-up0820-i40`). Each had the 0.8.21 drawer put in DH1:GFOpenUp as Cradle puts it, and was upgraded with `GFOpenUpTool UPDATE YES` (Novice, the answers from last time), then started cold.

| Check | Instance-32's copy | Instance-40's copy |
|---|---|---|
| Upgrade | GFOpenUp 0.8.21; "GFOpenGPU 0.27 installed (3 files changed)", OGLES3 1.1 unchanged | the same |
| `LIBS:GFOpenGPU/GL.module` | 19,523,528 bytes, `50dc2a4f...` (GL.module has no version string) | the same |
| Workbench after a cold start | up | up |
| `OGLES3Check` | OpenGL ES 3.1 Mesa 26.2.4 on virgl (AMD Radeon RX 460), GLSL ES 3.10, 145 extensions, compute 256 of 256 right | OpenGL ES 3.1 Mesa 26.2.4 on virgl (AMD Radeon RX 460), GLSL ES 3.10, 145 extensions, compute 256 of 256 right |
| Three demos started together (`juggler-ogles3 WIDTH=768 HEIGHT=432 MODERN VSYNC`, `boingswarm-ogles3 WIDTH=768 HEIGHT=432 VSYNC`, `boing2026-ogles3 WIDTH=768 HEIGHT=432 VSYNC`, each with `FRAMES=1500 TIMING`) | all three on virgl, all 1500 frames: Juggler RT 19.4 fps, Boing Swarm 19.3 fps, Boing 2026 19.0 fps; the runtime: 4,500 presents, **0 virgl errors**, none on softpipe | all three on virgl, all 1500 frames: Juggler RT 24.0 fps, Boing Swarm 24.8 fps, Boing 2026 23.9 fps; the runtime: 4,500 presents, **0 virgl errors**, none on softpipe |
| `GFOpenGPUCheck` | gfopengpu.library 0.27, virgl, golden/v1.1/v1.2 scenes right on the GPU and the 68k, "all right" | gfopengpu.library 0.27, virgl, golden/v1.1/v1.2 scenes right on the GPU and the 68k, "all right" |

The changes each upgrade made on the disk were: GL.module in `LIBS:GFOpenGPU/` and `LIBS:OpenGPU/`, `Uninstall-GFOpenUp`, and the manifest. The demos share one GPU and wait for the display (VSYNC), so 19 to 24 fps each is three windows sharing the frames (and other labs sharing the GPU), not the GPU's limit.

The pack's own checks (`check_part_versions`: 51 parts, the build_pack tests) pass. `SHA256SUMS` in this folder lists the archive.

## Known issues

- 0.8.20's known issues still stand.
