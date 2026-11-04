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
  comes from gfopenrtg's build under either name (gfopenrtg #176 renames it there). OpenAmigaPython's release archive
  (pinned by sha256) unpacks to OpenUp/, not GFOpenUp/.
- **GFOpenBrowser Lite 0.4.3**, the GFOpenBrowserLite part in SYS:Utilities/GFOpenBrowserLite (gfopenbrowser #169,
  release v0.4.3). 0.4.2 (open5, gfopenbrowser #155) opened GFOpenTLS as opentls.library, its name before the
  rename, so on a GFOpenUp 0.8.13 without 0.8.11's leftovers every page said "No network". 0.4.3 opens
  gfopentls.library.
- **GFOpenTLS by its GF name everywhere.** GFOpenMail, GFOpenFTP (and GFOpenFiles' FTP places) and GFOpenPrint's
  IPPS still opened opentls.library through their vendored headers, so with GFOpenUp 0.8.13's gfopentls.library they
  said GFOpenTLS wasn't installed. GFOpenMail 0.3.5 (gfopenmail #30), GFOpenFTP 0.2.3 (gfopenftp #23) and gfopenprint
  #45 open gfopentls.library; the pack is built from those branches. OpenAmigaPython 2.7.18-ac3's `_ssl` (a VBCC
  release, not rebuilt here) still opens opentls.library, so Python's ssl needs a rebuilt release.
- **GFOpenBench** goes in as SYS:System/OpenBench too, the name Cradle's Startup-Sequence and the GFOpenKick ROM
  start. Without it an Amiga upgraded from 0.8.11 booted the old OpenBench, which 0.8.11's uninstall puts back.
- **No OpenBrowser companion.** The 0.7 companion was built with open3, and its WebKit engine is the largest
  unproven part. It comes back with an engine built on open5.
- **OpenAmigaPython 2.7.18-ac3** is unchanged. It is built with VBCC, which this bug doesn't touch.

The open5 build stopped in four repositories, and each fix is its own PR there. The pack was built from those
branches: gfopenprefs #166 (Look) and #167 (Windows), both `-Werror=format-truncation` on GCC 16; gfopenmail #29
(main didn't compile: `set_status` was missing an argument); gfopenbench #73 (the built-in Tata needs `tata_ads.c`
since gfopentata 0.6.0); and gfoffice #120 and #121 (GFOpenLayout's header is `gfopenlayout.h`, and openlayout.c
needs its GFCSS sources). The upgrade fixes add gfopensocket #40, gfopenrtg #176, gfopenmail #30, gfopenftp #23,
gfopenprint #45 and gfopenbrowser #169 (GFOpenBrowserLite 0.4.3, draft release v0.4.3).

## Upgrading from OpenUp 0.8.11

0.8.11 is the last pack before the GF rename: its list is ENVARC:OpenAmiga/OpenUp.manifest, and its programs and
settings have the names without GF. The first 0.8.13 build left it installed beside 0.8.13, and both S:User-Startup
blocks started their programs. Now Install-GFOpenUp, where it finds that list:

1. Runs **GFOpenUpTool OLDNAMES** before anything else. It keeps a copy of the old list and S:User-Startup in
   SYS:Storage/GFOpenUp-Superseded/OpenUp-before-GF, and copies each old setting to its GF name where that has none
   (ENV: and ENVARC: OpenDock, OpenPrefs, OpenGadTools, OpenSocket, OpenTLS and the rest; OpenUp's Setup.done,
   PrefsView, Sound and Startup.off, the last with the GF program names). Nothing old is deleted.
2. Ticks the parts 0.8.11 had, by their old names (GFOpenPuTTY, GFOpenFTP, GFOpenBench, the GPU SDK, OpenRexx...).
3. After the questions, takes 0.8.11 out as its own Uninstall-OpenUp did: OpenTypes UNDO, OpenUpMenu QUIT, then
   SYS:C/OpenUpTool UNINSTALL, which puts back everything it replaced. Workbench's screen mode is kept. If
   ";BEGIN OpenUp" is still in S:User-Startup afterwards, it stops before installing anything, so the two never
   both start. All of this goes into T:GFOpenUp.log.
4. Installs GFOpenUp, GFOpenSocket included: its keep line now knows the stacks from before the renames
   ("OpenSocket (AmigaChrome)"...).
5. Runs **GFOpenUpTool OLDNAMES DOCK** after the parts: dock buttons for old programs (SYS:Utilities/OpenFiles)
   point to the GF programs that replaced them.

gfopensocket (#40) also reads ENV:OpenSocket/Interface and ENVARC:OpenSocket/Interface when the GF names are
missing, and no longer mistakes 0.8.11's DEVS:Networks/opensocket.device for another driver.

## Tested

Labs are copies of Instance-32, which has 0.8.11. Screenshots are in daletop's
`open5-rebuild-20261103/lab-screenshots/0813-upgrade` and `0813-fresh`.

- **Upgrade 0.8.11 to 0.8.13** (t5-up0813-upgrade3, the final pack): the Installer copied the settings, removed
  0.8.11 (1434 files removed, 129 put back) and installed every part, GFOpenSocket included. After a reboot only GF
  programs run: GFOpenDock (one dock), GFOpenUpMenu, GFOpenBlanker, GFOpenSnap, Tata, GFOpenWindows, GFOpenMenus,
  GFOpenTitle and GFOpenBench (as SYS:System/OpenBench). Nothing named Open* is left in S:User-Startup or SYS:C.
  github.com resolves and 1.1.1.1 answers ping through GFOpenSocket. GFOpenFiles 0.4.2, GFOpenBrowserLite 0.4.3
  (https://example.com/), GFOpenPrefs 2.6 and GFOpenUp Setup 0.1.7 open, and GFOpenGPUCheck says "all right". The
  dock keeps 0.8.11's buttons, now pointing at the GF programs.
- **Fresh install** (t5-up0813-fresh, after 0.8.11 was taken out with its own OpenUpTool UNINSTALL): every part
  installed. After a reboot, GFOpenUp Setup comes up, GFOpenBench and one dock run, and the network works. GFOpenBrowser
  Lite 0.4.3 loads https://example.com/ (0.4.2 couldn't), and GFOpenMailCheck 0.3.5 reaches imap.gmail.com:993 over
  TLS (0.3.4 couldn't).
- **Uninstall** (t5-up0813-fresh): Uninstall-GFOpenUp's GFOpenUpTool UNINSTALL takes the GFOpenUp block out of
  S:User-Startup and puts back what it replaced (1163 files removed, 114 put back). After a reboot no GF program
  runs; the Amiga is as it was before the install (Instance-32's own SYS:System/OpenBench 0.1.16 is back).
