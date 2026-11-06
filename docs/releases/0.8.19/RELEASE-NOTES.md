# GFOpenUp 0.8.19 (6 November 2026)

This release is GFOpenUp 0.8.18 with **z.library 53.13** and the programs that need it: GFOpenCompress 0.3.3 and the GFOpenImage datatypes. It also has **gfopengpu.library 0.27** and **gfopencrypto.library 2.2**. Install it before the open ROM full36-t5, because that ROM's PNG datatype needs z.library 53.13 on the disk.

## What's fixed and new

**z.library's jump table** (amicradle-guest #632). GFOpenZlib 53.12 laid out its calls in zlib.h's order. It matched the Aminet z.library up to -96 and was wrong from -102 (InflateInit2) on.
- **What 53.12 broke:** a program built for the Aminet z.library called the wrong function past -96.
- **What 53.13 changes:** it uses the Aminet z.library's own order and registers for all 41 calls, adds the seven calls it lacked, and moves GFOpenZlib's own four calls to -276 to -294.
- **What refuses 53.12:** GFOpenCompress and the datatypes below now refuse 53.12. That's why they ship together here: a disk with one and not the others loses PNG, gzip or zip.

**GFOpenCompress 0.3.3** (gfopencompress #24, #26). gzip, zip, ADZ and tar.gz now inflate through z.library at run time, so GFOpenCompress carries no zlib of its own. Without z.library those formats say it's needed; ADF, DMS, LhA and plain tar work without it.

**GFOpenImage 0.5** (gfopendatatypes #28 to #33):
- gfopengif.datatype 47.4 saves GIFs (its own LZW; colour-index pictures keep their palette).
- The animation class saves every film as IFF ANIM-5.
- gfopenpng.datatype 47.6, gzip.datatype 47.3 and stream.datatype 47.3 open z.library through the Aminet jump table.
- webp.datatype 47.3's lossless save no longer hangs. libnix's qsort was the cause, and the datatypes now carry a sort of their own.
- The alpha channel comes through.

**GFOpenGPU 0.27** (gfopenrtg #177, #180, #184): gfopengpu.library 0.27, stream v1.10.
- `RLGR_DEC`, `RFX_TILE` and `RFX_YCBCR` compute RemoteFX Progressive tiles for GFAmigaRDP.
- `DWT_INV`'s buffers are big-endian on every host.
- The 3D rasteriser's wrapping steps are defined C.
- SDL2.module, GL.module and the SDL 2 satellites are 0.8.18's.

**GFOpenCrypto 2.2** (gfopentls #24): SHA-1 (`OC_SHA1`, `OC_SHA1Blocks`).

## Parts changed against 0.8.18

| Part | Change |
|---|---|
| GFOffice | 3.2: `LIBS:z.library` 53.12 to 53.13 (the part's version is owfwrite.library's, unchanged) |
| GFOpenImage | 0.4 to 0.5: the datatypes above and `LIBS:z.library` 53.13 |
| GFOpenCompress | 0.3.2 to 0.3.3 |
| GFOpenGPU | 0.26 to 0.27: gfopengpu.library 0.27 |
| GFOpenCrypto | 2.1 to 2.2 |
| GFOpenUp | 0.8.18 to 0.8.19: the Installer's and the uninstaller's version lines. GFOpenUpTool is still 0.8.16, because its source hasn't changed. |

## How it was built

The pack is 0.8.18's archive with only those parts changed, as 0.8.16 to 0.8.18 were made. Every other member is copied from `GFOpenUp-0.8.18.lha` byte for byte.
- **Compiler:** the changed parts were built from their repositories' mains with their own scripts, on full35's frozen GFOpenAmigaGCC 16.2.0-open6 stove. That's the compiler of 0.8.16 to 0.8.18 and of the open ROM.
- **Why frozen:** the live stoves gained amicradle-guest #631's fpuauto `specs` on 6 November, and this release doesn't take it yet.

| File | From | Bytes | sha256 |
|---|---|---|---|
| gfopengpu.library 0.27 | gfopenrtg 9a4c37d `library/opengpu/build-amiga.sh` | 260,624 | `533c620656bc3594fed50ff90e2b20db5a97eb6498319ee5a356d48fb689d70b` |
| z.library 53.13 | amicradle-guest d506dccd `libraries/openzlib/build-amiga.sh` | 34,736 | `a7ed2eace39f83e43a8d904736668534b7a3b8516fc2be925a7cf90566b41698` |
| gfopencrypto.library 2.2 | gfopentls 028034e `build-amiga.sh` | 54,868 | `94b3d908af691f71ae3b0fd7479868c428bd7770b5a650025755ad2679da8fe9` |
| GFOpenCompress 0.3.3 | gfopencompress 762a9dd `build-os3.sh` | 153,792 | `d3a01155236dd182da249ac157b922196c40568a31c88b5be5bd61037828ecee` |
| gfopenpng.datatype 47.6 | gfopendatatypes ed39481 `Datatypes/build.sh` | 19,132 | `01afc714b7578372555a7b62d5ec1a84a8f917305831d0955a225dcfe49cb1fe` |
| gzip.datatype 47.3 | the same | 7,816 | `3f725566c4e4f6284fc428ba3f7a855d4210f0d3c4eda26b6f4a71d223717cfa` |
| stream.datatype 47.3 | the same | 8,660 | `33b7e3f9ffc50c29c241c4a4ba95f9a076c4bc3d8d6f50188ec25d57f7680b83` |
| webp.datatype 47.3 | the same | 227,184 | `55847205def2fc54073c7fa5b4bc353a8b2c2c322f8ba8d08a8da7b03e0d7513` |
| gfopengif.datatype 47.4 | the same | 24,048 | `837cc7e21a1674679f1d61b5e5c5e788acfe7594fad5fc72d7a6486a97e5a2aa` |

## Tested

Lab `t4-up0819`, a copy of Instance-32 (0.8.18 on the open ROM full35-t5), upgraded with GFOpenUpTool UPDATE from the 0.8.19 drawer. Lab `t4-up0818-base`, a second copy left on 0.8.18, for comparison.

| Check | Result |
|---|---|
| Upgrade | the manifest lists GFOpenUp 0.8.19, GFOpenImage 0.5, GFOpenCompress 0.3.3, GFOpenGPU 0.27 and GFOpenCrypto 2.2; `LIBS:z.library` is 53.13 |
| ZlibABITest (53.13 against itself) | 295 answers, 0 differences |
| GFOpenCompress 0.3.3 Test on t.zip, t.tar.gz and plain.txt.gz | all three: "1 file tested, 4,400 bytes, all good" |
| GFOpenGPUCheck | gfopengpu.library 0.27, golden/v1.1/v1.2 scenes right on the GPU and on the 68k, "all right" |
| WebP lossless save | SAME, no hang |
| PNG save (`dtsave` same class) | empty files on Instance-32's copy, **on 0.8.18 as well**: its old `DEVS:DataTypes/OpenPNG` descriptor (openpng.datatype 47.3, which can't save) is tried first. With that descriptor aside, all four saves (alpha, opaque, STORE, 16 colours) read back SAME; on Instance-40's copy, which has no OpenPNG, SAME as installed (see Known issues) |
| OpenTyrian (SDL 2), its demo left running | still playing after 12 minutes, no freeze |
| gzip.datatype read through OS 3.2's datatypes.library | ioerr 212, **on 0.8.18 as well** (see Known issues) |

The pack's own checks (`check_part_versions`, the build_pack tests) pass. `SHA256SUMS` in this folder lists the archive.

## Known issues

- **A disk with the old `DEVS:DataTypes/OpenPNG` descriptor saves empty PNGs.** That descriptor comes from the pack before the GFOpen rename and is tried before GFOpenPNG, so openpng.datatype 47.3 (which can't save) opens PNGs. GFOpenDatatypes' own Installer moves it aside; GFOpenUp's GFOpenImage part doesn't yet. Instance-32 has it. Move `DEVS:DataTypes/OpenPNG` out of DEVS:DataTypes until GFOpenUp does it.
- **The ROM's datatype classes are used ahead of the disk's.** A ROM class is in memory from boot, so datatypes.library uses it and never loads a newer one from Classes/DataTypes; on full34/full35 that is gfopenpng.datatype 47.5, built for z.library 53.12's table. full36-t5 carries 47.6, so put full36 in after 0.8.19. (A program that asks for gfopengpu.library 0.27 by version gets the disk's copy; `Version gfopengpu.library` still names the ROM's.)
- gzip.datatype fails to open a `.gz` (ioerr 212) under OS 3.2's own datatypes.library; it works under GFOpenDatatypes' datatypes.library 47.100. Same on 0.8.18. GFOpenCompress opens the same files.

- Blob Wars stops with a memory-corruption alert while it loads 8-bit RGB pictures, so ports keep converting their PNGs to RGBA (as 0.8.18).
- picture.datatype's `PDTA_AlphaChannel` isn't reported back through the system's picture class. Pixels are right. picture.datatype is a system class, so it isn't in GFOpenUp.
