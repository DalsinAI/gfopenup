# GFOpenUp 0.8.25 (8 November 2026) -- release candidate 1

This release brings together the two packs made after 0.8.22: **0.8.23**, which was tested and never published, and **0.8.24-rc1**, the new datatypes. 0.8.24-rc1 was built on 0.8.22, so it didn't have 0.8.23's changes. 0.8.25 has both.

## What's new

From 0.8.23 (its notes are in `docs/releases/0.8.23/RELEASE-NOTES.md`):
- **z.library 53.14** (amicradle-guest #643): CRC-32, Adler-32 and its copies and clears go through AC090's helpers on AmiCradle, and run its own C on any other 68k.
- **GFOpenWindows 0.12.2** (gfopenprefs #181, #184): no snapshot pin on system requesters, and a click on a pushed-in pin unsnaps the window.
- **gfopenfont.library 1.8** (gfopenfont #36): an Intellifont font handed to bullet.library has its space width again.

From 0.8.24-rc1:
- **Datatypes from gfopendatatypes `edd58bd`** in the GFOpenImage part: recognition, the on-CPU decoders (PNG, TGA, GIF, WebP, WebM, CDXL, modules, AIFF, WAV, FLAC, Vorbis, MP3) and the writers as on main, with new descriptors for AAC, M2TS, M4A, MP3, AIFF and WAVE. GFOpenImage stays 0.5.

## Parts changed against 0.8.22

| Part | Change |
|---|---|
| GFOffice | still 3.2: `LIBS:z.library` 53.13 to 53.14, and its licence note |
| GFOpenImage | still 0.5: its copy of `LIBS:z.library` 53.14, and 36 datatype files (classes and descriptors; the list is `CARRIED-FROM-0.8.24.txt` beside the build) |
| GFOpenWindows | 0.12 to 0.12.2 |
| GFOpenFont | 1.7 to 1.8 |
| GFOpenUp | 0.8.22 to 0.8.25: the version lines in the Installer, both uninstaller copies, the readme and the part |

## How it was built

The pack is Team 4's 0.8.23 archive (sha256 `750b3ef9...`), with every member kept. To that were added the files 0.8.24-rc1 changed or added against 0.8.22 (sha256 `003db29c...`). All of them are in GFOpenImage, and none of them was changed by 0.8.23. The version lines then went from 0.8.23 to 0.8.25. The script checks each of those points, and stops if one fails.

The drawer icon `GFOpenUp.info` is back: 0.8.24-rc1's archive didn't have it.

| | |
|---|---|
| Archive | `GFOpenUp-0.8.25.lha`, 63,318,154 bytes |
| sha256 | `341278afa7b9c781973f459326b74d7334ee109d9483a7bef301efbcd57f7ef7` |

`tools/build_pack.py` on main doesn't yet build GFOpenImage's new datatypes. A pack built from scratch with it would have 0.8.23's datatypes, so packs are made by replacing files, as before.

## Tested

- The archive tests clean.
- The version lines read 0.8.25, gfopenfont.library is 1.8 (`87f66ce6...`), and the 16 datatype classes are there.
- **To do before the final release:** a fresh Open ROM machine with the pack's standard parts installed by `OpenUpTool INSTALL`, then booted to the desktop. This is the PiStorm card's machine.

## Known issues

- 0.8.22's known issues still stand.
- The datatypes' AC090 native helpers are off, as in 0.8.24-rc1. The classes run their own C.
