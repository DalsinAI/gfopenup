# GFOpenUp 0.8.23 (7 November 2026) -- release candidate 1

This release is GFOpenUp 0.8.22 with a faster z.library, the snapshot pin as a toggle, and Intellifont text with its spaces back.

## What's new

- **z.library 53.14** (GFOpenZlib, amicradle-guest #643). CRC-32 and Adler-32 (from 32 bytes) and the library's copies and clears (from 64 bytes) go through AC090's helpers. On AmiCradle they run as host code: in the labs CRC-32 was about 30 times and Adler-32 about 60 times as fast as 53.13 (ZlibBench, 1 MB); compress and uncompress are bound by their 68k Huffman work and change little. On any other 68k (a real Amiga, a PiStorm, another emulator) the same library runs its own C, as before. Results are unchanged. z.library ships in the pack, not the ROM.
- **GFOpenWindows 0.12.2** (gfopenprefs #184 and #181). A click on a pushed-in (snapped) pin now unsnaps the window: the place is forgotten in both WindowPlaces files and the pin stands again. Shift and a click still forgets, as before. System requesters (EasyRequest and the like) no longer get a pin (0.12.1).
- **gfopenfont.library 1.8** (gfopenfont #36). When it hands an Intellifont font (OS 3.2's CGTriumvirate, CGTimes) to the system's bullet.library, the space has its width again, so words no longer run together. The text measures what OS 3.2's diskfont.library makes of the same font.

## Parts changed against 0.8.22

| Part | Change |
|---|---|
| GFOffice | still 3.2 (the part's version is owfwrite.library's): `LIBS:z.library` 53.13 to 53.14, and its licence note. GFOpenUpTool's update copies the changed file. |
| GFOpenImage | still 0.5 (a version of its own): its copy of `LIBS:z.library` is the same 53.14. |
| GFOpenWindows | 0.12 to 0.12.2: `SYS:Tools/Commodities/GFOpenWindows`. |
| GFOpenFont | 1.7 to 1.8: `LIBS:gfopenfont.library`. |
| GFOpenUp | 0.8.22 to 0.8.23: the version lines in the Installer and the uninstaller. |

Not in this pack: GFOpenPrefs stays 2.7. The Windows page's new label ("click again to unsnap", gfopenprefs #184) is in GFOpenPrefs 2.8, which is still a release candidate (`GFOpenPrefs-2.8-rc1`, with #165 and #180). The pin itself works without it.

## How it was built

The pack is 0.8.22's released archive (sha256 `003db29c...`) with only those files changed, as 0.8.16 to 0.8.22 were made. Every other member is copied from `GFOpenUp-0.8.22.lha` byte for byte. Each binary was built by Cradle's stove builds from its repository's main, with the `os32-gcc16-open6` stove (GFOpenAmigaGCC 16.2.0-open6).

| File | From | Bytes | sha256 |
|---|---|---|---|
| z.library 53.14 | amicradle-guest 1d3cb7f8 `libraries/openzlib/build-amiga.sh` | 42,348 | `e31b36670a048881d88baa6e10c6765aa44929e4357ffabdccbf1ee49943fbb8` |
| GFOpenWindows 0.12.2 | gfopenprefs 9a760a05 `Windows/build.sh` (the same bytes built again by hand) | 78,404 | `5ea05648e0a799e3749c2053a7379aee4f21bb706ed0a95719dbd6c1a5fc16e9` |
| gfopenfont.library 1.8 | gfopenfont dfc484b6 `build-amiga.sh` | 271,624 | `87f66ce6c815863ff70dc81975c059e94bc99651133b099cd6ab34942f71e559` |

`check_part_versions`: all 51 parts match their programs.

## Tested

Two pairs of labs on the DevDrive, copies of Instance-32 (AA+, OS 3.2's Installer 47.19) and Instance-40 (AGA, OpenInstaller 1.0.0-rc2 put in the lab's `SYS:Utilities/Installer`), both on ROM full37 with 0.8.22 installed. In the first pair, the 0.8.23 drawer was put in DH1:GFOpenUp as Cradle puts it and `GFOpenUpTool UPDATE YES` (Novice) upgraded each: "GFOpenFont: 1.7 to 1.8", "GFOpenUp: 0.8.22 to 0.8.23", "GFOpenWindows: 0.12 to 0.12.2"; 5 files copied (z.library, its licence note, GFOpenWindows, gfopenfont.library, the uninstaller) and the manifest changed, the same on both. The second pair were fresh copies that got those changes from the deploy tool, as the instances will, and were then started cold.

| Check | Instance-32's copy | Instance-40's copy |
|---|---|---|
| `Version` | z.library 53.14 in memory and on disk, gfopenfont.library 1.8, GFOpenWindows 0.12.2 | the same |
| ZlibSelf: CRC-32 and Adler-32 at 17 lengths (0 to 256 KB), 4 start offsets, 3 start values | all 408 the same as the host's zlib | the same |
| ZlibSelf: Compress2 levels 0, 1, 6, 9 and Uncompress | every round trip SAME; the host's zlib inflates the Amiga's level 6 stream, and the Amiga inflates the host's | the same |
| ZlibBench (1 MB, best of 3) | CRC32 6,887 MB/s, Adler32 13,909 MB/s, compress2 8.2 MB/s, uncompress 33.9 MB/s | CRC32 6,277 MB/s, Adler32 15,093 MB/s (53.13 on the same lab: 213 and 257 MB/s) |
| `dtsave` PNG: alpha, opaque, STORE, 16 colours | all four SAME | all four SAME |
| Snapshot pin | a click snaps (the place in ENV: and ENVARC:), a second click unsnaps (both gone), the pin stands again | the same |
| OpenFontLibTest INTELLIFONT | CGTriumvirate 83: space 20, sentence 1391; 72: 17, 1206; CGTimes 83: 21, 1354; 72: 18, 1175: each as diskfont.library's | not run: Instance-40 has no Intellifont fonts |
| Intellifont on screen | CGTriumvirate 40 through gfopenfont.library and 41 through diskfont.library: the words spaced the same | DejaVu Sans 32 and 33: the same |
| GFOpenUp Setup | opens at step 1 of 3; Later closes it | the same |
| Workbench | the dock, menus, title bar, blanker, snap and Windows commodities running | the same |
| GFOpenBenchmark | runs through; GL on virgl | runs through; GL on virgl: present 707 fps, fill 2,060 Mpix/s, triangles 36.4 Mtris/s, shader 480 fps |

OpenFontLibTest's other FAILs are the labs', the same with 1.7 and 1.8 (no Liberation fonts in FONTDIR, the GPU pixel hash). The GFOpenBenchmark GL figures on Instance-32's copy (present 192 fps) were taken with daletop's load near 35 and other teams' GL labs running; Instance-40's copy with 0.8.22 put back gave the same (136 fps) under the same load, and 707 fps once the load fell. Nothing in this release touches GL.

## Known issues

- 0.8.22's known issues still stand.
