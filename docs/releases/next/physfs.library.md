# Release notes for the next GFOpenUp pack: physfs.library

For whoever builds the next pack after 0.8.28-rc1 (no pack has been built
with it yet). Paste the sections below into that pack's RELEASE-NOTES.md,
with its own archive, sizes and lab runs.

## What's new

- **physfs.library 1.1** (#224, in the GFOpenGPU part, still 0.29). PhysicsFS 3.2.0, the file layer many of the games we port use (Block Attack, DXX-Rebirth, Hedgewars, Warzone 2100, SuperTux and others), as one shared library instead of a copy inside every game:
  - `LIBS:physfs.library` 1.1 (gfopenphysfs 21cfa03, 114,484 bytes, sha256 `c998d88e52a94a54d3ca0870f42172b14b0b1359b565e9d1d344e3b74d26c598`). Each program that opens it gets its own PhysicsFS: search path, write drawer, open files.
  - All ten archivers: zip, 7z, GRP, HOG, MVL, WAD, QPAK, SLB, ISO9660, VDF.
  - `LIBS:z.library` (GFOpenZlib 53.14, the build GFOffice and GFOpenImage carry): deflated zip members inflate through it.
  - `SYS:Utilities/GFOpenUp/Licences/PhysicsFS.txt` (zlib).

  Games built against it link `-lphysfs_auto` and start with "This program needs physfs.library 1 or newer in LIBS:" when it isn't there. Block Attack is the first (DalsinAI/amigablockattack, 73 KB smaller).

  The GFOpenGPUDev kit gets physfs's developer files when gfopenrtg's `make_sdk.py --physfs` is in (gfopenrtg #237): `include/physfs.h`, `proto/` and `inline/` headers, `lib/libphysfs_auto.a`, `lib/libphysfs.a` with `include-static/physfs.h`, `fd/physfs_lib.fd`, `lib/pkgconfig/physfs.pc`, `lib/cmake/PhysFS/`. The Open SDK carries the headers, the FD and `-lphysfs_auto` too (gfopensdk, 0.3).

## Parts changed

| Part | Change |
|---|---|
| GFOpenGPU | still 0.29: `LIBS:physfs.library` 1.1 and `LIBS:z.library` 53.14 (new to this part), `Licences/PhysicsFS.txt` |
| GFOpenGPUDev | physfs's headers, libraries, FD, pkg-config and CMake files (with gfopenrtg #237) |

## Tested so far (the library itself, 9 November 2026)

- **The Team's OpenROM** (exec 47.13, dos 47.100; lab of Instance-40, z.library 53.14 as Instance-40 has it): PhysFSTest 39/39 (deflated zip members through z.library included), PhysFSTestStatic 39/39; two at once and four at once all 39/39; 20 runs in a row all rc 0; `Avail FAST` 1,857,506,520 before, 1,857,346,424 with the library loaded after 20 runs, 1,857,506,520 after `Avail FLUSH` (0 bytes lost); after the flush, with the file renamed away, PhysFSTest gets rc 20 (the library had left memory), and passes when it is put back.
- **Stock Kickstart 3.2.3** (lab of Instance-23, Team 2, gfopenphysfs #3): the same results; without z.library 37/39, only the two deflate checks failing, no crash.
- **Block Attack** on the library (OpenROM lab): menus and a single-player game, its user/ drawer made and written through physfs.library, Ctrl-C back to the Shell; PhysFSTest 39/39 while the game ran.

Still to run with the pack: `GFOpenUpTool INSTALL Parts/GFOpenGPU` on a fresh lab, then the checks above with the installed files.

## Known

- `GFOpenUpTool REMOVE` of GFOffice, GFOpenImage or GFOpenGPU takes `LIBS:z.library` with it while another of them still uses it, as GFOffice and GFOpenImage already do today. z.library wants a part of its own.
- The kit's `proto/physfs.h` at 21cfa03 can't be included from C++ (PhysFSBase is declared twice with different linkage); `<physfs.h>` can, and that is what ports include. gfopenphysfs 56e3b9a fixes it without changing the library's bytes. The pins now name d3a0ad7, which has it: see physfs-zlib-native.md.
