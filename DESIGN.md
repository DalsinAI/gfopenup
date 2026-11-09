# GFOpenUp: design

We, 4 October 2026:
- "one 'upgrade' or some cool name for us installing it on a base 3.2.3
  install"
- "so it's: install 3.2.3, run our 'upgrade' or 'unifier' or 'extender' pack"
- "and it's ready to boot on a PiStorm, our appliance, real Amiga"

This is the GFOpen family as one thing a user installs: the platform of
`OPEN_SDK_DESIGN.md`, and the mission test, delivered.

## 1. The promise

1. Install AmigaOS 3.2.3 as Hyperion ships it (the user's own copy).
2. Run one Installer from the pack.
3. Reboot. RTG works on every monitor, and so do the network, MUI programs,
   printing, mail, archives and file types, with nothing more to read.

**The same pack** works on a real Amiga (with whatever cards it has), on a
PiStorm (Emu68), in an AmiCradle instance, and on the appliance.

## 2. What it finds, and what it installs

| Machine | Found by | Display | Network | Also |
| --- | --- | --- | --- | --- |
| AmiCradle instance, the appliance | AmiCradle Team $DA15 boards in expansion.library's list: ACRTG (product 9, the 16 MiB board, or 15, the 64 MiB one), the GFOpenSocket card (6), ACStorage | GFOpenRTG with `ACRTG.card`, `ACRTG.chip` and GFOpenGPU's `ACRTG.gpu` | GFOpenSocket's host backend | accontrol: the shared clipboard, files and pace |
| PiStorm | Emu68's own resources | The PiStorm's RTG through GFOpenRTG once its driver exists; until then the installed Picasso96 stays. VideoCore later for GFOpenGPU and GFOpenMedia | GFOpenSocketDirect over `wifipi.device` or `genet.device` | |
| A real Amiga | The boards in expansion.library's list, the CPU and FPU, the chipset | GFOpenRTG where it has a driver for the card, and AGA through GFOpenRTG's AGA card; otherwise the installed Picasso96 or CyberGraphX stays | GFOpenSocketDirect over the card's SANA-II driver, or the installed Roadshow or AmiTCP if the user keeps it | |

**The rest is the same everywhere:**
- `zunemaster.library`, plus the `muimaster.library` passthrough where no MUI
  is installed;
- GFOpenGadTools' classes and the look;
- GFOpenPrint and GFOpenView, GFOpenTypes, GFOpenCompress, GFOpenMail, and OpenBrowser
  when it's ready;
- the SDL and Mesa runtime libraries;
- the Shell's font: DejaVu Sans Mono in `FONTS:` as Amiga bitmap fonts, 8
  to 24 pixels high (gfopenfont's `fonts/amiga`), for Font prefs' System
  Default Text (24 October 2026). The GFOpenKick ROM carries the 11 and makes
  it the system default text where no Font prefs name one; GFOpenUp writes no
  Font prefs. [docs/shell-font.png](docs/shell-font.png): the Shell in topaz 8,
  in the 12 from Font prefs, and in the ROM's 11 on its dark background.
- The Shell icon: `SYS:System/Shell.info`'s tool type
  `WINDOW=CON:0///130/AmigaShell/CLOSE/ICONIFY` loses its 130 (24 October
  2026), so a Shell from the icon opens 80 x 24 of the system font, as
  NewShell's own window does. Only that tool type changes; the icon as it was
  is kept in `SYS:Storage/GFOpenUp-Superseded`, and `GFOpenUpTool REMOVE
  Parts/ShellIcon` puts the 130 back ([docs/shell-icon.png](docs/shell-icon.png)).
- GFOpenFont: gfopenfont.library (gfopenfont) in `LIBS:`, and DejaVu Sans as an
  outline font it draws (`FONTS:DejaVuSans.font`, `.otag` and
  `_ttf/DejaVuSans.ttf`), on a 68040 or 68060 with its FPU (24 October
  2026). GFOpenBench's theme names DejaVu Sans 13 and the Open programs' look
  DejaVu Sans 12; without the library their text falls back to the screen's
  font ([docs/openbench-labels.png](docs/openbench-labels.png)).
  With them, AmiSans (9 November 2026): TeX Gyre Heros (GUST Font License),
  the open Helvetica closest to OS 3.2.3's CGTriumvirate, as
  `FONTS:AmiSans.font` with gfopenfont's hinted bitmap sizes 12 and 13
  (`FONTS:AmiSans/`), `AmiSans.otag` (CGTriumvirate's `OT_YSizeFactor`) and
  the unmodified `_otf/texgyreheros-regular.otf`, from gfopenfont's
  `fonts/outline`. The open ROM's tall Workbench takes it where there is no
  CGTriumvirate, before DejaVu Sans, in the same 24-pixel bars; GFOpenUp
  still writes no Font prefs.
- GFOpenBlanker 1.4 (0.8.16): its credits are CGTriumvirate and CGTimes, OS
  3.2's own, where they are. The open ROM's disk has neither, so the part
  brings DejaVu Sans and DejaVu Serif (DejaVu fonts 2.37, free) as outline
  fonts in `FONTS:GFOpenBlanker/` (a drawer Font prefs doesn't list), whose
  `.otag`s have `OT_YSizeFactor` so ascent and descent fit the size
  (gfopenfont.library 1.7 reads it), with `FONTS:_ttf/DejaVuSerif.ttf` and its
  licence; the sans is GFOpenFont's `FONTS:_ttf/DejaVuSans.ttf`. GFOpenFont's
  own `DejaVuSans.otag` is unchanged, so the Open programs' text keeps its
  size. CGTriumvirate and CGTimes are never in the pack.
- GFOpenDock 0.6.13 (0.8.16): a Shell button (`NewShell`) takes
  `SYS:System/Shell`'s icon, and on a disk without one (the open ROM's, where
  NewShell is the ROM's) GFOpenIcons' Shell picture, which the GFOpenDock part
  puts in `SYS:Prefs/Presets/GFOpenDock/Shell.info`.
- GFOpenService 1.2 (0.8.17): `MEDIA:`, gfopenservice's read-only DOS handler
  on top of `media.decode/1` (`L:media-handler`, and its mountlist
  `DEVS:DOSDrivers/MEDIA`, mounted at boot), so shell scripts and programs
  that know nothing of datatypes can decode files by copying them:
  `Copy MEDIA:ilbm/Work:grad.qoi RAM:grad.iff`. The part also brings
  `C:ServiceTest`, which lists the services and opens one. A `MEDIA`
  mountlist of the user's own stays. The part's version is its own:
  gfopenservice.device is still 1.1.
- GFOpenGPU 0.26 (0.8.18): SDL2.module from gfopenrtg #179. SDL's audio
  thread runs at priority 10 and waited on SDL's emulated atomics with a spin
  lock; when the program (priority 0) held one, the thread spun for ever and
  every task at 5 or below, the file systems too, stopped. The atomics now run
  inside `Forbid()`, a waiting lock sleeps, and `SDL_Delay(0)` sleeps for a
  millisecond. With gfopenrtg #181 too: relative mouse mode works with an
  absolute mouse, the desktop mode is the Workbench's, and only SDL's own
  audio threads go above priority 0. gfopengpu.library, GL.module and the
  SDL 2 satellites are unchanged.
- GFOpenUp 0.8.19: z.library 53.13 (amicradle-guest #632, the Aminet
  z.library's jump table; 53.12's was wrong from -102 on) in the GFOffice and
  GFOpenImage parts, and with it the programs that refuse 53.12:
  GFOpenCompress 0.3.3 (gzip, zip, ADZ and tar.gz through z.library at run
  time, gfopencompress #24) and GFOpenImage 0.5 (gfopendatatypes #28-#32: GIF
  and IFF ANIM saving, alpha through picture.datatype, the Aminet LVOs, and
  the WebP lossless save that hung on libnix's qsort). They ship together: a
  disk with one and not the others loses PNG, gzip or zip. GFOpenGPU 0.27:
  gfopengpu.library 0.27, stream v1.10 (RLGR_DEC, RFX_TILE and RFX_YCBCR for
  RemoteFX Progressive, DWT_INV big-endian; gfopenrtg #177, #180, #184). The
  modules are 0.26's. GFOpenCrypto 2.2 (gfopentls #24): SHA-1.
- GFOpenUp 0.8.20: the OGLES3 part, ogles3.library 1.1 on GFOpenGPU
  (gfopenrtg #190): OpenGL ES 3.1 for programs built for ladislawv's ogles3,
  a face of GL.module (virgl on the PC's GPU, softpipe without), with
  C:OGLES3Check and its README and credits in GFOpenUp's Licences drawer. It
  installs with GFOpenGPU, as gfopenrtg's Install-ogles3 has it: "needs
  AmiCradle", "needs part GFOpenGPU" (ACRTG.gpu and GL.module), and a keep
  line that leaves the part out when LIBS:ogles3.library is another one (its
  version string lacks "on GFOpenGPU"), so a PiStorm's ogles3.library is
  never touched.
- GFOpenUp 0.8.21: GL.module from gfopenrtg #192, in the GFOpenGPU part
  (still 0.27: the part's version is gfopengpu.library's). A virgl
  fence whose result the board had forgotten (done, but 16 or more batches
  ago: OGPU_ERR_EXPIRED) was taken as a failure, so with several GL programs
  running one dropped to softpipe or later lost its virgl context ("Illegal
  command buffer"). It is now taken as done. gfopengpu.library, the other
  modules and ogles3.library are unchanged.
- GFOpenUp 0.8.22: the GFOpenGPU part is 0.29, matching ROM full37 (whose
  gfopengpu.library 0.29 and ACRTG.gpu 1.6 shadow the disk's on an open-ROM
  machine). gfopengpu.library 0.29 keeps the last 256 batches' fence results
  where 0.27 kept 16 (gfopenrtg #194) and puts WriteChunkyPixels and
  WritePixelArray8 on planar bitmaps through GFOpenGfx's core (#185; on the
  open ROM graphics.library does that itself); ACRTG.gpu 1.6 keeps 256 too
  (amicradle-guest #638). GL.module says on the program's output and in the
  runtime's log when a program falls to softpipe and why, and
  ENV:GFOpenGPU/GLDriver gpu or cpu chooses (gfopenrtg #195); C:GLQuery
  (new) and C:OGLES3Check print the driver. GFOpenGPUDev 0.7 carries the
  matching libGL.a and gla_core.h. ogles3.library is unchanged (OGLES3 1.1).
- GFOpenUp 0.8.23: LIBS:z.library 53.14 (amicradle-guest #643; in the
  GFOffice and GFOpenImage parts, whose versions are their own programs' and
  stay as they were): CRC-32, Adler-32 and its copies and clears go through
  AC090's helpers, so on AmiCradle they run as host code, and on any other
  68k the same library runs its own C as before. GFOpenWindows 0.12.2
  (gfopenprefs #181 and #184): no snapshot pin on system requesters, and a
  click on a pushed-in pin unsnaps the window. GFOpenFont 1.8 (gfopenfont
  #36): an Intellifont font handed to bullet.library has a space with a
  width again. The Windows page's new label ("click again to unsnap") is in
  GFOpenPrefs 2.8, which isn't in this pack (it is still a release
  candidate), so the pack's GFOpenPrefs stays 2.7.
- GFOpenUp 0.8.24-rc1 put gfopendatatypes edd58bd in the GFOpenImage part,
  but was built on 0.8.22. GFOpenUp 0.8.25 is 0.8.23 with those datatype
  files added (none of them was changed by 0.8.23), so it has both. 0.8.23
  itself was never published.
- GFOpenUp 0.8.26: SDL2.module from gfopenrtg #217 (b407b01), in the
  GFOpenGPU part (still 0.29: the part's version is gfopengpu.library's).
  It hardens full-screen exit. An older SDL2.module (6c85c04c, from before
  0.8.18, still in LIBS:OpenGPU/ where the GFOpenGPU part was never
  installed) left SDL's screens in front and the board at the game's mode
  when a program left full screen or quit in it; 0.8.25's (8d6e9570) passes
  the same checks. Patch 0019 sends a screen Intuition won't close behind
  the Workbench and closes it later, brings the Workbench to the front with
  RemakeDisplay() and repaints it, and does the same on SDL_DestroyWindow and
  SDL_Quit. The part copies the module to LIBS:OpenGPU/ too, so installing
  it replaces the older one there. gfopengpu.library, GL.module, the SDL 2
  satellites and ogles3.library are unchanged.
- GFOpenUp 0.8.26 itself is 0.8.26-rc1 (the blanker's roll call, GFOpenMenus
  0.4.8, the off-screen drawer menu, smaller PDFs, Reinstall and the asking
  Uninstall with GFOpenUpTool 0.8.17, Setup 0.1.8, the old datatype
  descriptors moved aside) with that SDL2.module put in: rc1 was built on
  0.8.25 and still had 8d6e9570. The SDL2-only pack on 0.8.25 (e693f86c)
  was never published. gfopenrtg #218 (Right Amiga keys) isn't in it.
- GFOpenUp 0.8.27: OpenAL.module (gfopenrtg library/modules/openal,
  OpenAL 1.1 and ALC 1.1 on AHI, or ACAHI's voices; no EFX, no capture) in
  the GFOpenGPU part, at LIBS:GFOpenGPU/OpenAL.module and copied to
  LIBS:OpenGPU/ like the other modules, and AL/, libopenal.a,
  libopenal_static.a, openal.pc and find_package(OpenAL) in the GFOpenGPU
  developer kit (gfopenrtg #233). It is its own file: gfopengpu.library is
  unchanged, and only programs that use OpenAL load it. Ports that carried
  their own mixer can link -lopenal instead.
- The CPU floor (the user, 9 November 2026): a 68040 or 68060 with its FPU
  and MMU, nothing less, even for handy things. The GFOpenGPU part says
  "needs 68040" (GFOpenUpTool 0.8.18 leaves it out on less, from AttnFlags:
  68040 or 68060, and FPU40; an FPU there means the full chip, MMU too), and
  the Installer leaves it out with a message, as it does Warp3D. Its modules
  are -m68040 -m68881 and used to install on any CPU.

TLS is GFOpenTLS's (gfopentls.library, with GFOpenCrypto): GFOpenMail, GFOpenFTP's
FTPS, GFOpenPrint's IPPS and httpget use it, and the pack doesn't look for
or offer AmiSSL.

## 3. Rules

- **The base OS stays as shipped.** The pack adds. Anything it replaces goes
  to `SYS:Storage/GFOpenUp-Superseded`, listed in a log, and one uninstaller
  puts everything back.
- **It never overwrites another stack, RTG system or MUI without asking.**
  These are the rules of GFOpenSocket, GFOpenRTG and GFOpenMUI.
- **Novice and Expert.** A Novice user gets the recommended answer to every
  question; an Expert sees each choice.
- **An icon that is already there keeps its default tool.** GFOpenUp offers
  GFOpenView for pictures, text, guides and documents through GFOpenTypes' kinds
  of file (the default icons, `ENVARC:Sys/def_<kind>`; `GFOpenTypes SWITCH`
  without `IN`), never by rewriting the `.info` files of existing files. 0.6.14
  to 0.7.0 did (`SWITCH ... IN SYS:`, with a GFOpenTypes bug that changed about
  40 of 3.2.3's own icons: datatype descriptors, pointer presets, DOSDrivers,
  Picasso96 guides, Help). `GFOpenUpTool FIXTOOLS <table> [ALL]` puts them back,
  once, in 0.7.1's upgrade: an icon is changed only if its default tool is
  GFOpenView now and its original is known, from GFOpenTypes' own record
  (`ENVARC:GFOpenTypes/Changes` and the icons it kept in `Backup`) first, else
  from `pack/StockDefaultTools` (names and tool strings of 3.2.3's icons, made
  by `tools/stock_default_tools.py` from a clean install). The kinds of file
  that named MultiView stay on GFOpenView (that is the offer); `ALL`, which
  UNINSTALL gives, puts those back too. `ENVARC:GFOpenAmiga/DefaultTools-repair.log`
  says what changed; `DefaultTools-repaired` marks that it ran.
- **Startup changes are marked.** They go in a marked block in `S:User-Startup`.
  The one exception is `C:GFOpenRTG`, early in `S:Startup-Sequence` on
  machines whose board has no boot ROM. It too is marked, with the old file
  kept.
- **Re-running a newer pack upgrades in place.** A manifest in
  `ENVARC:GFOpenAmiga/` lists what is installed, with versions; ACBuild's
  AmiCradle Tools tab can read it. An upgrade keeps what the user chose: a
  look already saved in `ENVARC:GFOpenGadTools/Look` keeps its theme, mode,
  profile and other lines, and its backdrop (the Installer's look question
  starts on that theme; another theme picked changes only the theme, through
  `GFOpenUpTool THEME`). Only a new install gets GFOpenUp's own look. A part's
  `face` lines write an icon only when it hasn't got the picture yet.
  UNINSTALL takes GFOpenUp's own marks out of `ENVARC:GFOpenAmiga`
  (`Reconcile.done`, `Tidy.done`, `Setup.done`, `Backdrop.tone`) and leaves
  the user's settings there (`Sound`, `PrefsView`, `Startup.off`).
- **Errors say what to do next,** in plain words (the mission test).
- **Every app and prefs editor has an animated GlowIcon,** drawn and
  shipped as [ICONS.md](ICONS.md) sets out.

## 4. Where it ships

- **Aminet:** one LhA archive.
- **Real machines:** an ADF and an HDF.
- **AmiCradle:** the AmiCradle Team tools drive in instances, and Cradle's recipes
  for new OS 3.2.3 instances. Instances and real machines get the same pack.
- **The appliance:** its first build carries it, demonstrating every Open
  standard.

## 5. Phases

| Phase | Delivers | Done when |
| --- | --- | --- |
| 1 | The Installer, machine detection, the manifest and the uninstaller; GFOpenUp Setup with the startup switches (section 6); what is ready now: GFOpenSocket, GFOpenPrint and GFOpenView, and accontrol's tools in AmiCradle | A fresh OS 3.2.3 scratch copy, plus the pack, prints and reaches the network after one reboot; the uninstaller returns it to clean. **0.1 done, 5 October 2026, for AmiCradle instances, with GFOpenLook too (`docs/history/VALIDATION.md`); Setup and the startup switches are next** |
| 2 | Each Open part as it lands: GFOpenRTG, `zunemaster.library` and the passthrough, GFOpenGadTools, GFOpenTypes, GFOpenCompress, GFOpenMail, OpenBrowser | Each works after the pack is re-run |
| 3 | PiStorm and real-Amiga detection, with testers' hardware | The same pack on a PiStorm and on a real A1200 with an RTG card |
| 4 | Cradle's OS 3.2.3 recipe applies the pack; the appliance ships it | A new instance from Cradle comes up with the GFOpen family |
| 5 | GFOpenUp Update (section 6); OpenStart if we say yes. GFOpenUp Picks is no longer a requirement (the Team, 8 October 2026) | A pack installed today updates itself on request. **Update built (GFOpenUpTool UPDATE, Cradle's side live, 8 October 2026); the GitHub step is next** |

**Name and repository:** GFOpenUp (We, 4 October 2026: "GFOpenUp it is"),
`DalsinAI/gfopenup`.

## 6. What AmiKit tries to be, in GFOpenUp

We, 4 October 2026: "I would like a lot of what AmiKit tries to be, to be
part of GFOpenUp".

**What AmiKit is.** Jan Zahurancik's AmiKit (since 2005, now AmiKit X13;
commercial, by subscription) turns the user's own AmigaOS 3.x into a modern,
good-looking desktop with over 400 programs installed and set up.
- **Where it runs:**
  - in WinUAE or Amiberry on Windows, macOS, Linux and the Raspberry Pi;
  - from a bootable USB stick;
  - on a PiStorm: an A1200 with a PiStorm32-lite and a Pi 4, on OS 3.2 with
    the 3.2.2.1 hotfix and Picasso96 2.0.
- **The user supplies** the Kickstart and AmigaOS.
- **Updates** come by its LiveUpdate.

**What it tries to be:** an Amiga that is beautiful and ready the moment it
starts, kept up to date, with nothing to hunt for. That is our mission too,
and it's the part we take.

| AmiKit | GFOpenUp | |
| --- | --- | --- |
| Everything set up out of the box | One Installer on the user's own OS 3.2.3; a Novice gets the recommended answer to every question | Take |
| AmiKit Setup, a first-boot app for settings | **GFOpenUp Setup**, a GadTools app that runs once at first boot. It sets the theme, the screen mode (with GFOpenRTG's boot to RTG), the network on or off, the printer, and which programs to add. It stays in Prefs to run again | Take |
| Morpheuz's Startup Manager: each patch on or off | **Startup switches** in GFOpenUp Setup: every line GFOpenUp put in its marked startup block, in plain words, each with a switch that cleanly takes it out | Take |
| Visual themes, Dark mode, a 32-bit desktop, FullHD wallpapers | GFOpenGadTools' themes (Classic, Open in the OS 4 style, Graphite in the AmiKit style, Retro; Open and Graphite light or dark, with simple gradient backgrounds; its THEME_SPEC.md), and its Theme Builder in Utilities for making your own on a true-colour desktop from boot (GFOpenRTG `RESIDENT`), with AGA and four-colour forms. Our own art and wallpapers | Take, our way |
| A dozen overlapping look patches (VisualPrefs, Birdie, MCP, MagicMenu and more) | One look in GFOpenGadTools, one prefs page, "off is off" | Ours is simpler |
| Antialiased TrueType (TTEngine, AfA_OS) | GFOpenGadTools' smooth text (its section 2b), or AAText until that is ready | Take |
| DualPNG icons with their own icon.library | GlowIcons-style colour icons for everything, on OS 3.2's own icon.library | Ours |
| Directory Opus 5 in place of Workbench | GFOpenFiles beside Workbench; making it the default for drawers is a switch, off by default | Ours |
| AmiStart start menu (six designs) and a Mini Launcher | To decide: question 1 below | Ask |
| Registered MUI 5, under AmiKit's own licence | GFOpenMUI: MUI programs run through Zune with nothing to buy. A user's own MUI is left alone | Ours, free |
| 400+ programs installed | ~~GFOpenUp Picks~~: no longer a requirement (the Team, 8 October 2026). GFOpenUp installs the GFOpen family only; other programs stay the user's choice | Not taken |
| FPU builds swapped for non-FPU ones | The Installer picks each program's build for the CPU and FPU it finds (section 2) | Take |
| LiveUpdate, by subscription | **GFOpenUp Update**, free: it checks for a newer pack and re-runs it in place (section 3's manifest). It never runs on its own, and it shows what will change first | Take, free |
| KIT, an optional AI helper (AmigaGPT 3) | Guided tours first: show, don't send people to guides. An assistant later, optional and off by default (question 3) | Later |
| Rabbit Hole (host programs from the Amiga desktop); Dropbox and Google Drive folders | On AmiCradle, the shared clipboard, files dropped in and the PC's folders already work. Starting host programs from Workbench is a candidate for Cradle | Partly there |
| Emulators first (WinUAE, Amiberry); its PiStorm edition still being tuned | The same pack on a real Amiga, a PiStorm, AmiCradle and the appliance | Ours |
| Closed, by subscription | MIT and free; one uninstaller puts the OS back | Ours |
| Grown from OS 3.9 and AfA_OS; OS 3.2 users can't move straight to X13 | Built on OS 3.2.3 as Hyperion ships it | Ours |

AmiKit is credited as the precedent (the claims register lists it with
ClassicWB and Coffin). We don't copy its artwork, themes, wallpapers or
exclusive builds.

**Questions for us:**
1. ~~A start menu or launcher~~ **Decided (5 October 2026): GFOpenDock**,
   not a start menu: "an opendock that reads existing dock formats, but is
   closer to the behavior of the mac version". It reads the docks people
   already have (ToolManager docks, AmigaOS 4's AmiDock, DOpus button banks
   among them) and behaves like the Mac's: running programs marked, click to
   start or bring to the front, drag to add or take away, magnification.
2. ~~**GFOpenUp Picks:** which programs, and how many ticked by default?~~ **Decided (8 October 2026): Picks is not a requirement.**
3. **An assistant like KIT:** later, or not at all?

## 7. GFOpenUp in Workbench's Tools menu

The user, 7 October 2026: "introduce menu options under tools for the
openup applications, an extra menu item called preferences".

`GFOpenUpMenu` (`src/openupmenu.c`, about 10 KB, installed by the GFOpenUp part
to `SYS:C/GFOpenUpMenu`) puts two sub-menus in Workbench's Tools menu:

| Tools > | Holds |
| --- | --- |
| GFOpenUp | GFOpenFiles, GFOpenEdit, GFOpenWrite, GFOpenPlay, GFOpenView, GFOpenCompress, GFOpenMail, OpenBrowser; then "GFOpenUp Update..." |
| Preferences | Theme, Dock, Sound, Date & Time, Input, Gamepads, Serial, Printers, Network, File Types, Tata, GFOpenUp Setup |

- **Only what is installed shows.** Each entry is shown when its program is
  where the pack puts it (`tools/build_pack.py`), so a part left out leaves
  no dead entry. Since 10 October 2026 every editor of ours in SYS:Prefs is
  Open... (GFOpenLook, GFOpenDock, OpenSound, GFOpenDateTime, GFOpenInput, OpenSerial,
  GFOpenTypes, GFOpenTata, GFOpenUp-Setup; the user: "either all are Open x or none
  are"), and the OS's own editors we replace are in SYS:Prefs/Classic. "GFOpenUp
  Update..." needs a GFOpenUpTool with `UPDATE`. GFOpenBlanker has no editor
  yet, so it has no entry. Since 24 October 2026 (the no-ROM-volume design,
  section 10) GFOpenPrefs's pages are icons in `SYS:Preferences` by their plain
  names: an entry for a page opens its icon, else
  `SYS:Preferences/GFOpenPrefs <SECTION>`, and shows when either is there.
- **Choosing one is a double-click:** `OpenWorkbenchObjectA`, so the icon's
  stack and tool types count. Icons selected on Workbench go with a GFOpenUp
  program as files to open (not GFOpenEdit's, which is an IconX script).
  "GFOpenUp Update..." runs `GFOpenUpTool UPDATE` in a window, where it shows
  what changes and asks first.
- **Starting.** A `startup Run >NIL: C:GFOpenUpMenu` line in GFOpenUp's block in
  `S:User-Startup`. That runs before `LoadWB`, so GFOpenUpMenu tries every two
  seconds until Workbench takes its entries, then only waits on its two
  message ports (no polling). That is sturdier than a WBStartup icon: it
  needs no icon, is in Setup's startup switches like every GFOpenUp line, and
  goes with the block on uninstall. The Installer also starts it at the
  end, so the menus are there before the reboot.
- **One at a time.** Its public port is `GFOpenUpMenu`. Run again, it tells the
  running one to build its menus anew (the Installer does this after an
  upgrade). `GFOpenUpMenu QUIT`, or Ctrl-C, takes the entries out and ends it;
  Uninstall-GFOpenUp runs `QUIT` before `GFOpenUpTool UNINSTALL`, so the menus
  go at once. `GFOpenUpMenu LIST` prints what the menus hold.
- **Older Workbenches.** Sub-menus need workbench.library 44.1511. Before
  that, the entries are flat: "GFOpenUp: GFOpenFiles", "Preferences: Look".
- **The GFOpenMonitor hotkeys (0.1.7).** Dale, 3 November 2026: "an OS wide
  option to bring up monitor by pressing ctrl alt help on an amiga and ctrl
  alt f12 on a pc keyboard". GFOpenUpMenu runs from the boot on every
  install, whatever the desktop, so it holds them: a commodity, "GFOpenUp
  Hotkeys", with two filters. Ctrl+Alt+Help is raw key 0x5F; a PC's F12 is
  raw key 0x6F (AROS, OS 4 and the keyboard adapters use that code), which
  AmiCradle sends for Ctrl+Alt+F12. Either Alt counts; Shift, an Amiga key or
  a repeat does not. GFOpenMonitor comes to the front through its own port
  when it runs; else it is started (`SYS:Utilities/GFOpenMonitor WAIT`, the
  ROM's by name without it) with the default public screen brought to the
  front. The switch is `ENV:GFOpenPrefs/Hotkeys` (`monitor off`; on by
  default), set in GFOpenPrefs' Input page, which sends Ctrl-F to have it
  read again. Exchange can disable it; its Remove takes the hotkeys out and
  leaves the menus. The keys and words are `src/openup_hotkeys.h`, tested by
  `tests/test_openup_hotkeys.c`.

![Tools > GFOpenUp on OS 3.2.3, with GFOpenMenus and the Open light theme](docs/tools-menu-openup.png)
![Tools > Preferences](docs/tools-menu-preferences.png)
