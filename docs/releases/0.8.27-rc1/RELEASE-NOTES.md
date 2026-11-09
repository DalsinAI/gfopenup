# GFOpenUp 0.8.27-rc1 (9 November 2026), release candidate 1

This pack is the released GFOpenUp 0.8.26 with two additions: a library each for AmiCradle's two helper boards, Mitchy (the services card) and AkikoPlus (the media board), and their test commands.

## What's new

- **mitchy.library 1.0** (gfopenservice #34, in the GFOpenService part, now 1.3). Any program can use the services card through this library: it opens a service by name, such as `media.decode/1` or `opentls.key/1`, and sends requests to it. A program can wait for a request, or send it and carry on until a signal or a message says it has finished. It can also cancel a request. The library finds the boards that carry services (Mitchy, AkikoPlus and the CPU cores board). When no board or paired Cradle offers a service, it says so plainly (`MERR_NOTFITTED`), and the program does the work on the 68k. The library goes through `gfopenservice.device`, so the device still owns the boards and every program shares them.
- **ServiceTest 0.2** (`C:ServiceTest`), rewritten on mitchy.library. It lists the boards and services, then checks `echo/1` waited for, many at once, cancelled, and with buffers that are too small or wrongly marked. It also runs one checked job on each real service on offer: pads, the log, web fetch, `media.encode/1`'s formats, an X25519 key exchange, an SVG decoded to pixels, CDXL, AkikoPlus's byte swaps and documents. It returns 0 when everything passes and 5 when no card is fitted.
- **AkikoPlus 2.0, new part** (gfosautoconfigboards #4 at 58c064c). This is `akikoplus.library`. When AmiCradle's AkikoPlus media board is fitted, these jobs run on it (with AVX2 or NEON on the host): chunky to planar, byte swaps, fills, scaling, YUV to ARGB, colour reduction and zlib. When the board isn't fitted they run on the 68k, with the same results. The part includes:
  - `C:AkikoTest`, which compares the board with the 68k and times both;
  - `C:APTest`;
  - the developer files in `SYS:Developer/AkikoPlus`.

  The library reaches the board through mitchy.library. Install-GFOpenUp installs this part together with GFOpenService.

The emulator side is gfosmotherboards #60 (and the amicradle pin, #951). Mitchy now also offers `media.encode/1`. With AkikoPlus fitted, the `media.*` services move to that board. Both libraries work with the runtime 0.8.26 shipped with: on a runtime without AkikoPlus, the media services stay on Mitchy, and akikoplus.library works on the 68k.

## Parts changed against 0.8.26

| Part | Change |
|---|---|
| GFOpenService | 1.2 to 1.3: `LIBS:mitchy.library` (new), `C:ServiceTest` (0.1 to 0.2) |
| AkikoPlus | new, 2.0: `LIBS:akikoplus.library`, `C:AkikoTest`, `C:APTest`, and `SYS:Developer/AkikoPlus/` (autodoc, SFD, FD, headers, and `boardonly/akikoplus.library`, the build without the 68k fallback) |
| GFOpenUp | 0.8.26 to 0.8.27-rc1: `Install-GFOpenUp` (the version, and AkikoPlus installed with GFOpenService), both copies of `Uninstall-GFOpenUp` (the version), the readme (the version, and the GFOpenService entry names mitchy.library and AkikoPlus), and the part |

`tools/build_pack.py` now puts mitchy.library in GFOpenService 1.3. Two things aren't in it yet: the AkikoPlus part, and the Install-GFOpenUp lines that install AkikoPlus in `pack/`. They follow once gfosautoconfigboards #4 is merged.

## How it was built

The pack is 0.8.26's released archive (sha256 `33e3d67b...`) with members added and replaced. Of its 2,566 files, 7 changed and the other 2,559 are byte for byte 0.8.26's; 14 were added, 2,580 in all. The script (`mkpack.py`, beside the build) checks four things:
- the base archive's sha256;
- that each program is a hunk file;
- that each replaced member differs from 0.8.26's;
- that every AkikoPlus file matches its hand-over's SHA256SUMS.

mitchy.library and ServiceTest were built from gfopenservice 239d0ad (#34) with the os32-gcc16 stove. A rebuild from that commit is byte-identical. The AkikoPlus files are the AkikoPlus helper's build of gfosautoconfigboards 58c064c, and Cradle stove job j20261109T141535-615bf9 builds them byte for byte.

| File | From | Bytes | sha256 |
|---|---|---|---|
| `Libs/mitchy.library` | gfopenservice 239d0ad | 4,812 | `b09db5b20c3788cf75fe22193bbba2dd1da9744527270d3c74a26590559030d5` |
| `C/ServiceTest` | gfopenservice 239d0ad | 24,472 | `330c6d787891f216f0989006766aeab7ea5b6b7b31c0a64aaaaeafdd642227b3` |
| `Libs/akikoplus.library` | gfosautoconfigboards 58c064c | 19,400 | `3d2684d67adc32101f529dc49648c8bdfbe7f7566fdef1cf9e5bb9cfe1d19274` |
| `C/AkikoTest` | gfosautoconfigboards 58c064c | 22,948 | `17c6813db69825c553e3a04ba6c263cb16f99d5a330e5273d0197f53654b8950` |
| `C/APTest` | gfosautoconfigboards 58c064c | 18,644 | `88bf32784b94d972862686fdb67f97585c3231449c1b5e7197b449c0d067e41e` |
| `Developer/AkikoPlus/boardonly/akikoplus.library` | gfosautoconfigboards 58c064c | 6,360 | `f33080c411eeae0a100968fc32fca1106c3232ee4869acafb17f1985f4025c42` |

**Archive:** `GFOpenUp-0.8.27-rc1.lha`, 63,412,402 bytes, sha256 `9c24dae6f5c61469854de5473f960e0dc14d73ad4246b15e112c8a176b9c6e48`. Every file tests clean.

## Tested

In lab `t5-mitchy-20261109`, a copy of Instance-40, with the pack as `DH1:GFOpenUp`. The lab first had no mitchy.library and had ServiceTest 0.1. The two parts went in with `C/GFOpenUpTool INSTALL Parts/GFOpenService` and `INSTALL Parts/AkikoPlus`, the commands Install-GFOpenUp runs. They reported "GFOpenService 1.3 installed" and "AkikoPlus 2.0 installed (12 files changed)".

- **Mitchy and AkikoPlus fitted** (a runtime built from gfosmotherboards #60):
  - ServiceTest: 20 passed, 0 failed. `media.*` and `akiko.*` were found on AkikoPlus. `doc.render/1` was skipped because the test PC has no office suite.
  - AkikoTest: the board and the 68k agree on every job.
  - APTest: 172/172.
- **Both boards pulled:**
  - ServiceTest: "Mitchy (the services card) is not fitted", "no services card fitted, and no paired Cradle offers it", rc 5.
  - AkikoTest: 68k only, every job ran, with the same CRCs as the board run.
  - APTest: 172/172.
- **The runtime 0.8.26 shipped with:** ServiceTest 15/15 with Mitchy. That runtime has no AkikoPlus.

Not tested:
- **The Installer's own window.** The lab's OS has no `Installer` program. The added Install-GFOpenUp lines were checked for balanced brackets and run as the same GFOpenUpTool commands.
- **The LAN path.** No Cradle was paired.
- **Uninstall.**

Note that Reinstall puts back only parts that are already installed. On a machine that has 0.8.26, the Installer's default for a newer pack is "Install or upgrade", which brings AkikoPlus with GFOpenService.

**Not in this candidate:** a new GL.module (gfopenrtg #223, #226 and #228), still in testing. If it passes, it goes into rc2.
