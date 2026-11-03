# GFOpenUp 0.8.13 (3 November 2026)

Dale, 3 November: "rebuild everything on open5". GFOpenUp 0.8.13 is the first pack built entirely by a compiler with
the autoinc-store fix. Every part GFOpenUp compiles is rebuilt on **GFOpenAmigaGCC 16.2.0-open5**, the os32-gcc16 stove,
from its repository's main of 3 November. It is also the first pack in the GF layout: the drawer GFOpenUp,
Install-GFOpenUp, Parts/GFOpenUp, the manifest ENVARC:GFOpenAmiga/GFOpenUp.manifest, and GFOpenUp-0.8.13.lha.

## Why

0.8.9 to 0.8.12 were built with open3. GCC 15 and 16's late-combine pass can compile a copy into
`move (An)+,(d,An,Xn)`. The 68k steps An before it works out the destination, so each element lands one place on
(GCC PR 123853). 0.8.11 turned the pass off in the parts that probe for it; open4 and open5 carry the fix itself
(gfopengcc `patches/gcc/0013`). The autoinc scanner (amicradle-guest `tools/scan_autoinc_overlap.py`) found 64
findings in 0.8.11's stripped programs that it could not take for data, 55 of them in GFOpenGPU's SDL2.module and
GL.module. A stripped program doesn't let it tell code from data.

0.8.13's unstripped builds settle it. GL.module.debug has 0 failing findings. The same 19 "WRONG" addresses in the
stripped GL.module are a switch table and other data in its code hunk. Mesa's objects have one finding, in
`ttn_emit_instruction`, which is the jump table after its `jmp %pc@(...,d0:w)`. Across the parts' build output, the
only other finding is in webm.datatype's `vp9_kf_y_mode_prob` table.

## What's in it

- Every compiled part from source on open5, through the stove16 wrapper (`-fno-tree-loop-distribute-patterns
  -fno-delete-null-pointer-checks`). No compiler cache, and a fresh GL work folder and GFOpenGPU SDK, so nothing
  compiled by open3 comes in. gfopenfont.library is built by its own build-amiga.sh on the stove.
- **GFOpenUpTool 0.8.13**: the pack's version, as it has been since 0.8.11.
- Part versions follow their programs: **GFOpenGPU 0.25** (gfopengpu.library 0.25; 0.8.12 stopped at this check),
  **GFOpenFiles 0.4.2**, **GFOpenMenus 0.4.7**, **GFOpenUpSetup 0.1.7**, **GFOpenWrite 2.0.23**.
- `build_pack.py` catches up with the GF rename where the other repositories are already renamed. Built from
  gfopensocket, the GFOpenSocket part takes gfopensocket.library, gfopensocket.device, gfopensocketwifi.device,
  GFOpenSocketControl and C:GFOpenSocket; amicradle's vendored manifest still lists the old names. C:GFOpenGPUCheck
  comes from gfopenrtg's OpenGPUCheck, which keeps its old name there. OpenAmigaPython's release archive (pinned by
  sha256) unpacks to OpenUp/, not GFOpenUp/.
- **OpenBrowserLite 0.4.1** is unchanged. It was built with open3, and the scanner finds no store in it.
  GFOpenBrowserLite 0.4.2, rebuilt on open5 (gfopenbrowser #155, draft release v0.4.2), comes in once that release
  is published and pinned here (its drawer and program have the GF names).
- **No OpenBrowser companion.** The 0.7 companion was built with open3, and its WebKit engine is the largest
  unproven part. It comes back with an engine built on open5.
- **OpenAmigaPython 2.7.18-ac3** is unchanged. It is built with VBCC, which this bug doesn't touch.

The open5 build stopped in four repositories, and each fix is its own PR there. The pack was built from those
branches: gfopenprefs #166 (Look) and #167 (Windows), both `-Werror=format-truncation` on GCC 16; gfopenmail #29
(main didn't compile: `set_status` was missing an argument); gfopenbench #73 (the built-in Tata needs `tata_ads.c`
since gfopentata 0.6.0); and gfoffice #120 and #121 (GFOpenLayout's header is `gfopenlayout.h`, and openlayout.c
needs its GFCSS sources).

## Tested

Installed with its own Installer in a copy of Instance-32 (lab t5-openup0813) over GFOpenUp 0.8.11, then rebooted.
GFOpenFiles, GFOpenMail, OpenBrowserLite (example.com), GFOpenPrefs, GFOpenUp Setup and the dock all work.

Two things to fix before this pack replaces 0.8.11 on existing Amigas:
- Over 0.8.11 the GF Installer doesn't see the old manifest (ENVARC:OpenAmiga/OpenUp.manifest), so 0.8.11 stays
  installed beside 0.8.13, and both S:User-Startup blocks start their commodities.
- gfopensocket's renamed stack reads ENV:GFOpenSocket/Interface, but 0.8.11 keeps its settings in ENV:OpenSocket. Put
  over 0.8.11 it has no network. The Installer leaves GFOpenSocket out on such an Amiga (0.8.11's bsdsocket.library
  isn't "ours" by its markers), so the old stack goes on working.

Not tested: a fresh install on a new instance. There the second point matters too, if nothing writes
ENV:GFOpenSocket/Interface.
