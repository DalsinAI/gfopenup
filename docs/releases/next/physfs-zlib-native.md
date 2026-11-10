# Release notes for the next GFOpenUp pack: faster physfs.library and z.library 53.15

For whoever builds the next pack. Paste the sections below into that pack's RELEASE-NOTES.md, with its own archive,
sizes and lab runs. This goes with physfs.library.md (the library's first release); the pins below replace the ones
named there (gfopenphysfs 21cfa03, z.library 53.14).

## What's new

- **physfs.library is faster on archives, and right when seeking back in a deflated zip member** (gfopenphysfs #5, main d3a0ad7; still 1.1, the same functions and LVOs):
  - Files open for reading go through a read-ahead buffer. It fills 256 bytes after an open or seek, doubling up to 4 KB. Before this, each small read was a DOS packet: mounting a zip read its directory in tens of thousands of 64-byte reads.
  - A seek backwards inside a deflated zip member no longer reads stale input (it used to give wrong bytes).
  - The C++ fix for `proto/physfs.h` (56e3b9a) is in this commit too. gfopenrtg's `make_sdk.py` pins the same commit (its own PR), so the GFOpenGPUDev kit's headers match the library.
- **z.library 53.15** (amicradle-guest #690, from #677): inflate runs a block's codes as one hostlib call, and so does a whole-input deflate. AmiCradle runs those calls as host code; any other 68k runs the same C built for the 68k. Every result is the same, but the compressed bytes can differ from 53.14's: the whole-input encoder picks other block boundaries. Both are valid, and they inflate to the same data. The GFOffice, GFOpenImage and GFOpenGPU parts all carry this one file. The pack refuses an older one for physfs.library (`ZLIB_MIN_FOR_PHYSFS`).

## Parts changed

| Part | Change |
|---|---|
| GFOpenGPU | still 0.29: `LIBS:physfs.library` 1.1 at gfopenphysfs d3a0ad7, `LIBS:z.library` 53.15 |
| GFOffice, GFOpenImage | `LIBS:z.library` 53.15 |
| GFOpenGPUDev | physfs's files at d3a0ad7 (with the gfopenrtg PR that moves PHYSFS_COMMIT) |

## Tested so far (Team 2, 10 November 2026; the libraries themselves, not a pack yet)

PhysFSBench (gfopenphysfs tests/physfs_bench.c) reads a 3000-file zip (40 MB, two thirds deflated) and a 1500-file PAK. Both are made by tools/make_bench_data.py. Times are in ms. Every read and seek sum matches PhysicsFS on the host.

- **The read-ahead buffer** (Pi 5, gfopenphysfs main before #5 against d3a0ad7, deployed runtime):

  | phase | before | after |
  |---|---|---|
  | zip mount | 1017 | 32 |
  | zip enumerate + stat | 629 | 161 |
  | PAK mount | 94 | 10 |

  Before #5, seek in deflated members gave wrong bytes. After it, they are right.
- **z.library's hostlib inflate** (an unpinned lab runtime: amicradle main 03cc4ae with guest main's hostlib; AC_HOSTLIB=0 against on; median of 6 passes):

  | phase | Pi 5 | daletop |
  |---|---|---|
  | zip read | 2069 → 1583 (-23%) | 2267 → 1710 (-25%) |
  | zip seek | 2245 → 1661 (-26%) | 2318 → 1677 (-28%) |
  | whole run | -15% | |

  PAK files are stored, so they don't change.
- **Release checks**, in a lab of Instance-32, hostlib off and on:
  - PhysFSTest 39/39;
  - ZlibABITest 0 differ;
  - ZlibABITest PROBE round trips all right;
  - ZlibBench, 1 MB: Uncompress 37.5 → 110.6 MB/s, Compress2/6 5.2 → 12.5 MB/s.
- **Host:** tests/test_build_pack.py. The GFOpenGPU part takes physfs.library only at GFOPENPHYSFS's commit, and with z.library 53.15 or newer. 53.14, or a file with no version, stops the build.

Still to run with the pack: the PhysFSBench rerun on the Pi 5 with the pinned runtime (amicradle #990), and `GFOpenUpTool INSTALL Parts/GFOpenGPU` on Instance-32 with the checks above.

## Known

- The hostlib inflate needs a runtime with the zlib hostlib ids (amicradle #990). On an older runtime z.library 53.15 runs its 68k body, at 53.14's speed.
