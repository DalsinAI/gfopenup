# GFOpenUp 0.8.28 (11 November 2026)

This release is GFOpenUp 0.8.28-rc1 with everything gfopenup main merged after it. rc1 brought AmiSans, OpenAL.module, the CPU floor and HostClock; its notes are in `docs/releases/0.8.28-rc1/`. The final adds:
- physfs.library, with a faster z.library;
- arexx.class on GFOS;
- the GFOffice Suite's fixes and the ARexx apps' return codes;
- GFOpenUpTool 0.8.19;
- minigl.library 30;
- GFOpenImage 0.6.

## What's new

From 0.8.28-rc1:
- AmiSans (#218), OpenAL.module (#219), the CPU floor (#220) and HostClock (#221). See rc1's notes.

New in the final:
- **physfs.library 1.1, in the GFOpenGPU part** (#224, #233; the part is still 0.29). This is PhysicsFS 3.2.0, the file layer many of our game ports use, as one shared library instead of a copy inside every game:
  - `LIBS:physfs.library` at gfopenphysfs d3a0ad7.
    - Each program that opens it gets its own PhysicsFS: its own search path, write drawer and open files.
    - It reads all ten archive kinds: zip, 7z, GRP, HOG, MVL, WAD, QPAK, SLB, ISO9660 and VDF.
    - Files open for reading go through a read-ahead buffer (gfopenphysfs #5). Mounting a zip used to read its directory in tens of thousands of 64-byte DOS packets.
    - A seek backwards inside a deflated zip member now gives the right bytes.
  - `LIBS:z.library` 53.15, which deflated zip members inflate through. The pack refuses an older one for physfs.library (`ZLIB_MIN_FOR_PHYSFS`).
  - `SYS:Utilities/GFOpenUp/Licences/PhysicsFS.txt` (zlib).

  Games built against it link `-lphysfs_auto`. Block Attack is the first.
- **z.library 53.15** (amicradle-guest #690) in the GFOffice, GFOpenImage and GFOpenGPU parts, one file for all three:
  - Inflate runs a block's codes as one hostlib call, and so does a whole-input deflate. On AmiCradle that is host code; on any other 68k the same C runs on the 68k. Zip members read about a quarter faster on AmiCradle (zip read -23 to -28%, zip seek -26 to -28%, on the Pi 5 and on daletop).
  - Every result is the same as 53.14's. The compressed bytes can differ, because the whole-input encoder picks other block boundaries. Both are valid and inflate to the same data.
  - It needs a runtime with the zlib hostlib ids (amicradle #990). On an older runtime it runs its 68k body, at 53.14's speed.
- **The GFOpenGPU developer kit** (GFOpenGPUDev, still 0.7) gets physfs's developer files, as gfopenrtg's `make_sdk.py --physfs` puts them there:
  - `include/physfs.h`, plus `proto/physfs.h` (usable from C++ since gfopenphysfs 56e3b9a) and `inline/physfs.h`;
  - `include-static/physfs.h`;
  - `lib/libphysfs_auto.a` and `lib/libphysfs.a`;
  - `fd/physfs_lib.fd`;
  - `lib/pkgconfig/physfs.pc`;
  - `lib/cmake/PhysFS/`;
  - `Licences/PhysicsFS.txt`, with their lines in the README and in `Licences/README`.

  The kit's `SHA256SUMS` was made again.
- **arexx.class on GFOS** (#231). GFOS had no `SYS:Classes/arexx.class`, so no program's ARexx port opened there. On the GFOpenKick ROM, where there is none, the GFOpenUp part now brings:
  - our `SYS:Classes/arexx.class` 47.100 (GFOpenARexx, from amicradle-guest's GFOpenReaction, MIT);
  - `Licences/GFOpenReaction.txt`.

  Under OS 3.2.3's ROM nothing changes.
- **GFOpenCalc saves again** (gfoffice #124, #125; amicradle-guest #687, #688). Saving a deflated ODS or XLSX crashed GFOpenCalc, for two reasons:
  - gfoffice's copy of the z.library SDK had GFOpenZlib 53.12's offsets;
  - gfmalloc's size check wrapped.
- **GFOffice fixes from fuzzing** (gfoffice #125, #126). Pictures with impossible sizes are refused, the CSV save no longer pads a far-out cell to billions of commas, ODS covered-cell runs are written as runs, bad merges are dropped, and XLS cells past column IV are dropped with a note.
- **ARexx return codes, and no requesters** (gfopenprint #49, gfopenedit #28, gfopenftp #26, gfoffice #124). ARexx commands return 0, 5, 10 or 20 and set `LASTERROR`. An error in an ARexx command no longer opens a requester.
- **GFOpenUpTool 0.8.19** (#226, #227, #229):
  - `STARTUPSEQ`: when the disk's `S:Startup-Sequence` is older than the ROM's default, or isn't the ROM's and has no version, Install-GFOpenUp offers the ROM's. Yours keeps running either way, and is never replaced unless you ask.
  - `KICKSET`: installs the slim ROM's module set (the KickstartSet part).
- **minigl.library 30.0** (gfopenrtg #221 to #230, gfopenup #228, in the Warp3D part). This is the clean-room interface plus the virgl route on the PC's GPU, which `ENV:MiniGL/Driver Auto` (the default) takes when there is one. MGLTest 1.1 comes with it. 0.8.27 and rc1 had 29.10, and the pack now refuses anything older than 30.
- **GFOpenImage 0.6** (#225):
  - `gfopensound.datatype` 47.3 sends Ogg Vorbis, FLAC and MP3 to the services card through mitchy.library.
  - Without the card, it opens the new `LIBS:gfopensound-decoders.library` 47.1, which decodes them on the 68k.

## Parts changed against 0.8.28-rc1

| Part | Change |
|---|---|
| GFOpenGPU | still 0.29: `LIBS:physfs.library` 1.1 and `LIBS:z.library` 53.15 (both new to this part), `Licences/PhysicsFS.txt` |
| GFOpenGPUDev | still 0.7: physfs's headers, libraries, FD, pkg-config and CMake files, its licence, the README and `Licences/README`; `SHA256SUMS` made again |
| GFOffice | 3.2 to 3.3: owfwrite, owfcalc, owfpub and owfpresent.library, `C:OWConvert`, `LIBS:z.library` 53.15 |
| GFOpenWrite | 2.0.23 to 2.0.26: `GFOpenWrite` |
| GFOpenCalc | 0.1.27 to 0.1.67: `GFOpenCalc` |
| GFOpenEdit | still 0.1.2: `C:GFOpenEdit` |
| GFOpenFTP | still 0.2.3: `GFOpenFTP` |
| GFOpenPrint | still 0.4: `C:GFOpenView` (and its copies as `GFOpenView` and `MultiView`), `C:OAPDiscover`, `C:OAPPrinters` (and `Printers and Queue`), `WBStartup/OAPSpooler` |
| GFOpenImage | 0.5 to 0.6: `gfopensound.datatype` 47.3, `LIBS:gfopensound-decoders.library` 47.1 (new), `LIBS:z.library` 53.15 |
| Warp3D | still 5.0: `LIBS:minigl.library` 30.0, `Tools/MGLTest` 1.1 |
| GFOpenUp | 0.8.28-rc1 to 0.8.28: `C/GFOpenUpTool` 0.8.19 and its copy in the part, `Install-GFOpenUp` and both copies of `Uninstall-GFOpenUp` (from main's `pack/`), the readme (main's `gfopenup.readme`), and on the GFOpenKick ROM `SYS:Classes/arexx.class` and `Licences/GFOpenReaction.txt` |

The KickstartSet part (#226) isn't in this pack: it is made from a slim ROM build's `thin-support/` (`--thinsupport`), and the slim ROM isn't the default yet. Install-GFOpenUp installs it only when it is there, so nothing asks for it. GFOpenDraw isn't a GFOpenUp part yet.

## How it was built

The pack is 0.8.28-rc1's released archive (sha256 `d5698d44...`) with members replaced and added, as the packs since 0.8.16 are made:
- 2,613 files, against rc1's 2,596: 17 files added, and the rest replaced or byte for byte rc1's.
- The Part lines are written by `tools/build_pack.py`'s own code at main: `physfs_into_gpu()`, `openkick_classes_lines()` and `Part.add()`.
- The kit's new files and lines come from gfopenrtg's `tools/make_sdk.py`: `copy_physfs()`, `pc_text()`, `CMAKE_PHYSFS`, its README and notice lines, and its `SHA256SUMS` step.
- Before it changed anything, the script checked that `build_pack.py` writes rc1's GFOpenGPUDev Part from rc1's kit exactly.
- `check_part_versions()` passes: all 53 parts match their programs.
- The six GFOffice and ARexx app parts are the ones Instance-32 has run since 10 November. Their Part lines past the first are rc1's, and every binary has the sha256 in the table below.
- GFOpenWrite's part now says 2.0.26, its binary's version; main's `build_pack.py` said 2.0.23 (this PR fixes it).
- The scripts' `@VERSION@` and `@DATE@` are filled in as `build_pack.py` fills them.
- GFOpenUpMenu is rc1's: its source hasn't changed since.

| File | From | Bytes | sha256 |
|---|---|---|---|
| `Libs/physfs.library` 1.1 | gfopenphysfs d3a0ad7 (GFOPENPHYSFS), `build_physfs()`, os32-gcc16 | 114,960 | `5a3eaca8175435958b0c6f4df611c6f46b7390ef3a783ecd34190faf0cbdd34c` |
| `Libs/z.library` 53.15 | amicradle-guest 0f5428f, `libraries/openzlib/build-amiga.sh`, os32-gcc16 | 58,384 | `792694cd10d0cbe03ddc038ccb8dbbe71af79fad0baa005c28a901604a6c7342` |
| `lib/libphysfs_auto.a`, `lib/libphysfs.a` (kit) | the same physfs build | 632 and 136,072 | `35e87b21...`, `3bec0d37...` |
| `OpenKick/classes/arexx.class` 47.100 | amicradle-guest 0f5428f, `libraries/openreaction/arexx/build-amiga.sh` | 11,400 | `1f2e5644e96f3b1469127f7ef416edb2e78aaa735a0430d888d28039dd5dca53` |
| `C/GFOpenUpTool` 0.8.19 | gfopenup 9f4adf0 (main), as `build_pack.py` builds it (the os32 stove) | 113,564 | `2681bf9723458ac955eb3541cd1fc496b883d58c99f9513895e5fc800ff1577c` |
| `Libs/minigl.library` 30.0 | gfopenrtg 95f7333 (main), `library/minigl/build.sh`, the os32 stove | 472,892 | `74a04497b00589fe4649e02e513cb2952bb84ce5212a69e9e623b7c5afe0a145` |
| `Tools/MGLTest` 1.1 | the same build | 34,984 | `d636f47ef5877ee1006ae1b59ed16632ed4773566afb0aec5188505c6934d095` |
| `Classes/DataTypes/gfopensound.datatype` 47.3 | gfopendatatypes ba3eb5b (main), `Datatypes/build.sh`, os32-gcc16, AC helpers from amicradle-guest f842038 (its pin) | 19,328 | `298e6e54d278f1772479113586d43904523c5b4ca587f6b387aeaa8261d44a7f` |
| `Libs/gfopensound-decoders.library` 47.1 | the same build | 189,820 | `3a233301b41c874eb3aead8969194355f760ef04862090d62d1cef37e0ab8d6a` |

The GFOffice, GFOpenWrite, GFOpenCalc, GFOpenEdit, GFOpenFTP and GFOpenPrint binaries were built on 10 November 2026:

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

Host checks: `tests/test_build_pack.py` passes, including the physfs, z.library, arexx.class, minigl and KickstartSet checks.

**Archive:** `GFOpenUp-0.8.28.lha`, 64,057,127 bytes, sha256 `90c19d5c84769b9b5c98e268db42e885f976333a5269234cd43378359be4e5b9`. It tests clean with `lhasa t`.

## Tested

The tests ran on 11 November 2026 on two fresh lab copies, each with the archive extracted to `DH1:`, using GFOpenUpTool 0.8.19 from the pack:
- a copy of Instance-32 (AmigaOS 3.2.3, GFOpenUp 0.8.27 and the six app parts);
- a copy of Instance-51 (the open ROM).

**Instance-32 lab:**
- **Install:** `REINSTALL Parts` reinstalled 45 parts, then `INSTALL Parts/GFOpenImage`, `Parts/GFOpenGPU` and `Parts/GFOpenUp`. All returned rc 0. The parts that changed files were:
  - GFOpenUp, 3 files;
  - GFOpenFont, 6;
  - GFOpenImage, 3;
  - GFOpenRTG, 2;
  - GFOpenGPU, 6;
  - Warp3D, 2;
  - GFOpenGPUDev, 22.
- **Versions after the install:**
  - GFOpenUpTool 0.8.19;
  - physfs.library 1.1;
  - z.library 53.15 (it was 53.14);
  - gfopensound.datatype 47.3 and gfopensound-decoders.library 47.1;
  - minigl.library 30.0;
  - owfcalc.library 3.9;
  - GFOpenWrite 2.0.26 and GFOpenCalc 0.1.67.
- **PhysFSTest:** 39/39, including a deflated zip member through z.library and a seek back in one.
- **PhysFSBench** (the 3000-file zip and the 1500-file PAK): rc 0, and every read and seek sum is the same as in the task 42 runs (zip read `5f9b5706`, PAK read `2c383bdb`, PAK seek `1b619044`). This lab's times aren't comparable with the bench numbers above: they were taken on a busy host.
- **`STARTUPSEQ CHECK`:** "This ROM has no default Startup-Sequence: nothing to compare", rc 0.
- The runtime's log has no stack alert.

**Open ROM lab:**
- Before the install there was no `SYS:Classes/arexx.class`.
- `INSTALL Parts/GFOpenUp` (19 files changed) and `INSTALL Parts/GFOpenGPU` (5 files changed) both returned rc 0.
- After them:
  - `SYS:Classes/arexx.class` is 47.100 (ours, 11,400 bytes);
  - `Licences/GFOpenReaction.txt` is there;
  - physfs.library is 1.1 and z.library is 53.15;
  - PhysFSTest is 39/39.

Tested before this pack, with the same binaries:
- the six GFOffice and ARexx app parts on Instance-32, 10 November: each repository's ARexx test ALL PASS, and the GFOpenCalc ODS and XLSX saves;
- arexx.class with an app's ARexx port on a GFOS lab (#231);
- physfs.library and z.library 53.15 on the Pi 5 and daletop (task 42).

Not tested:
- the Installer's own window, and the STARTUPSEQ choice on a ROM that has a default;
- minigl.library 30 drawing in this pack (the GLQuake runs were on its own build, gfopenrtg #230);
- Ogg Vorbis, FLAC and MP3 playback through gfopensound-decoders.library;
- Uninstall-GFOpenUp.

Reinstall puts back only the parts that are already installed. To add a part to a machine that has an older pack, use "Install or upgrade", or `GFOpenUpTool INSTALL Parts/<part>`.

## Known

- GFOpenCalc 0.1.67 links the os32-gcc16 stove's libgfmalloc as it was on 10 November, before amicradle-guest #687 fixed the size wrap. With the right z.library table no save asks for such a size (the Calc save passes; see Tested), and the next pack's Calc will be built on the fixed stove.
- `GFOpenUpTool REMOVE` of GFOffice, GFOpenImage or GFOpenGPU takes `LIBS:z.library` with it while another of them still uses it. z.library wants a part of its own.
- gfopenup main's `src/openuptool.c` doesn't build with the GCC 16 stove's `-Werror` (a `strncpy` truncation warning in the KICKSET code). This pack's GFOpenUpTool is built as `tools/build_pack.py` builds it, with the os32 stove.
