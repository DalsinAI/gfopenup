# Release notes for the next GFOpenUp pack: the GFOffice Suite and the ARexx apps

For whoever builds the next pack after 0.8.28-rc1. Paste the sections below into that pack's RELEASE-NOTES.md, with its own archive, sizes and lab runs. The binaries below were built on 10 November 2026 from each repository's main with this repository's `tools/build_pack.py` recipes, and put on Instance-32 as parts (see "Tested so far").

## What's new

- **GFOpenCalc saves again with its own allocator** (gfoffice #125, #124; amicradle-guest #687, #688). Saving a deflated ODS or XLSX crashed GFOpenCalc. There were two causes:
  - gfoffice's copy of the z.library SDK was GFOpenZlib 53.12's, with the wrong offsets from -102 on. `DeflateBound` called `InflateSetDictionary` and gave a bound of about 4 GB.
  - gfmalloc's size check wrapped, so that malloc returned 8 bytes and deflate wrote past them.

  gfoffice now takes the one z.library SDK (amicradle-guest `libraries/openzlib/sdk`, also in the Open SDK through gfopensdk #67). gfmalloc refuses sizes it can't hold.
- **GFOffice fixes from fuzzing** (gfoffice #125, #126):
  - PDF and RTF pictures with impossible sizes are refused.
  - The CSV save no longer pads a far-out cell to billions of commas.
  - ODS covered-cell runs are written as runs.
  - Inverted and one-cell merges are dropped.
  - XLS cells past column IV are dropped, with a note in the load report.
- **ARexx return codes and no requesters** (tasks 38 and 39: gfopenprint #49, gfopenedit #28, gfopenftp #26, gfoffice #124):
  - ARexx commands return 0, 5, 10 or 20, and set `LASTERROR`.
  - An error in an ARexx command no longer opens a requester.

## Parts changed against 0.8.27

| Part | Change |
|---|---|
| GFOffice | 3.2 to 3.3: owfwrite, owfcalc, owfpub and owfpresent.library, `C:OWConvert` |
| GFOpenWrite | still 2.0.23: `GFOpenWrite` |
| GFOpenCalc | 0.1.27 to 0.1.67: `GFOpenCalc` |
| GFOpenEdit | still 0.1.2: `C:GFOpenEdit` |
| GFOpenFTP | still 0.2.3: `GFOpenFTP` |
| GFOpenPrint | still 0.4: `C:GFOpenView` (and its copies as `GFOpenView` and `MultiView`), `C:OAPDiscover`, `C:OAPPrinters` (and `Printers and Queue`), `WBStartup/OAPSpooler`. GFOpenPrint, OAVWorker, the driver and oapspool.device are byte for byte 0.8.27's. |

GFOpenDraw (gfopendraw #13) is not a GFOpenUp part yet, so it isn't in this list.

## How they were built

| Repository | Commit | Recipe |
|---|---|---|
| gfoffice | 46ccce6 | `build-os3.sh build/os3-library` with `OWF=library`, os32-gcc16, gfopenlayout 2fb0eae |
| gfopenedit | e64ba91 | `build.sh`, os32 stove, gfopengadtools 01c1fd9 |
| gfopenprint | fed4ffa | `build-amiga.sh`, `build-browser.sh`, `build-viewer.sh` with GCC 16 |
| gfopenftp | 39de1a3 | `build-os3.sh`, os32-gcc16, gfopengadtools 01c1fd9; `fpcr_check`: none |

| File | Bytes | sha256 |
|---|---|---|
| `Libs/owfwrite.library` | 234,504 | `4f5ccef71ab8bc197ff815c675b3a701f3b1c3e15781a43682680bf76d2e8e9a` |
| `Libs/owfcalc.library` | 735,100 | `0becfabd1ce1b9af0dd09d01ec4da6e97740ea1f11e3cb6e07eafcb7ad687402` |
| `Libs/owfpub.library` | 69,796 | `76886ccafcdf11a408a2cd4abdeb478b7e411d33492833305542d23ddc5c768b` |
| `Libs/owfpresent.library` | 313,648 | `e38edc344acce1d482d6ab9a98afb4523883dac085f4a470e051a063ba0d1648` |
| `C/OWConvert` | 168,892 | `2e1fbd38b6a15b9cb8c40380f36373cbdd2ec27e215fe0fb2e617d597e64826a` |
| `GFOpenWrite` | 411,812 | `86f0e608d188ce75f9d2d3ff7e4622edddc6aa0fd5c4353b65f6e3862d28f650` |
| `GFOpenCalc` | 457,428 | `b45b57b2d716adf4c56b857c4a69e9c56cfeaddd7eeeab58aefec12acbb80a31` |
| `C/GFOpenEdit` | 134,156 | `b641682ab2d65d9ecb291309c3ba2af07aa834c34b735da73cae9cc8be2a3945` |
| `GFOpenFTP` | 216,764 | `4de013a6d5ea82211ae669ee4b66fa3b5bcc1098a463c2eb106dbedfa14774fc` |
| `C/GFOpenView` | 103,416 | `29a042ac2e0025953f78e35cd86c3c08903959e2a7465cb8fafb0b6eee274470` |
| `C/OAPDiscover` | 59,424 | `62baa5bd3cff430d62438f4ec7097b9e4f3b9a9efb0fd52762814933bacb2d4d` |
| `C/OAPPrinters` | 71,332 | `eefd32f5657de9f53e346618b5d66db481407009f676ed53d63f82d805a9fab6` |
| `WBStartup/OAPSpooler` | 8,180 | `365ceca888444f32bbc8c078eb9c8965eca19f073d3af681d794d1570e1be499` |

GFOpenCalc links the stove's libgfmalloc. Until the stoves carry amicradle-guest #687, that copy still has the size wrap. With the right z.library table no save asks for such a size, but the stoves should be rebuilt before the pack is.

## Tested so far

Instance-32 (AmigaOS 3.2.3, GFOpenUp 0.8.27), 10 November 2026, 18:42 to 18:50 UTC:
- **Install:** the six parts were made from 0.8.27's with the binaries above. They went in `DH1:T2-Apps-20261110/Parts`, and `C:GFOpenUpTool INSTALL Parts/<part>` was run for each, in the order above. All six returned rc 0: GFOffice 5 files changed, GFOpenWrite 1, GFOpenCalc 4, GFOpenPrint 7, GFOpenEdit 1, GFOpenFTP 1. On the host, every installed file has the sha256 above.
- **The Calc save (LV-426):** GFOpenCalc 0.1.67 saved a deflated ODS and a deflated XLSX with `SAVEAS`. Both returned rc 0, `unzip -t` passes on both, and the values (42, 43, "hello") are in them.
- **ARexx:** each repository's own test (`tests/arexx/calc.rexx`, `tests/arexx/write.rexx`, gfopenedit's, gfopenprint's and gfopenftp's `tests/arexx.rexx`) printed ALL PASS against the installed program. Every error returned 5, 10 or 20 with `LASTERROR` set, and no requester opened. FTP ran without a server (the offline half). View's picture step was skipped: Instance-32 has no `Busy.pointer`.

Before the install, Instance-32's DH0 was backed up to `AmiCradle-backups/t2-gfoffice-apps-20261110/` on daletop's DevDrive, with an `undo.sh`.
