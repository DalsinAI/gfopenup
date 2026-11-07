# GFOpenUp 0.8.20 (7 November 2026)

This release is GFOpenUp 0.8.19 with one new part: **ogles3.library on GFOpenGPU**. Programs built for ladislawv's ogles3 (OpenGL ES 3.1 for AmigaOS 3.x on the PiStorm) now run on AmiCradle instances unchanged, drawn on the PC's graphics chip.

## What's new

**OGLES3 1.1** (gfopenrtg #190). ogles3.library is a face of GFOpenGPU's GL.module, as Warp3D.library and minigl.library are, and holds no GL of its own.
- **Its interface** is ogles3's: the name `ogles3.library`, version 1, its two vectors and its dispatch table (ABI 1, 380 entries), taken from ogles3's MIT SDK.
- **Where it draws:** on **virgl** (the PC's GPU) when gfopengpu.library has it, on **softpipe** (the 68k) otherwise. `SetEnv OGLES3/Driver CPU` asks for softpipe.
- **OGLES3Check** (in `C:`) prints what a context offers (GL strings, compute limits) and runs a compute shader.
- **Credits:** ogles3's API and its demos are ladislawv's and R-Typer's. The library's README, with the credits and both licences, is in `SYS:Utilities/GFOpenUp/Licences/ogles3.txt`.

It comes with the GFOpenGPU part, and it keeps gfopenrtg's `Install-ogles3` rules:
- **AmiCradle only.** The part needs AmiCradle and the GFOpenGPU part (`LIBS:GFOpenGPU/ACRTG.gpu` and `GL.module`). Anywhere else GFOpenUpTool leaves it out, so a PiStorm's own ogles3.library is never touched.
- **Never over another one.** A `LIBS:ogles3.library` whose version string lacks "on GFOpenGPU" is another ogles3.library: GFOpenUpTool leaves it and the part out (a warning, not an error).

## Parts changed against 0.8.19

| Part | Change |
|---|---|
| OGLES3 | new, 1.1: `LIBS:ogles3.library`, `C:OGLES3Check`, `SYS:Utilities/GFOpenUp/Licences/ogles3.txt`. Installed after GFOpenGPU when GFOpenGPU is picked. |
| GFOpenUp | 0.8.19 to 0.8.20: the Installer (its version lines, the OGLES3 step and the GFOpenGPU help text) and the uninstaller's version line. GFOpenUpTool is still 0.8.16, because its source hasn't changed. |

## How it was built

The pack is 0.8.19's released archive (sha256 `0105312a...`) with only those files changed or added, as 0.8.16 to 0.8.19 were made. Every other member is copied from `GFOpenUp-0.8.19.lha` byte for byte.

The library and OGLES3Check were built from gfopenrtg main at **e0fdf789** (the merge of #190) with `library/ogles3/build-amiga.sh`, by Cradle's `stove_build` on the `os32-gcc16-open6` stove (GFOpenAmigaGCC 16.2.0-open6). The Part file and `ogles3.txt` are written by `tools/build_pack.py`'s `ogles3_part`.

| File | From | Bytes | sha256 |
|---|---|---|---|
| ogles3.library 1.1 | gfopenrtg e0fdf789 `library/ogles3/build-amiga.sh` | 37,676 | `3d7bcdffbc47514a9ec47603cefe4821e0d431a278f032b385b4d05aa16728b2` |
| OGLES3Check | the same | 17,712 | `0724d203313a5e4de1253a64159612734d8118d445a1b938a01c4d16069779c4` |
| ogles3.txt | gfopenrtg e0fdf789 `library/ogles3/README.md` and `ogles3_api.txt`'s notice | 8,713 | `6e36925ef9205dd4b9cfb1085c3ceba298e98f9a7fdd8890000d0be2e88803d6` |

## Tested

Two labs on the DevDrive: `t4-up0820-i32`, a copy of Instance-32 (0.8.19, OS 3.2's Installer), and `t4-up0820-i40`, a copy of Instance-40 (0.8.19, OpenInstaller 1.0.0-rc2). Each had the 0.8.20 drawer put in DH1:GFOpenUp as Cradle puts it, and was upgraded with `GFOpenUpTool UPDATE YES` (Novice, the answers from last time).

| Check | Instance-32's copy | Instance-40's copy |
|---|---|---|
| Upgrade | GFOpenUp 0.8.20; "OGLES3 1.1 installed (3 files changed)"; the manifest lists OGLES3 1.1 | the same |
| `Version LIBS:ogles3.library FULL` | ogles3.library 1.1, "on GFOpenGPU, Dalsin Limited / the AmiCradle Team" | the same |
| Workbench after a cold start | up | up |
| `OGLES3Check` | OpenGL ES 3.1 Mesa 26.2.4 on virgl (AMD Radeon RX 460), GLSL ES 3.10, 145 extensions, compute 256 of 256 right | the same |
| A demo (ladislawv's ogles3 1.0-beta2, pinned by gfopenrtg's test) | Boing Swarm, 16,384 boids, 600 frames at 185 fps | Juggler RT (1987, ES 3.1), 600 frames at 278 fps |
| `GFOpenGPUCheck` | gfopengpu.library 0.27, virgl, golden/v1.1/v1.2 scenes right on the GPU and the 68k, "all right" | the same |
| Another ogles3.library in LIBS: | OGLES3 removed, a stand-in without "on GFOpenGPU" put in LIBS:, then INSTALL: "belongs to another program, so GFOpenUp leaves it and this part out", WARN, the file unchanged; with it gone, INSTALL put 1.1 back | not run |

The changes each upgrade made on the disk were: the three OGLES3 files, `Uninstall-GFOpenUp`, the manifest, and the GFOpenGPU SDK's `clib/Warp3D_protos.h` (0.8.19's repacked header, which the instances' earlier 0.8.19 drawer didn't have). Boing Swarm's rate is lower than gfopenrtg's 301 fps because a dozen other labs shared the GPU.

The pack's own checks (`check_part_versions`: 51 parts, the build_pack tests) pass. `SHA256SUMS` in this folder lists the archive.

## Known issues

- Those of ogles3.library 1.1 (gfopenrtg `library/ogles3/README.md`, "Not yet"): an ES 2.0 context reports "OpenGL ES 3.1"; the default framebuffer isn't multisampled; ogles3's `.v3dp` shader binaries can't work on another GPU.
- 0.8.19's known issues still stand.
