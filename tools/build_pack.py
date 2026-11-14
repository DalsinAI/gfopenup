#!/usr/bin/env python3
"""Builds GFOpenUp: a drawer to run the Installer from, and the same as an LhA
archive (DESIGN.md sections 3 and 4).

    python3 tools/build_pack.py [--version 0.1] [--out build/pack]
        [--amigachrome DIR] [--openamigaprint DIR] [--opengadtools DIR]
        [--guest DIR] [--openamigaservice DIR] [--opendatatypes DIR] [--openamigartg DIR]
        [--openamigafilemanager DIR] [--openamigamedia DIR] [--openamigamediaplayer DIR] [--openamigamui DIR]
        [--openamigaedit DIR] [--gfoffice DIR] [--openamigalayout DIR]
        [--openamigamesa DIR] [--openamigatls DIR] [--openamigaputty DIR] [--openamigatata DIR]
        [--openamigaftp DIR] [--openamigatelegram DIR] [--opengpu-sdk DIR] [--openamigainput DIR]
        [--openamigasocket DIR] [--openamigaphysfs DIR] [--openamigabench DIR] [--openamigafont DIR] [--openamigapython FILE]
        [--openbrowserlite FILE] [--no-build] [--lha]

GL.module's build (gfopenrtg library/modules/gl) needs meson on PATH and
the os32-gcc16 stove; GL_WORK names its work directory (about 1 GB). GFOpenDemos
is built against it. The GCC 16 builds (GFOpenService, the datatypes, SDL 2, GL,
GFOpenDemos, gfopengfx.library, GFOpenCrypto, GFOpenPuTTY) take their stove from
OS32_GCC16 / STOVE16 / SDL2_STOVE; the 0.6.17 pack used a stove whose gcc adds
-fno-tree-loop-distribute-patterns (GCC 16.2.0b's loop-shift miscompile).

Each part is built by its own repository's scripts, from checkouts given on
the command line (or found beside this one), then laid out as it goes, with
a Part file that C/GFOpenUpTool reads:

  GFOpenUp/               GFOpenUpTool and the uninstaller; C:ACROMFix, SetPatch for the open ROM
                        (amicradle-guest tools/acromfix), in the boot's GFOpenUp lines (first on a new install); GFOpenUpMenu: the
                        Tools menu's GFOpenUp and Preferences sub-menus, and
                        GFOpenMonitor 1.0, the task manager, and its icon in SYS:Utilities, on every
                        ROM (the open ROM has the same 1.0 from P6 on; an open ROM from before has
                        0.3.3 or 0.3.4, and the newer version wins);
                        LIBS:softfpu.library, the FPU in software for a 68LC040 or
                        68EC040 under the open ROM (amicradle-guest libraries/opensoftfpu); on the GFOpenKick
                        ROM, where none is, what it leaves to a disk: iffparse, diskfont, commodities and
                        lowlevel.library, clipboard, serial and parallel.device (OPENKICK_DISK, the OpenKick
                        boot disk's files), and SYS:Classes/arexx.class, so programs' ARexx ports open
                        (OPENKICK_CLASSES: amicradle-guest's GFOpenReaction, with its licence)
  GFOpenSocket/           bsdsocket.library and its tools, built from gfopensocket
                        (--openamigasocket) and laid out by the manifest of amicradle's
                        vendored payload (vendor/opensocket-os32); without the checkout,
                        that vendored payload as it is
  GFOpenPrint/            GFOpenPrint and GFOpenView (gfopenprint's package)
  GFOpenPrinter/          our printer.device in front of GFOpenPrint, with printer.prefs naming GFOpenPrint's driver;
                        OS 3.2.3's printer drivers, editors, tools and their help archived (amicradle-guest
                        devices/openprinter.device; AmiCradle instances only)
  AmiCradleSharing/   accontrol.device, ACClip and ACDrop (amicradle-guest)
  HostClock/            C:HostClock, GFOpenBattClock as a command: at boot it puts the host-backed
                        battclock.resource (amicradle-guest resources/openbattclock.resource) in place of
                        the Kickstart's, sets the system time from it once, and from then on the resource
                        keeps the Amiga's clock with the host's (snapshot resumes, the host's clock set
                        after boot). On every ROM of an AmiCradle instance; the open ROM from full40 has
                        it in ROM already, and HostClock then only sets the time
  Backdrops/            the themes' desktops as Workbench backdrops, each with its .tone
                        (gfopengadtools); not the AmiCradle Background Pack's art, an
                        optional install from Cradle's Media Library
  GFOpenIcons/            GFOpenIcons look B, the Team's own icon set (MIT), as new pictures for
                        the system's icons, made at build time by its generator from the
                        amicradle-guest checkout (--guest; tools/openicons_part.py)
  GFOpenTypes/            GFOpenPrefs's File Types page: no files of its own since the no-ROM-volume
                        design (24 October 2026); it archives its old name's stub in SYS:Prefs
  GFOpenCompress/         GFOpenCompress, its window and command, and its two libraries in LIBS:
                        (gfopencompress_archive and _crunch.library: RAR, 7z, bzip2, xz, crunchers)
  GFOpenService/          mitchy.device (with gfopenservice.device, its old name) and Nursery: services from boards and
                        paired Cradles; ServiceTest; MEDIA:, files decoded through
                        media.decode/1 (L:media-handler, DEVS:DOSDrivers/MEDIA); mitchy.library (gfopenservice)
  AkikoPlus/              akikoplus.library 2: C2P, swaps, fills, scaling, YUV, colours and zlib on
                        AmiCradle's AkikoPlus media board (through mitchy.library) or on the 68k;
                        C:AkikoTest, C:APTest and SYS:Developer/AkikoPlus. Installed with GFOpenService
                        (gfosautoconfigboards cards/akikoplus/guest; left out without --akikoplus)
  KickstartSet/           the slim ROM's disk part (amicradle-guest's thin profile, docs/design/thin-loader.md):
                        the module set the ROM leaves out, as SYS:Kickstart/default/ with its Manifest, and
                        the files the full ROM held on demand (asl, locale, version, datatypes and its
                        classes, realtime, the maths libraries, the L: handlers, CDFileSystem, ACRTG.gpu...),
                        all from the same build as the ROM (its OUT/thin-support/, --thinsupport). Installed
                        by GFOpenUpTool KICKSET on a machine on the slim ROM, and before one moves to it;
                        left out without --thinsupport
  GFOpenImage/            datatypes: PNG, Targa (TGA), WebP, WebM and music modules (MOD,
                        MED, XM...) on the 68k, and AVIF, HEIC, SVG, FLAC, MP4, DOCX and more
                        through GFOpenService (gfopendatatypes)
  GFOpenBlanker/          the screen blanker and its show, Gods and Angels: always installed (gfopenprefs), with
                        DejaVu Sans and Serif for its credits where OS 3.2's CGTriumvirate and CGTimes
                        aren't (FONTS:GFOpenBlanker; --dejavu-serif)
  GFOpenSnap/             GFOpenSnap, the snapshot commodity: Shift+Right Amiga+S takes the front screen and
                        Shift+Right Amiga+W the active window, as dated PNGs in SYS:Snapshots (through GFOpenImage's
                        openpng.datatype; IFF without it) and as an IFF ILBM on the clipboard; its keys are its
                        icon's tool types. Always installed (src/opensnap/)
  ShellFont/            DejaVu Sans Mono as Amiga bitmap fonts, 8 to 24 pixels high, in FONTS:, with its
                        licence, for Font prefs' System Default Text; always installed. The GFOpenKick ROM
                        has the 11 and makes it the Shell's font itself; no Font prefs are written
                        (gfopenfont's fonts/amiga; left out without --openamigafont)
  GFOSFonts/            GFOS Sans, GFOS Sans Bold and GFOS Mono, GFOS's own hand-drawn bitmap fonts, 11, 13 and 16
                        pixels, in FONTS: (GFOSSans.font, GFOSSansBold.font, GFOSMono.font), with their licence;
                        always installed. Built here from a gfosfonts checkout's grids (its tools/amigafont.py);
                        no Font prefs are written (left out without --gfosfonts)
  ShellIcon/            the Shell icon's WINDOW tool type without its 130, so a Shell from SYS:System/Shell
                        opens 80 x 24 as NewShell's own window does; always installed, after GFOpenIcons
                        (GFOpenUpTool REMOVE Parts/ShellIcon puts the 130 back)
  GFOpenFont/             gfopenfont.library, and DejaVu Sans as an outline font it draws (FONTS:DejaVuSans.font,
                        .otag and _ttf/DejaVuSans.ttf), with their licences: the font GFOpenBench's theme and
                        the Open programs' look name; and AmiSans, TeX Gyre Heros (FONTS:AmiSans.font with
                        hinted sizes 12 and 13, .otag and _otf/texgyreheros-regular.otf, GUST Font License),
                        the open stand-in for OS 3.2.3's CGTriumvirate that the ROM's tall Workbench falls
                        back to. Installed on a 68040 or 68060 with its FPU (the --openamigafont
                        checkout's build-amiga.sh and fonts/outline; DejaVu Sans from --dejavu-sans)
  GFOpenPrefs/            GFOpenPrefs, every editor of ours as a page of one program, in its own
                        drawer, SYS:Preferences/GFOpenPrefs, with a project icon per page by its
                        plain name (Theme, Sound, Date & Time...) and SYS:Preferences on the
                        Shell's path. The old stubs in SYS:Prefs, SYS:Prefs/OpenPrefs and
                        C:OpenPrefs are archived. The boot's GFOpenPrefs APPLY line stays only
                        where the ROM doesn't apply the settings itself ("unless GFOpenKick").
                        Always installed (gfopenprefs; the no-ROM-volume design of 24 October
                        2026, sections 8 and 10). With the themes in SYS:Prefs/Presets/Themes and the
                        desktop profiles in SYS:Prefs/Presets/GFOpenPrefs, which its Theme page lists
  GFOpenWindows/          snapping, Amiga+Tab, double-click to front, the wheel under the
                        pointer and window places; its editor is GFOpenPrefs's Windows (gfopenprefs)
  GFOpenDock/             a dock, which takes over ToolManager's, AmiDock's or AmiStart's
                        buttons; its editor is GFOpenPrefs's Dock (gfopenprefs); GFOpenIcons' Shell picture
                        for a Shell button where there is no SYS:System/Shell
  GFOpenMenus/            GFOpenMenus, the Team's own menus (pull-down or pop-up); their editor is
                        GFOpenPrefs's Menus (gfopenprefs)
  GFOpenUpSetup/          GFOpenUp Setup: the first-start wizard and the startup switches
                        (gfopenprefs)
  GFOpenRTG/              Workbench on GFOpenRTG's own 16-bit screen, without Picasso96:
                        gfopenrtg.library (Native modes that follow the host window from
                        0.17), its monitor driver and CyberGraphX compatibility, with
                        gfopengpu.library and ACRTG.gpu (gfopenrtg; AmiCradle's ACRTG
                        board). No screen mode: its Mode/ drawer (part GFOpenRTGMode) has
                        Workbench's, which the Installer puts in only when a new install
                        on Kickstart 3.2.3 says Yes
  GFOpenGPU/              gfopengpu.library and LIBS:GFOpenGPU/: ACRTG.gpu (the ACRTG
                        board's ring, amicradle-guest), SDL2.module and GL.module
                        (Mesa), loaded when a program first asks; C:GFOpenGPUCheck.
                        On the CPU where there is no driver (gfopenrtg). LIBS:physfs.library 1.1
                        (PhysicsFS for the game ports, gfopenphysfs at GFOPENPHYSFS's commit) with
                        LIBS:z.library, which it inflates zip members through (--openamigaphysfs)
  OGLES3/               ogles3.library on GFOpenGPU (OpenGL ES 3.1 for programs built for
                        ladislawv's ogles3) and C:OGLES3Check, with GFOpenGPU on AmiCradle
                        only; another ogles3.library (the PiStorm's) is left alone (gfopenrtg
                        library/ogles3)
  Warp3D/               Warp3D.library 5.0 over GFOpenGPU or the CPU, C:Warp3DPrefs,
                        W3DTest and W3DCheck; Wazp3D's settings are taken over
                        (gfopenrtg; needs a 68040 or 68060 with its FPU)
  GFOpenDemos/            Boing, Chrome Gears and Copper Tunnel on GL.module, off
                        by default (--openamigamesa)
  GFOpenCrypto/           gfopencrypto.library 2.1: SSH's and TLS's sums, on x86 or ARM64 cores through
                        AC090 on AmiCradle, on the 68k elsewhere (--openamigatls)
  GFOpenTLS/              gfopentls.library (a TLS 1.3 and 1.2 client, its maths through GFOpenCrypto)
                        and S:GFOpenTLS/: Mozilla's CA list (MPL-2.0) from this machine's
                        ca-certificates package, with its licence; off by default, picked with
                        GFOpenFTP (--openamigatls)
  GFOpenPuTTY/            GFOpenPuTTY 0.2, PuTTY on GFOpenCrypto, off by default (--openamigaputty)
  Tata/                 Tata, the file inspector: C:Tata (the hooks, the question and the
                        Shell command, started from S:User-Startup), its window
                        SYS:Prefs/Tata, and the list, S:Tata.known (the window's "Update
                        the list" reads Parts/Tata/S/Tata.known), and the known fixes,
                        S:Tata.fixes (Tata 0.7.0); off by default while
                        Tata is 0.x (--openamigatata)
  GFOpenFTP/              GFOpenFTP, FTP and FTPS in SYS:Utilities/GFOpenFTP, off by default;
                        FTPS works through GFOpenTLS (--openamigaftp)
  Telegram/             Unofficial Telegram for Amiga (Open), SYS:Utilities/UnofficialTelegram,
                        off by default; it needs setting up: the user's own Telegram API
                        key and the telegram.td/1 service (--openamigatelegram)
  GFOpenGPUDev/           the GFOpenGPU developer files (gfopenrtg tools/make_sdk.py's kit) in
                        SYS:Developer/GFOpenGPU-SDK with the GFOpenGPU-SDK: assign, off by
                        default (--opengpu-sdk DIR: the kit is made in, or read from, DIR)
  GFOpenInput/            gfopeninput.library (game controllers for every program), C:OIList,
                        and the docs (Gamepads is GFOpenPrefs's); off by default; the boot's
                        Gamepads USE is in GFOpenPrefs APPLY (--openamigainput)
  GFOpenFiles/            GFOpenFiles, the two-pane file manager (gfopenfilemanager)
  GFOpenPlay/             GFOpenPlay, the media player for every datatype (gfopenmediaplayer)
  GFOpenTitle/            GFOpenTitle: logo, free memory, clock and the settings cog on the title bar, with
                        GFOpenNetwork, the LAN and Wi-Fi icons in its row (gfopenprefs)
  GFOpenDateTime/         GFOpenPrefs's Date & Time page (since W48): no files of its own since the
                        no-ROM-volume design (24 October 2026); it archives its old name's stub in
                        SYS:Prefs
  GFOpenBench/            GFOpenBench, the Team's own Workbench, in SYS:System, off unless picked
                        (--openamigabench). It runs where the Startup-Sequence starts
                        SYS:System/GFOpenBench in LoadWB's place; GFOpenUp doesn't change the
                        Startup-Sequence yet
  GFOpenLibs/             the open libraries, one switch each, off by default (0.8.0): S:GFOpenLibs-Startup
                        (OS 3.2's LoadModule, from S:Pre-LoadModule-Startup), and a part of its own per library in
                        GFOpenLibs/<name> (utility.module, mathieeedoubbas.library, input.device
                        50.2, OpenRexx's ARexx libraries), which GFOpenUpTool REMOVE takes out alone (amicradle-guest,
                        gfopeninput; OPEN_LIBRARIES.md section 3)
  Sound/                GFOpenPrefs Sound and GFOpenSpeaker, the menu bar speaker (gfopenprefs), and
                        GFOpenAHI's files for the ACAHI sound board: acaudio.audio, its modes file
                        and AHI's settings on Host mix, with GFOpenAHI's own ahi.device
                        (amicradle's vendor/amigachrome-guest; all "when AmiCradle")
  GFOpenEdit/             GFOpenEdit, the text editor with tabs (gfopenedit)
  GFOpenDrawer/           GFOpenDrawer, the drawer windows of the Team's own Workbench, beside the OS's
                        Workbench, in SYS:Utilities; off by default (gfopengadtools workbench/)
  GFOffice/             what the GFOffice Suite's programs share: their four libraries (LIBS:owfwrite.library,
                        owfcalc.library, owfpub.library and owfpresent.library), LIBS:z.library (ours,
                        GFOpenZlib), the spelling dictionary in SYS:Dictionaries and C:OWConvert, with their
                        licences; it retires LIBS:owf.library, which the four replace; installed with
                        GFOpenWrite (gfoffice; amicradle-guest's openzlib)
  GFOpenWrite/            GFOpenWrite, the word processor, on owfwrite.library; it needs GFOffice ("needs part")
                        (gfoffice, gfopenlayout)
  GFOpenCalc/             GFOpenCalc, the GFOffice Suite's spreadsheet, on owfcalc.library; it needs GFOffice. Left out when gfoffice's
                        build didn't make it; no Installer choice yet (GFOpenUpTool INSTALL Parts/GFOpenCalc)
  GFOpenPresentation/     GFOpenPresentation, the GFOffice Suite's slides (ODP, PPTX in, PDF, print, show, the
                        GFOPENPRESENTATION ARexx port), libowf linked in; it needs GFOffice (z.library). Left out
                        when gfoffice's linked build didn't make it; no Installer choice yet
  GFOpenMUI/              muimaster.library, the Team's own MUI on GFOpenLayout (only where no MUI is), and its
                        examples in SYS:Tools/GFOpenMUI (gfopenmui muimaster/; left out without a build)
  GFOpenMulticore/        gfopenmulticore.library: jobs on a cores board's other cores, or on
                        the main CPU without one, OMCTest and OMCBench (gfopenmulticore)
  GFOpenMail/             GFOpenMail, the mail reader, and C:GFOpenMailCheck; needs GFOpenTLS (gfopenmail)
  OpenAmigaPython/      OpenAmigaPython, Python 2.7.18 for AmigaOS 3 (any Amiga with a 68020 or
                        later), in SYS:Tools/OpenAmigaPython/<version> with Python: and the Shell's
                        path; off by default, and GFOpenTLS comes with it. Not built here (VBCC and
                        PosixLib aren't on the stoves): the Part its release makes, from the
                        release's OpenAmigaPython-<version>-GFOpenUp.lha, pinned by its sha256
                        (OPENAMIGAPYTHON below; --openamigapython FILE)
  GFOpenBrowserLite/    GFOpenBrowser Lite 0.4.3, the small web browser (GadTools, no WebKit, about
                        100 KB), in SYS:Utilities/GFOpenBrowserLite: pages laid out by its weblayout
                        engine on GFOpenLayout (built in), pictures through datatypes, CSS; https://
                        through GFOpenTLS. In the standard set: ticked on a new install on a 68040
                        or 68060 with its FPU. Not built here: the program from gfopenbrowser's
                        release GFOpenBrowserLite-<version>.lha, fetched by its address and pinned by
                        its sha256 (OPENBROWSERLITE below; --openbrowserlite FILE)
  OpenBrowser/          OpenBrowser, the web browser, in its own archive beside the pack,
                        GFOpenUp-<version>-OpenBrowser.lha (--openbrowser; gfopenbrowser, pinned by commit
                        and sha256: OPENBROWSER below)

Icons are OS 3.2-style, made with gfopenprint's icon writer (ACBuild's
amiga-icon.js). MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import ast
import re
import importlib.util
import json
import os
import shutil
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEV = ROOT.parent
STOVE = Path(os.environ.get("STOVE", Path.home() / "AmiCradle/stoves/os32"))
# amicradle-guest's scripts: the GCC 16 open6 stove by the name the Cradle hosts give it (the scripts' own default,
# os32-gcc16-16.2.0-open6, is a folder daletop doesn't have: Corwin's 14 Nov build)
GUEST_STOVE = Path(os.environ.get("OS32_GCC16_OPEN6", Path.home() / "AmiCradle/stoves/os32-gcc16-open6"))
CC = STOVE / "prefix/bin/m68k-amigaos-gcc"
# soft float: -m68040 links libnix's libm020, not libm881 (specs/m68k-soft-m68040.specs)
SOFT_SPECS = Path(__file__).resolve().parent.parent / "specs/m68k-soft-m68040.specs"
# Every 68k compile here: address 0 is real memory on the Amiga, and without
# this GCC puts TRAP #7 (80000027) where it can prove a pointer null
# (amicradle #320). The parts' own scripts carry it too.
NULL_OK = "-fno-delete-null-pointer-checks"

# GFOpenSocket's files that must be ours, or absent: never another stack's (DESIGN.md section 3).
# The names before the renames are ours too (0.8.13): OpenUp 0.8.11's and Cradle's own installer's stack says
# "OpenSocket (AmigaChrome)", and the rename's sed had turned the ACNet ones into names no stack ever had.
OPENSOCKET_MARKERS = ["GFOpenSocket (AmiCradle)", "OpenSocket (AmiCradle)", "OpenSocket (AmigaChrome)",
                      "ACNet (AmiCradle)", "AmiCradle ACNet", "ACNet (AmigaChrome)", "AmigaChrome ACNet"]

# GFOpenPrinter (24 October 2026, the plan "Retiring the legacy printing, in favour of GFOpenPrint"): on AmiCradle
# instances what programs print the old way (printer.device, PRT:) goes to GFOpenPrint. Our printer.device
# (amicradle-guest devices/openprinter.device) takes OS 3.2.3's place; printer.prefs names GFOpenPrint's driver on
# oapspool.device unit 0. OS 3.2.3's own printer drivers, the Printer, PrinterGfx and PrinterPS editors, CMD,
# GraphicDump, InitPrinter and PrintFiles, and their help and catalogs are archived (retire: SYS:Storage/
# GFOpenUp-Superseded; REMOVE and UNINSTALL put them back). The part leaves a printer.device that is neither OS 3.2's
# nor ours (TurboPrint's, say) and changes nothing then.
# The last marker is the name a printer.device built before 1 November 2026 carries (the Team was the
# AmigaChrome Team until then), so an update still knows ours.
OPENPRINTER_MARKERS = ["printer.device 47.", "GFOpenPrinter, AmiCradle Team", "GFOpenPrinter, AmigaChrome Team"]
LEGACY_PRINTER_DRIVERS = ("EpsonQ", "EpsonX", "EpsonXOld", "Generic", "HP_DeskJet", "HP_DeskJet_340", "HP_DeskJet_400",
                          "HP_DeskJet_600", "HP_DeskJet_895C", "HP_LaserJet", "HP_LaserJet_2100", "HP_LaserJet_4",
                          "HP_LaserJet_III", "NEC_Pinwriter", "PostScript")
LEGACY_PRINTER_EDITORS = ("Printer", "PrinterGfx", "PrinterPS")
LEGACY_PRINTER_TOOLS = ("CMD", "GraphicDump", "InitPrinter", "PrintFiles")
LEGACY_PRINTER_HELP = ("Sys/Prefs/printer.help", "Sys/Prefs/printergfx.help", "Sys/Prefs/printerps.help",
                       "Sys/Tools/graphicdump.help", "Sys/Tools/initprinter.help", "Sys/Tools/printfiles.help",
                       "Sys/Tools/cmd.help")
# OS 3.2.3's languages (amicradle scripts/os32_host_install.py LANGUAGE_IDS); their help and catalog files are
# taken to have english's names. A retire line for a file that isn't there does nothing.
OS32_LANGUAGES = ("czech", "dansk", "deutsch", "english", "english_british", "español", "français", "greek",
                  "italiano", "nederlands", "norsk", "polski", "português", "russian", "srpski", "svenska", "türkçe")


def legacy_printing_paths() -> list[str]:
    """What the GFOpenPrinter part archives, where each may be: the drivers in DEVS:Printers (and SYS:Storage/Printers,
    where a disk the real Installer made keeps them), the editors in SYS:Prefs/Classic (RECONCILE's) and SYS:Prefs,
    the tools, the help and catalogs in each language, and SYS:Storage/Printers.info (an icon with no drawer)."""
    out = []
    for where in ("SYS:Devs/Printers", "SYS:Storage/Printers"):
        for d in LEGACY_PRINTER_DRIVERS:
            out += [f"{where}/{d}", f"{where}/{d}.info"]
    out.append("SYS:Devs/postscript_init.ps")
    for where in ("SYS:Prefs/Classic", "SYS:Prefs"):
        for e in LEGACY_PRINTER_EDITORS:
            out += [f"{where}/{e}", f"{where}/{e}.info"]
    for t in LEGACY_PRINTER_TOOLS:
        out += [f"SYS:Tools/{t}", f"SYS:Tools/{t}.info"]
    for lang in OS32_LANGUAGES:
        out += [f"SYS:Locale/Help/{lang}/{h}" for h in LEGACY_PRINTER_HELP]
        out.append(f"SYS:Locale/Catalogs/{lang}/Sys/hp_printer.catalog")
    out.append("SYS:Storage/Printers.info")
    return out


def iff_chunk(cid: bytes, body: bytes) -> bytes:
    return cid + struct.pack(">I", len(body)) + body + (b"\0" if len(body) & 1 else b"")


def printer_prefs(driver: str = "GFOpenPrint", device: str = "oapspool", unit: int = 0, paper_size: int = 9,
                  paper_length: int = 66, pitch: int = 0, spacing: int = 0, left: int = 5, right: int = 75) -> bytes:
    """ENVARC:Sys/printer.prefs as OS 3.2's Printer editor writes it: FORM PREF with PRHD, PTXT, PUNT and PDEV in
    the NDK 3.2 layouts (prefs/printertxt.h). The editor's numbers: paper size 9 is A4 (0 US Letter, 1 US Legal),
    pitch 0 pica (1 elite, 2 fine), spacing 0 six lines to the inch (1 eight). PUNT names the port device without
    ".device", as the editor does: "oapspool" is oapspool.device, GFOpenPrint's."""
    ptxt = (b"\0" * 16 + driver.encode("latin-1").ljust(30, b"\0")[:30] + b"\0\0"     # reserved, pt_Driver, pt_Port 0, pad
            + struct.pack(">8H", 1, paper_size, paper_length, pitch, spacing, left, right, 1))   # single sheets, ... letter quality
    punt = b"\0" * 16 + struct.pack(">iI", unit, 0) + device.encode("latin-1").ljust(32, b"\0")[:32]
    pdev = b"\0" * 16 + struct.pack(">i", unit) + driver.encode("latin-1").ljust(32, b"\0")[:32]
    body = b"PREF" + iff_chunk(b"PRHD", b"\0" * 6) + iff_chunk(b"PTXT", ptxt) + iff_chunk(b"PUNT", punt) + iff_chunk(b"PDEV", pdev)
    return b"FORM" + struct.pack(">I", len(body)) + body


def printergfx_prefs(aspect: int = 0, shade: int = 2, threshold: int = 7, density: int = 2) -> bytes:
    """ENVARC:Sys/printergfx.prefs as OS 3.2's PrinterGfx editor writes it: FORM PREF with PRHD and PGFX
    (prefs/printergfx.h). Aspect 0 horizontal, shade 2 colour, density 2 (GFOpenPrint's driver: 100 dots to the inch,
    as the lab's proof of printer.device against OS 3.2.3's was run)."""
    pgfx = (b"\0" * 16 + struct.pack(">HHHh", aspect, shade, 0, threshold)
            + bytes([0, 0, 0, 0]) + struct.pack(">H", 0) + bytes([density, 0]) + struct.pack(">HH", 0, 0) + b"\0\0")
    body = b"PREF" + iff_chunk(b"PRHD", b"\0" * 6) + iff_chunk(b"PGFX", pgfx)
    return b"FORM" + struct.pack(">I", len(body)) + body


def openprinter_part(top: Path, printer_device: bytes) -> "Part":
    """The GFOpenPrinter part (the comment at OPENPRINTER_MARKERS). Its version is the printer.device's."""
    version = binary_version(printer_device, "printer.device")
    if not version:
        raise SystemExit("GFOpenPrinter: the printer.device has no version string")
    op = Part(top, "GFOpenPrinter", version, "an open printer.device that sends what older programs print to GFOpenPrint; "
              "the OS's printer drivers, editors and tools archived")
    op.line("needs AmiCradle")
    op.line('keep "SYS:Devs/printer.device" ' + " ".join(f'"{x}"' for x in OPENPRINTER_MARKERS))
    op.add("Devs/printer.device", printer_device, "SYS:Devs/printer.device")
    # the settings only where there are none: GFOpenPrefs's Printing page changes them after, and an update keeps them
    for name, data in (("printer.prefs", printer_prefs()), ("printergfx.prefs", printergfx_prefs())):
        op.add(f"Prefs/{name}", data, f"ENVARC:Sys/{name}", default=True)
        op.add(f"Prefs/{name}", data, f"ENV:Sys/{name}", default=True)     # in use before the next start
    for path in legacy_printing_paths():
        op.line(f'retire "{path}"')
    return op


def run(cmd, cwd=None, env=None):
    print("  $", " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], cwd=cwd, env=env, check=True)


def wbconfig_backdrop() -> bytes:
    """ENVARC:Sys/WBConfig.prefs with Workbench as a backdrop: WBCF is flags (0:
    no saved box), 0, the window box (0, 11, 640, 389, as a stock 3.2.3 writes)
    and the backdrop word (1)."""
    wbcf = struct.pack(">IIhhhhhh", 0, 0, 0, 11, 640, 389, 1, 0)
    prhd = b"\0" * 6
    body = b"PREF" + b"PRHD" + struct.pack(">I", len(prhd)) + prhd + b"WBCF" + struct.pack(">I", len(wbcf)) + wbcf
    return b"FORM" + struct.pack(">I", len(body)) + body


def pack_version(v: str) -> str:
    """--version: 0.8.29, or a release candidate 0.8.29-rc1 (DECISION-RELEASE-CYCLE-20261114-1302's numbering),
    which binary_version reads back from the Installer's and uninstaller's $VER (Corwin's item 10)."""
    if not re.fullmatch(r"[0-9]+(\.[0-9]+)+(-rc[1-9][0-9]*)?", v):
        raise argparse.ArgumentTypeError(f"{v!r}: digits and dots, then -rcN (N from 1) for a release candidate (0.8.29-rc1)")
    return v


def first(*paths: Path) -> Path | None:
    for p in paths:
        if p and p.is_dir():
            return p
    return None


class Part:
    """A part's drawer: its files and its Part file's lines."""

    def __init__(self, top: Path, name: str, version: str | None, title: str):
        """version None: the part's version is its main program's own (PART_MAIN), read when the part is
        written, so a part can't fall behind its program (Corwin's 14 Nov build: seven had)."""
        self.dir = top / "Parts" / name
        if self.dir.exists():
            shutil.rmtree(self.dir)
        self.dir.mkdir(parents=True)
        self.name = name
        self.lines = [f"part {name} {version}" if version else "", f'; {title}']

    def add(self, rel: str, data: bytes, dest: str, default: bool = False, own: bool = False, cond: str = ""):
        """default: only where nothing is (the user's settings); own: where
        nothing is or GFOpenUp put it, so an upgrade reaches it but another
        program's file stays. cond: a condition in front of the line
        ("unless GFOpenKick", GFOpenUpTool 0.8.9)."""
        p = self.dir / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        self.lines.append(f'{cond + " " if cond else ""}{"add" if own else "default" if default else "copy"} "{rel}" "{dest}"')

    def line(self, text: str):
        self.lines.append(text)

    def needs_part(self, name: str):
        """Another part that must be installed first: GFOpenUpTool INSTALL leaves this one out without it, and
        REMOVE of that part refuses while this one is installed (src/openup_lines.h, 25 October 2026)."""
        self.lines.append(f"needs part {name}")

    def write(self):
        if not self.lines[0]:
            self.lines[0] = f"part {self.name} {program_version(self.dir, self.name)}"
        (self.dir / "Part").write_text("\n".join(self.lines) + "\n", encoding="latin-1")


def program_version(part_dir: Path, name: str) -> str:
    """A part's version from its main program's $VER (PART_MAIN); stops the build when it has none."""
    if name not in PART_MAIN:
        raise SystemExit(f"{name}: no main binary in PART_MAIN to take the version from")
    rel, vername = PART_MAIN[name]
    f = part_dir / rel
    got = binary_version(f.read_bytes(), vername) if f.is_file() else None
    if not got:
        raise SystemExit(f"{name}: {rel} says no version, so the part has none")
    return got


def icon_maker(openamigaprint: Path):
    """gfopenprint's icon writer (classic icons, then OS 3.2-style ones)."""
    spec = importlib.util.spec_from_file_location("oap_make_package", openamigaprint / "tools" / "make_package.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def os32_icons(oap: Path, icons_mod, jobs: list[tuple[Path, str | None]]) -> None:
    """Rewrites classic icons as OS 3.2-style ones (OS 3.5 colour icons with
    the classic picture as fallback), as gfopenprint's package does: art
    from its icon_art, written by ACBuild's amiga-icon.js; None means a drawer
    (Boxie's OS 3.2 drawer, MIT)."""
    import tempfile
    drawer = icons_mod.BOXIE / "os3" / "containers" / "drawer-blue.info"
    if not icons_mod.ACBUILD_ICON_JS.is_file() or not drawer.is_file() or not shutil.which("node"):
        raise RuntimeError("needs node, ACBuild's amiga-icon.js and the Boxie set")
    sys.path.insert(0, str(oap / "tools"))
    import icon_art
    with tempfile.TemporaryDirectory() as tmp:
        spec = []
        for info, art in jobs:
            if art is None:
                spec.append({"info": str(info), "from": str(drawer)})
                continue
            if callable(art):                       # a picture of the pack's own
                rgba = Path(tmp) / f"{art.__name__}.rgba"
                rgba.write_bytes(art())
                spec.append({"info": str(info), "rgba": str(rgba), "w": icon_art.W, "h": icon_art.H})
                continue
            rgba = Path(tmp) / f"{art}.rgba"
            if not rgba.exists():
                rgba.write_bytes(icon_art.draw(art))
            spec.append({"info": str(info), "rgba": str(rgba), "w": icon_art.W, "h": icon_art.H})
        (Path(tmp) / "jobs.json").write_text(json.dumps(spec))
        subprocess.run(["node", str(oap / "tools" / "os32_icons.js"), str(icons_mod.ACBUILD_ICON_JS), str(Path(tmp) / "jobs.json")],
                       check=True, stdout=subprocess.DEVNULL)


OPEN_ART = ROOT / "icons" / "open"     # the Open apps' pictures, 64 x 64 (tools/open_icons_art.py draws them)
ICON_SCALE = 0.85                      # shown at 85 per cent (the approved look)


def open_icons(amigachrome: Path, jobs: list[tuple[Path, str]], drawers: dict[Path, list[Path]]) -> None:
    """The Open apps' own icons: each job's icon keeps what it is (type,
    default tool, tool types, stack) and gets its picture from icons/open as
    a GlowIcon, dithered for a 16-bit screen. Icons in drawers we share with
    the system (Prefs, Utilities, Tools) are left unplaced, so Workbench puts
    them where there is room; `drawers` maps each drawer icon of our own to
    the icons in it, which are laid out on a grid with the window sized to
    them (amicradle's icon_theme, as Cradle's new icon set does).

    Every picture is shown at ICON_SCALE. A drawer of our own is
    drawer-NAME.png: its badge on our own drawer."""
    from PIL import Image
    sys.path.insert(0, str(amigachrome / "scripts"))
    import amiga_icon
    import icon_theme

    def picture(name: str) -> Image.Image:
        return Image.open(OPEN_ART / f"{name}.png").convert("RGBA")

    def frames(name: str) -> list[Image.Image]:
        """NAME.png at rest and NAME-selected.png chosen (ICONS.md)."""
        rest = picture(name)
        chosen = picture(f"{name}-selected") if (OPEN_ART / f"{name}-selected.png").exists() else rest
        if chosen.size != rest.size:
            raise SystemExit(f"{name}-selected.png: {chosen.size}, but {name}.png is {rest.size}")
        return [rest, chosen]

    def hard(im: Image.Image) -> tuple[int, int, bytes]:
        """At ICON_SCALE, every pixel solid or see-through (a GlowIcon)."""
        im = im.resize((max(1, round(im.width * ICON_SCALE)), max(1, round(im.height * ICON_SCALE))), Image.LANCZOS)
        rgba = bytearray(im.tobytes())
        for i in range(0, len(rgba), 4):
            rgba[i:i + 4] = rgba[i:i + 3] + b"\xff" if rgba[i + 3] >= 110 else b"\0\0\0\0"
        return im.width, im.height, bytes(rgba)

    def art(name: str) -> list[tuple[int, int, bytes]]:
        """The icon's two frames, dithered for a 16-bit screen, the glow round the chosen one."""
        (w, h, rest), (_, _, sel) = (hard(f) for f in frames(name))
        return [(w, h, amiga_icon.dither_rgb565(w, h, rest)),
                (w, h, amiga_icon.dither_rgb565(w, h, amiga_icon.glow_selected(w, h, sel)))]

    def meta(icon: dict, x: int, y: int, drawer) -> dict:
        return {"type": icon["type"], "defaultTool": icon.get("defaultTool"), "toolTypes": icon.get("toolTypes") or [],
                "toolWindow": icon.get("toolWindow"), "stack": icon.get("stack") or 4096, "x": x, "y": y, "drawer": drawer}

    none = amiga_icon.NO_POSITION
    pictures = {Path(info): name for info, name in jobs if Path(info).is_file()}   # a part that changed shape keeps its own
    places: dict[Path, tuple[int, int]] = {}
    windows: dict[Path, dict] = {}
    for i, (drawer_info, inside) in enumerate(drawers.items()):
        inside = [f for f in inside if f.is_file()]
        if not Path(drawer_info).is_file():
            continue
        rows = [{"rel": f"d/{f.name}", "folder": "d", "name": f.name[:-5],
                 "family": icon_theme.FAMILY.get(amiga_icon.read_icon(f.read_bytes())["type"], "project")} for f in inside]
        lay = icon_theme.layout(rows, (1920, 1024), 64)
        for f in inside:
            places[f] = lay["icons"][f"d/{f.name}"]
        windows[Path(drawer_info)] = {**lay["windows"]["d"], "left": 120 + 40 * i, "top": 80 + 30 * i}   # stepped, so none hides another
    for info in sorted(set(pictures) | set(places)):
        icon = amiga_icon.read_icon(info.read_bytes())
        x, y = places.get(info, (none, none))
        window = windows.get(info)
        drawer = None
        if icon["type"] in amiga_icon.DRAWER_TYPES:
            drawer = {**(icon.get("drawer") or {}), **(window or {}), "currentX": 0, "currentY": 0}
        if info in pictures:
            info.write_bytes(amiga_icon.write_os35(meta(icon, x, y, drawer), art(pictures[info])))
        else:                                         # not ours to redraw: only placed
            info.write_bytes(icon_theme.place_only(info.read_bytes(), x, y, drawer))


def screenmode_prefs(mode_id: int, w: int, h: int, depth: int) -> bytes:
    """ScreenMode.prefs (IFF PREF: PRHD, SCRM) for Workbench's screen."""
    import struct
    prhd = b"PRHD" + struct.pack(">IBBI", 6, 0, 0, 0)
    scrm = b"SCRM" + struct.pack(">I", 28) + bytes(16) + struct.pack(">IHHHH", mode_id, w, h, depth, 1)
    body = b"PREF" + prhd + scrm
    return b"FORM" + struct.pack(">I", len(body)) + body


def archive_art() -> bytes:
    """GFOpenCompress's icon: an archive box with a green arrow coming out, drawn
    as icon_art draws (four times the size, reduced, hard-edged), 48 x 40 RGBA."""
    from PIL import Image, ImageDraw
    W, H, S = 48, 40, 4
    img = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    r = lambda *v: tuple(int(x * S) for x in v)
    d.rounded_rectangle(r(5, 15, 33, 39), radius=2 * S, fill=(40, 40, 60, 110))                       # shadow
    d.rectangle(r(3, 13, 31, 37), fill=(214, 168, 104, 255), outline=(80, 50, 20, 255), width=S)        # the box
    d.rectangle(r(1, 7, 33, 13), fill=(232, 192, 132, 255), outline=(80, 50, 20, 255), width=S)          # its lid
    d.rectangle(r(12, 18, 22, 21), fill=(80, 50, 20, 255))                                              # the handle
    d.polygon([r(22, 22), r(34, 22), r(34, 16), r(46, 27), r(34, 38), r(34, 32), r(22, 32)],
              fill=(60, 180, 80, 255), outline=(20, 90, 40, 255))                                       # the arrow, out
    img = img.resize((W, H), Image.LANCZOS)
    px = img.load()
    for y in range(H):
        for x in range(W):
            rr, g, b, a = px[x, y]
            px[x, y] = (rr, g, b, 255) if a >= 128 else (0, 0, 0, 0)
    return img.tobytes()


def gf_socket_name(path: str) -> str:
    """path with its file name in gfopensocket's GF form: opensocket.library -> gfopensocket.library,
    OpenSocketControl -> GFOpenSocketControl, C/OpenSocket -> C/GFOpenSocket; other names as they are."""
    head, _, name = path.rpartition("/")
    if name.startswith("opensocket"):
        name = "gf" + name
    elif name.startswith("OpenSocket"):
        name = "GF" + name
    return f"{head}/{name}" if head else name


def late_combine_off(cc) -> list[str]:
    """["-fno-late-combine-instructions"] when cc knows the option, else []. GFOpenAmigaGCC 16.2.0-open3's late-combine
    pass can store a copy one place late (GCC PR 123853): built with it, openuptool.c's manifest_save copied the
    manifest's first line (GFOpenUp, a tab, 0.1) as "move.b (a0)+,(a0,d1.l)", a byte late. Off until GFOpenAmigaGCC
    open4 (gfopengcc #29). The os32 stove's GCC 6.5 refuses the option, and gets none."""
    try:
        r = subprocess.run([str(cc), "-fno-late-combine-instructions", "-S", "-o", "/dev/null", "-x", "c", "/dev/null"],
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except OSError:
        return []
    return ["-fno-late-combine-instructions"] if r.returncode == 0 else []


_STOVE_DEFAULT = re.compile(r'^\s*STOVE=\$\{STOVE:-"?([^"}]+)"?\}', re.M)


def guest_stove(script: Path, env: dict, home: Path | None = None, guest: Path | None = None) -> dict:
    """env for an amicradle-guest script: as given, but when the script's own STOVE default is a stove folder this
    host doesn't have and its name is an open6 one (os32-gcc16-16.2.0-open6), GUEST_STOVE instead, if this host has
    that (Corwin's 14 Nov build: 13 guest scripts stopped on m68k-amigaos-gcc: not found). The default's $STOVES is
    env's STOVES, else $HOME/AmiCradle/stoves, as the scripts read it (Londo, #254). A STOVE already in env stays."""
    if "STOVE" in env:
        return env
    guest = guest or GUEST_STOVE
    m = _STOVE_DEFAULT.search(script.read_text(encoding="latin-1")) if script.is_file() else None
    if not m or not guest.is_dir():
        return env
    h = str(home or Path.home())
    stoves = env.get("STOVES") or f"{h}/AmiCradle/stoves"
    d = Path(m.group(1).replace("$STOVES", stoves).replace("${STOVES}", stoves).replace("$HOME", h))
    if d.is_dir() or "open6" not in d.name:
        return env
    return dict(env, STOVE=str(guest))


def filemanager_env(a, env: dict) -> dict:
    """GFOpenFiles' build.sh: GFOpenGadTools and the guest, and GFOpenCompress, GFOpenFTP and GFOpenMedia by the
    checkouts this build was given, not whatever sits beside gfopenfilemanager (Corwin's 14 Nov build: no MEDIA)."""
    e = dict(env, OGT=str(a.opengadtools), GUEST=str(a.guest), OC=str(a.openamigacompress))
    if getattr(a, "openamigaftp", None):
        e["FTP"] = str(a.openamigaftp)
    if getattr(a, "openamigamedia", None):
        e["MEDIA"] = str(a.openamigamedia)
    return e


# GFOpenCompress's run-time libraries (gfopencompress build-os3.sh since 6 Nov 2026): RAR, 7z, bzip2 and xz, and
# crunched files (PowerPacker, Imploder, XPK...). GFOpenCompress's part owns them; GFOpenFiles opens the same ones.
COMPRESS_LIBS = ("gfopencompress_archive.library", "gfopencompress_crunch.library")


def compress_part(top: Path, oc_dir: Path, icons) -> "Part":
    """Parts/GFOpenCompress: the program in SYS:Utilities and its two libraries in LIBS:; the build stops when one
    of the three wasn't built."""
    b = oc_dir / "build/os3"
    oc = Part(top, "GFOpenCompress", None, "GFOpenCompress: archives and disk images, from its window or the Shell")
    for f in ("GFOpenCompress",) + COMPRESS_LIBS:
        if not (b / f).is_file():
            raise SystemExit(f"GFOpenCompress: {b / f} wasn't built (gfopencompress build-os3.sh)")
    oc.add("Utilities/GFOpenCompress", (b / "GFOpenCompress").read_bytes(), "SYS:Utilities/GFOpenCompress")
    oc.add("Utilities/GFOpenCompress.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/GFOpenCompress.info")
    for lib in COMPRESS_LIBS:
        oc.add(f"Libs/{lib}", (b / lib).read_bytes(), f"LIBS:{lib}", own=True)
    oc.write()
    return oc


def build_parts(a):
    """Each part's own build, into its own build folder."""
    env = dict(os.environ, AMIGA_CC=str(CC), PATH=f"{CC.parent}:{os.environ['PATH']}")
    late = late_combine_off(CC)   # for this file's own compiles below; the parts' scripts have their own probe
    (ROOT / "build/os3").mkdir(parents=True, exist_ok=True)   # a fresh checkout has no build folder
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-std=gnu99", "-Wall", "-Werror", "-O2", NULL_OK, *late, "-fno-common",
         "-o", ROOT / "build/os3/GFOpenUpTool", ROOT / "src/openuptool.c"], env=env)
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-std=gnu99", "-Wall", "-Werror", "-O2", NULL_OK, *late, "-fno-common",
         "-o", ROOT / "build/os3/GFOpenUpMenu", ROOT / "src/openupmenu.c"], env=env)
    # GFOpenMonitor 1.0, the task manager (src/openmonitor/, its own build.sh, GCC 16 as gfopengadtools wants),
    # built resident (libnix -resident: each run its own variables), so one seglist serves every run. The open
    # ROM builds the same file through amicradle-guest's romprefs.sh.
    gcc16_root = os.environ.get("OS32_GCC16", str(Path.home() / "AmiCradle/stoves/os32-gcc16"))
    run(["sh", ROOT / "src/openmonitor/build.sh", ROOT / "build/os3/GFOpenMonitor"],
        env=dict(os.environ, OS32_GCC16=gcc16_root, OGT=str(a.opengadtools), GUEST=str(a.guest)))
    # GFOpenSnap: its PNGs through openpng.datatype's OIM_ENCODEPNG, whose header is gfopendatatypes'
    run(["sh", ROOT / "src/opensnap/build.sh", ROOT / "build/os3/GFOpenSnap"],
        env=dict(env, STOVE=str(STOVE), ODT=str(a.opendatatypes)))
    run(["sh", a.opengadtools / "look/build.sh"], env=env)
    run(["sh", a.opengadtools / "workbench/build.sh"], env=env)   # GFOpenDrawer
    run([sys.executable, a.opengadtools / "tools/make_backdrops.py"], env=env)
    g = a.guest
    out = g / "build/guest/os32"
    run(["sh", g / "devices/accontrol.device/build.sh", out], env=guest_stove(g / "devices/accontrol.device/build.sh", env))
    inc = g / "devices/accontrol.device/include"
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-O2", NULL_OK, *late, "-Wall", f"-I{inc}", g / "tools/acclip/acclip.c", "-o", out / "ACClip"], env=env)
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-O2", NULL_OK, *late, "-Wall", f"-I{inc}", f"-I{g / 'common/amiga'}",
         g / "tools/acdrop/acdrop.c", g / "common/amiga/ac_stack.c", "-o", out / "ACDrop"], env=env)
    run(["sh", g / "libraries/acrtg.gpu/build.sh", out], env=guest_stove(g / "libraries/acrtg.gpu/build.sh", env))        # GFOpenGPU's driver for the ACRTG board
    # gfopengfx.library 1.2: the stub that sends gfopengfx.library's calls to gfopengpu.library 0.6 (GCC 16)
    run(["sh", g / "libraries/opengfx/build-amiga.sh"], env=dict(os.environ, STOVE=gcc16_root))
    run(["sh", a.openamigaprefs / "OpenTypes/build.sh"], env=env)
    # GFOpenPrefs: every editor of ours as a page of one program (W48). Its build also makes the small body and the old
    # names' stubs, which GFOpenUp no longer installs (the no-ROM-volume design, section 8)
    run(["sh", a.openamigaprefs / "OpenPrefs/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Look/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Windows/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Blanker/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Dock/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Setup/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Menus/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Menus/engine/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "TitleBar/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Network/build.sh"], env=dict(env, OGT=str(a.opengadtools)))   # GFOpenNetwork
    run(["sh", a.openamigaprefs / "DateTime/build.sh"], env=env)     # GFOpenDateTime
    run(["sh", a.openamigaprefs / "Sound/build.sh"], env=env)
    # acaudio.audio, the AHI driver for the ACAHI board, and its modes file (amicradle's vendored guest tree)
    run(["sh", a.amigachrome / "vendor/amigachrome-guest/devices/acaudio.audio/build.sh", ROOT / "build/acaudio"],
        env=dict(env, STOVE=str(STOVE)))
    # ahi.device, GFOpenAHI's own (MIT; amicradle #388): the other half of AHI on the ACAHI board
    run(["sh", a.amigachrome / "vendor/amigachrome-guest/devices/ahi.device/build.sh", ROOT / "build/ahidevice"],
        env=dict(env, STOVE=str(STOVE)))
    # GFOpenSocket from its own repository's main, with its own scripts and the os32 stove (as the vendored
    # payload's manifest says it was built), httpget with GFOpenTLS's headers (its own copy). GFOpenUp 0.7.5 took the
    # vendored payload, last refreshed on 4 October, so it shipped GFOpenSocketControl 1.0 after 1.0.1 and
    # 1.0.2 (the addresses' byte order) were merged in gfopensocket.
    if a.openamigasocket:
        so_out = ROOT / "build/opensocket"
        run(["sh", a.openamigasocket / "guest/network/acnet/build.sh", so_out], env=dict(env, STOVE=str(STOVE)))
        run(["sh", a.openamigasocket / "tools/actcptools/build.sh", so_out], env=dict(env, STOVE=str(STOVE)))
    run(["sh", a.openamigacompress / "build-os3.sh"], env=env)
    run(["sh", a.openamigaservice / "build.sh"], env=dict(os.environ))   # its own compiler (os32-gcc16)
    run(["sh", a.opendatatypes / "Datatypes/build.sh"], env=dict(os.environ))   # os32-gcc16 too
    run(["sh", a.openamigartg / "library/build.sh"], env=env)   # also Warp3D and, with the GCC 16 stove, SDL2.module
    run(["sh", a.openamigartg / "tools/build.sh"], env=env)     # GFOpenGPUCheck
    # ogles3.library on GFOpenGPU and OGLES3Check (gfopenrtg #190), with the GCC 16 open6 stove its script asks for
    run(["sh", a.openamigartg / "library/ogles3/build-amiga.sh", a.openamigartg / "build/ogles3"],
        env=dict(os.environ, STOVE=os.environ.get("OS32_GCC16_OPEN6", str(Path.home() / "AmiCradle/stoves/os32-gcc16-open6"))))
    # GL.module (Mesa) with the GCC 16 stove, which its script puts first on PATH; then GFOpenDemos on it
    gl_out = a.openamigartg / "build/gl"
    run(["sh", a.openamigartg / "library/modules/gl/module/build.sh", gl_out], env=dict(os.environ))
    # OpenAL.module (gfopenrtg library/modules/openal: OpenAL 1.1 on AHI, or ACAHI's voices) with the GCC 16 stove,
    # named here: the script takes STOVE as the prefix, and this script's own STOVE is the os32 stove's root. For the
    # 68040 or 68060 with its FPU, like SDL2.module and GL.module (the part's CPU world). make_sdk.py finds
    # build/openal and puts AL/, libopenal.a and openal.pc in the kit
    run(["sh", a.openamigartg / "library/modules/openal/build.sh", a.openamigartg / "build/openal"],
        env=dict(os.environ, STOVE=str(Path(gcc16_root) / "prefix")))
    if a.openamigamesa:                     # the GCC 16 stove first on PATH, as demos/README.md says
        stove16 = Path(os.environ.get("STOVE16", Path.home() / "AmiCradle/stoves/os32-gcc16/prefix"))
        run(["sh", a.openamigamesa / "demos/build.sh"],
            env=dict(os.environ, RTG=str(a.openamigartg), CROSS="mesa/cross/m68k-amigaos.ini", GL_OUT=str(gl_out),
                     PATH=f"{stove16 / 'bin'}:{os.environ['PATH']}"))
    # GFOpenFiles' zlib is our openzlib, from the guest checkout (GUEST), not a libz.a beside it
    run(["sh", a.openamigafilemanager / "build.sh"], env=filemanager_env(a, env))
    run(["sh", a.openamigamediaplayer / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    # GFOpenMUI: the clean-room muimaster.library (gfopenmui muimaster/build-amiga.sh, GCC 16 open6, on GFOpenLayout
    # library 18 at the commit its script pins: GFOPENLAYOUT names the script's own deps folder, so it fetches that one)
    mui_out = a.openamigamui / "muimaster/build-amiga"
    run(["sh", a.openamigamui / "muimaster/build-amiga.sh"], env=gfopenmui_build_env(mui_out, os.environ))
    run(["sh", a.openamigaedit / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    # The GFOffice Suite (gfoffice) carries its own GFOpenGadTools copy and needs GCC 16 (its build-os3.sh stops on
    # the os32 stove's 6.5); GFOpenWrite's pages are GFOpenLayout's. Built on its four libraries (GFO_LIBRARY,
    # OWF=library: gfoffice docs/suite/owf-libraries.md), each program on its own, OWConvert on all four
    # (gfoffice_parts). Deflate is z.library's (openzlib, below), so there is no zlib source in the build
    gfo_env = dict(env, OS32_GCC16=gcc16_root, OPENLAYOUT_SRC=str(a.openamigalayout))
    run(["sh", a.gfoffice / "build-os3.sh", a.gfoffice / GFO_LIBRARY], env=dict(gfo_env, OWF="library"))
    # GFOpenPresentation has no library build yet (build-os3.sh makes it with libowf linked in only), so the suite
    # is built a second time, OWF=linked, for it alone (gfoffice_presentation_part)
    run(["sh", a.gfoffice / "build-os3.sh", a.gfoffice / GFO_LINKED], env=dict(gfo_env, OWF="linked"))
    # GFOpenMail: GCC 16 (its build-os3.sh's own default), TLS through GFOpenTLS, whose headers it carries
    run(["sh", a.openamigamail / "build-os3.sh"], env=dict(env, STOVE=gcc16_root))
    # gfopenmulticore.library with the os32 stove, as its build.sh says (built with
    # os32-gcc16 it passes jobs the wrong arguments, 8 October 2026)
    omc_env = {k: v for k, v in env.items() if k not in ("CC", "AMIGA_CC")}
    run(["sh", a.openamigamulticore / "library/build.sh"],
        env=dict(omc_env, STOVE=str(STOVE), OPENSERVICE=str(a.openamigaservice)))
    # GFOpenCrypto and GFOpenPuTTY (GCC 16), GFOpenPuTTY linked with this GFOpenCrypto's headers
    if a.openamigatls:
        run(["sh", a.openamigatls / "build-amiga.sh"],
            env=dict(os.environ, STOVE=gcc16_root, FPCR_CHECK=str(a.openamigartg / "tools/fpcr_check.py")))
        # GFOpenTLS: gfopentls.library (GCC 16), and its CA bundle from this
        # machine's ca-certificates package (Mozilla's list, MPL-2.0; only the
        # package's own files: make_ca_bundle.py refuses anything else)
        if (a.openamigatls / "build-amiga-tls.sh").is_file():
            run(["sh", a.openamigatls / "build-amiga-tls.sh"],
                env=dict(os.environ, STOVE=gcc16_root, FPCR_CHECK=str(a.openamigartg / "tools/fpcr_check.py")))
            run([sys.executable, a.openamigatls / "tools/make_ca_bundle.py", a.openamigatls / "build-ca-bundle"])
        if a.openamigaputty:
            run(["sh", a.openamigaputty / "build.sh"], env=dict(os.environ, OS32_GCC16=gcc16_root, CPU="040",
                                                                OPENCRYPTO=str(a.openamigatls)))
            run([sys.executable, a.openamigaputty / "tools/make_icons.py", a.amigachrome / "scripts/amiga_icon.py",
                 a.openamigaputty / "build/icons"])
    # Tata: its own build.sh (os32 stove; its window with this GFOpenGadTools' lib/)
    if a.openamigatata:
        run(["sh", a.openamigatata / "build.sh"], env=dict(os.environ, STOVE=str(STOVE), OGT=str(a.opengadtools)))
    # GFOpenFTP: GCC 16 and GFOpenGadTools' lib/. FTPS works through GFOpenTLS, whose
    # headers come with gfopenftp.
    if a.openamigaftp:
        run(["sh", a.openamigaftp / "build-os3.sh"], env=dict(os.environ, OS32_GCC16=gcc16_root, OGT=str(a.opengadtools),
                                                             FPCR_CHECK=str(a.openamigartg / "tools/fpcr_check.py")))
    # Unofficial Telegram for Amiga (Open): the Amiga program (os32 stove)
    if a.openamigatelegram:
        run(["sh", a.openamigatelegram / "build-os3.sh"], env=dict(os.environ, STOVE=gcc16_root, OS32_GCC16=gcc16_root))
    # The GFOpenGPU developer files: gfopenrtg's kit, from this build's libraries
    # GFOpenInput (gfopeninput): gfopeninput.library and OIList (os32 stove), and
    # GFOpenPrefs Gamepads, its editor (gfopenprefs, with GFOpenInput's headers)
    if a.openamigainput:
        run(["sh", a.openamigainput / "library/build.sh", a.openamigainput / "build"], env=dict(env, STOVE=str(STOVE)))
        run(["sh", a.openamigaprefs / "Gamepads/build.sh"], env=dict(env, STOVE=str(STOVE)))
    # GFOpenLibs (0.8.0): the open libraries the Installer offers one switch each, off by default
    # (amicradle-guest libraries/OPEN_LIBRARIES.md, section 3), each built with its own script as written:
    # utility.module and the IEEE maths (amicradle-guest, their default os32-gcc16 stove), and input.device 50.5
    # (gfopeninput device/build.sh, os32 stove). The ROM ones load with OS 3.2's own C:LoadModule (OPENLIBS_STARTUP).
    # Their own stove: STOVE here is the os32 one, so it is left out of their environment.
    ol = ROOT / "build/openlibs"
    g = a.guest / "libraries"
    genv = {k: v for k, v in os.environ.items() if k not in ("STOVE", "CC")}
    run(["sh", g / "openutility/build-amiga.sh", "module"], env=guest_stove(g / "openutility/build-amiga.sh", dict(genv, OUT=str(ol / "openutility"))))
    run(["sh", g / "openmathieee/build-amiga.sh"], env=guest_stove(g / "openmathieee/build-amiga.sh", dict(genv, OUT=str(ol / "openmathieee"))))
    # OpenRexx (amicradle-guest libraries/openrexx, its default os32-gcc16 stove): rexxsyslib.library,
    # rexxsupport.library and RexxMast
    run(["sh", g / "openrexx/build-amiga.sh"], env=guest_stove(g / "openrexx/build-amiga.sh", dict(genv, OUT=str(ol / "openrexx"))))
    # z.library 53.15 (amicradle-guest libraries/openzlib, GCC 16; the Aminet jump table since #632; since #643, 0.8.23,
    # CRC-32, Adler-32 and its copies and clears through AC090's helpers, plain 68k code on any other machine; since
    # #677/#690, 53.15, a block's codes and a whole-input deflate as hostlib calls, ZLIB_MIN_FOR_PHYSFS): the
    # GFOffice part's (and GFOpenImage's). GFOpenCompress 0.3.3 and the datatypes' zlibrary.c refuse 53.12, so they ship with it.
    # libowf opens it when a document first needs deflate (saving, PNG pictures in PDFs) or inflate (faster than
    # libowf's own inflater, which reads zip packages without it)
    run(["sh", g / "openzlib/build-amiga.sh"], env=dict(genv, STOVE=gcc16_root, OUT=str(ROOT / "build/openzlib")))
    # physfs.library 1.1 (gfopenphysfs at GFOPENPHYSFS's commit, GCC 16): the GFOpenGPU part's, with that z.library
    # (53.15 or newer, ZLIB_MIN_FOR_PHYSFS);
    # its SDK files go in the developer kit (make_sdk.py --physfs)
    if a.openamigaphysfs:
        build_physfs(a.openamigaphysfs, ROOT / "build/physfs", gcc16_root)
    # softfpu.library (amicradle-guest libraries/opensoftfpu, its default os32-gcc16 stove): the GFOpenUp part's
    run(["sh", g / "opensoftfpu/build-amiga.sh"], env=guest_stove(g / "opensoftfpu/build-amiga.sh", dict(genv, OUT=str(ROOT / "build/opensoftfpu"))))
    # The disk libraries and devices the open ROM leaves to a disk (the OpenKick boot disk's, gfopenkick
    # disks/openkick-boot/manifest.json): the GFOpenUp part's, on the GFOpenKick ROM, where none is (OPENKICK_DISK)
    for name, (src, _out, _dest) in OPENKICK_DISK.items():
        run(["sh", a.guest / src / "build-amiga.sh"], env=guest_stove(a.guest / src / "build-amiga.sh", dict(genv, OUT=str(ROOT / "build/openkickdisk" / name))))
    # GFOpenReaction's classes GFOS lacks (OPENKICK_CLASSES: arexx.class), each with its own script, the same way
    for name, (src, _out, _dest) in OPENKICK_CLASSES.items():
        run(["sh", a.guest / src / "build-amiga.sh"], env=guest_stove(a.guest / src / "build-amiga.sh", dict(genv, OUT=str(ROOT / "build/openkickclasses" / name))))
    # ACROMFix (amicradle-guest tools/acromfix, the os32 stove as its build.sh): the GFOpenUp part's
    run(["sh", a.guest / "tools/acromfix/build-amiga.sh"], env=dict(genv, STOVE=str(STOVE), OUT=str(ROOT / "build/acromfix")))
    # HostClock (amicradle-guest resources/openbattclock.resource built -DOBC_INSTALL, its default os32-gcc16 stove):
    # the HostClock part's
    run(["sh", a.guest / "resources/openbattclock.resource/build-amiga.sh"], env=guest_stove(a.guest / "resources/openbattclock.resource/build-amiga.sh", dict(genv, OUT=str(ROOT / "build/hostclock"))))
    # printer.device (amicradle-guest devices/openprinter.device, its default os32-gcc16 stove): the GFOpenPrinter part's
    run(["sh", a.guest / "devices/openprinter.device/build-amiga.sh"], env=guest_stove(a.guest / "devices/openprinter.device/build-amiga.sh", dict(genv, OUT=str(ROOT / "build/openprinter"))))
    if a.openamigainput:
        run(["sh", a.openamigainput / "device/build.sh", ol / "input"], env=dict(env, STOVE=str(STOVE)))
    if a.opengpu_sdk:
        # make_sdk.py won't write over a kit: a rebuild's old one goes to archive/ beside it
        for old in Path(a.opengpu_sdk).glob("GFOpenGPU-SDK-*/"):
            arch = Path(a.opengpu_sdk) / "archive"
            arch.mkdir(parents=True, exist_ok=True)
            old.rename(arch / f"{old.name}-{time.strftime('%Y%m%dT%H%M%S')}")
        # 0.7 (0.8.22): libGL.a and gla_core.h from gfopenrtg #195 (gla_last_why); make_sdk.py's default is still 0.6
        # physfs.library's SDK (0.8.28) when it was built and make_sdk.py takes it (gfopenrtg's --physfs)
        physfs_sdk = []
        if (ROOT / "build/physfs/COMMIT").is_file() and "--physfs" in (a.openamigartg / "tools/make_sdk.py").read_text():
            physfs_sdk = ["--physfs", ROOT / "build/physfs"]
        run([sys.executable, a.openamigartg / "tools/make_sdk.py", "--build", a.openamigartg / "build",
             "--gl", gl_out, *physfs_sdk, "--version", "0.7", a.opengpu_sdk], env=dict(os.environ))
    # GFOpenBench (gfopenbench desktop/build.sh, GCC 16 since gfopenbench #31: it stops when given the
    # os32 stove's GCC 6.5): the desktop with the dock and the Open editors built in, from this build's
    # gfopengadtools, gfopenprefs, gfopentata and gfopenfilemanager (its drag and drop)
    if a.openamigabench:
        bench_env = dict(env, STOVE=gcc16_root, OGT=str(a.opengadtools), OAP=str(a.openamigaprefs),
                         FILES=str(a.openamigafilemanager))
        if a.openamigatata:
            bench_env["TATA"] = str(a.openamigatata)
        run(["sh", a.openamigabench / "desktop/build.sh"], env=bench_env)
    # GFOpenPrint, GFOpenPrint's browser and GFOpenView with GCC 16 (GFOpenNDK's os32 libamiga.a, for GCC 6.5,
    # has no SetRexxVar, which GFOpenView calls); IPPS through GFOpenTLS, whose headers gfopenprint carries
    p = a.openamigaprint
    cc16 = Path(gcc16_root) / "prefix/bin/m68k-amigaos-gcc"
    print_env = dict(os.environ, AMIGA_CC=str(cc16), PATH=f"{cc16.parent}:{os.environ['PATH']}")
    for script in ("build-amiga.sh", "build-browser.sh", "build-viewer.sh"):
        run(["sh", p / script], cwd=p, env=print_env)
    run([sys.executable, p / "tools/make_package.py", "--out", p / "build/package"], cwd=p, env=print_env)


def files_art() -> bytes:
    """GFOpenFiles' icon: a window with two panes of files, a drawer and pages
    in them, drawn as archive_art is, 48 x 40 RGBA."""
    from PIL import Image, ImageDraw
    W, H, S = 48, 40, 4
    img = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    r = lambda *v: tuple(int(x * S) for x in v)
    d.rounded_rectangle(r(4, 6, 46, 38), radius=2 * S, fill=(40, 40, 60, 110))                       # shadow
    d.rectangle(r(2, 3, 44, 35), fill=(238, 240, 246, 255), outline=(48, 48, 64, 255), width=S)        # the window
    d.rectangle(r(2, 3, 44, 8), fill=(40, 90, 200, 255), outline=(48, 48, 64, 255), width=S)           # its title bar
    d.line(r(23, 8, 23, 35), fill=(48, 48, 64, 255), width=S)                                          # between the panes
    for x in (5, 26):
        d.rectangle(r(x, 11, x + 6, 16), fill=(232, 192, 100, 255), outline=(110, 80, 20, 255))         # a drawer
        d.rectangle(r(x, 10, x + 3, 11), fill=(232, 192, 100, 255))
        for i, y in enumerate((20, 24, 28, 32)):
            d.line(r(x, y, x + 14 - (4 if i % 2 else 0), y), fill=(90, 90, 110, 255), width=S)          # files
    d.rectangle(r(26, 23, 41, 25), fill=(40, 90, 200, 255))                                           # the chosen one
    img = img.resize((W, H), Image.LANCZOS)
    px = img.load()
    for y in range(H):
        for x in range(W):
            rr, g, b, a = px[x, y]
            px[x, y] = (rr, g, b, 255) if a >= 128 else (0, 0, 0, 0)
    return img.tobytes()


def play_art() -> bytes:
    """GFOpenPlay's icon: a film frame with a green play button on it, drawn
    as files_art is, 48 x 40 RGBA."""
    from PIL import Image, ImageDraw
    W, H, S = 48, 40, 4
    img = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    r = lambda *v: tuple(int(x * S) for x in v)
    d.rounded_rectangle(r(4, 6, 46, 38), radius=3 * S, fill=(40, 40, 60, 110))                       # shadow
    d.rounded_rectangle(r(2, 3, 44, 35), radius=3 * S, fill=(70, 120, 210, 255), outline=(20, 50, 110, 255), width=S)   # the screen
    for x in range(5, 44, 6):
        d.rectangle(r(x, 4, x + 3, 6), fill=(230, 236, 246, 255))                                      # film holes
        d.rectangle(r(x, 32, x + 3, 34), fill=(230, 236, 246, 255))
    d.ellipse(r(14, 9, 32, 27), fill=(70, 160, 50, 255), outline=(30, 80, 20, 255), width=S)           # the play button
    d.polygon([r(20, 13)[0:2], r(28, 18)[0:2], r(20, 23)[0:2]], fill=(255, 255, 255, 255))
    img = img.resize((W, H), Image.LANCZOS)
    px = img.load()
    for y in range(H):
        for x in range(W):
            rr, g, b, a = px[x, y]
            px[x, y] = (rr, g, b, 255) if a >= 128 else (0, 0, 0, 0)
    return img.tobytes()


def binary_version(data: bytes, name: str) -> str | None:
    """The version a program's $VER (or a library's or device's ID string)
    gives for name: "$VER: GFOpenDock 0.4 (...)" or "gfopengpu.library 0.9 (...)"."""
    n = re.escape(name.encode("latin-1"))
    idstr = rb" ([0-9]+\.[0-9][0-9.]*) \("
    m = (re.search(rb"\$VER: ?" + n + rb" ([0-9][0-9.]*[0-9](?:-rc[0-9]+)?)", data) or re.search(rb"(?<![A-Za-z0-9_.])" + n + idstr, data)
         # an ID string right after another constant's last byte (gfopeninput.library 1.3 in Corwin's 14 Nov
         # build had a letter before it): taken when no clean one is there
         or re.search(n + idstr, data))
    return m.group(1).decode() if m else None


# The oldest minigl.library the Warp3D part takes: 30.0, the virgl route (gfopenrtg #230)
MINIGL_MIN = (30, 0)


# The old editors' names: each was a stub in SYS:Prefs that opened its section of GFOpenPrefs (W48), with an icon
# from 21 Oct 2026. Since the no-ROM-volume design (24 Oct 2026, section 10) GFOpenUp archives them all, with their
# icons: the 13 old ones, then the 9 GFOpenUp added on 21 October.
OPENPREFS_NAMES = ("Look", "OpenLook", "Windows", "TitleBar", "Menus", "OpenDock", "OpenSound", "OpenInput",
                   "OpenGamepads", "OpenDateTime", "OpenSerial", "OpenTypes", "OpenSystem",
                   # a name for each section no editor had (gfopenprefs, 21 Oct 2026)
                   "OpenScreenMode", "OpenPointer", "OpenFonts", "OpenColours", "OpenBackdrops", "OpenWorkbench",
                   "OpenFileRequesters", "OpenBlanker", "OpenIControl")

# GFOpenPrefs's home since the no-ROM-volume design (24 Oct 2026, section 8): a drawer of its own
OPENPREFS_HOME = "SYS:Preferences/GFOpenPrefs"

# GFOpenPrefs's pages by their plain names (the user's, design section 8, Q9), each a project icon in
# SYS:Preferences with no file behind it (Q8): GFOpenPrefs opens the page its icon names, or its SECTION= tool type
# (gfopenprefs, P6). Each with GFOpenPrefs's SECTION word and its picture in icons/open.
PREFERENCES_PAGES = (("Theme", "THEME", "Look"), ("Screen Mode", "SCREENMODE", "ScreenMode"),
                     ("Pointer", "POINTER", "Pointer"), ("Fonts", "FONTS", "Fonts"), ("Colours", "COLOURS", "Colours"),
                     ("Backdrops", "BACKDROPS", "Backdrops"), ("Workbench", "WORKBENCH", "Workbench"),
                     ("Icons", "ICONS", "Icons"),        # GFOpenPrefs 2.3's page (gfopenprefs #137)
                     ("File Requesters", "FILEREQUESTERS", "FileRequesters"), ("Windows", "WINDOWS", "Windows"),
                     ("Title Bar", "TITLEBAR", "TitleBar"), ("Menus", "MENUS", "Menus"),
                     ("IControl", "ICONTROL", "IControl"), ("Dock", "DOCK", "Dock"), ("Sound", "SOUND", "Sound"),
                     ("Input", "INPUT", "MouseKeys"), ("Gamepads", "GAMEPADS", "Gamepads"),
                     ("Date & Time", "DATETIME", "DateTime"), ("Serial", "SERIAL", "Serial"),
                     ("Printing", "PRINTING", "Printing"),   # GFOpenPrefs 2.4's page (gfopenprefs #144)
                     ("File Types", "TYPES", "GFOpenTypes"), ("System", "SYSTEM", "System"),
                     ("Blanker", "BLANKER", "Blanker"))


def openprefs_part(top: Path, opapp: bytes, icons, version: str, themes=(), profiles=()) -> "Part":
    """The GFOpenPrefs part (the no-ROM-volume design of 24 October 2026, sections 8 and 10).

    GFOpenPrefs (W48, the user, 17 Oct 2026: "We should just combine them into a single app, prefs editors that is")
    has every editor of ours as a page of one program. Its home is a drawer of its own, SYS:Preferences: the
    program, its icon, and a project icon per page by its plain name (Q8, Q9), whose SECTION= tool type opens
    that page; SYS:Preferences goes on the Shell's path. SYS:Prefs keeps Env-Archive, Presets, Classic, GFOpenTata
    and GFOpenUp-Setup (Q13).

    Archived ("retire": SYS:Storage/GFOpenUp-Superseded, and UNINSTALL puts them back): the old editors' names'
    stubs in SYS:Prefs and their icons, SYS:Prefs/OpenPrefs and its icon, and C:OpenPrefs, the small body, which
    is no longer built (it looked for the program in SYS:Prefs and ROM:Prefs only, so it can't stay first on
    the path).

    The boot's GFOpenPrefs APPLY (Sound's levels, Gamepads' patch, the stack caps) stays only where the ROM doesn't
    apply them itself: on the open ROM whose prefs task does (prefs.resource 47.101, "GFOpenKick" to GFOpenUpTool
    0.8.9) the line goes; elsewhere (OS 3.2.3's ROM, Q12, and an older open ROM) it runs the program by its
    full path.

    The pieces its pages list (4 November 2026, Instance-40: a new install on the open ROM had none, and the Theme
    page's lists were empty): the themes (gfopengadtools themes/*.theme) in SYS:Prefs/Presets/Themes and the desktop
    profiles (gfopenprefs Look/profiles) in SYS:Prefs/Presets/GFOpenPrefs. GFOpenUp 0.8.2 dropped them with the
    GFOpenLook part; they are only the choices the Theme page offers, and nothing is applied until the user picks
    one, so Workbench keeps the OS's own look. "add": a user's own theme beside them stays."""
    opp = Part(top, "GFOpenPrefs", version, "GFOpenPrefs: every Open editor in one window, in SYS:Preferences with an icon per page")
    opp.add("Preferences.info", icons.icon(icons.WBDRAWER, "drawer", window=(520, 300)), "SYS:Preferences.info", own=True)
    opp.add("Preferences/GFOpenPrefs", opapp, OPENPREFS_HOME)
    opp.add("Preferences/GFOpenPrefs.info", icons.icon(icons.WBTOOL, "page", stack=32768), OPENPREFS_HOME + ".info")
    for page, word, _ in PREFERENCES_PAGES:
        opp.add(f"Preferences/{page}.info", icons.icon(icons.WBPROJECT, "page", OPENPREFS_HOME, [f"SECTION={word}"], stack=32768),
                f"SYS:Preferences/{page}.info")
    for n in OPENPREFS_NAMES:
        opp.line(f'retire "SYS:Prefs/{n}"')
        opp.line(f'retire "SYS:Prefs/{n}.info"')
    opp.line('retire "SYS:Prefs/OpenPrefs"')
    opp.line('retire "SYS:Prefs/OpenPrefs.info"')
    opp.line('retire "SYS:C/OpenPrefs"')
    for name, data in themes:
        opp.add(f"Themes/{name}", data, f"SYS:Prefs/Presets/Themes/{name}", own=True)
    for name, data in profiles:
        opp.add(f"Profiles/{name}", data, f"SYS:Prefs/Presets/GFOpenPrefs/{name}", own=True)
    opp.line("startup Path >NIL: SYS:Preferences ADD")
    opp.line(f"unless GFOpenKick startup {OPENPREFS_HOME} APPLY")
    return opp


# GFOpenSnap's settings, its icon's tool types (src/opensnap/opensnap.c says what each does)
OPENSNAP_TOOLTYPES = ["DONOTWAIT", "CX_PRIORITY=0", "SCREENKEY=shift rcommand s", "WINDOWKEY=shift rcommand w",
                      "DRAWER=SYS:Snapshots", "CLIPBOARD=YES"]


def opensnap_part(top: Path, program: bytes, icons) -> "Part":
    """GFOpenSnap (24 October 2026), on every install: the commodity in SYS:Tools/Commodities, started at boot,
    with its keys as its icon's tool types. The icon goes only where none is ("default"), so keys a user set
    stay through an upgrade; the program is replaced. SYS:Snapshots is made by GFOpenSnap, with its drawer icon,
    the first time it saves."""
    sn = Part(top, "GFOpenSnap", binary_version(program, "GFOpenSnap") or "?",
              "GFOpenSnap: Shift+Right Amiga+S takes the screen, +W the active window, as PNGs in SYS:Snapshots and on "
              "the clipboard")
    sn.add("Tools/Commodities/GFOpenSnap", program, "SYS:Tools/Commodities/GFOpenSnap")
    sn.add("Tools/Commodities/GFOpenSnap.info", icons.icon(icons.WBTOOL, "picture", None, OPENSNAP_TOOLTYPES, stack=32768),
           "SYS:Tools/Commodities/GFOpenSnap.info", default=True)
    sn.line("startup Run >NIL: SYS:Tools/Commodities/GFOpenSnap")
    return sn


def acromfix_lines(core: "Part", acromfix: bytes) -> None:
    """ACROMFix (amicradle-guest tools/acromfix): SetPatch for the open ROM. It brings a GFOpenKick ROM it knows up
    to the fixes merged after that ROM was built, until Team 4's next ROM build has them (on 25 October 2026:
    #516, a boot's Workbench in its ScreenMode prefs, and #518, system requesters measured in their font, for
    Instance-32's full14b; #518 for the standard full15). On OS 3.2.3's Kickstart, an open ROM it doesn't know and
    one with every fix it changes nothing, so the file and its line go in on every ROM: an install can move to
    another ROM. QUIET: no boot window; its STATUS says what it did."""
    core.add("C/ACROMFix", acromfix, "SYS:C/ACROMFix")
    core.line("startup C:ACROMFix QUIET")


# What the GFOpenKick ROM leaves to a disk and our programs open (4 November 2026: Instance-40, the open ROM with no OS
# disk, had none of these, and GFOpenPrefs's Screen Mode, Pointer, Fonts, Colours, Backdrops, Workbench, File
# Requesters, IControl and Printing ended for want of iffparse.library; GFOpenEdit, GFOpenBench, GFOpenFiles and
# GFOpenMonitor open diskfont.library, GFOpenMenus, GFOpenSpeaker, GFOpenBlanker and GFOpenWindows commodities.library,
# SDL2.module lowlevel.library, ACClip clipboard.device, printer.device serial and parallel.device). The same files as
# the OpenKick boot disk's (gfopenkick disks/openkick-boot/manifest.json), from amicradle-guest:
#   name: (source drawer, the file in its build's OUT, where it goes)
OPENKICK_DISK = {
    "iffparse": ("libraries/openiffparse", "libs/iffparse.library", "LIBS:iffparse.library"),
    "diskfont": ("libraries/opendiskfont", "libs/diskfont.library", "LIBS:diskfont.library"),
    "commodities": ("libraries/opencommodities", "libs/commodities.library", "LIBS:commodities.library"),
    "lowlevel": ("libraries/openlowlevel", "libs/lowlevel.library", "LIBS:lowlevel.library"),
    "clipboard": ("devices/openclipboard.device", "devs/clipboard.device", "DEVS:clipboard.device"),
    "serial": ("devices/openserial.device", "devs/serial.device", "DEVS:serial.device"),
    "parallel": ("devices/openparallel.device", "devs/parallel.device", "DEVS:parallel.device"),
}


def openkick_disk_lines(core: "Part", built: Path) -> None:
    """The GFOpenUp part, on the GFOpenKick ROM: the disk libraries and devices the ROM leaves to a disk
    (OPENKICK_DISK), each only where none is ("default"), so OS 3.2.3's own on an OS 3.2.3 disk, and a newer one
    the OpenKick boot disk's Install-OpenKick put there, stay. Under OS 3.2.3's own ROM its disk has them."""
    for name, (src, out, dest) in OPENKICK_DISK.items():
        f = built / name / out
        if not f.is_file():
            raise SystemExit(f"{f} wasn't built (amicradle-guest {src}/build-amiga.sh)")
        core.add(f"OpenKick/{out}", f.read_bytes(), dest, default=True, cond="when GFOpenKick")


# The BOOPSI classes GFOS has no copy of, which OS 3.2's ReAction puts in SYS:Classes (Dale, 10 Nov 2026: GFOS had
# no SYS:Classes/arexx.class, so no program's ARexx port opened there). Ours, GFOpenReaction from amicradle-guest
# libraries/openreaction (MIT, its default os32-gcc16 stove), on the GFOpenKick ROM where none is, so OS 3.2.3's own
# on an OS 3.2.3 disk stays.
#   name: (source drawer, the file in its build's OUT, where it goes)
OPENKICK_CLASSES = {
    "arexx": ("libraries/openreaction/arexx", "classes/arexx.class", "SYS:Classes/arexx.class"),
}


def openkick_classes_lines(core: "Part", built: Path, guest_licence: bytes) -> None:
    """The GFOpenUp part, on the GFOpenKick ROM: OPENKICK_CLASSES, each only where none is ("default"), with
    GFOpenReaction's licence (amicradle-guest's) in SYS:Utilities/GFOpenUp/Licences."""
    for name, (src, out, dest) in OPENKICK_CLASSES.items():
        f = built / name / out
        if not f.is_file():
            raise SystemExit(f"{f} wasn't built (amicradle-guest {src}/build-amiga.sh)")
        if not binary_version(f.read_bytes(), Path(out).name):
            raise SystemExit(f"{f} has no {Path(out).name} version")
        core.add(f"OpenKick/{out}", f.read_bytes(), dest, default=True, cond="when GFOpenKick")
    core.add("Utilities/GFOpenUp/Licences/GFOpenReaction.txt",
             b"GFOpenReaction's classes (DalsinAI/amicradle-guest libraries/openreaction):\n\n" + guest_licence,
             "SYS:Utilities/GFOpenUp/Licences/GFOpenReaction.txt", cond="when GFOpenKick")


def openup_rom_lines(core: "Part", openmonitor: bytes, icon: bytes) -> None:
    """What the GFOpenUp part does by the ROM (the no-ROM-volume design of 24 October 2026, sections 5.5, 7 and 10).

    GFOpenMonitor: the program comes with its icon on every ROM. The open ROM has GFOpenMonitor 1.0 from the
    GFOpenMonitor task-manager design's P6 on, the same as the disk's; an open ROM built before has 0.3.3 or 0.3.4, which
    "when GFOpenKick" can't tell apart (it asks for prefs.resource 47.101), and there the disk's, newer, wins.
    So the file stays on every ROM; the newer version wins, and on a ROM with 1.0 neither is newer.

    CommandSwap leaves the ROM with ROM: (Q20): on the open ROM its pins, ENVARC:Sys/CommandSwap.prefs, are
    archived."""
    core.add("Utilities/GFOpenMonitor", openmonitor, "SYS:Utilities/GFOpenMonitor")
    core.add("Utilities/GFOpenMonitor.info", icon, "SYS:Utilities/GFOpenMonitor.info")
    core.line('when GFOpenKick retire "ENVARC:Sys/CommandSwap.prefs"')


# The GFOffice Suite's build (gfoffice build-os3.sh, OUT_DIR under the checkout), on its libraries (OWF=library:
# build-library.sh's owfwrite, owfcalc, owfpub and owfpresent.library in OUT_DIR/library; gfoffice
# docs/suite/owf-libraries.md): GFOpenWrite on owfwrite.library, GFOpenCalc on owfcalc.library and OWConvert on all
# four, so each program loads its own part of libowf, once however many of the suite's programs run. (Until
# 25 October 2026 it was built twice: OWConvert and GFOpenCalc with libowf linked in, GFOpenWrite on owf.library.)
GFO_LIBRARY = "build/os3-library"
GFO_LINKED = "build/os3-linked"      # OWF=linked: GFOpenPresentation's (owfpresent.library has no app on it yet)
GFO_LIBS = ("owfwrite", "owfcalc", "owfpub", "owfpresent")


def gfoffice_parts(top: Path, src: Path, zlib: bytes, guest_licence: bytes, icons) -> tuple["Part", "Part", "Part | None"]:
    """The GFOffice Suite's parts (gfoffice docs/gfoffice/Design-Suite-Shared.md, section 12, 25 October 2026).

    GFOffice holds what the suite's programs share, so those files have one owner: the suite's four libraries,
    LIBS:owfwrite.library, owfcalc.library, owfpub.library and owfpresent.library (libowf split by program, gfoffice
    docs/suite/owf-libraries.md), LIBS:z.library (ours, GFOpenZlib: deflate, for saving compressed and for PNG
    pictures in PDFs; libowf reads zip packages without it since its own inflater), the spelling dictionaries in
    SYS:Dictionaries (where GFOpenWrite looks after its own drawer), C:OWConvert (on the four), and their licences in
    SYS:Utilities/GFOpenUp/Licences. It retires LIBS:owf.library, the one library the four replace (GFOpenUp keeps it
    in SYS:Storage/GFOpenUp-Superseded). Its version is owfwrite.library's (3.0 at the split, after owf.library's
    2.x).

    Each program's part needs it ("needs part GFOffice", the design's recommended way): GFOpenUpTool INSTALL leaves
    the program out until GFOffice is installed, and REMOVE of GFOffice refuses while a program that needs it is
    installed. GFOpenWrite is built on owfwrite.library, in SYS:Utilities/GFOpenWrite as before; the dictionary an earlier
    part put beside it is archived (it finds SYS:Dictionaries' instead; the user's own words, user.words, stay).
    GFOpenCalc, the spreadsheet, on owfcalc.library, goes in SYS:Utilities/GFOpenCalc; it is None, and left out, when gfoffice's build
    didn't make it. The parts are returned unwritten."""
    onlib = src / GFO_LIBRARY
    lic = "Utilities/GFOpenUp/Licences/"
    libs = {name: (onlib / f"library/{name}.library").read_bytes() for name in GFO_LIBS}
    version = binary_version(libs["owfwrite"], "owfwrite.library")
    if not version:
        raise SystemExit(f"GFOffice: {onlib / 'library/owfwrite.library'} has no version string")
    gf = Part(top, "GFOffice", version, "GFOffice: what the GFOffice Suite's programs share: their libraries "
              "(owfwrite, owfcalc, owfpub and owfpresent.library), z.library, the dictionary in SYS:Dictionaries "
              "and C:OWConvert")
    for name in GFO_LIBS:
        gf.add(f"Libs/{name}.library", libs[name], f"LIBS:{name}.library")
    gf.add("Libs/z.library", zlib, "LIBS:z.library")
    words = sorted((src / "Dictionaries").glob("*.words"))
    for d in words:
        gf.add(f"Dictionaries/{d.name}", d.read_bytes(), f"SYS:Dictionaries/{d.name}")
    gf.add("C/OWConvert", (onlib / "OWConvert").read_bytes(), "SYS:C/OWConvert")
    gf.add(lic + "GFOffice.txt", b"owfwrite.library, owfcalc.library, owfpub.library, owfpresent.library and C:OWConvert "
           b"are the GFOffice Suite's (DalsinAI/gfoffice):\n\n"
           + (src / "LICENSE").read_bytes(), "SYS:" + lic + "GFOffice.txt")
    zver = binary_version(zlib, "z.library")
    gf.add(lic + "z.library.txt", f"LIBS:z.library{' ' + zver if zver else ''} is GFOpenZlib, the AmiCradle Team's own "
           "(DalsinAI/amicradle-guest libraries/openzlib):\n\n".encode() + guest_licence, "SYS:" + lic + "z.library.txt")
    if words:
        scowl = src / "licenses/SCOWL-ASPELL-English.txt"
        if not scowl.is_file():
            raise SystemExit(f"GFOffice: the dictionaries' licence, {scowl}, is missing")
        gf.add(lic + "GFOffice-Dictionaries.txt", f"The spelling dictionaries in SYS:Dictionaries ({', '.join(d.name for d in words)}) "
               "are from the SCOWL/Aspell English word list, under these terms:\n\n".encode() + scowl.read_bytes(),
               "SYS:" + lic + "GFOffice-Dictionaries.txt")

    # GFOpenWrite: the word processor, as its 1.0 release laid it out, on owf.library
    # 2.0.26 (0.8.28): gfoffice main since #124 (ARexx return codes, no requesters during a command)
    wr = Part(top, "GFOpenWrite", None, "GFOpenWrite: a word processor for DOCX, ODT and Amiga documents, with PDF and printing")
    wr.needs_part("GFOffice")
    wr.add("Utilities/GFOpenWrite/GFOpenWrite", (onlib / "GFOpenWrite").read_bytes(), "SYS:Utilities/GFOpenWrite/GFOpenWrite")
    wr.add("Utilities/GFOpenWrite/GFOpenWrite.info", icons.icon(icons.WBTOOL, "page", stack=32768), "SYS:Utilities/GFOpenWrite/GFOpenWrite.info")
    # the newest release's notes, as the release archive carries them
    notes = sorted(src.glob("RELEASE_NOTES_*.md"), key=lambda f: [int(n) for n in f.stem.split("_")[-1].split(".")])
    for name, s in (("LICENSE", "LICENSE"), ("THIRD_PARTY.txt", "THIRD_PARTY.md"), ("Release-Notes.txt", notes[-1].name if notes else "RELEASE_NOTES_1.0.md")):
        if (src / s).is_file():
            wr.add(f"Utilities/GFOpenWrite/{name}", (src / s).read_bytes(), f"SYS:Utilities/GFOpenWrite/{name}")
    for f in sorted((src / "licenses").glob("*")):
        wr.add(f"Utilities/GFOpenWrite/licenses/{f.name}", f.read_bytes(), f"SYS:Utilities/GFOpenWrite/licenses/{f.name}")
    wr.add("Utilities/GFOpenWrite.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)), "SYS:Utilities/GFOpenWrite.info", own=True)
    for d in words:                             # GFOffice's now, in SYS:Dictionaries
        wr.line(f'retire "SYS:Utilities/GFOpenWrite/Dictionaries/{d.name}"')

    # owf.library 2, the one library before the split: the four replace it, so it is archived
    gf.line('retire "LIBS:owf.library"')

    # GFOpenCalc: the spreadsheet (gfoffice app/opencalc.c), on owfcalc.library
    oc = None
    prog = onlib / "GFOpenCalc"
    if prog.is_file():
        data = prog.read_bytes()
        oc = Part(top, "GFOpenCalc", binary_version(data, "GFOpenCalc") or "?",
                  "GFOpenCalc: the GFOffice Suite's spreadsheet, with formulas, for XLSX, ODS, XLS and CSV")
        oc.needs_part("GFOffice")
        oc.add("Utilities/GFOpenCalc/GFOpenCalc", data, "SYS:Utilities/GFOpenCalc/GFOpenCalc")
        oc.add("Utilities/GFOpenCalc/GFOpenCalc.info", icons.icon(icons.WBTOOL, "page", stack=32768), "SYS:Utilities/GFOpenCalc/GFOpenCalc.info")
        if (src / "LICENSE").is_file():
            oc.add("Utilities/GFOpenCalc/LICENSE", (src / "LICENSE").read_bytes(), "SYS:Utilities/GFOpenCalc/LICENSE")
        oc.add("Utilities/GFOpenCalc.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)), "SYS:Utilities/GFOpenCalc.info", own=True)
    else:
        print(f"  GFOpenCalc: left out, {prog} wasn't built")
    return gf, wr, oc


def gfoffice_presentation_part(top: Path, src: Path, icons) -> "Part | None":
    """GFOpenPresentation, the GFOffice Suite's slides (gfoffice app/openpresent.c): opens ODP and PPTX, saves ODP,
    exports PDF, prints through GFOpenPrint, runs a show, and has the GFOPENPRESENTATION ARexx port (gfoffice #130).
    It is built with libowf linked in (GFO_LINKED), so it needs no owf library, but it needs GFOffice for
    LIBS:z.library (deflate, to save ODP) and the toolbar's pictures beside it (PROGDIR:Images/Toolbar). It goes in
    SYS:Utilities/GFOpenPresentation; None, and left out, when the linked build didn't make it. Unwritten."""
    linked = src / GFO_LINKED
    prog = linked / "GFOpenPresentation"
    if not prog.is_file():
        print(f"  GFOpenPresentation: left out, {prog} wasn't built")
        return None
    data = prog.read_bytes()
    d = "Utilities/GFOpenPresentation/"
    gp = Part(top, "GFOpenPresentation", binary_version(data, "GFOpenPresentation") or "?",
              "GFOpenPresentation: the GFOffice Suite's slides, for ODP and PPTX, with PDF, printing and a show")
    gp.needs_part("GFOffice")
    gp.add(d + "GFOpenPresentation", data, "SYS:" + d + "GFOpenPresentation")
    gp.add(d + "GFOpenPresentation.info", icons.icon(icons.WBTOOL, "page", stack=65536), "SYS:" + d + "GFOpenPresentation.info")
    for f in sorted((linked / "Images/Toolbar").glob("*.ilbm")):
        gp.add(d + "Images/Toolbar/" + f.name, f.read_bytes(), "SYS:" + d + "Images/Toolbar/" + f.name)
    if (src / "LICENSE").is_file():
        gp.add(d + "LICENSE", (src / "LICENSE").read_bytes(), "SYS:" + d + "LICENSE")
    gp.add("Utilities/GFOpenPresentation.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)),
           "SYS:Utilities/GFOpenPresentation.info", own=True)
    return gp


# Each part's main binary: the part's declared version (its Part file, the
# Installer's log, the manifest) must be that binary's $VER version.
PART_MAIN = {
    "GFOpenUp": ("Utilities/GFOpenUp/Uninstall-GFOpenUp", "Uninstall-GFOpenUp"),
    "GFOpenBlanker": ("Tools/Commodities/GFOpenBlanker", "GFOpenBlanker"),
    "GFOpenSnap": ("Tools/Commodities/GFOpenSnap", "GFOpenSnap"),
    "GFOpenSocket": ("Libs/bsdsocket.library", "bsdsocket.library"),
    "GFOpenPrinter": ("Devs/printer.device", "printer.device"),
    "GFOpenPrefs": ("Preferences/GFOpenPrefs", "GFOpenPrefs"),
    "GFOpenCompress": ("Utilities/GFOpenCompress", "GFOpenCompress"),
    "GFOpenMulticore": ("Libs/gfopenmulticore.library", "gfopenmulticore.library"),
    "GFOpenRTG": ("Libs/gfopenrtg.library", "gfopenrtg.library"),
    "GFOpenGPU": ("Libs/gfopengpu.library", "gfopengpu.library"),
    "GFOpenMUI": ("Libs/muimaster.library", "muimaster.library"),
    "OGLES3": ("Libs/ogles3.library", "ogles3.library"),
    "AkikoPlus": ("Libs/akikoplus.library", "akikoplus.library"),
    "Warp3D": ("Libs/Warp3D.library", "Warp3D.library"),
    "GFOpenCrypto": ("Libs/gfopencrypto.library", "gfopencrypto.library"),
    "GFOpenTLS": ("Libs/gfopentls.library", "gfopentls.library"),
    "GFOpenFont": ("Libs/gfopenfont.library", "gfopenfont.library"),
    "GFOpenPuTTY": ("Utilities/GFOpenPuTTY/GFOpenPuTTY", "GFOpenPuTTY"),
    "Tata": ("C/Tata", "Tata"),
    "GFOpenFTP": ("Utilities/GFOpenFTP/GFOpenFTP", "GFOpenFTP"),
    "Telegram": ("Utilities/UnofficialTelegram", "UnofficialTelegram"),
    "GFOpenInput": ("Libs/gfopeninput.library", "gfopeninput.library"),
    "GFOpenWindows": ("Tools/Commodities/GFOpenWindows", "GFOpenWindows"),
    "GFOpenDock": ("C/GFOpenDock", "GFOpenDock"),
    "GFOpenUpSetup": ("Prefs/GFOpenUp-Setup", "GFOpenUp-Setup"),
    "GFOpenMenus": ("C/GFOpenMenus", "GFOpenMenus"),
    "HostClock": ("C/HostClock", "HostClock"),
    "GFOpenFiles": ("Utilities/GFOpenFiles", "GFOpenFiles"),
    "GFOpenPlay": ("Utilities/GFOpenPlay", "GFOpenPlay"),
    "GFOpenTitle": ("C/GFOpenTitle", "GFOpenTitle"),
    "GFOpenBench": ("System/GFOpenBench", "GFOpenBench"),
    "GFOpenEdit": ("C/GFOpenEdit", "GFOpenEdit"),
    "GFOpenDrawer": ("Utilities/GFOpenDrawer", "GFOpenDrawer"),
    "GFOpenWrite": ("Utilities/GFOpenWrite/GFOpenWrite", "GFOpenWrite"),
    "GFOffice": ("Libs/owfwrite.library", "owfwrite.library"),  # the GFOffice Suite's shared files: owfwrite.library's version
    "GFOpenCalc": ("Utilities/GFOpenCalc/GFOpenCalc", "GFOpenCalc"),
    "GFOpenPresentation": ("Utilities/GFOpenPresentation/GFOpenPresentation", "GFOpenPresentation"),
    "GFOpenMail": ("Utilities/GFOpenMail/GFOpenMail", "GFOpenMail"),
    "OpenBrowser": ("Utilities/OpenBrowser/OpenBrowser", "OpenBrowser"),
    "GFOpenBrowserLite": ("Utilities/GFOpenBrowserLite/GFOpenBrowserLite", "GFOpenBrowserLite"),
}
# Parts with a version of their own: bundles of several programs, or files
# without a binary of their own to read. GFOpenPrint 0.4 is GFOpenPrint (0.3) with
# GFOpenView 0.5 and the copy at SYS:Utilities/MultiView. Sound 0.2.2 is GFOpenSpeaker (0.1.4; the editor is GFOpenPrefs's since W48) with GFOpenAHI's
# ahi.device (6.1.1), acaudio.audio and modes (AmiCradle machines only). GFOpenImage 0.4 adds
# opentga.datatype (47.1). GFOpenService 1.3 is gfopenservice.device (1.1) with media-handler (1.0, MEDIA:),
# mitchy.library (1.0) and ServiceTest (0.2); 1.4 is mitchy.device (2.0), with gfopenservice.device (2.0) its old name
# opening it. ShellFont 1.0 is fonts, no program: DejaVu Sans Mono 2.37's bitmap sizes (gfopenfont
# fonts/amiga). GFOSFonts 1.0 is fonts too: gfosfonts' grids, built by its tools/amigafont.py. ShellIcon 1.0 is one tool type line, no file. A part's own version goes up when what it ships changes, so GFOpenUpTool UPDATE
# and the Installer's log name the change.
# OpenAmigaPython's version is its release's (2.7.18-ac3, which the binary's $VER can't give in
# binary_version's form): openamigapython_part checks it against the pin and the release's manifest.
PART_OWN_VERSION = {"KickstartSet", "GFOpenLibs", "AmiCradleSharing", "GFOpenImage", "GFOpenService", "Backdrops", "GFOpenIcons", "GFOpenDemos", "GFOpenGPUDev",
                    "GFOpenPrint", "Sound", "OpenAmigaPython", "ShellFont", "ShellIcon", "GFOSFonts",
                    # pages of GFOpenPrefs with no files of their own since the no-ROM-volume design: GFOpenPrefs's version
                    "GFOpenTypes", "GFOpenDateTime"}


# OpenAmigaPython (SacredTrees/openamigapython, the ac3-classic line): the pack takes the Part
# its release makes (Tools/release/build_release.py), as it is. Python isn't built here: its
# VBCC and PosixLib aren't on the stoves. The release's archive is pinned by its sha256, so a
# new release is a new pin here. Fetch it with
#   gh release download v2.7.18-ac3 -R SacredTrees/openamigapython \
#       -p OpenAmigaPython-2.7.18-ac3-OpenUp.lha -D build
# (build/ is where --openamigapython looks first). The Part has no "needs" line: it goes on any
# Amiga. The Installer checks for a 68020 and installs GFOpenTLS (with GFOpenCrypto) with it.
OPENAMIGAPYTHON = {
    "version": "2.7.18-ac3",
    "asset": "OpenAmigaPython-2.7.18-ac3-OpenUp.lha",
    "sha256": "61cb642d3855d2ea1f6948923e3cb15efb116f7ec4dc938873e55e5e2ae7a626",
    "release": "https://github.com/SacredTrees/openamigapython/releases/tag/v2.7.18-ac3",
}


def openamigapython_part(top: Path, archive: Path) -> Path:
    """Parts/OpenAmigaPython from the release's GFOpenUp archive (GFOpenUp/Parts/OpenAmigaPython,
    Part and Payload/), copied as it is. Stops the build unless the archive's sha256 is the
    pinned one, its Part is the pinned version with no "needs" line and every copy line's file
    in it, and every payload file is the one the release's own SHA256SUMS names."""
    import hashlib
    import tempfile
    pin = OPENAMIGAPYTHON
    where = f"{pin['asset']} from {pin['release']}"
    got = hashlib.sha256(archive.read_bytes()).hexdigest()
    if got != pin["sha256"]:
        raise SystemExit(f"--openamigapython: {archive} has sha256 {got}; the pack takes {where}, "
                         f"sha256 {pin['sha256']}")
    tmp = Path(tempfile.mkdtemp(prefix="openamigapython-"))
    try:
        run([shutil.which("jlha") or "lha", "xq", archive.resolve()], cwd=tmp)
        # the release archive (pinned above) unpacks to OpenUp/Parts/OpenAmigaPython: it predates the GF names
        src = next((d for d in (tmp / "GFOpenUp/Parts/OpenAmigaPython", tmp / "OpenUp/Parts/OpenAmigaPython") if d.is_dir()),
                   tmp / "GFOpenUp/Parts/OpenAmigaPython")
        payload = src / "Payload"
        lines = (src / "Part").read_text(encoding="latin-1").splitlines()
        wrong = []
        if lines[0].split()[:3] != ["part", "OpenAmigaPython", pin["version"]]:
            wrong.append(f"its Part begins {lines[0]!r}")
        if any(line.startswith("needs") for line in lines):
            wrong.append("its Part has a needs line, so GFOpenUp would leave it out on some Amigas")
        sums = {}
        for line in (payload / "SHA256SUMS").read_text(encoding="latin-1").splitlines():
            digest, rel = line.split("  ", 1)
            sums[rel] = digest
        have = {p.relative_to(payload).as_posix() for p in payload.rglob("*") if p.is_file()}
        if have - {"SHA256SUMS"} != set(sums):
            wrong.append(f"its payload and SHA256SUMS differ: {sorted(have.symmetric_difference(set(sums) | {'SHA256SUMS'}))[:5]}")
        wrong += [f"{rel} isn't the file SHA256SUMS names" for rel, digest in sorted(sums.items())
                  if rel in have and hashlib.sha256((payload / rel).read_bytes()).hexdigest() != digest]
        if json.loads((payload / "openamigapython-release.json").read_text())["version"] != pin["version"]:
            wrong.append("its openamigapython-release.json is another version")
        copies = [re.match(r'copy "([^"]+)" "([^"]+)"$', line) for line in lines if line.startswith("copy ")]
        wrong += [f"its Part copies {m.group(1) if m else '?'}, which isn't there"
                  for m in copies if not m or not (src / m.group(1)).is_file()]
        if len(copies) != len(have):
            wrong.append(f"its Part copies {len(copies)} files of the {len(have)} in its payload")
        if wrong:
            raise SystemExit(f"--openamigapython: {where}: " + "; ".join(wrong[:8]))
        dest = top / "Parts" / "OpenAmigaPython"
        if dest.exists():
            shutil.rmtree(dest)
        shutil.copytree(src, dest)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"  OpenAmigaPython {pin['version']}: {len(copies)} files; sha256 and SHA256SUMS checked")
    return dest


# OpenBrowser Lite (DalsinAI/gfopenbrowser): the pack takes the program, its readme and its
# licence from the release's archive, as its CI built and packed them; it isn't built here. The
# archive is pinned by its address and sha256, so a new release is a new pin here. build/ is
# looked in first; without the archive there it is fetched from "url" into build/. Until the
# release is published the sha256 is a placeholder, and the pack stops (it never leaves the
# part out): publish gfopenbrowser's v0.4.1 (its docs/RELEASE_0.4.1.md), then put the
# sha256 its CI printed (OpenBrowserLite-0.4.1.lha.sha256 beside the asset) here.
# 0.4.3 (GFOpenUp 0.8.13): gfopenbrowser #155's 0.4.2 (0.4.1's source with the GF names, built on open5) opened
# GFOpenTLS as opentls.library, its name before the rename, so on a GFOpenUp 0.8.13 without 0.8.11's leftovers every page
# said "No network" (lab t5-up0813-fresh, 4 Nov 2026). 0.4.3 (gfopenbrowser #169) opens gfopentls.library. Its archive
# is on the v0.4.3 draft release until Team 5 publishes it; until then give it with --openbrowserlite.
# Since the GF rename the archive's drawer and program are GFOpenBrowserLite ("name").
OPENBROWSERLITE = {
    "version": "0.4.3",
    "name": "GFOpenBrowserLite",
    "asset": "GFOpenBrowserLite-0.4.3.lha",
    "url": "https://github.com/DalsinAI/gfopenbrowser/releases/download/v0.4.3/GFOpenBrowserLite-0.4.3.lha",
    "sha256": "dacfcb8324bfb2eb40cffde651f94b4d0c1663484f90352efe5e037d35dc9a8c",
    "release": "https://github.com/DalsinAI/gfopenbrowser/releases/tag/v0.4.3",
}


def check_openbrowserlite_pin():
    """Stops the build while OPENBROWSERLITE's sha256 is not a sha256 (the placeholder)."""
    pin = OPENBROWSERLITE
    if not re.fullmatch(r"[0-9a-f]{64}", pin["sha256"]):
        raise SystemExit(f"OpenBrowserLite: OPENBROWSERLITE's sha256 is still the placeholder "
                         f"({pin['sha256']!r}). Publish gfopenbrowser's v{pin['version']} release, then "
                         f"put the sha256 of {pin['url']} in tools/build_pack.py's OPENBROWSERLITE.")


def openbrowserlite_files(archive: Path | None) -> dict:
    """OpenBrowser Lite's files from its pinned release archive (OPENBROWSERLITE): the archive
    from build/ (or --openbrowserlite), else fetched from its address into build/. Stops the
    build while the pin's sha256 is the placeholder, when the archive can't be had, when its
    sha256 isn't the pinned one, or when its drawer lacks a file or holds another version.
    Returns {name in the drawer: bytes}."""
    import hashlib
    import tempfile
    import urllib.request
    pin = OPENBROWSERLITE
    where = f"{pin['asset']} from {pin['release']}"
    check_openbrowserlite_pin()
    if archive is None:
        archive = ROOT / "build" / pin["asset"]
        if not archive.is_file():
            archive.parent.mkdir(parents=True, exist_ok=True)
            print(f"  fetching {pin['url']}", flush=True)
            part = archive.with_suffix(".part")
            try:
                with urllib.request.urlopen(pin["url"], timeout=120) as r, open(part, "wb") as f:
                    shutil.copyfileobj(r, f)
            except OSError as e:
                part.unlink(missing_ok=True)
                raise SystemExit(f"OpenBrowserLite: could not fetch {pin['url']}: {e}")
            part.replace(archive)
    got = hashlib.sha256(archive.read_bytes()).hexdigest()
    if got != pin["sha256"]:
        raise SystemExit(f"OpenBrowserLite: {archive} has sha256 {got}; the pack takes {where}, "
                         f"sha256 {pin['sha256']}")
    tmp = Path(tempfile.mkdtemp(prefix="openbrowserlite-"))
    try:
        run([shutil.which("jlha") or "lha", "xq", archive.resolve()], cwd=tmp)
        nm = pin.get("name", "OpenBrowserLite")
        drawer = tmp / nm
        files = {}
        for name in (nm, f"{nm}.readme", "LICENSE"):
            if not (drawer / name).is_file():
                raise SystemExit(f"OpenBrowserLite: {where} has no {nm}/{name}")
            files[name] = (drawer / name).read_bytes()
        vl = re.search(rb"\$VER: " + nm.encode() + rb" ([0-9.]+)", files[nm])
        if not vl or vl.group(1).decode() != pin["version"]:
            raise SystemExit(f"OpenBrowserLite: {where}'s program is "
                             f"{vl.group(1).decode() if vl else 'of no version'}, not {pin['version']}")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"  {pin.get('name', 'OpenBrowserLite')} {pin['version']}: {pin['asset']}, sha256 checked")
    return files


# physfs.library (gfopenphysfs: PhysicsFS 3.2.0 as one shared library, one copy of its state per program), for the
# game ports built with the GFOpenGPU developer kit's -lphysfs_auto. Pinned by commit: main after gfopenphysfs #5
# (1.1: a RomTag and per-opener bases; zip members inflate through LIBS:z.library; since #5 files are read through
# a read-ahead buffer, and seeking back in a deflated member is right; the same functions and LVOs as 21cfa03, the
# 0.8.28-rc1 pin). The pack builds that commit from the --openamigaphysfs checkout whatever the checkout has out
# (git archive), so a newer main is a change here. gfopenrtg's make_sdk.py checks the same commit (its
# PHYSFS_COMMIT) before the kit takes the SDK files.
GFOPENPHYSFS = {
    "version": "1.1",
    "commit": "d3a0ad78e2a1cb5cc32306982593a9889f2e9ba4",
    "source": "https://github.com/DalsinAI/gfopenphysfs/commit/d3a0ad78e2a1cb5cc32306982593a9889f2e9ba4",
}

# The oldest z.library physfs.library is shipped with: 53.15 (amicradle-guest #690), whose inflate runs a block's
# codes as one hostlib call (host code on AmiCradle, the same C on any other 68k), so zip members read about a
# quarter faster on AmiCradle (Team 2, task 42). An older guest checkout stops the pack here rather than ship 53.14.
ZLIB_MIN_FOR_PHYSFS = (53, 15)


def build_physfs(checkout: Path, out: Path, stove16: str) -> None:
    """physfs.library and its SDK (gfopenphysfs build-amiga.sh, GCC 16) from GFOPENPHYSFS's commit into out, with
    LICENSE.txt (PhysicsFS's zlib licence, then the Team's) and COMMIT beside them, as make_sdk.py --physfs takes
    them. The commit is taken out of the checkout with git archive, so the checkout's own state doesn't matter."""
    pin = GFOPENPHYSFS["commit"]
    if subprocess.run(["git", "-C", str(checkout), "cat-file", "-e", pin + "^{commit}"], capture_output=True).returncode:
        raise SystemExit(f"physfs.library: {checkout} has no commit {pin[:12]} (GFOPENPHYSFS): "
                         f"git -C {checkout} fetch origin main")
    src = out.parent / "physfs-src"
    shutil.rmtree(src, ignore_errors=True)
    src.mkdir(parents=True)
    tar = subprocess.run(["git", "-C", str(checkout), "archive", pin], check=True, capture_output=True).stdout
    subprocess.run(["tar", "-x", "-C", str(src)], input=tar, check=True)
    run(["sh", src / "build-amiga.sh"], env=dict(os.environ, STOVE=stove16, OUT=str(out)))
    (out / "LICENSE.txt").write_bytes((src / "physfs/LICENSE.txt").read_bytes() + b"\n\n" + (src / "LICENSE").read_bytes())
    (out / "COMMIT").write_text(pin + "\n")


def physfs_into_gpu(gp: "Part", built: Path, zlib: bytes) -> None:
    """LIBS:physfs.library (built by build_physfs) into the GFOpenGPU part, with LIBS:z.library, which its zip
    reader opens to inflate members (the same GFOpenZlib file the GFOffice and GFOpenImage parts carry: z.library
    is no part of its own, so each part that needs it brings it), and PhysicsFS's licence."""
    if (built / "COMMIT").read_text().strip() != GFOPENPHYSFS["commit"]:
        raise SystemExit(f"physfs.library: {built} isn't gfopenphysfs {GFOPENPHYSFS['commit'][:12]} (GFOPENPHYSFS)")
    lib = (built / "Libs/physfs.library").read_bytes()
    ver = binary_version(lib, "physfs.library")
    if ver != GFOPENPHYSFS["version"]:
        raise SystemExit(f"physfs.library: {built} has version {ver}, GFOPENPHYSFS says {GFOPENPHYSFS['version']}")
    zver = binary_version(zlib, "z.library")
    if zver is None or tuple(int(x) for x in zver.split(".")[:2]) < ZLIB_MIN_FOR_PHYSFS:
        raise SystemExit(f"physfs.library: z.library is {zver or 'unversioned'}, it needs "
                         f"{'.'.join(map(str, ZLIB_MIN_FOR_PHYSFS))} or newer (ZLIB_MIN_FOR_PHYSFS: amicradle-guest main)")
    gp.add("Libs/physfs.library", lib, "LIBS:physfs.library")
    gp.add("Libs/z.library", zlib, "LIBS:z.library")
    gp.add("Utilities/GFOpenUp/Licences/PhysicsFS.txt",
           f"LIBS:physfs.library {ver} is PhysicsFS 3.2.0 as one shared library (DalsinAI/gfopenphysfs "
           f"{GFOPENPHYSFS['commit'][:12]}):\n\n".encode() + (built / "LICENSE.txt").read_bytes(),
           "SYS:Utilities/GFOpenUp/Licences/PhysicsFS.txt")


# OpenBrowser, the WebKit browser, in its own archive beside the pack (GFOpenUp-<version>-OpenBrowser.lha): the engine
# from gfopenbrowser's main at "commit", built and packed on the home PC (about 100 MB; scripts/build-webcore.sh
# and build-openbrowser.sh need the OAB_DEPS prefix, which only that machine has), so it is pinned by the commit and
# by the sha256 of the archive made from it, as OpenBrowser Lite's is by its release. There is no GitHub release of
# it. Until the archive's sha256 is put here, "sha256" is the placeholder and --openbrowser stops, printing the
# archive's sha256 (the pack itself, without --openbrowser, is not held up):
#   1. in a gfopenbrowser checkout at "commit": build the engine and the browser and pack the drawer
#      OpenBrowser/ (the program, its Libs/, readme and licences) as OpenBrowser-<version>-68040.lha;
#   2. sha256sum that archive, and put it in "sha256" below;
#   3. python3 tools/build_pack.py ... --openbrowser /path/to/OpenBrowser-<version>-68040.lha --lha
# 0.7 (GFOpenUp 0.8.8): d41a340 is 7530314 (OpenBrowser 0.7, WebKit patches 0001-0035) with only a lab script changed
# after it (#44), so the engine is the same.
OPENBROWSER = {
    "version": "0.7",
    "commit": "d41a340e4bf0711022587007948d26c6b65c1e8e",
    "asset": "OpenBrowser-0.7-68040.lha",
    "sha256": "eb85e19b48c365637223365f6f208c31cde811071f37075e1a5c30353b20198c",
    "source": "https://github.com/DalsinAI/gfopenbrowser/commit/d41a340e4bf0711022587007948d26c6b65c1e8e",
}


def check_openbrowser_pin(archive: Path) -> None:
    """Stops the build unless the OpenBrowser archive given (--openbrowser) is the pinned one: its sha256, and the
    program's $VER at the pinned version (checked again by check_part_versions). An unpacked folder can't be
    pinned by its sha256: only its version is checked, and the build says so."""
    import hashlib
    pin = OPENBROWSER
    if archive.is_file():
        got = hashlib.sha256(archive.read_bytes()).hexdigest()
        if not re.fullmatch(r"[0-9a-f]{64}", pin["sha256"]):
            raise SystemExit(f"OpenBrowser: OPENBROWSER's sha256 is still the placeholder. {archive} has sha256 {got}: "
                             f"if it is OpenBrowser {pin['version']} built from {pin['commit'][:7]}, put that in "
                             f"tools/build_pack.py's OPENBROWSER and build again.")
        if got != pin["sha256"]:
            raise SystemExit(f"OpenBrowser: {archive} has sha256 {got}; the pack takes {pin['asset']} "
                             f"(gfopenbrowser {pin['commit'][:7]}), sha256 {pin['sha256']}")
        print(f"  OpenBrowser {pin['version']}: {archive.name}, sha256 checked")
    else:
        print(f"  OpenBrowser: {archive} is a folder, so its sha256 isn't checked; its version must be {pin['version']}")


# DejaVu Sans for the GFOpenFont part: DejaVu fonts 2.37's DejaVuSans.ttf (Ubuntu's fonts-dejavu-core 2.37-8build1;
# OpenBrowser's LIBS:WebBrowser/Fonts copy is the same file). A new DejaVu is a new pin here.
DEJAVU_SANS_SHA256 = "b4c632e3cdf9acc7f28758fb5a323c8524d7fc6660d46904d9b6cbe2809c419c"
# DejaVu Serif for the GFOpenBlanker part (0.8.16): the same DejaVu fonts 2.37 package's DejaVuSerif.ttf.
DEJAVU_SERIF_SHA256 = "a52c2a69dfc93d0b511b98c1019a57eb3d6a62d328bdf1387f00851b02e3a315"
# AmiSans for the GFOpenFont part (9 November 2026): TeX Gyre Heros, texgyreheros-regular.otf 2.004 from GUST's
# e-foundry (qhv2.004otf.zip; tg2_501otf.zip has the same file), unmodified, in gfopenfont's fonts/outline/TeXGyreHeros
# with the GUST Font License, and gfopenfont's hinted bitmap sizes of it (fonts/outline/AmiSans.font). The open
# stand-in for OS 3.2.3's CGTriumvirate: the GFOpenKick ROM's tall Workbench font falls back to it (amicradle-guest
# screen_core.h OI_TALL_FONT_OPEN) and GFOpenPrefs' standard font is it on an open disk.
TEXGYRE_HEROS_SHA256 = "6ae1a09d5a940367b7aaaa91ee8bd8a2c333bfe193e7096e23f931357d62081f"
# Its .otag's OT_YSizeFactor: CGTriumvirate.otag's own 8782/10519 (OS 3.2.3), so a YSize draws the same em (13: 11
# pixels, as the bitmap size) and AmiSans 13 stands where CGTriumvirate 13 stood, in the same 24-pixel bars.
TEXGYRE_HEROS_YSIZE_FACTOR = (8782, 10519)


def ttf_name(data: bytes, nid: int) -> str | None:
    """A TrueType font's name table entry nid (its Windows Unicode one), or None."""
    for i in range(struct.unpack(">H", data[4:6])[0]):
        tag, _, off, _ = struct.unpack(">4sIII", data[12 + 16 * i:28 + 16 * i])
        if tag == b"name":
            _, count, strings = struct.unpack(">HHH", data[off:off + 6])
            for j in range(count):
                pid, eid, _, k, length, o = struct.unpack(">HHHHHH", data[off + 6 + 12 * j:off + 18 + 12 * j])
                if pid == 3 and eid in (1, 10) and k == nid:
                    return data[off + strings + o:off + strings + o + length].decode("utf-16-be")
    return None


def outline_otag(ttf: str, ysize_factor: tuple[int, int] | None = None) -> bytes:
    """An .otag for a TrueType font that GFOpenFont draws (amicradle-guest libraries/openbullet's README): OT_FileIdent
    (the file's length), OT_Engine "bullet" and OT_Spec1 | OT_Indirect, the font file's name (one without a device
    is taken in the .otag's own drawer); the two strings follow the list, as offsets from its start.
    ysize_factor (0.8.16): OT_YSizeFactor, (point height term, black height term): pointSize = ysize * the first /
    the second (diskfonttag.h), so the font's ascent and descent fit the YSize (gfopenfont.library 1.7 reads it)."""
    ot_fileident, ot_engine, ot_spec1 = 0x80001001, 0x80009002, 0x80009101   # TAG_USER, OT_Level1, OT_Indirect
    ot_ysizefactor = 0x80001011                                               # OT_Level1 | 0x11
    tags = 4 if ysize_factor else 3
    head, data, offs = (tags + 1) * 8, b"", []
    for s in (b"bullet\0", ttf.encode("latin-1") + b"\0"):
        offs.append(head + len(data))
        data += s + b"\0" * (len(s) % 2)
    total = head + len(data)
    total += -total % 4
    b = struct.pack(">6I", ot_fileident, total, ot_engine, offs[0], ot_spec1, offs[1])
    if ysize_factor:
        b += struct.pack(">II", ot_ysizefactor, (ysize_factor[0] << 16) | ysize_factor[1])
    b += struct.pack(">2I", 0, 0) + data
    return b + bytes(total - len(b))


def ttf_ysize_factor(data: bytes) -> tuple[int, int]:
    """OT_YSizeFactor for a TrueType font: its em (head's unitsPerEm) to its hhea ascent plus descent, the tallest
    and lowest of DejaVu's Latin-1 glyphs."""
    tab = {}
    for i in range(struct.unpack(">H", data[4:6])[0]):
        tag, _, off, _ = struct.unpack(">4sIII", data[12 + 16 * i:28 + 16 * i])
        tab[tag] = off
    upm = struct.unpack(">H", data[tab[b"head"] + 18:tab[b"head"] + 20])[0]
    asc, desc = struct.unpack(">hh", data[tab[b"hhea"] + 4:tab[b"hhea"] + 8])
    return upm, asc - desc


def openfont_part(top: Path, font_src: Path | None, ttf: Path):
    """The GFOpenFont part: gfopenfont.library from a gfopenfont checkout built by its build-amiga.sh, and DejaVu Sans
    as an outline font: FONTS:DejaVuSans.font (an outline .font, OFCH_ID $0F03, no bitmap sizes), FONTS:DejaVuSans.otag
    and FONTS:_ttf/DejaVuSans.ttf; the licences in SYS:Utilities/GFOpenUp/Licences (GFOpenFont's MIT with the Unicode
    licence of the character data in it, and DejaVu Sans's as the font file gives it, its name table's licence
    description). None when the library isn't built or the font isn't there."""
    import hashlib
    lib = font_src / "build/amiga/gfopenfont.library" if font_src else None
    if not lib or not lib.is_file():
        print("  GFOpenFont: left out (no build/amiga/gfopenfont.library in the --openamigafont checkout: its build-amiga.sh)")
        return None
    if not ttf.is_file():
        print(f"  GFOpenFont: left out ({ttf} isn't there: --dejavu-sans names DejaVu fonts 2.37's DejaVuSans.ttf)")
        return None
    sans = ttf.read_bytes()
    got = hashlib.sha256(sans).hexdigest()
    if got != DEJAVU_SANS_SHA256:
        raise SystemExit(f"GFOpenFont: {ttf} has sha256 {got}; the pack takes DejaVu fonts 2.37's DejaVuSans.ttf, "
                         f"sha256 {DEJAVU_SANS_SHA256}")
    licence = ttf_name(sans, 13)
    if not licence:
        raise SystemExit(f"GFOpenFont: {ttf} has no licence description in its name table")
    ofl = lib.read_bytes()
    p = Part(top, "GFOpenFont", binary_version(ofl, "gfopenfont.library") or "1.4",
             "GFOpenFont: gfopenfont.library, and DejaVu Sans for the Open programs' text")
    p.add("Libs/gfopenfont.library", ofl, "LIBS:gfopenfont.library")
    # "add": a DejaVu Sans of another program's stays
    p.add("Fonts/DejaVuSans.font", struct.pack(">HH", 0x0F03, 0), "FONTS:DejaVuSans.font", own=True)
    p.add("Fonts/DejaVuSans.otag", outline_otag("_ttf/DejaVuSans.ttf"), "FONTS:DejaVuSans.otag", own=True)
    p.add("Fonts/_ttf/DejaVuSans.ttf", sans, "FONTS:_ttf/DejaVuSans.ttf", own=True)
    heros = amisans(p, font_src)
    p.add("Utilities/GFOpenUp/Licences/GFOpenFont.txt",
          (font_src / "LICENSE").read_bytes() + b"\n\nThe Unicode character data in gfopenfont.library:\n\n" +
          (font_src / "LICENSES/Unicode-License-v3.txt").read_bytes(),
          "SYS:Utilities/GFOpenUp/Licences/GFOpenFont.txt")
    p.add("Utilities/GFOpenUp/Licences/DejaVuSans.txt",
          ("DejaVu Sans, DejaVu fonts 2.37 (FONTS:_ttf/DejaVuSans.ttf, unmodified, with FONTS:DejaVuSans.font and\n"
           "DejaVuSans.otag), under the licence the font file gives:\n\n" + licence.replace("\r\n", "\n") + "\n")
          .encode("latin-1", "replace"),
          "SYS:Utilities/GFOpenUp/Licences/DejaVuSans.txt")
    p.write()
    print(f"  GFOpenFont {p.lines[0].split()[-1]}: gfopenfont.library, DejaVu Sans" +
          (", AmiSans (TeX Gyre Heros)" if heros else "") + " (sha256 checked)")
    return p


def amisans(p: "Part", font_src: Path) -> bool:
    """AmiSans in the GFOpenFont part: TeX Gyre Heros, from gfopenfont's fonts/outline (9 November 2026).
    FONTS:AmiSans.font (an outline .font, OFCH_ID $0F03, with gfopenfont's hinted bitmap sizes 12 and 13,
    FONTS:AmiSans/12 and 13, the sizes the look uses), FONTS:AmiSans.otag for every other size (bullet,
    _otf/texgyreheros-regular.otf, OT_YSizeFactor CGTriumvirate's) and FONTS:_otf/texgyreheros-regular.otf, the
    OpenType file unmodified; the GUST Font License with it in SYS:Utilities/GFOpenUp/Licences/AmiSans.txt. AmiSans,
    not TeX Gyre Heros: the bitmap sizes are drawn from it, and GUST asks that derived works have a new name.
    False, and the part without it, when the checkout has none (an older gfopenfont)."""
    import hashlib
    o = font_src / "fonts/outline"
    otf = o / "TeXGyreHeros/texgyreheros-regular.otf"
    if not otf.is_file() or not (o / "AmiSans.font").is_file():
        print(f"  GFOpenFont: AmiSans left out ({o} has no AmiSans.font and TeX Gyre Heros: an older gfopenfont)")
        return False
    data = otf.read_bytes()
    got = hashlib.sha256(data).hexdigest()
    if got != TEXGYRE_HEROS_SHA256:
        raise SystemExit(f"GFOpenFont: {otf} has sha256 {got}; the pack takes TeX Gyre Heros 2.004's "
                         f"texgyreheros-regular.otf, sha256 {TEXGYRE_HEROS_SHA256}")
    contents = (o / "AmiSans.font").read_bytes()
    fid, n = struct.unpack(">HH", contents[:4])
    if fid != 0x0F03 or len(contents) != 4 + 260 * n:
        raise SystemExit(f"GFOpenFont: {o}/AmiSans.font isn't an outline font's contents file (OFCH_ID $0F03)")
    sizes = [contents[4 + 260 * i:4 + 260 * i + 256].split(b"\0")[0].decode("latin-1") for i in range(n)]
    notice = ttf_name(data, 0) or ""
    # "add": an AmiSans of another program's stays
    p.add("Fonts/AmiSans.font", contents, "FONTS:AmiSans.font", own=True)
    for rel in sizes:
        p.add(f"Fonts/{rel}", (o / rel).read_bytes(), f"FONTS:{rel}", own=True)
    p.add("Fonts/AmiSans.otag", outline_otag("_otf/texgyreheros-regular.otf", TEXGYRE_HEROS_YSIZE_FACTOR),
          "FONTS:AmiSans.otag", own=True)
    p.add("Fonts/_otf/texgyreheros-regular.otf", data, "FONTS:_otf/texgyreheros-regular.otf", own=True)
    p.add("Utilities/GFOpenUp/Licences/AmiSans.txt",
          ("AmiSans is TeX Gyre Heros 2.004 (FONTS:_otf/texgyreheros-regular.otf, unmodified, from GUST's e-foundry,\n"
           "https://www.gust.org.pl/projects/e-foundry/tex-gyre/heros), with FONTS:AmiSans.otag and bitmap sizes\n"
           "drawn from it (FONTS:AmiSans.font, " + ", ".join(f"FONTS:{r}" for r in sizes) + "; gfopenfont's\n"
           "tools/amigapropfont.py), named AmiSans as GUST asks of derived works. All of it is under the GUST Font\n"
           "License.\nThe font file's notice: " + notice + "\n\n" +
           (o / "TeXGyreHeros/GUST-FONT-LICENSE.txt").read_text("latin-1")).encode("latin-1", "replace"),
          "SYS:Utilities/GFOpenUp/Licences/AmiSans.txt")
    return True


def blanker_fonts(bl: "Part", sans_ttf: Path, serif_ttf: Path) -> None:
    """GFOpenBlanker's fonts on a disk without OS 3.2's CGTriumvirate and CGTimes (the open ROM's, Instance-40,
    4 November 2026): DejaVu Sans and DejaVu Serif (DejaVu fonts 2.37, free) as outline fonts gfopenfont.library
    draws, in FONTS:GFOpenBlanker/ (a drawer Font prefs doesn't list), with OT_YSizeFactor so their ascent and
    descent fit the size (gfopenfont.library 1.7); the GFOpenFont part's DejaVuSans.otag has none, as the Open
    programs' look is sized by it. The .otags name FONTS:_ttf/DejaVuSans.ttf (the GFOpenFont part's) and
    FONTS:_ttf/DejaVuSerif.ttf (this part's, with its licence). CGTriumvirate and CGTimes are OS 3.2's and never in
    the pack."""
    import hashlib
    fonts = []
    for name, ttf, pin in (("DejaVuSans", sans_ttf, DEJAVU_SANS_SHA256), ("DejaVuSerif", serif_ttf, DEJAVU_SERIF_SHA256)):
        if not ttf.is_file():
            raise SystemExit(f"GFOpenBlanker: {ttf} isn't there: --dejavu-sans/--dejavu-serif name DejaVu fonts 2.37's")
        data = ttf.read_bytes()
        got = hashlib.sha256(data).hexdigest()
        if got != pin:
            raise SystemExit(f"GFOpenBlanker: {ttf} has sha256 {got}; the pack takes DejaVu fonts 2.37's {name}.ttf, sha256 {pin}")
        fonts.append((name, data))
        # "add": a font of another program's stays
        bl.add(f"Fonts/GFOpenBlanker/{name}.font", struct.pack(">HH", 0x0F03, 0), f"FONTS:GFOpenBlanker/{name}.font", own=True)
        bl.add(f"Fonts/GFOpenBlanker/{name}.otag", outline_otag(f"FONTS:_ttf/{name}.ttf", ttf_ysize_factor(data)),
               f"FONTS:GFOpenBlanker/{name}.otag", own=True)
    serif = fonts[1][1]
    licence = ttf_name(serif, 13)
    if not licence:
        raise SystemExit(f"GFOpenBlanker: {serif_ttf} has no licence description in its name table")
    bl.add("Fonts/_ttf/DejaVuSerif.ttf", serif, "FONTS:_ttf/DejaVuSerif.ttf", own=True)
    bl.add("Utilities/GFOpenUp/Licences/DejaVuSerif.txt",
           ("DejaVu Serif, DejaVu fonts 2.37 (FONTS:_ttf/DejaVuSerif.ttf, unmodified, with FONTS:GFOpenBlanker/\n"
            "DejaVuSerif.font and DejaVuSerif.otag), under the licence the font file gives:\n\n" +
            licence.replace("\r\n", "\n") + "\n").encode("latin-1", "replace"),
           "SYS:Utilities/GFOpenUp/Licences/DejaVuSerif.txt")


def opentls_part(top: Path, tls_src: Path):
    """The GFOpenTLS part from a gfopentls checkout built by
    build-amiga-tls.sh and tools/make_ca_bundle.py (build-ca-bundle/), or
    None when it has no gfopentls.library."""
    lib = tls_src / "build-amiga-tls/lib/gfopentls.library"
    if not lib.is_file():
        return None
    otl = lib.read_bytes()
    bundle = tls_src / "build-ca-bundle"
    tls = Part(top, "GFOpenTLS", binary_version(otl, "gfopentls.library") or "1.2",
               "GFOpenTLS: gfopentls.library, secure connections for the Open programs, with Mozilla's CA list")
    tls.add("Libs/gfopentls.library", otl, "LIBS:gfopentls.library")
    # the bundle in S:, not ENVARC: (copied into ENV:, in RAM, at every boot);
    # "add": an upgrade replaces GFOpenUp's own copy, never someone else's
    lic = (bundle / "ca-bundle.LICENSE").read_bytes()
    tls.add("S/GFOpenTLS/ca-bundle.pem", (bundle / "ca-bundle.pem").read_bytes(), "S:GFOpenTLS/ca-bundle.pem", own=True)
    tls.add("S/GFOpenTLS/ca-bundle.LICENSE", lic, "S:GFOpenTLS/ca-bundle.LICENSE", own=True)
    # the licences with the others: Mozilla's list (MPL-2.0), and GFOpenTLS's
    # own (MIT) with BearSSL's (MIT) from its THIRD_PARTY_NOTICES
    tls.add("Utilities/GFOpenUp/Licences/GFOpenTLS-CA-bundle.txt", lic,
            "SYS:Utilities/GFOpenUp/Licences/GFOpenTLS-CA-bundle.txt")
    tls.add("Utilities/GFOpenUp/Licences/GFOpenTLS.txt",
            (tls_src / "LICENSE").read_bytes() + b"\n\n" + (tls_src / "THIRD_PARTY_NOTICES.md").read_bytes(),
            "SYS:Utilities/GFOpenUp/Licences/GFOpenTLS.txt")
    tls.write()
    return tls


# GFOS's own fonts (14 November 2026, Dale 04:36 "yes, go for it"; amicradle-teams designs/DISPLAY-PROFILES-20261113.md):
# GFOS Sans, GFOS Sans Bold and GFOS Mono, hand-corrected from DejaVu's bitmaps in the gfosfonts repo, kept there as
# pixel grids (src/*.txt) and built into Amiga bitmap fonts by its own tools/amigafont.py, so no font file is kept in
# git. Each face has 11, 13 and 16 pixels. GFOS Mono's .font is marked not proportional (tf_XSize 7, 8 and 10).
GFOS_FACES = (("GFOSSans", "gfos-sans"), ("GFOSSansBold", "gfos-sans-bold"), ("GFOSMono", "gfos-mono"))
GFOS_SIZES = (13, 11, 16)                      # amigafont.py build: the first grid, then the more ones


def gfos_fonts_part(top: Path, src: Path) -> "Part | None":
    """Parts/GFOSFonts from a gfosfonts checkout: FONTS:<face>.font and FONTS:<face>/<size> for each face, built by
    the checkout's tools/amigafont.py, and its LICENSE-FONTS in SYS:Utilities/GFOpenUp/Licences/GFOSFonts.txt. "add":
    a font of the same name of another program's stays. None (and the part left out) when the checkout lacks a grid."""
    tool = src / "tools/amigafont.py"
    grids = {face: [src / f"src/{stem}-{n}.txt" for n in GFOS_SIZES] for face, stem in GFOS_FACES}
    missing = [g for gs in grids.values() for g in gs if not g.is_file()]
    if not tool.is_file() or missing or not (src / "LICENSE-FONTS").is_file():
        print(f"  GFOSFonts: left out ({src} has no " +
              (str(missing[0].relative_to(src)) if missing else "tools/amigafont.py or LICENSE-FONTS") + ")")
        return None
    import tempfile
    p = Part(top, "GFOSFonts", "1.0", "GFOS Sans, Sans Bold and Mono, 11, 13 and 16 pixels (GFOS's own fonts)")
    with tempfile.TemporaryDirectory() as t:
        for face, gs in grids.items():
            subprocess.run([sys.executable, str(tool), "build", str(gs[0]), face, t] + [str(g) for g in gs[1:]],
                           check=True, stdout=subprocess.DEVNULL)
            o = Path(t)
            p.add(f"Fonts/{face}.font", (o / f"{face}.font").read_bytes(), f"FONTS:{face}.font", own=True)
            for n in sorted(GFOS_SIZES):
                p.add(f"Fonts/{face}/{n}", (o / face / str(n)).read_bytes(), f"FONTS:{face}/{n}", own=True)
    p.add("Utilities/GFOpenUp/Licences/GFOSFonts.txt", (src / "LICENSE-FONTS").read_bytes(),
          "SYS:Utilities/GFOpenUp/Licences/GFOSFonts.txt")
    return p


# GFOpenLibs (0.8.0): the open libraries, one switch each in the Installer's sixth page, off by default
# (amicradle-guest libraries/OPEN_LIBRARIES.md, section 3: "default off until its ABI test is clean"). Only a
# library whose ABI test against OS 3.2's own is clean, and which builds with the stoves as its script says, is
# here; the others, and why, are in docs/releases/0.8.0/RELEASE-NOTES.md. Each is a part of its own in
# Parts/GFOpenLibs/<name>, so GFOpenUpTool REMOVE takes it out alone and puts OS 3.2's own back.
#   name: (the file built, the part's file, where it goes, the name its version is read under, what it is)
OPENLIBS = {
    "utility": ("openutility/modules/utility.module", "Libs/GFOpenLibs/utility.module", "LIBS:GFOpenLibs/utility.module",
                "utility.library", "utility.library 47.100 in the ROM's place (a module, loaded by OS 3.2's LoadModule)"),
    "mathieeedoubbas": ("openmathieee/libs/mathieeedoubbas.library", "Libs/mathieeedoubbas.library",
                        "LIBS:mathieeedoubbas.library", "mathieeedoubbas.library",
                        "mathieeedoubbas.library 47.100 in LIBS:, for a 68040 or 68060 with its FPU"),
    "input": ("input/input.device", "Libs/GFOpenLibs/input.device", "LIBS:GFOpenLibs/input.device", "input.device",
              "input.device 50.5 in the ROM's place, with the wheel (gfopeninput #9, #27; a module, loaded by OS 3.2's LoadModule)"),
    # ARexx stays on the disk (Dale, 18 Oct 2026): OpenRexx's two libraries in OS 3.2's place in LIBS:, and its
    # RexxMast only where there is none (OS 3.2.3's RexxMast works with them, measured: amicradle-guest
    # docs/design/openrexx.md, and under stock Kickstart 3.2.3: W75). Off by default, as every switch here is.
    "rexx": ("openrexx/libs/rexxsyslib.library", "Libs/rexxsyslib.library", "LIBS:rexxsyslib.library",
             "rexxsyslib.library", "OpenRexx, ARexx's rexxsyslib.library and rexxsupport.library in OS 3.2's place, "
             "for a 68040 or 68060 with its FPU"),
}

# S:GFOpenLibs-Startup, from S:Pre-LoadModule-Startup, which OS 3.2's Startup-Sequence runs first of all: the ROM
# libraries switched on, in one call of OS 3.2's own C:LoadModule, as gfopeninput #9's install notes do for
# input.device. LoadModule reloads the set and restarts the machine when what is in memory isn't that set, so after
# a power-on the Amiga restarts once by itself, and a library switched off goes at the next start. One call only:
# two LoadModule calls with different sets would restart the machine for ever. The Team's C:OpenModule
# (amicradle-guest #79) is not used yet: in a lab copy of Instance-32 its modules were gone at the second reset
# while OpenModule LIST still listed them, so it never loaded them again, and OpenModule REMOVE then brought
# Guru 81000005 (docs/releases/0.8.0/RELEASE-NOTES.md).
OPENLIBS_STARTUP = """\
; $VER: GFOpenLibs-Startup 1.0 (13.10.2026)
; GFOpenUp's open ROM libraries (Install-GFOpenUp, page 6): each module in LIBS:GFOpenLibs, loaded in one call of
; OS 3.2's own LoadModule, which restarts the Amiga once when they aren't in memory yet. Taking a file out of
; LIBS:GFOpenLibs puts the ROM's own back at the next start. MIT, Copyright (c) 2026 AmiCradle Team.
FailAt 21
If EXISTS LIBS:GFOpenLibs/utility.module
  If EXISTS LIBS:GFOpenLibs/input.device
    C:LoadModule LIBS:GFOpenLibs/utility.module LIBS:GFOpenLibs/input.device >NIL:
  Else
    C:LoadModule LIBS:GFOpenLibs/utility.module >NIL:
  EndIf
Else
  If EXISTS LIBS:GFOpenLibs/input.device
    C:LoadModule LIBS:GFOpenLibs/input.device >NIL:
  EndIf
EndIf
FailAt 10
"""


def openlibs_parts(top: Path, a) -> list:
    """Parts/GFOpenLibs (the startup script and S:Pre-LoadModule-Startup, which the Installer adds with the first
    library switched on and takes out with the last; it never writes over another program's
    Pre-LoadModule-Startup) and Parts/GFOpenLibs/<name>, one per library."""
    ol = ROOT / "build/openlibs"
    base = Part(top, "GFOpenLibs", "1.0", "GFOpenLibs: the startup that loads the open ROM libraries with OS 3.2's LoadModule")
    # OS 3.2's LoadModule is for Kickstart ROMs: on the GFOpenKick ROM S:Pre-LoadModule-Startup stopped the boot with
    # "device not mounted" (Instance-32, 20 October 2026), so GFOpenUpTool 0.8.7 leaves this part, and the ROM modules'
    # parts below, out there (as amicradle's openup_cradle.arm leaves the screen mode out after its GFOpenKick check)
    base.line("needs Kickstart")
    base.add("S/GFOpenLibs-Startup", OPENLIBS_STARTUP.encode("latin-1"), "S:GFOpenLibs-Startup", own=True)
    base.add("S/Pre-LoadModule-Startup", b"; GFOpenUp's GFOpenLibs (Install-GFOpenUp, page 6)\nExecute S:GFOpenLibs-Startup\n",
             "S:Pre-LoadModule-Startup", own=True)
    base.write()
    made = []
    for name, (built, rel, dest, vername, title) in OPENLIBS.items():
        f = ol / built
        if not f.is_file():
            print(f"  GFOpenLibs: {name} left out ({f} wasn't built)")
            continue
        data = f.read_bytes()
        ver = binary_version(data, vername)
        if not ver:
            raise SystemExit(f"GFOpenLibs: {f} has no {vername} version")
        p = Part(top, f"GFOpenLibs/{name}", ver, f"GFOpenLibs: {title}")
        p.lines[0] = f"part GFOpenLibs-{name} {ver}"
        if dest.startswith("LIBS:GFOpenLibs/"):         # a ROM module, loaded by OS 3.2's LoadModule
            p.line("needs Kickstart")
        p.add(rel, data, dest)
        if name == "input":
            p.add("C/InputProbe", (ol / "input/InputProbe").read_bytes(), "SYS:C/InputProbe")
            p.add("Utilities/GFOpenUp/Licences/input.device.txt", (a.openamigainput / "LICENSE").read_bytes(),
                  "SYS:Utilities/GFOpenUp/Licences/input.device.txt")
        if name == "rexx":
            sup = (ol / "openrexx/libs/rexxsupport.library").read_bytes()
            if binary_version(sup, "rexxsupport.library") != ver:
                raise SystemExit(f"GFOpenLibs: rexxsupport.library isn't {ver} as rexxsyslib.library is")
            p.add("Libs/rexxsupport.library", sup, "LIBS:rexxsupport.library")
            p.add("System/RexxMast", (ol / "openrexx/C/RexxMast").read_bytes(), "SYS:System/RexxMast", default=True)
        p.write()
        made.append(name)
    print(f"  GFOpenLibs: {', '.join(made)}")
    return made


BACKDROPS_DEST = "SYS:Prefs/Presets/Backdrops/GFOpenUp"
ART_NAME_RE = re.compile(r"^AmiCradle-.+-\d+x\d+\.png$")


def art_backdrop_names(opengadtools: Path) -> set[str] | None:
    """The AmiCradle Background Pack's pictures, as gfopengadtools' tools/make_art_backdrops.py names them
    (AmiCradle-<design>-<W>x<H>.png from its ART and SIZES, read from the source, not imported: it needs PIL).
    None when that file is not there or its lists are not plain literals."""
    src = Path(opengadtools) / "tools/make_art_backdrops.py"
    try:
        tree = ast.parse(src.read_text(encoding="utf-8"))
    except (OSError, SyntaxError):
        return None
    found = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name) \
                and node.targets[0].id in ("ART", "SIZES"):
            try:
                found[node.targets[0].id] = ast.literal_eval(node.value)
            except ValueError:
                return None
    if set(found) != {"ART", "SIZES"}:
        return None
    return {f"AmiCradle-{name}-{w}x{h}.png" for name, _ in found["ART"] for w, h in found["SIZES"]}


def picture_tone(path: Path) -> str:
    """A picture's tone, "dark" or "light", by GFOpenUpTool's own rule (src/openuptool.c, picture_tone): twelve
    rows from top to bottom, every eighth pixel, Rec. 709 luma; a mean under 115 (45 per cent of 255) is dark.
    It goes beside the picture as <picture>.tone, which GFOpenUpTool reads first, so the Amiga never decodes a
    big picture at boot to set the icons' labels."""
    from PIL import Image
    with Image.open(path) as im:
        im = im.convert("RGB")
        w, h = im.size
        px = im.load()
        total = count = 0
        for i in range(12):
            y = (h - 1) * i // 11
            for x in range(0, w, 8):
                r, g, b = px[x, y]
                total += (2126 * r + 7152 * g + 722 * b) // 10000
                count += 1
    return "dark" if count and total // count < 115 else "light"


def backdrops_part(top: Path, backdrops: Path, opengadtools: Path, tone=picture_tone) -> "Part":
    """The Backdrops part: GFOpenUp's own desktops (gfopengadtools' make_backdrops.py, the themes' pictures, strips and
    logo versions), each with its .tone, into SYS:Prefs/Presets/Backdrops/GFOpenUp.

    make_backdrops.py also exports the AmiCradle Background Pack's art (gfopengadtools #73). That pack is not
    GFOpenUp's: it is an optional install, from Cradle's Media Library or its own InstallBackgrounds, and GFOpenUp keeps
    its small default set (the user, 24 October 2026). Its pictures are left out: by make_art_backdrops.py's own
    names, and by their AmiCradle-<design>-<W>x<H>.png pattern. Both, because make_backdrops.py doesn't empty its
    folder: an export from before pack 1.1 leaves its 2560x1440 and 3840x2160 art there, and 1.1's names no longer
    list them. None of GFOpenUp's own pictures is named so."""
    art = art_backdrop_names(opengadtools) or set()
    bd = Part(top, "Backdrops", "0.3", "The themes' desktops and the Ethereal ones as Workbench backdrops, with their tones")
    for png in sorted(Path(backdrops).glob("*.png")):
        if png.name in art or ART_NAME_RE.match(png.name):
            continue
        bd.add(f"Backdrops/{png.name}", png.read_bytes(), f"{BACKDROPS_DEST}/{png.name}")
        bd.add(f"Backdrops/{png.name}.tone", (tone(png) + "\n").encode("ascii"), f"{BACKDROPS_DEST}/{png.name}.tone")
    return bd


def ogles3_credits(src: Path) -> bytes:
    """SYS:Utilities/GFOpenUp/Licences/ogles3.txt: ogles3.library's README (what it is, credits, licence) and the
    notice of ogles3's MIT SDK header, whose dispatch table it keeps (ogles3_api.txt), as Latin-1 text."""
    readme = (src / "README.md").read_text(encoding="utf-8")
    notice = []
    for line in (src / "ogles3_api.txt").read_text(encoding="utf-8").splitlines():
        if not line.startswith("#"):
            break
        notice.append(line[2:] if line.startswith("# ") else line[1:])
    text = readme.rstrip() + "\n\n## ogles3's SDK notice (ogles3_api.txt)\n\n" + "\n".join(notice).strip() + "\n"
    return text.encode("latin-1", errors="replace")


def ogles3_part(top: Path, built: Path, src: Path) -> Part | None:
    """Parts/OGLES3 from gfopenrtg library/ogles3's build (libs/ogles3.library and OGLES3Check), when it was
    built. Its version is the library's."""
    lib = built / "libs/ogles3.library"
    if not lib.is_file():
        print("  OGLES3: left out, ogles3.library wasn't built")
        return None
    data = lib.read_bytes()
    og = Part(top, "OGLES3", binary_version(data, "ogles3.library") or "1.0",
              "ogles3.library on GFOpenGPU: OpenGL ES 3.1 for ogles3 programs, on the PC's GPU (AmiCradle only)")
    og.line("needs AmiCradle")
    og.needs_part("GFOpenGPU")
    og.line('keep "LIBS:ogles3.library" "on GFOpenGPU"')
    og.add("Libs/ogles3.library", data, "LIBS:ogles3.library")
    og.add("C/OGLES3Check", (built / "OGLES3Check").read_bytes(), "C:OGLES3Check")
    og.add("Utilities/GFOpenUp/Licences/ogles3.txt", ogles3_credits(src), "SYS:Utilities/GFOpenUp/Licences/ogles3.txt")
    og.write()
    return og


# AkikoPlus (0.8.27): akikoplus.library 2 (gfosautoconfigboards #4, cards/akikoplus/guest), built with the os32-gcc16
# stove. --akikoplus is a folder laid out as the part: Libs/akikoplus.library, C/AkikoTest, C/APTest and
# Developer/AkikoPlus/ (the autodoc, SFD, FD, headers and boardonly/akikoplus.library, the AP_NO_FALLBACK build).
AKIKOPLUS_FILES = (
    ("Libs/akikoplus.library", "LIBS:akikoplus.library"),
    ("C/AkikoTest", "C:AkikoTest"),
    ("C/APTest", "C:APTest"),
    ("Developer/AkikoPlus/akikoplus.doc", "SYS:Developer/AkikoPlus/akikoplus.doc"),
    ("Developer/AkikoPlus/boardonly/akikoplus.library", "SYS:Developer/AkikoPlus/boardonly/akikoplus.library"),
    ("Developer/AkikoPlus/fd/akikoplus_lib.fd", "SYS:Developer/AkikoPlus/fd/akikoplus_lib.fd"),
    ("Developer/AkikoPlus/include/clib/akikoplus_protos.h", "SYS:Developer/AkikoPlus/include/clib/akikoplus_protos.h"),
    ("Developer/AkikoPlus/include/inline/akikoplus.h", "SYS:Developer/AkikoPlus/include/inline/akikoplus.h"),
    ("Developer/AkikoPlus/include/libraries/akikoplus.h", "SYS:Developer/AkikoPlus/include/libraries/akikoplus.h"),
    ("Developer/AkikoPlus/include/pragmas/akikoplus_pragmas.h", "SYS:Developer/AkikoPlus/include/pragmas/akikoplus_pragmas.h"),
    ("Developer/AkikoPlus/include/proto/akikoplus.h", "SYS:Developer/AkikoPlus/include/proto/akikoplus.h"),
    ("Developer/AkikoPlus/sfd/akikoplus_lib.sfd", "SYS:Developer/AkikoPlus/sfd/akikoplus_lib.sfd"),
)


def akikoplus_part(top: Path, src: Path | None) -> Part | None:
    """Parts/AkikoPlus from a built AkikoPlus drawer (--akikoplus). No needs line: without mitchy.library or the
    board every call runs on the 68k; Install-GFOpenUp installs it with GFOpenService, which brings mitchy.library.
    Its version is the library's."""
    if not src or not (src / "Libs/akikoplus.library").is_file():
        print("  AkikoPlus: left out, no --akikoplus with Libs/akikoplus.library")
        return None
    missing = [rel for rel, _ in AKIKOPLUS_FILES if not (src / rel).is_file()]
    if missing:
        raise SystemExit(f"AkikoPlus: {src} lacks " + ", ".join(missing))
    data = (src / "Libs/akikoplus.library").read_bytes()
    ak = Part(top, "AkikoPlus", binary_version(data, "akikoplus.library") or "2.0",
              "AkikoPlus: akikoplus.library, the media board's helpers (C2P, swaps, fills, scaling, YUV, colours, "
              "zlib) on the board when fitted, on the 68k when not; AkikoTest and APTest")
    for rel, dest in AKIKOPLUS_FILES:
        ak.add(rel, (src / rel).read_bytes(), dest)
    ak.write()
    return ak


# The slim ROM's disk part (9 November 2026; amicradle-guest docs/design/thin-loader.md). The thin ROM keeps what a
# machine needs to reach its disk and loads the rest from SYS:Kickstart/default/ (kickload, once per power-on); the
# files the full ROM held on demand are files on the disk. trial-1m/build.sh --profile thin lays both out as
# OUT/thin-support/ from the same build as the ROM (tools/openkickrom/thinsupport.py), with MANIFEST.txt (path, bytes,
# SHA-256, version, build output) and Kickstart/default/Manifest (kickload's). --thinsupport is that folder.
KICKSET_MAN_HEAD = "# GFOpenKickLoad manifest "


def gfopenmui_build_env(mui_out: Path, base) -> dict:
    """The environment for gfopenmui's muimaster/build-amiga.sh in a pack build: GCC 16 open6, GFOpenLayout in the
    script's own deps folder, out to mui_out, and LAB_PROGRAMS=0, so the lab's AROS programs (APL, never packed) are
    neither fetched nor built."""
    return dict(base, STOVE=base.get("OS32_GCC16_OPEN6", str(Path.home() / "AmiCradle/stoves/os32-gcc16-open6")),
                GFOPENLAYOUT=str(mui_out / "deps/gfopenlayout"), OUT=str(mui_out), LAB_PROGRAMS="0")


def gfopenmui_part(top: Path, gfopenmui: Path, icons) -> Part | None:
    """Parts/GFOpenMUI from gfopenmui main's clean-room muimaster.library (muimaster/build-amiga.sh, built by
    build_parts into muimaster/build-amiga/): the library, only where no muimaster.library is or GFOpenUp's own is
    ("add": a real MUI stays), and its three examples in SYS:Tools/GFOpenMUI. Zune (AROS Public License) left
    gfopenmui main at 9bf57d2; it is on reference/zune only, and nothing of it is packed. Without a build the part
    is left out, said in one line. The part's version is the library's."""
    b = gfopenmui / "muimaster" / "build-amiga"
    lib = b / "lib" / "muimaster.library"
    if not lib.is_file():
        print(f"  GFOpenMUI: left out, no {lib} (build gfopenmui's muimaster/build-amiga.sh)")
        return None
    data = lib.read_bytes()
    ver = binary_version(data, "muimaster.library")
    if not ver:
        raise SystemExit(f"GFOpenMUI: {lib} has no muimaster.library version string")
    om = Part(top, "GFOpenMUI", ver, f"GFOpenMUI: muimaster.library {ver}, the AmiCradle Team's own MUI on GFOpenLayout, "
                                     "where no MUI is, and its examples")
    om.add("Libs/muimaster.library", data, "LIBS:muimaster.library", own=True)
    for ex in ("MUIClasses", "MUIAddons", "MUITime"):
        f = b / "tests" / ex
        if f.is_file():
            om.add(f"Tools/GFOpenMUI/{ex}", f.read_bytes(), f"SYS:Tools/GFOpenMUI/{ex}")
            if icons:
                om.add(f"Tools/GFOpenMUI/{ex}.info", icons.icon(icons.WBTOOL, "install", stack=16384),
                       f"SYS:Tools/GFOpenMUI/{ex}.info")
    om.write()
    return om


def kickstartset_part(top: Path, support: Path | None) -> Part | None:
    """Parts/KickstartSet from a thin build's thin-support/. The set's modules are copied over whatever is there (a
    set is one unit: kickload refuses a set whose files aren't its Manifest's), the Manifest last, so a copy cut
    short is never taken for the set; GFOpenUpTool KICKSET moves an older set's module the new Manifest doesn't
    list aside. The files on demand are "add": where the disk has none or GFOpenUp put it, so a disk's own stays.
    Its version is the ROM's (SOURCES.txt beside thin-support/: "GFOpenKick 47.100 full41-thin")."""
    import hashlib
    if not support:
        print("  KickstartSet: left out, no --thinsupport")
        return None
    support = Path(support)
    kick = support / "Kickstart/default"
    man = kick / "Manifest"
    listing = support / "MANIFEST.txt"
    if not man.is_file() or not listing.is_file():
        raise SystemExit(f"KickstartSet: {support} is no thin-support/ (no Kickstart/default/Manifest or MANIFEST.txt)")
    lines = man.read_text(encoding="latin-1").splitlines()
    if not lines or not lines[0].startswith(KICKSET_MAN_HEAD):
        raise SystemExit(f"KickstartSet: {man} isn't a GFOpenKickLoad Manifest")
    modules = []
    v2 = False
    for line in lines:
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        if w[0] == "manifest":
            if w != ["manifest", "2"]:
                raise SystemExit(f"KickstartSet: {man} has a version line that isn't 'manifest 2'")
            v2 = True
            continue
        need = 6 if v2 else 5
        f = kick / w[0]
        data = f.read_bytes() if f.is_file() else b""
        if len(w) != need or len(data) != int(w[3]) or hashlib.sha256(data).hexdigest() != w[4].lower():
            raise SystemExit(f"KickstartSet: {f} isn't the module its Manifest lists")
        modules.append((w[0], data))
    rom_id, version = "", "47.100"
    sources = support.parent / "SOURCES.txt"
    if sources.is_file():
        for line in sources.read_text(encoding="latin-1", errors="replace").splitlines()[:30]:
            if "GFOpenKick " in line:
                words = line[line.index("GFOpenKick "):].split()
                if len(words) >= 3 and words[1][:1].isdigit():
                    version, rom_id = words[1], words[2].strip('"')
                    break
    ks = Part(top, "KickstartSet", version,
              f"KickstartSet: the slim ROM's module set and its files on the disk{' (' + rom_id + ')' if rom_id else ''}: "
              f"{len(modules)} modules in SYS:Kickstart/default/")
    for name, data in modules:
        ks.add(f"Kickstart/default/{name}", data, f"SYS:Kickstart/default/{name}")
    files = 0
    for line in listing.read_text(encoding="latin-1").splitlines():
        if line.startswith("#") or not line.strip():
            continue
        rel = line.split("\t")[0]
        if rel.startswith("Kickstart/"):
            continue
        f = support / rel
        if not f.is_file():
            raise SystemExit(f"KickstartSet: {f} (in MANIFEST.txt) isn't there")
        ks.add(rel, f.read_bytes(), "SYS:" + rel, own=True)
        files += 1
    ks.add("Kickstart/default/Manifest", man.read_bytes(), "SYS:Kickstart/default/Manifest")
    ks.write()
    print(f"  KickstartSet {version}: {len(modules)} modules and {files} files on demand{' (' + rom_id + ')' if rom_id else ''}")
    return ks


def check_part_versions(parts: Path) -> None:
    """Stops the build when a part's declared version is not its main
    binary's $VER, or when a part says neither which binary that is nor
    that it has a version of its own."""
    wrong = []
    for pf in sorted(parts.glob("*/Part")):
        name, version = pf.read_text(encoding="latin-1").split("\n", 1)[0].split()[1:3]
        if name in PART_OWN_VERSION:
            continue
        if name not in PART_MAIN:
            wrong.append(f"{name} {version}: no main binary in PART_MAIN (or add it to PART_OWN_VERSION)")
            continue
        rel, vername = PART_MAIN[name]
        f = pf.parent / rel
        got = binary_version(f.read_bytes(), vername) if f.is_file() else None
        if got != version:
            wrong.append(f"{name}: the part says {version}, {rel} says {got or 'no version'}")
    # GFOpenLibs' libraries, one part each in Parts/GFOpenLibs/<name>: the version is the library's own
    for pf in sorted(parts.glob("GFOpenLibs/*/Part")):
        name, version = pf.read_text(encoding="latin-1").split("\n", 1)[0].split()[1:3]
        _, rel, _, vername, _ = OPENLIBS[name.split("-", 1)[1]]
        f = pf.parent / rel
        got = binary_version(f.read_bytes(), vername) if f.is_file() else None
        if got != version:
            wrong.append(f"{name}: the part says {version}, {rel} says {got or 'no version'}")
    if wrong:
        raise SystemExit("part versions don't match what they ship:\n  " + "\n  ".join(wrong))
    print(f"  part versions: all {sum(1 for _ in parts.glob('*/Part'))} match their programs")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--version", default="0.1", type=pack_version,
                    help="the pack's version: digits and dots, and a release candidate's -rcN (0.8.29-rc1)")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "pack")
    ap.add_argument("--amigachrome", type=Path, default=first(DEV / "amicradle", DEV / "wt-deploy"))
    ap.add_argument("--openamigaprint", type=Path, default=first(DEV / "gfopenprint", DEV / "review/gfopenprint"))
    ap.add_argument("--opengadtools", type=Path, default=first(DEV / "gfopengadtools", DEV / "opengadtools-export"))
    ap.add_argument("--guest", type=Path, default=first(DEV / "amicradle-guest", DEV / "guest"))
    ap.add_argument("--openamigaprefs", type=Path, default=first(DEV / "gfopenprefs"))
    ap.add_argument("--openamigacompress", type=Path, default=first(DEV / "gfopencompress"))
    ap.add_argument("--openamigaservice", type=Path, default=first(DEV / "gfopenservice", DEV.parent / "gfopenservice"))
    ap.add_argument("--opendatatypes", "--openamigaimage", dest="opendatatypes", type=Path,
                    default=first(DEV / "gfopendatatypes", DEV.parent / "gfopendatatypes"),
                    help="gfopendatatypes, for the GFOpenImage part's datatypes (every datatype has lived there since 23 October 2026; --openamigaimage is the old name of this option)")
    ap.add_argument("--openamigartg", type=Path, default=first(DEV / "gfopenrtg", DEV.parent / "gfopenrtg"))
    ap.add_argument("--openamigafilemanager", type=Path,
                    default=first(DEV / "gfopenfilemanager", DEV.parent / "gfopenfilemanager"))
    ap.add_argument("--openamigamedia", type=Path, default=first(DEV / "gfopenmedia", DEV.parent / "gfopenmedia"),
                    help="gfopenmedia, which GFOpenFiles' build needs (its MEDIA)")
    ap.add_argument("--openamigamediaplayer", type=Path,
                    default=first(DEV / "gfopenmediaplayer", DEV.parent / "gfopenmediaplayer"))
    ap.add_argument("--openamigamui", type=Path, default=first(DEV / "gfopenmui", DEV.parent / "gfopenmui"))
    ap.add_argument("--openamigaedit", type=Path, default=first(DEV / "gfopenedit", DEV.parent / "gfopenedit"))
    ap.add_argument("--gfoffice", "--openamigawrite", dest="gfoffice", type=Path,
                    default=first(DEV / "gfoffice", DEV.parent / "gfoffice", DEV / "openamigawrite", DEV.parent / "openamigawrite"),
                    help="gfoffice, the GFOffice Suite: GFOpenWrite and the suite's other apps (the repository was openamigawrite until 25 October 2026; --openamigawrite is the old name of this option, and a checkout still in a folder of that name is found)")
    ap.add_argument("--openamigamulticore", type=Path, default=first(DEV / "gfopenmulticore", DEV.parent / "gfopenmulticore"))
    ap.add_argument("--openamigamail", type=Path, default=first(DEV / "gfopenmail", DEV.parent / "gfopenmail"))
    ap.add_argument("--openbrowser", type=Path, help="a built OpenBrowser release (.lha or its unpacked folder): "
                    "makes GFOpenUp-<version>-OpenBrowser.lha beside the pack")
    ap.add_argument("--openamigalayout", type=Path, default=first(DEV / "gfopenlayout", DEV.parent / "gfopenlayout"),
                    help="gfopenlayout, GFOpenWrite's pages (GFOpenLayout)")
    ap.add_argument("--openamigamesa", type=Path, default=first(DEV / "gfopenmesa", DEV.parent / "gfopenmesa"),
                    help="gfopenmesa, for the GFOpenDemos part (left out without it)")
    ap.add_argument("--openamigatls", type=Path, default=first(DEV / "gfopentls", DEV.parent / "gfopentls"),
                    help="gfopentls, for the GFOpenCrypto and GFOpenTLS parts (left out without it)")
    ap.add_argument("--openamigaputty", type=Path, default=first(DEV / "gfopenputty", DEV.parent / "gfopenputty"),
                    help="gfopenputty, for the GFOpenPuTTY part (needs --openamigatls; left out without it)")
    ap.add_argument("--openamigatata", type=Path, default=first(DEV / "gfopentata", DEV.parent / "gfopentata"),
                    help="gfopentata, for the Tata part (left out without it)")
    ap.add_argument("--openamigaftp", type=Path, default=first(DEV / "gfopenftp", DEV.parent / "gfopenftp"),
                    help="gfopenftp, for the GFOpenFTP part (left out without it)")
    ap.add_argument("--openbrowserlite", type=Path, default=None,
                    help=f"OpenBrowser Lite's release archive {OPENBROWSERLITE['asset']} (default: build/'s, "
                         f"else fetched from {OPENBROWSERLITE['url']} into build/), for the OpenBrowserLite "
                         "part, taken only with its pinned sha256; the pack stops without it")
    ap.add_argument("--openamigatelegram", type=Path, default=first(DEV / "gfopentelegram", DEV.parent / "gfopentelegram"),
                    help="gfopentelegram, for the Telegram part (left out without it)")
    ap.add_argument("--openamigainput", type=Path, default=first(DEV / "gfopeninput", DEV.parent / "gfopeninput"),
                    help="gfopeninput, for the GFOpenInput part (left out without it)")
    ap.add_argument("--openamigasocket", type=Path, default=first(DEV / "gfopensocket", DEV.parent / "gfopensocket"),
                    help="gfopensocket, for the GFOpenSocket part's files, built here (without it, amicradle's "
                         "vendored payload, which can be older than gfopensocket's main)")
    ap.add_argument("--openamigaphysfs", type=Path, default=first(DEV / "gfopenphysfs", DEV.parent / "gfopenphysfs"),
                    help="gfopenphysfs: physfs.library for the GFOpenGPU part and the kit, built at GFOPENPHYSFS's commit")
    ap.add_argument("--openamigabench", type=Path, default=first(DEV / "gfopenbench", DEV.parent / "gfopenbench"),
                    help="gfopenbench checkout: GFOpenBench, the Team's own Workbench (desktop/build.sh)")
    ap.add_argument("--openamigafont", type=Path, default=first(DEV / "gfopenfont", DEV.parent / "gfopenfont"),
                    help="gfopenfont checkout: the ShellFont part, DejaVu Sans Mono as Amiga bitmap fonts "
                         "(its fonts/amiga; left out without it), and the GFOpenFont part's gfopenfont.library "
                         "(its build-amiga.sh's build/amiga/gfopenfont.library; left out without it)")
    ap.add_argument("--gfosfonts", type=Path, default=first(DEV / "gfosfonts", DEV.parent / "gfosfonts"),
                    help="gfosfonts checkout: the GFOSFonts part, GFOS Sans, Sans Bold and Mono 11/13/16 built from its "
                         "src/*.txt grids by its tools/amigafont.py (left out without it)")
    ap.add_argument("--dejavu-serif", type=Path, default=Path("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf"),
                    help="DejaVu fonts 2.37's DejaVuSerif.ttf (sha256 checked): GFOpenBlanker's serif where OS 3.2's CGTimes isn't")
    ap.add_argument("--dejavu-sans", type=Path, default=Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
                    help="DejaVu fonts 2.37's DejaVuSans.ttf, for the GFOpenFont part (default: the build PC's "
                         "fonts-dejavu-core); taken only with its sha256")
    py_asset = ROOT / "build" / OPENAMIGAPYTHON["asset"]
    ap.add_argument("--openamigapython", type=Path, default=py_asset if py_asset.is_file() else None,
                    help=f"OpenAmigaPython's release archive {OPENAMIGAPYTHON['asset']} (from "
                         f"{OPENAMIGAPYTHON['release']}; build/ is looked in first), for the "
                         "OpenAmigaPython part, taken only with its pinned sha256 (left out without it)")
    ap.add_argument("--thinsupport", type=Path, default=None,
                    help="a thin ROM build's OUT/thin-support/ (amicradle-guest trial-1m/build.sh --profile thin), for the "
                         "KickstartSet part (left out without it)")
    ap.add_argument("--akikoplus", type=Path, default=None,
                    help="a built AkikoPlus drawer (Libs/akikoplus.library, C/AkikoTest, C/APTest, Developer/AkikoPlus/): "
                         "gfosautoconfigboards cards/akikoplus/guest's build; the part is left out without it")
    ap.add_argument("--opengpu-sdk", type=Path, default=None,
                    help="where the GFOpenGPU developer kit is made (or, with --no-build, read): the GFOpenGPUDev part (left out without it)")
    ap.add_argument("--no-build", action="store_true", help="use what the parts' builds already made")
    ap.add_argument("--no-openicons", action="store_true", help="leave out the GFOpenIcons part (made from --guest's icons/openicons)")
    ap.add_argument("--lha", action="store_true", help="also make GFOpenUp-<version>.lha with the host's lha")
    a = ap.parse_args()
    for name in ("amigachrome", "openamigaprint", "opengadtools", "guest", "openamigaprefs", "openamigacompress", "openamigaservice",
                 "opendatatypes", "openamigartg", "openamigafilemanager", "openamigamediaplayer",
                 "openamigamui", "openamigaedit", "gfoffice", "openamigalayout"):
        if not getattr(a, name) or not getattr(a, name).is_dir():
            ap.error(f"--{name}: give the checkout's folder")
    check_openbrowserlite_pin()            # before hours of building, not after
    if a.openbrowser:
        check_openbrowser_pin(a.openbrowser)
    if not a.no_build:
        build_parts(a)
    icons = icon_maker(a.openamigaprint)
    date = time.strftime("%d.%m.%Y").lstrip("0").replace(".0", ".")
    top = a.out / "GFOpenUp"
    if top.exists():
        shutil.rmtree(top)
    top.mkdir(parents=True)
    files: list[tuple[str, bytes]] = []   # for the OS 3.2-style icon pass

    def put(rel: str, data: bytes):
        p = top / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        files.append((rel, data))

    tool = (ROOT / "build/os3/GFOpenUpTool").read_bytes()
    for script in ("Install-GFOpenUp", "Uninstall-GFOpenUp"):
        text = (ROOT / "pack" / script).read_text(encoding="latin-1").replace("@VERSION@", a.version).replace("@DATE@", date)
        put(script, text.encode("latin-1"))
        # NOLOG: the Installer's own install_log_file (and its icon) went in SYS: or Work: on every run, in no
        # list, so it stayed after Uninstall-GFOpenUp; what GFOpenUp does is in T:GFOpenUp.log and the manifest.
        # The Installer (47.19) lists NOLOG as a tooltype too, but in the labs a start from this icon still
        # wrote the log; NOLOG on the command line (GFOpenUpTool UPDATE) does stop it
        put(script + ".info", icons.icon(icons.WBPROJECT, "install", "Installer",
                                          [f"APPNAME=GFOpenUp", "MINUSER=NOVICE", "DEFUSER=NOVICE", "NOLOG"]))
    put("C/GFOpenUpTool", tool)
    readme = (ROOT / "gfopenup.readme").read_bytes()
    put("GFOpenUp.readme", readme)

    # GFOpenUp itself: the tool and the uninstaller, where they can be found later,
    # and GFOpenUpMenu: Workbench's Tools menu gets a GFOpenUp sub-menu (the
    # programs) and a Preferences one (the settings editors), each showing only
    # what is installed (DESIGN.md section 7). Started at boot; it waits for
    # Workbench, as S:User-Startup runs before LoadWB.
    core = Part(top, "GFOpenUp", a.version, "GFOpenUpTool, the uninstaller, ACROMFix, GFOpenUp's programs and settings in Workbench's Tools menu, and GFOpenMonitor")
    core.add("C/GFOpenUpTool", tool, "SYS:C/GFOpenUpTool")
    acromfix = ROOT / "build/acromfix/C/ACROMFix"
    if not acromfix.is_file():
        raise SystemExit(f"{acromfix} wasn't built (amicradle-guest tools/acromfix/build-amiga.sh)")
    acromfix_lines(core, acromfix.read_bytes())
    core.add("C/GFOpenUpMenu", (ROOT / "build/os3/GFOpenUpMenu").read_bytes(), "SYS:C/GFOpenUpMenu")
    core.line("startup Run >NIL: C:GFOpenUpMenu")
    # AmigaOS 3.2.3's own icons and their default tools (names and tool strings): FIXTOOLS puts back the ones
    # GFOpenUp 0.6.14 to 0.7.0 changed to GFOpenView by mistake, on the 0.7.1 upgrade and on UNINSTALL
    core.add("StockDefaultTools", (ROOT / "pack/StockDefaultTools").read_bytes(), "ENVARC:GFOpenAmiga/StockDefaultTools")
    un = (top / "Uninstall-GFOpenUp").read_bytes()
    core.add("Utilities/GFOpenUp/Uninstall-GFOpenUp", un, "SYS:Utilities/GFOpenUp/Uninstall-GFOpenUp")
    core.add("Utilities/GFOpenUp/Uninstall-GFOpenUp.info", (top / "Uninstall-GFOpenUp.info").read_bytes(),
             "SYS:Utilities/GFOpenUp/Uninstall-GFOpenUp.info")
    core.add("Utilities/GFOpenUp.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)), "SYS:Utilities/GFOpenUp.info", own=True)
    # GFOpenMonitor, the task manager: tasks, CPU and the cores, memory and the GPU, and their actions; on every ROM
    # until 1.0 goes into it; CommandSwap's pins go on the open ROM (openup_rom_lines)
    openup_rom_lines(core, (ROOT / "build/os3/GFOpenMonitor").read_bytes(), icons.icon(icons.WBTOOL, "install", stack=32768))
    # softfpu.library (amicradle-guest libraries/opensoftfpu): the FPU in software, out of the 1M open ROM since
    # W61 (no room). On a 68LC040 or 68EC040 the open ROM's dos opens it from LIBS: before the Startup-Sequence
    # (the F-line emulator) and the open maths libraries open it by name; with an FPU, or under OS 3.2's own ROM,
    # nothing opens it. In every install, as the CPU can change under the same disk.
    sfl = ROOT / "build/opensoftfpu/libs/softfpu.library"
    if not sfl.is_file():
        raise SystemExit(f"{sfl} wasn't built (amicradle-guest libraries/opensoftfpu/build-amiga.sh)")
    core.add("Libs/softfpu.library", sfl.read_bytes(), "LIBS:softfpu.library")
    openkick_disk_lines(core, ROOT / "build/openkickdisk")
    openkick_classes_lines(core, ROOT / "build/openkickclasses", (a.guest / "LICENSE").read_bytes())
    core.write()

    # GFOpenSocket: the files and places amicradle's vendored payload's manifest lists (the same list Cradle's own
    # installer uses), each from this build of gfopensocket (build_parts) when there is one, else the vendored
    # file. The version check reads only bsdsocket.library, so a stale GFOpenSocketControl in the vendored payload
    # went into 0.7.5 unseen; the line below says where the part's files came from.
    payload = a.amigachrome / "vendor/opensocket-os32"
    m = json.loads((payload / "manifest.json").read_text())
    so_out = ROOT / "build/opensocket" if a.openamigasocket else None
    sock = Part(top, "GFOpenSocket", None, "bsdsocket.library through the x86 or ARM64 cores (" + m.get("identity", "") + ")")
    sock.line("needs AmiCradle")
    sock.line('keep "SYS:Libs/bsdsocket.library" ' + " ".join(f'"{x}"' for x in OPENSOCKET_MARKERS))
    for f in m["files"]:
        if f.get("installPath"):
            path, place = f["path"], f["installPath"]
            if so_out:
                # gfopensocket's own build has the GF names since its rename (gfopensocket.library, .device,
                # GFOpenSocketControl, C:GFOpenSocket), and its programs open those; the vendored manifest still
                # lists the old ones, so the build's files go in under the GF names in the manifest's drawers
                path, place = gf_socket_name(path), gf_socket_name(place)
            src = so_out / Path(path).name if so_out else payload / path
            if not src.is_file():
                raise SystemExit(f"GFOpenSocket: {path} wasn't built ({src})")
            sock.add(path, src.read_bytes(), "SYS:" + place)
    sock.write()
    osc = next(c for c in (sock.dir / "Tools/Commodities/GFOpenSocketControl",
                           sock.dir / "Tools/Commodities/OpenSocketControl") if c.is_file()).read_bytes()
    print(f"  GFOpenSocket: {'built from ' + str(a.openamigasocket) if so_out else 'the vendored payload (gfopensocket ' + m.get('source', {}).get('commit', '?') + ')'}; "
          f"GFOpenSocketControl {binary_version(osc, 'GFOpenSocketControl') or '(no version)'}")

    # GFOpenPrint and GFOpenView: gfopenprint's own package, laid out as its Installer does
    pkg = a.openamigaprint / "build/package/GFOpenPrint"
    pr = Part(top, "GFOpenPrint", "0.4", "GFOpenPrint and GFOpenView: printing to PDF and IPP, and viewing; GFOpenView is MultiView's replacement")
    for p in sorted(pkg.rglob("*")):
        if not p.is_file():
            continue
        rel = p.relative_to(pkg).as_posix()
        if rel.startswith("C/"): dest = "SYS:C/" + rel[2:]
        elif rel.startswith("Devs/"): dest = "SYS:Devs/" + rel[5:]
        elif rel.startswith("WBStartup/"): dest = "SYS:WBStartup/" + rel[10:]
        elif rel.startswith("Drawer/"): dest = "SYS:Utilities/GFOpenPrint/" + rel[7:]
        elif rel == "Icons/GFOpenPrint.info": dest = "SYS:Utilities/GFOpenPrint.info"
        else: continue                    # its own Installer script and ReadMe
        pr.add(rel, p.read_bytes(), dest)
    # GFOpenView stands in MultiView's place (GFOpenUp 0.7.2): a copy at SYS:Utilities/MultiView. Everything that runs
    # MultiView (default tools, Help, programs, scripts) then runs GFOpenView, and no icon is edited. The original is
    # kept by the usual rule for a replaced file (SYS:Storage/GFOpenUp-Superseded) and Uninstall-GFOpenUp puts it back.
    # A copy, not a launcher or a link: it works on every file system, passes Workbench's and the Shell's arguments
    # untouched, and needs no second program.
    pr.add("Drawer/MultiView", (pkg / "Drawer/GFOpenView").read_bytes(), "SYS:Utilities/MultiView")
    pr.write()

    # GFOpenPrinter: our printer.device in front of GFOpenPrint, and the OS's printing archived (OPENPRINTER_MARKERS)
    opd = ROOT / "build/openprinter/devs/printer.device"
    if not opd.is_file():
        raise SystemExit(f"{opd} wasn't built (amicradle-guest devices/openprinter.device/build-amiga.sh)")
    openprinter_part(top, opd.read_bytes()).write()

    # AmiCradle sharing: the control device, the clipboard and the files tools
    gout = a.guest / "build/guest/os32"
    icondir = a.amigachrome / "resources/icons/collections/amigachrome-guest/os3"
    sh = Part(top, "AmiCradleSharing", "1.4", "accontrol.device, ACClip and ACDrop: the clipboard and files, in and out of the Amiga (Send Out icon optional)")
    sh.line("needs AmiCradle")
    sh.add("Devs/accontrol.device", (gout / "accontrol.device").read_bytes(), "SYS:Devs/accontrol.device")
    for tool_name in ("ACClip", "ACDrop"):
        sh.add(f"C/{tool_name}", (gout / tool_name).read_bytes(), f"SYS:C/{tool_name}")
        sh.add(f"WBStartup/{tool_name}.info", (icondir / f"{tool_name}.info").read_bytes(), f"SYS:WBStartup/{tool_name}.info")
    sh.write()

    # HostClock: battclock.resource is the host's clock (Dale, 9 Nov 2026: "battclock is the way, even for a 3.2.3
    # ROM"). C:HostClock is GFOpenBattClock 47.101 as a command: early in the boot it takes the Kickstart's
    # battclock.resource off the list and puts itself there (its code stays), so ReadBattClock, SetClock and
    # timer.device read the host's time through ACBridge's $4C, and its own task sets the system time again on
    # each snapshot resume and each step of the host's clock ($48). It replaces ACClock (amicradle-guest #650),
    # which was never in a part.
    hostclock = ROOT / "build/hostclock/resources/HostClock"
    if not hostclock.is_file():
        raise SystemExit(f"{hostclock} wasn't built (amicradle-guest resources/openbattclock.resource/build-amiga.sh)")
    hc = Part(top, "HostClock", binary_version(hostclock.read_bytes(), "HostClock") or "0",
              "HostClock: the Amiga's battery clock is the host's clock, and the Amiga's time follows it")
    hc.line("needs AmiCradle")
    hc.add("C/HostClock", hostclock.read_bytes(), "SYS:C/HostClock")
    hc.line("startup C:HostClock QUIET")
    hc.write()

    # GFOpenLook (C:GFOpenLook, its themes, GFOpenPrefs Look and its profiles) was a part up to GFOpenUp 0.8.1. Since 0.8.2
    # Workbench's look is the OS's own (the GFOpenKick ROM's: the user, 16 October 2026, "it should be default,
    # straight from the rom ... only the background will be the one that gets the attention"), and the
    # Installer's OLDLOOK takes out what an earlier GFOpenUp wrote for it.
    o = a.opengadtools

    # The background pack: each theme's desktop at four screen shapes (gfopengadtools), with their tones
    backdrops_part(top, o / "build/backdrops", o).write()

    # GFOpenIcons: the Team's own icon set (look B, MIT) as new pictures for the system's own icons
    # (ICONS.md section 8). Its .info files are the generator's build output, never committed: they are made
    # here from the amicradle-guest checkout the pack is built with (--guest), so nothing is fetched from
    # outside our repositories and the pack's icons are that commit's.
    if not a.no_openicons:
        sys.path.insert(0, str(ROOT / "tools"))
        import openicons_part
        oi = Part(top, "GFOpenIcons", "0.4", "GFOpenIcons, the Team's own icon set: new pictures for the system's drawers, programs and kinds of file")
        for text in openicons_part.build(oi.dir, a.guest):
            oi.line(text)
        oi.write()

    # GFOpenPrefs in SYS:Preferences (openprefs_part): with every install, before the parts whose pages it has
    opb = a.openamigaprefs / "OpenPrefs/build/os3"
    opapp = (opb / "GFOpenPrefs").read_bytes()
    opver = binary_version(opapp, "GFOpenPrefs") or "2.0"
    themes = [(t.name, t.read_bytes()) for t in sorted((a.opengadtools / "themes").glob("*.theme"))]
    profiles = [(f.name, f.read_bytes()) for f in sorted((a.openamigaprefs / "Look/profiles").glob("*.profile"))]
    if not themes or not profiles:
        sys.exit(f"build_pack: no themes in {a.opengadtools}/themes or no profiles in {a.openamigaprefs}/Look/profiles")
    opp = openprefs_part(top, opapp, icons, opver, themes, profiles)
    opp.write()

    # GFOpenTypes: which program opens each kind of file (gfopenprefs); since W48 a page of GFOpenPrefs, and since the
    # no-ROM-volume design (24 Oct 2026) its icon is SYS:Preferences/File Types (the GFOpenPrefs part). The part
    # stays, with no files, so the Installer's choice keeps its meaning; it archives its old name's stub.
    ty = Part(top, "GFOpenTypes", opver, "GFOpenTypes: which program opens each kind of file (GFOpenPrefs's File Types page)")
    ty.line('retire "SYS:Prefs/OpenTypes"')
    ty.write()

    # GFOpenCompress: one program, the window from Workbench and the command in
    # the Shell (SYS:Utilities is in the Shell's path)
    # 0.3.3 (0.8.19): inflate through z.library at run time (gfopencompress #24), 53.12 refused
    compress_part(top, a.openamigacompress, icons)

    # GFOpenService: the services device and Nursery (gfopenservice). It
    # does nothing on the LAN until ENV:GFOpenService/Paired lists a Cradle.
    # 1.2 (0.8.17): MEDIA:, gfopenservice's read-only DOS handler on top of media.decode/1 (L:media-handler and its
    # mountlist DEVS:DOSDrivers/MEDIA, mounted at boot; docs/MEDIA_HANDLER.md), and ServiceTest, which lists the
    # services and opens one. A MEDIA mountlist of the user's own stays.
    # 1.3 (0.8.27): LIBS:mitchy.library 1.0, the services card's (Mitchy's) library for every program, on the
    # device (gfopenservice #34), and ServiceTest 0.2 on it, with one checked job for each service on offer.
    # 1.4: the services card is Mitchy, and the device mitchy.device (gfopenservice, team3/mitchy-device-20261111). gfopenservice.device
    # stays as a small device that opens it, for programs (and mitchy.library) built with the old name: the two go
    # together, as the stub needs mitchy.device.
    sv = Part(top, "GFOpenService", "1.4", "GFOpenService: services from AutoConfig boards and paired Cradles, and MEDIA:")
    sv.add("Devs/mitchy.device", (a.openamigaservice / "bin/mitchy.device").read_bytes(), "DEVS:mitchy.device")
    sv.add("Devs/gfopenservice.device", (a.openamigaservice / "bin/gfopenservice.device").read_bytes(), "DEVS:gfopenservice.device")
    sv.add("Libs/mitchy.library", (a.openamigaservice / "bin/mitchy.library").read_bytes(), "LIBS:mitchy.library")
    sv.add("C/Nursery", (a.openamigaservice / "bin/Nursery").read_bytes(), "C:Nursery")
    sv.add("C/ServiceTest", (a.openamigaservice / "bin/ServiceTest").read_bytes(), "C:ServiceTest")
    sv.add("L/media-handler", (a.openamigaservice / "bin/media-handler").read_bytes(), "L:media-handler")
    sv.add("Devs/DOSDrivers/MEDIA", (a.openamigaservice / "devs/MEDIA").read_bytes(), "DEVS:DOSDrivers/MEDIA", own=True)
    sv.write()

    # AkikoPlus 2.0 (0.8.27): akikoplus.library (gfosautoconfigboards #4), on AmiCradle's media board through
    # mitchy.library when it is fitted, on the 68k when not; Install-GFOpenUp installs it with GFOpenService.
    akikoplus_part(top, a.akikoplus)
    kickstartset_part(top, a.thinsupport)

    # GFOpenImage: datatypes (gfopendatatypes). PNG, Targa (opentga.datatype), WebP, WebM and music modules
    # (openmodule.datatype: MOD, MED, Oktalyzer, DigiBooster, XM, S3M, IT,
    # played by libxmp on the Amiga itself) decode on the 68k; the rest ask
    # media.decode/1 or doc.render/1 through GFOpenService. Every class and
    # descriptor Datatypes/build.sh makes goes in. A
    # datatype or descriptor already there (Aminet's flac.datatype, say)
    # stays: ours only go where nothing is or where GFOpenUp put them (so a
    # newer pack updates them), and a machine without a Cradle keeps what it
    # had. Their own classes have names nobody else uses.
    # The files are the GFOpenDatatypes pack's (gfopendatatypes' tools/build_pack.py, the user's word of 23 October
    # 2026: "GFOpenUp uses the pack"), laid out by it from what Datatypes/build.sh made and read from its manifest, so
    # GFOpenImage and the pack never differ. Its datatypes.library and system classes stay out: an open library goes
    # in GFOpenLibs, off by default, once its ABI test is clean (below). A checkout without the pack builder yet
    # gives Datatypes/out as before.
    # 0.5 (0.8.19): GIF and IFF ANIM saving, alpha, z.library's Aminet LVOs, WebP save (gfopendatatypes #28-#32)
    # 0.6: LIBS:gfopensound-decoders.library, gfopensound.datatype 47.3's Ogg Vorbis, FLAC and MP3 decoders on the
    # 68k (Dale, 9 November 2026): the ROM's gfopensound.datatype carries none and sends those formats to
    # media.decode/1 through mitchy.library (the services card); without the card it opens this library, which
    # the pack installs with gfopensound's choice (its CLASS_LIBS), so a disk install always has it.
    img = Part(top, "GFOpenImage", "0.6", "GFOpenImage: datatypes for today's pictures (PNG and Targa on the Amiga itself), sounds, music modules, documents and video")
    packer = a.opendatatypes / "tools/build_pack.py"
    if packer.is_file():
        pk = ROOT / "build/gfopendatatypes"
        run([sys.executable, packer, "--no-build", "--out", pk, "--library-out", pk / "no-library",
             "--zlib-out", ROOT / "build/openzlib"])
        man = json.loads((pk / "GFOpenDatatypes.json").read_text())
        for c in man["choices"] + man["always"]:
            for f in c["files"]:
                name = f["dest"].rsplit("/", 1)[-1]
                img.add(f["pack"], (pk / "GFOpenDatatypes" / f["pack"]).read_bytes(), f["dest"],
                        own=not f["pack"].startswith("Classes/") or name in ("webp.datatype", "webm.datatype"))
        # z.library is the Team's GFOpenZlib (the user, 23 October 2026), the same file the GFOffice part carries:
        # gzip.datatype and openpng.datatype open it, so it comes with them too. The pack lists it under "zlib"
        # when it was built (gfopendatatypes builds it from the guest checkout).
        if man.get("zlib"):
            zl = man["zlib"]["files"][0]
            img.add(zl["pack"], (pk / "GFOpenDatatypes" / zl["pack"]).read_bytes(), zl["dest"])
    else:
        dt = a.opendatatypes / "Datatypes/out"
        for c in sorted((dt / "Classes/DataTypes").glob("*.datatype")):
            img.add(f"Classes/DataTypes/{c.name}", c.read_bytes(), f"SYS:Classes/DataTypes/{c.name}",
                    own=c.name in ("webp.datatype", "webm.datatype"))
        for d in sorted((dt / "Devs/DataTypes").iterdir()):
            if d.is_file() and not d.name.endswith(".info"):
                img.add(f"Devs/DataTypes/{d.name}", d.read_bytes(), f"DEVS:DataTypes/{d.name}", own=True)
        for lib in sorted((dt / "Libs").glob("*.library")) if (dt / "Libs").is_dir() else []:
            img.add(f"Libs/{lib.name}", lib.read_bytes(), f"LIBS:{lib.name}")   # gfopensound-decoders.library
    img.write()

    # GFOpenWindows: the commodity, started at boot, and its editor (gfopenprefs)
    # 0.12.2 (0.8.23): no snapshot pin on system requesters (gfopenprefs #181, 0.12.1), and the pin is a toggle: a click
    # on a pushed-in pin unsnaps the window (#184)
    ow = Part(top, "GFOpenWindows", None, "GFOpenWindows: snapping, Amiga+Tab, double-click to front, the wheel scrolls the list last clicked, "
                                            "window places, new windows centred until snapped and a snapshot gadget (both on unless switched off in Prefs/Windows)")
    ow.add("Tools/Commodities/GFOpenWindows", (a.openamigaprefs / "Windows/build/os3/GFOpenWindows").read_bytes(),
           "SYS:Tools/Commodities/GFOpenWindows")
    ow.line("startup Run >NIL: SYS:Tools/Commodities/GFOpenWindows")
    ow.write()

    # GFOpenBlanker: the screen blanker, started at boot, on every install
    # (gfopenprefs): after five minutes with no key or mouse it plays Gods
    # and Angels, AmiCradle's homage to the Amiga's makers and keepers
    # 1.4 (0.8.16): CGTriumvirate and CGTimes, OS 3.2's, else DejaVu Sans and DejaVu Serif (blanker_fonts): the open
    # ROM's disk has no CGTriumvirate or CGTimes
    bl = Part(top, "GFOpenBlanker", None, "GFOpenBlanker: the screen blanker, and its show, Gods and Angels")
    bl.add("Tools/Commodities/GFOpenBlanker", (a.openamigaprefs / "Blanker/build/os3/GFOpenBlanker").read_bytes(),
           "SYS:Tools/Commodities/GFOpenBlanker")
    bl.add("Tools/Commodities/GFOpenBlanker.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Tools/Commodities/GFOpenBlanker.info")
    bl.add("Settings/Blanker", b"300\n", "ENVARC:GFOpenPrefs/Blanker", default=True)
    bl.add("Settings/Blanker.env", b"300\n", "ENV:GFOpenPrefs/Blanker", default=True)
    blanker_fonts(bl, a.dejavu_sans, a.dejavu_serif)
    bl.line("startup Run >NIL: SYS:Tools/Commodities/GFOpenBlanker")
    bl.write()

    # GFOpenSnap: pictures of the screen by hotkey (the user, 24 October 2026), on every install
    sn = opensnap_part(top, (ROOT / "build/os3/GFOpenSnap").read_bytes(), icons)
    sn.write()

    # The Shell's font (24 October 2026), on every install: DejaVu Sans Mono as Amiga bitmap fonts, fixed width,
    # 8 to 24 pixels high, made from the TrueType font by gfopenfont's tools/amigafont.py and kept in its
    # fonts/amiga, with the Bitstream Vera licence they come under (its LICENSE-DejaVu.txt), for Font prefs'
    # System Default Text. "add": a DejaVuSansMono.font of another program's stays. The GFOpenKick ROM has the 11
    # and makes it the system default text where no Font prefs name one; GFOpenUp writes no Font prefs.
    if a.openamigafont and (a.openamigafont / "fonts/amiga/DejaVuSansMono.font").is_file():
        fa = a.openamigafont / "fonts/amiga"
        sf = Part(top, "ShellFont", "1.0", "DejaVu Sans Mono 2.37, 8 to 24 pixels, for the Shell (Font prefs' System Default Text)")
        sf.add("Fonts/DejaVuSansMono.font", (fa / "DejaVuSansMono.font").read_bytes(), "FONTS:DejaVuSansMono.font", own=True)
        for f in sorted((fa / "DejaVuSansMono").iterdir(), key=lambda p: int(p.name)):
            sf.add(f"Fonts/DejaVuSansMono/{f.name}", f.read_bytes(), f"FONTS:DejaVuSansMono/{f.name}", own=True)
        # with one line first for the ROM's fonts drawn from DejaVu Sans Mono too: the system's copy of the notice
        # (the ROM has no room for one; amicradle-guest and gfopengfx keep theirs beside the sources)
        rom = (b"This licence also covers the GFOpenKick ROM's DejaVuSansMono.font 11 and its topaz.font 8 and 9, "
               b"which are drawn from DejaVu Sans Mono.\n\n")
        sf.add("Utilities/GFOpenUp/Licences/DejaVuSansMono.txt", rom + (fa / "LICENSE-DejaVu.txt").read_bytes(),
               "SYS:Utilities/GFOpenUp/Licences/DejaVuSansMono.txt")
        sf.write()
    else:
        print("  ShellFont: left out (no --openamigafont checkout with fonts/amiga)")

    # GFOS's own fonts, on every install (gfos_fonts_part); no Font prefs are written here
    gf = gfos_fonts_part(top, a.gfosfonts) if a.gfosfonts else None
    if gf:
        gf.write()

    # The Shell icon (24 October 2026, the Team's decision): OS 3.2.3's SYS:System/Shell.info opens its window
    # CON:0///130, 130 pixels high: about 7 rows of the GFOpenKick ROM's DejaVu Sans Mono 11. Its WINDOW tool type
    # loses the 130, so a Shell from the icon opens 80 x 24 of the system font, as NewShell's own window does
    # (amicradle-guest #472). Only that tool type changes: the picture and the other tool types stay; the icon
    # as it was goes to SYS:Storage/GFOpenUp-Superseded the first time, so UNINSTALL puts it back; and
    # GFOpenUpTool REMOVE Parts/ShellIcon puts the 130 back. A place GFOpenWindows keeps for the window still wins.
    si = Part(top, "ShellIcon", "1.0", "The Shell icon opens its window 80 x 24, as NewShell's")
    si.line('tooltype "SYS:System/Shell.info" "WINDOW=CON:0///130/AmigaShell/CLOSE/ICONIFY" '
            '"WINDOW=CON:0////AmigaShell/CLOSE/ICONIFY"')
    si.write()

    # GFOpenFont (24 October 2026): gfopenfont.library, and DejaVu Sans as an outline font it draws. GFOpenBench's
    # theme names "DejaVu Sans" 13 and the Open programs' look "DejaVu Sans" 12; gfopengadtools' ogt_font opens it
    # with gfopenfont.library's OF_OpenTextFont and, without the library, falls back to CGTriumvirate, helvetica or
    # the screen's font. The Installer puts it on a 68040 or 68060 with its FPU, which the library is built for.
    openfont_part(top, a.openamigafont, a.dejavu_sans)

    # GFOpenDock: the dock and its editor (gfopenprefs). The Installer runs
    # "SYS:Preferences/GFOpenPrefs DOCK SAVE" once, on a first install, so the dock has the
    # buttons of a dock already there; that save starts it (its editor 0.5.3).
    # No User-Startup line (Dale, 17 Oct 2026): GFOpenBench 0.1.23 starts the dock
    # (the ROM's or this C:GFOpenDock, the newer by $VER) when ENV:GFOpenDock/Dock
    # is there, and GFOpenDock 0.6.6 ends at once when one runs already, so an
    # old "Run >NIL: C:GFOpenDock" line left by an earlier GFOpenUp does no harm.
    dock = Part(top, "GFOpenDock", "0.6.13", "GFOpenDock: a dock that takes over ToolManager's, AmiDock's or AmiStart's, and its editor")
    dock.add("C/GFOpenDock", (a.openamigaprefs / "Dock/build/os3/GFOpenDock").read_bytes(), "SYS:C/GFOpenDock")
    # its editor is GFOpenPrefs's Dock page (W48), SYS:Preferences/Dock (the GFOpenPrefs part). C:GFOpenDock stays on the
    # open ROM too: the dock left the ROM (the no-ROM-volume design, section 7.4)
    # Workbench as a backdrop: GFOpenDock sits just above it, so Workbench's own
    # window would cover the dock. A stock 3.2.3 opens Workbench as a window;
    # the backdrop flag is the word at offset 16 of WBConfig.prefs' WBCF chunk
    # (Workbench writes it on Window > Snapshot, not when Backdrop is toggled).
    # GFOpenUpTool keeps the original in SYS:Storage and UNINSTALL puts it back.
    dock.add("Prefs/Env-Archive/Sys/WBConfig.prefs", wbconfig_backdrop(), "ENVARC:Sys/WBConfig.prefs")
    # 0.6.13: the Shell button's picture where there is no SYS:System/Shell (the open ROM's disk: NewShell is the
    # ROM's), GFOpenIcons' Shell; GFOpenDock 0.6.13 takes it after SYS:System/Shell's icon
    if not a.no_openicons and (oi.dir / "Icons/Shell.info").is_file():
        dock.add("Prefs/Presets/GFOpenDock/Shell.info", (oi.dir / "Icons/Shell.info").read_bytes(),
                 "SYS:Prefs/Presets/GFOpenDock/Shell.info")
    dock.write()

    # GFOpenMenus: the engine, started at boot, and its editor (gfopenprefs).
    # The Installer takes MagicMenu's settings over once, when there are any.
    menus = Part(top, "GFOpenMenus", "0.4.8", "GFOpenMenus: menus in the theme's look, the menu bar at any edge, right-click menus on Workbench, and their editor")
    menus.add("C/GFOpenMenus", (a.openamigaprefs / "Menus/build/os3/GFOpenMenus").read_bytes(), "SYS:C/GFOpenMenus")
    menus.line("startup Run >NIL: C:GFOpenMenus")
    menus.write()

    # GFOpenUp Setup: the first-start wizard (gfopenprefs). A project in
    # WBStartup starts it once; it does nothing after ENVARC:GFOpenAmiga/Setup.done.
    setup = Part(top, "GFOpenUpSetup", None, "GFOpenUp Setup: the first-start wizard, with the startup switches")
    setup.add("Prefs/GFOpenUp-Setup", (a.openamigaprefs / "Setup/build/os3/GFOpenUp-Setup").read_bytes(), "SYS:Prefs/GFOpenUp-Setup")
    setup.add("Prefs/GFOpenUp-Setup.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/GFOpenUp-Setup.info")
    setup.add("WBStartup/GFOpenUp-Setup", b"", "SYS:WBStartup/GFOpenUp-Setup")
    setup.add("WBStartup/GFOpenUp-Setup.info", icons.icon(icons.WBPROJECT, "page", "SYS:Prefs/GFOpenUp-Setup", ["DONOTWAIT"], stack=16384),
              "SYS:WBStartup/GFOpenUp-Setup.info")
    setup.write()

    # GFOpenRTG: Workbench on GFOpenRTG's own screens, without Picasso96, on
    # AmiCradle's ACRTG board. LoadMonDrvs runs its monitor driver; the
    # Picasso96 monitor for the same board is moved aside (UNINSTALL puts it
    # back), and Workbench's screen mode becomes GFOpenRTG.1 800x600 16-bit.
    # gfopengpu.library and ACRTG.gpu come with it, so GFOpenRTG's drawing goes
    # to the board's ring without the GFOpenGPU part; that part has the same
    # files, and whichever installs last leaves the same bytes.
    rtg = a.openamigartg / "build"
    acrtg_gpu = (a.guest / "build/guest/os32/ACRTG.gpu").read_bytes()
    # amicradle-guest keeps an ACRTG.gpu in git, and on 12 October 2026 that one was 1.1 (8.10.2026) while the
    # source said 1.2.1: build_parts builds over it, but a --no-build pack from a fresh checkout would take the old
    # driver. Its version must be the source's.
    want = re.search(r"\$VER: ACRTG\.gpu ([0-9][0-9.]*) ",
                     (a.guest / "libraries/acrtg.gpu/acrtg_gpu.c").read_text(encoding="latin-1", errors="replace"))
    if want and binary_version(acrtg_gpu, "ACRTG.gpu") != want.group(1):
        raise SystemExit(f"ACRTG.gpu: build/guest/os32/ACRTG.gpu says {binary_version(acrtg_gpu, 'ACRTG.gpu')}, "
                         f"its source {want.group(1)}: build it (libraries/acrtg.gpu/build.sh) first")
    rtv = binary_version((rtg / "gfopenrtg.library").read_bytes(), "gfopenrtg.library") or "0.17"
    rt = Part(top, "GFOpenRTG", rtv, "GFOpenRTG: Workbench on GFOpenRTG's own screens, without Picasso96, and Native modes")
    rt.line("needs AmiCradle")
    rt.line("needs ACRTG")
    rt.line('retire "DEVS:Monitors/acrtg"')
    rt.line('retire "DEVS:Monitors/acrtg.info"')
    rt.add("Libs/gfopenrtg.library", (rtg / "gfopenrtg.library").read_bytes(), "LIBS:gfopenrtg.library")
    rt.add("Libs/gfopengpu.library", (rtg / "gfopengpu.library").read_bytes(), "LIBS:gfopengpu.library")
    rt.add("Libs/GFOpenGPU/ACRTG.gpu", acrtg_gpu, "LIBS:GFOpenGPU/ACRTG.gpu")
    rt.add("Libs/GFOpenRTG/cybergraphics.library", (rtg / "cybergraphics.library").read_bytes(), "LIBS:GFOpenRTG/cybergraphics.library")
    rt.add("C/GFOpenRTG", (rtg / "GFOpenRTG").read_bytes(), "C:GFOpenRTG")
    rt.add("Devs/Monitors/GFOpenRTG", (rtg / "GFOpenRTG-Monitor").read_bytes(), "DEVS:Monitors/GFOpenRTG")
    rt.write()
    # Workbench's screen mode is the user's (0.8.2; the user, 16 October 2026): the part writes none. On the GFOpenKick
    # ROM Workbench opens on GFOpenRTG's 16-bit screen by itself; on Kickstart 3.2.3 a new install asks, and only a Yes
    # installs Parts/GFOpenRTG/Mode. Width and Height on Default (0xFFFF, as ScreenMode prefs writes it): the screen is
    # the size of whichever mode is chosen (a fixed 800 x 600 kept Workbench at 800 x 600 after the user picked
    # 1920 x 1080, 10 Oct 2026). Up to 0.8.1 Parts/GFOpenRTG wrote it on every new install, and Parts/GFOpenRTG/Upgrade
    # (the same part without it) was for upgrades; since the part itself has none, that drawer is gone.
    mode = screenmode_prefs(0x61011100, 0xFFFF, 0xFFFF, 16)
    md = rt.dir / "Mode"
    (md / "Prefs").mkdir(parents=True)
    (md / "Prefs/ScreenMode.prefs").write_bytes(mode)
    (md / "Prefs/ScreenMode-ENV.prefs").write_bytes(mode)
    (md / "Part").write_text(f"part GFOpenRTGMode {rtv}\n; Workbench on GFOpenRTG's 16-bit Native mode, asked for on a new install\n"
                             'copy "Prefs/ScreenMode.prefs" "ENVARC:Sys/ScreenMode.prefs"\n'
                             'copy "Prefs/ScreenMode-ENV.prefs" "ENV:Sys/ScreenMode.prefs"\n', encoding="latin-1")

    # GFOpenGPU: gfopengpu.library (gfopenrtg) and its drivers and modules in
    # LIBS:GFOpenGPU/. ACRTG.gpu (amicradle-guest) puts the work on the ACRTG
    # board's ring, so the PC draws it; with no board it steps aside and
    # gfopengpu.library draws on the CPU. SDL2.module and GL.module (Mesa) load
    # when a program linked with libSDL2.a or libGL.a first calls them.
    # Picasso96 screens too: it doesn't need the GFOpenRTG part.
    gl = rtg / "gl"
    # 0.26 (0.8.18): SDL2.module from gfopenrtg #179, which can't freeze the machine when SDL's audio thread (priority
    # 10) waits on a lock the program holds; gfopengpu.library is unchanged
    # 0.27 (0.8.19): gfopengpu.library 0.27, stream v1.10 (RemoteFX Progressive ops, BE DWT_INV; gfopenrtg #177,
    # #180, #184); the modules as 0.26
    # 0.29 (0.8.22): gfopengpu.library 0.29 (gfopenrtg #194, the last 256 batches' results; #185, WriteChunkyPixels' and
    # WritePixelArray8's c2p through GFOpenGfx's core), ACRTG.gpu 1.6 (amicradle-guest #638, 256 results), GL.module
    # with the softpipe notice and ENV:GFOpenGPU/GLDriver (gfopenrtg #195), and C:GLQuery
    gp = Part(top, "GFOpenGPU", "0.29", "GFOpenGPU: gfopengpu.library with GFOpenGfx, the ACRTG board's driver, and SDL 2 and GL (Mesa) for programs")
    # 0.8.27: the CPU floor (the user, 9 November 2026: a 68040 or 68060 with its FPU and MMU). SDL2.module, GL.module
    # and OpenAL.module are -m68040 -m68881; GFOpenUpTool 0.8.18 leaves the part out on less (older ones ignore it)
    gp.line("needs 68040")
    gp.add("Libs/gfopengpu.library", (rtg / "gfopengpu.library").read_bytes(), "LIBS:gfopengpu.library")
    gp.add("Libs/GFOpenGPU/ACRTG.gpu", acrtg_gpu, "LIBS:GFOpenGPU/ACRTG.gpu")
    # 0.8.26: SDL2.module from gfopenrtg #217 (b407b01; full-screen exit hardened, back to the Workbench); the part
    # stays 0.29, gfopengpu.library's version, and GFOpenUpTool INSTALL puts the changed file in place
    gp.add("Libs/GFOpenGPU/SDL2.module", (rtg / "SDL2.module").read_bytes(), "LIBS:GFOpenGPU/SDL2.module")
    # SDL2_image and SDL2_mixer (gfopenrtg #54): resident modules beside SDL2.module,
    # decoding on the x86 or ARM64 cores first; their licences go in GFOpenUp's drawer
    sats = [n for n in ("SDL2_image", "SDL2_mixer") if (rtg / f"{n}.module").is_file()]
    for n in sats:
        gp.add(f"Libs/GFOpenGPU/{n}.module", (rtg / f"{n}.module").read_bytes(), f"LIBS:GFOpenGPU/{n}.module")
    # 0.8.21: GL.module from gfopenrtg #192 (a virgl fence whose result expired is done, not failed); the part stays
    # 0.27, gfopengpu.library's version, and GFOpenUpTool INSTALL puts the changed file in place
    gp.add("Libs/GFOpenGPU/GL.module", (gl / "GL.module").read_bytes(), "LIBS:GFOpenGPU/GL.module")
    # 0.8.27: OpenAL.module (gfopenrtg library/modules/openal; OpenAL 1.1, no EFX or capture), which programs linked
    # with the kit's libopenal.a open from LIBS:GFOpenGPU/; MIT, the Team's, like the rest of the part
    gp.add("Libs/GFOpenGPU/OpenAL.module", (rtg / "openal" / "OpenAL.module").read_bytes(), "LIBS:GFOpenGPU/OpenAL.module")
    # 0.8.16: the same modules under the drawer's name from before the GF rename too, LIBS:OpenGPU, for programs built
    # against it: OpenTunnel opens LIBS:OpenGPU/SDL2.module and couldn't start on Instance-32 after 0.8.15. The modules
    # open gfopengpu.library themselves. The pack carries each file once; the line copies it to both places.
    gp.line("; 0.8.16: the modules under the drawer's name from before the GF rename too (LIBS:OpenGPU), for programs built against it (OpenTunnel opens LIBS:OpenGPU/SDL2.module)")
    for n in ["SDL2", *sats, "GL", "OpenAL"]:
        gp.line(f'copy "Libs/GFOpenGPU/{n}.module" "LIBS:OpenGPU/{n}.module"')
    # gfopenrtg's tools/build.sh still writes it as OpenGPUCheck (not yet renamed there): either name
    gpu_check = next((c for c in (rtg / "GFOpenGPUCheck", rtg / "OpenGPUCheck") if c.is_file()), rtg / "GFOpenGPUCheck")
    gp.add("C/GFOpenGPUCheck", gpu_check.read_bytes(), "C:GFOpenGPUCheck")
    # 0.8.28: LIBS:physfs.library 1.1 (gfopenphysfs, pinned: GFOPENPHYSFS), which the game ports linked with the kit's
    # -lphysfs_auto open, with LIBS:z.library for its zip members. Here, as tinygl.library is: a library the ports
    # built with this part's kit need, on the same CPU floor (-m68040). The part stays 0.29, gfopengpu.library's
    if (ROOT / "build/physfs/COMMIT").is_file():
        physfs_into_gpu(gp, ROOT / "build/physfs", (ROOT / "build/openzlib/libs/z.library").read_bytes())
    else:
        print("  GFOpenGPU: physfs.library left out (no --openamigaphysfs checkout)")
    # 0.8.22: C:GLQuery (gfopenrtg tests/amiga/glquery, linked with this GL build's libGL.a): GL's version and renderer,
    # and which driver GL.module took (virgl or softpipe) and why, when the gfopenrtg build has it
    for glq in (rtg / "GLQuery", rtg / "c/GLQuery"):
        if glq.is_file():
            gp.add("C/GLQuery", glq.read_bytes(), "C:GLQuery")
            break
    # tinygl.library 53 (gfopenrtg library/tinygl, from #50): the stand-in TinyGL programs
    # open; their calls are in the program (libtinygl.a) and run on GL.module
    if (rtg / "tinygl.library").is_file():
        gp.add("Libs/tinygl.library", (rtg / "tinygl.library").read_bytes(), "LIBS:tinygl.library")
    # gfopengfx.library 1.2 (amicradle-guest): GFOpenGfx is inside gfopengpu.library 0.6, and the
    # stub hands a program that opens gfopengfx.library the same calls there
    gp.add("Libs/gfopengfx.library", (a.guest / "libraries/opengfx/build-amiga/lib/gfopengfx.library").read_bytes(),
           "LIBS:gfopengfx.library")
    # The modules' third-party licences (Mesa: MIT; SDL 2 and its Amiga back ends:
    # Zlib), in GFOpenUp's drawer: not in LIBS:GFOpenGPU/, where the library looks for drivers
    gl_work = Path(os.environ.get("GL_WORK", a.openamigartg / "library/modules/gl/build"))
    sdl = rtg / "sdl2-work"
    for name, f in ((("Mesa.txt", next(gl_work.glob("mesa-*/docs/license.rst"), None)),
                    ("SDL2.txt", next(sdl.glob("SDL2-*/LICENSE.txt"), None)),
                    ("libSDL2-amigaos3.txt", next(sdl.glob("SDL2-*/LICENSE.libSDL2-amigaos3.txt"), None)))
                   + ((("SDL2_image.txt", next(sdl.rglob("SDL2_image-*/LICENSE.txt"), None)),) if "SDL2_image" in sats else ())
                   + ((("SDL2_mixer.txt", next(sdl.rglob("SDL2_mixer-*/LICENSE.txt"), None)),
                       ("libxmp.txt", next((f for f in sdl.rglob("libxmp-*/*") if f.name in ("COPYING", "LICENSE", "LICENSE.txt", "README")), None)))
                      if "SDL2_mixer" in sats else ())):
        if not f:
            raise SystemExit(f"GFOpenGPU: {name}'s licence text wasn't found (GL_WORK, {sdl})")
        gp.add(f"Utilities/GFOpenUp/Licences/{name}", f.read_bytes(), f"SYS:Utilities/GFOpenUp/Licences/{name}")
    gp.write()

    # OGLES3 (0.8.20): ogles3.library on GFOpenGPU (gfopenrtg library/ogles3, #190), OpenGL ES 3.1 for programs built
    # for ladislawv's ogles3, a face of GFOpenGPU's GL.module (virgl on the PC's GPU, softpipe without), and
    # C:OGLES3Check. As Install-ogles3: AmiCradle only (ladislawv's own library is the PiStorm's), with GFOpenGPU's
    # ACRTG.gpu and GL.module (the GFOpenGPU part's), and never over another ogles3.library: "keep" leaves the part
    # out unless LIBS:ogles3.library is absent or this one ("on GFOpenGPU" in its version string).
    ogles3_part(top, rtg / "ogles3", a.openamigartg / "library/ogles3")

    # Warp3D: Warp3D.library 5.0 (gfopenrtg), drawing through GFOpenGPU or
    # on the CPU. Another Warp3D.library (Wazp3D's, say) is kept in
    # SYS:Storage/GFOpenUp-Superseded, and the Installer takes Wazp3D's
    # settings over once with Warp3DPrefs FROMWAZP3D SAVE. Needs a 68040 or
    # 68060 with its FPU, as Warp3D always has: the Installer checks.
    w3 = Part(top, "Warp3D", "5.0", "Warp3D 5.0 on GFOpenGPU, with Warp3DPrefs, W3DTest and W3DCheck")
    w3.add("Libs/Warp3D.library", (rtg / "Warp3D.library").read_bytes(), "LIBS:Warp3D.library")
    w3.add("C/Warp3DPrefs", (rtg / "Warp3DPrefs").read_bytes(), "C:Warp3DPrefs")
    w3.add("C/W3DCheck", (rtg / "W3DCheck").read_bytes(), "C:W3DCheck")
    w3.add("Tools/W3DTest", (rtg / "W3DTest").read_bytes(), "SYS:Tools/W3DTest")
    w3.add("Tools/W3DTest.info", icons.icon(icons.WBTOOL, "install", stack=65536), "SYS:Tools/W3DTest.info")
    # MiniGL (gfopenrtg #37): minigl.library, OpenGL 1.1 on the same 3D
    # core, with its MGLTest demo, when the gfopenrtg checkout builds them.
    # From the pack after 0.8.27 it is minigl.library 30 (gfopenrtg #221, #222,
    # #224, #225, #229, #230): the clean-room interface and the virgl route on
    # the PC's GPU, which ENV:MiniGL/Driver Auto (the default) takes when there
    # is one (GLQuake 119.5 fps against 5.1 on the CPU route, 9 Nov 2026). 0.8.27
    # shipped 29.10 from an older build/ directory, so an older one stops the
    # build here rather than going into the pack again.
    if (rtg / "minigl.library").is_file():
        mgl = (rtg / "minigl.library").read_bytes()
        mglv = binary_version(mgl, "minigl.library") or "0"
        if tuple(int(x) for x in mglv.split(".")[:2]) < MINIGL_MIN:
            raise SystemExit(f"Warp3D: {rtg / 'minigl.library'} is {mglv}; the pack needs minigl.library "
                             f"{MINIGL_MIN[0]}.{MINIGL_MIN[1]} or newer (gfopenrtg library/minigl/build.sh at main)")
        w3.lines[1] = "; Warp3D 5.0 and MiniGL on GFOpenGPU, with Warp3DPrefs, W3DTest, W3DCheck and MGLTest"
        w3.add("Libs/minigl.library", mgl, "LIBS:minigl.library")
        if (rtg / "MGLTest").is_file():
            w3.add("Tools/MGLTest", (rtg / "MGLTest").read_bytes(), "SYS:Tools/MGLTest")
            w3.add("Tools/MGLTest.info", icons.icon(icons.WBTOOL, "install", stack=65536), "SYS:Tools/MGLTest.info")
    w3.write()

    # GFOpenDemos (gfopenmesa): Boing, Chrome Gears and Copper Tunnel, linked
    # with libGL.a, so GL comes from the GFOpenGPU part's GL.module: on the PC's
    # graphics chip on AmiCradle, on the 68k elsewhere. One icon a demo, a
    # script IconX runs without a window.
    dm = None
    if a.openamigamesa:
        demos = gl_work / "mesa-build-m68k-amigaos/GFOpenDemos.stripped"
        dm = Part(top, "GFOpenDemos", "0.1", "GFOpenDemos: Boing, Chrome Gears and Copper Tunnel, in GL on GFOpenGPU")
        dm.add("Utilities/GFOpenDemos/GFOpenDemos", demos.read_bytes(), "SYS:Utilities/GFOpenDemos/GFOpenDemos")
        for name, demo in (("Boing", "boing"), ("Chrome Gears", "gears"), ("Copper Tunnel", "tunnel")):
            dm.add(f"Utilities/GFOpenDemos/{name}", f"Run >NIL: SYS:Utilities/GFOpenDemos/GFOpenDemos {demo}\n".encode(),
                   f"SYS:Utilities/GFOpenDemos/{name}")
            dm.add(f"Utilities/GFOpenDemos/{name}.info", icons.icon(icons.WBPROJECT, "page", "C:IconX", ["WINDOW=NIL:"], stack=8192),
                   f"SYS:Utilities/GFOpenDemos/{name}.info")
        dm.add("Utilities/GFOpenDemos.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 120)), "SYS:Utilities/GFOpenDemos.info", own=True)
        dm.write()

    # GFOpenCrypto (gfopentls): gfopencrypto.library 2.0. On AmiCradle its sums run
    # on the PC (AC090's magic functions); elsewhere its own 68k code runs.
    cr = None
    if a.openamigatls:
        ocl = (a.openamigatls / "build-amiga/lib/gfopencrypto.library").read_bytes()
        ocv = re.search(rb"opencrypto\.library ([0-9]+\.[0-9]+)", ocl)
        cr = Part(top, "GFOpenCrypto", ocv.group(1).decode() if ocv else "2.0",
                  "GFOpenCrypto: gfopencrypto.library, SSH's and TLS's sums, on x86 or ARM64 cores on AmiCradle")
        cr.add("Libs/gfopencrypto.library", ocl, "LIBS:gfopencrypto.library")
        cr.write()

    # GFOpenTLS (gfopentls): gfopentls.library, the TLS client the Open programs
    # use (GFOpenFTP's FTPS first), with its maths through GFOpenCrypto; and the
    # CA bundle in S:GFOpenTLS/ with its licence (MPL-2.0), refreshed by each
    # GFOpenUp ("add": only GFOpenUp's own copy is replaced). The user's own CAs
    # go in ENVARC:GFOpenTLS/certs/, which GFOpenUp never touches.
    tls = opentls_part(top, a.openamigatls) if a.openamigatls else None

    # OpenAmigaPython: its release's own Part, pinned (OPENAMIGAPYTHON). Off by default; the
    # Installer leaves it out below a 68020 and installs GFOpenTLS (with GFOpenCrypto) with it.
    if a.openamigapython:
        openamigapython_part(top, a.openamigapython)

    # GFOpenPuTTY (gfopenputty), as its release lays it out, in a drawer in
    # SYS:Utilities; its gfopencrypto.library is the GFOpenCrypto part's
    pt = None
    if a.openamigatls and a.openamigaputty:
        pu = a.openamigaputty
        pv = re.search(r'OPENPUTTY_VERSION "([0-9.]+)"', (pu / "amiga/main.c").read_text(errors="replace"))
        stripped = pu / "build/os3-040/GFOpenPuTTY.stripped"
        gcc16 = Path(os.environ.get("OS32_GCC16", Path.home() / "AmiCradle/stoves/os32-gcc16"))
        run([gcc16 / "prefix/bin/m68k-amigaos-strip", "-o", stripped, pu / "build/os3-040/GFOpenPuTTY"])
        pi = pu / "build/icons"
        pt = Part(top, "GFOpenPuTTY", pv.group(1) if pv else "0.2", "GFOpenPuTTY: PuTTY's SSH, Telnet and serial terminal, on GFOpenCrypto")
        d = "Utilities/GFOpenPuTTY"
        for rel, data in (("GFOpenPuTTY", stripped.read_bytes()), ("GFOpenPuTTY.info", (pi / "GFOpenPuTTY.info").read_bytes()),
                          ("ReadMe", (pu / "docs/ReadMe.txt").read_bytes()), ("ReadMe.info", (pi / "Document.info").read_bytes()),
                          ("LICENSE", (pu / "LICENSE").read_bytes()), ("LICENSE.info", (pi / "Document.info").read_bytes()),
                          ("Licences/PuTTY.txt", (pu / "upstream/putty/LICENCE").read_bytes()),
                          ("Licences/PuTTY.txt.info", (pi / "Document.info").read_bytes()),
                          ("Licences.info", (pi / "Drawer.info").read_bytes())):
            pt.add(f"{d}/{rel}", data, f"SYS:{d}/{rel}")
        pt.add("Utilities/GFOpenPuTTY.info", (pi / "Drawer.info").read_bytes(), "SYS:Utilities/GFOpenPuTTY.info", own=True)
        pt.write()

    # Tata (gfopentata): warns before a program or file known to fail on
    # AmigaOS 3.2.3 or with GFOpenUp loads. The list is S:Tata.known (the user,
    # 8 October 2026: S:, not LIBS:), replaced by each GFOpenUp, the old one
    # kept in SYS:Storage; the user's own ENVARC:Tata/Known and decisions are
    # never touched. The Installer leaves it unticked while Tata is 0.x.
    # Tata 0.7.0's known fixes: S:Tata.fixes, the Team's database of fix-ups
    # applied in memory after LoadSeg (Lupus Reditus' copper list first), replaced
    # by each GFOpenUp as S:Tata.known is; the user's own are ENVARC:Tata/KnownFixes,
    # and the switches are in ENVARC:Tata/Settings, both never touched.
    tt = None
    if a.openamigatata:
        tt_src = a.openamigatata
        tb_os3 = tt_src / "build/os3"
        tv = re.search(r'TATA_VERSION\s+"([0-9.]+)"', (tt_src / "src/tata_priv.h").read_text(errors="replace"))
        tt = Part(top, "Tata", tv.group(1) if tv else "0.2",
                  "Tata: warns before loading programs and files known to fail (off unless picked while 0.x)")
        tt.add("C/Tata", (tb_os3 / "Tata").read_bytes(), "SYS:C/Tata")
        tt.add("Prefs/GFOpenTata", (tb_os3 / "Tata-Prefs").read_bytes(), "SYS:Prefs/GFOpenTata")
        tt.add("Prefs/GFOpenTata.info", (tt_src / "icons/Tata.info").read_bytes(), "SYS:Prefs/GFOpenTata.info")
        tt.add("S/Tata.known", (tt_src / "data/Tata.known").read_bytes(), "S:Tata.known")
        if (tt_src / "data/Tata.ads").exists():   # Tata 0.6.0: the ad and tracker list GFOpenBrowser reads
            tt.add("S/Tata.ads", (tt_src / "data/Tata.ads").read_bytes(), "S:Tata.ads")
        if (tt_src / "data/Tata.fixes").exists():   # Tata 0.7.0: the known fixes (offsets and bytes, never a program)
            tt.add("S/Tata.fixes", (tt_src / "data/Tata.fixes").read_bytes(), "S:Tata.fixes")
        tt.line("startup Run >NIL: C:Tata START")
        tt.write()

    # GFOpenFTP (gfopenftp), in a drawer in SYS:Utilities. FTPS works through
    # GFOpenTLS (gfopentls.library), which the Installer picks with it. Plain FTP
    # works without it, and GFOpenFTP says in words that FTPS needs GFOpenTLS.
    # Off by default.
    ftp = None
    if a.openamigaftp:
        fs = a.openamigaftp
        fv = re.search(r'#define VERSION_STRING "([0-9.]+)"', (fs / "app/openftp.c").read_text(errors="replace"))
        ftp = Part(top, "GFOpenFTP", fv.group(1) if fv else "0.1",
                   "GFOpenFTP: FTP and FTPS in two panes (FTPS works through GFOpenTLS)")
        d = "Utilities/GFOpenFTP"
        for rel, data in (("GFOpenFTP", (fs / "build/os3/GFOpenFTP").read_bytes()), ("GFOpenFTP.info", (fs / "package/GFOpenFTP.info").read_bytes()),
                          ("ReadMe", (fs / "README.md").read_bytes()), ("ReadMe.info", (fs / "package/ReadMe.info").read_bytes()),
                          ("LICENSE", (fs / "LICENSE").read_bytes()), ("LICENSE.info", (fs / "package/ReadMe.info").read_bytes())):
            ftp.add(f"{d}/{rel}", data, f"SYS:{d}/{rel}")
        ftp.add("Utilities/GFOpenFTP.info", (fs / "package/Drawer.info").read_bytes(), "SYS:Utilities/GFOpenFTP.info", own=True)
        ftp.write()

    # Unofficial Telegram for Amiga (Open) (gfopentelegram): the Amiga
    # program only. It needs setting up: the user's own Telegram API key
    # (never shipped) and the telegram.td/1 service on the x86 or ARM64 cores
    # or a Cradle; without them it says so in its window. Off by default.
    tg = None
    if a.openamigatelegram:
        ts = a.openamigatelegram
        gv = re.search(r'#define APP_VERSION "([0-9.]+)"', (ts / "app/otg_main.c").read_text(errors="replace"))
        tg = Part(top, "Telegram", gv.group(1) if gv else "0.1",
                  "Unofficial Telegram for Amiga (Open): needs setting up, with your own Telegram API key")
        tg.add("Utilities/UnofficialTelegram", (ts / "build/os3/UnofficialTelegram").read_bytes(), "SYS:Utilities/UnofficialTelegram")
        tg.add("Utilities/UnofficialTelegram.info", icons.icon(icons.WBTOOL, "page", stack=32768), "SYS:Utilities/UnofficialTelegram.info")
        tg.add("Utilities/GFOpenUp/Licences/UnofficialTelegram.txt", (ts / "LICENSE").read_bytes(),
               "SYS:Utilities/GFOpenUp/Licences/UnofficialTelegram.txt")
        tg.write()

    # The GFOpenGPU developer files (gfopenrtg tools/make_sdk.py): the kit as
    # it is, in SYS:Developer/GFOpenGPU-SDK, with an assign (amicradle
    # docs/design/Design-GFOpenUp-GFOpenGPU-Developer-Files.md). Off by default.
    dev = None
    if a.opengpu_sdk:
        kits = sorted(a.opengpu_sdk.glob("GFOpenGPU-SDK-*/"))
        if not kits:
            raise SystemExit(f"no GFOpenGPU-SDK-* kit in {a.opengpu_sdk}")
        kit = kits[-1]
        dev = Part(top, "GFOpenGPUDev", kit.name.rsplit("-", 1)[1],
                   "GFOpenGPU developer files: SDL 2, OpenGL and GLA headers and libraries for building programs")
        for f in sorted(p for p in kit.rglob("*") if p.is_file()):
            rel = f.relative_to(kit).as_posix()
            dev.add(f"Developer/GFOpenGPU-SDK/{rel}", f.read_bytes(), f"SYS:Developer/GFOpenGPU-SDK/{rel}")
        # only where there is none (as Developer.info): the GFOpenIcons part gives it GFOpenIcons' picture, and
        # "add" put the plain drawer back over that picture on every upgrade, so both parts wrote it each run
        dev.add("Developer/GFOpenGPU-SDK.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)),
                "SYS:Developer/GFOpenGPU-SDK.info", default=True)
        dev.add("Developer.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 120)), "SYS:Developer.info", default=True)
        dev.line("startup Assign GFOpenGPU-SDK: SYS:Developer/GFOpenGPU-SDK")
        dev.write()

    # GFOpenInput (gfopeninput): gfopeninput.library, every game controller in one
    # standard layout; C:OIList; GFOpenPrefs Gamepads (gfopenprefs) to test, map
    # and choose ports; the docs. "Gamepads USE" at boot applies the saved settings
    # (the ReadJoyPort patch too, when the user switched it on). Off by default.
    # On AmiCradle the cores' pad window (amicradle #352) is still to come, so
    # an instance sees only the Amiga's ports until it is there.
    oin = None
    if a.openamigainput:
        oib = a.openamigainput / "build"
        oil = (oib / "gfopeninput.library").read_bytes()
        oiv = re.search(rb"openinput\.library ([0-9]+\.[0-9]+)", oil)
        oin = Part(top, "GFOpenInput", oiv.group(1).decode() if oiv else "1.2",
                   "GFOpenInput: game controllers for every program, and Gamepads in Prefs (on AmiCradle: the Amiga's ports for now)")
        oin.add("Libs/gfopeninput.library", oil, "LIBS:gfopeninput.library")
        oin.add("C/OIList", (oib / "OIList").read_bytes(), "SYS:C/OIList")
        # Gamepads is a section of GFOpenPrefs (W48); its name's stub is in the GFOpenPrefs part
        d = "Utilities/GFOpenUp/Docs"
        for name, src in (("GFOpenInput.txt", a.openamigainput / "README.md"),
                          ("Design-GFOpenInput.txt", a.openamigainput / "Design-GFOpenInput.md"),
                          ("Gamepads.txt", a.openamigaprefs / "Gamepads/gamepads.readme")):
            oin.add(f"{d}/{name}", src.read_bytes(), f"SYS:{d}/{name}")
            oin.add(f"{d}/{name}.info", icons.icon(icons.WBPROJECT, "page", "SYS:Utilities/MultiView", stack=16384), f"SYS:{d}/{name}.info")
        oin.add("Utilities/GFOpenUp/Docs.info", icons.icon(icons.WBDRAWER, "drawer", window=(320, 120)), "SYS:Utilities/GFOpenUp/Docs.info", default=True)
        oin.add("Utilities/GFOpenUp/Licences/GFOpenInput.txt", (a.openamigainput / "LICENSE").read_bytes(),
                "SYS:Utilities/GFOpenUp/Licences/GFOpenInput.txt")
        # the boot's Gamepads USE is in GFOpenPrefs APPLY (the GFOpenPrefs part) since W48
        oin.write()

    openlibs_parts(top, a)

    # GCC 16's FPCR clash (gfopenrtg tools/fpcr_check.py) must not be in
    # anything the graphics parts ship, whichever stove built it.
    objdump = next((p for p in (Path.home() / "AmiCradle/stoves/os32-gcc16/prefix/bin/m68k-amigaos-objdump",
                                STOVE / "prefix/bin/m68k-amigaos-objdump") if p.is_file()), None)
    if objdump:
        shipped = [f for part in (rt, gp, w3, dm, cr, tls, pt, ftp) if part for f in sorted(part.dir.rglob("*"))
                   if f.is_file() and f.name != "Part" and not f.name.endswith((".info", ".prefs")) and f.read_bytes()[:4] == b"\0\0\x03\xf3"]
        run([sys.executable, a.openamigartg / "tools/fpcr_check.py", objdump] + shipped)

    # GFOpenFiles: the file manager (gfopenfilemanager). Double-click opens
    # files with their icon's program, else GFOpenView (the GFOpenPrint part).
    of = Part(top, "GFOpenFiles", None, "GFOpenFiles: two panes with tabs, the drives bar, safe copies, a trashcan and undo")
    of.add("Utilities/GFOpenFiles", (a.openamigafilemanager / "build/os3/GFOpenFiles").read_bytes(), "SYS:Utilities/GFOpenFiles")
    of.add("Utilities/GFOpenFiles.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/GFOpenFiles.info")
    of.write()

    # GFOpenPlay: the media player for every datatype (gfopenmediaplayer).
    # Files can be shift-clicked with it or dropped on its window.
    op = Part(top, "GFOpenPlay", "0.1.6", "GFOpenPlay: plays everything the datatypes open, with a playlist")
    op.add("Utilities/GFOpenPlay", (a.openamigamediaplayer / "build/os3/GFOpenPlay").read_bytes(), "SYS:Utilities/GFOpenPlay")
    op.add("Utilities/GFOpenPlay.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/GFOpenPlay.info")
    op.write()

    # GFOpenTitle: the title bar's logo, free memory, clock and settings cog, and its editor (gfopenprefs)
    tbo = a.openamigaprefs / "TitleBar/build/os3"
    tbb = (tbo / "GFOpenTitle").read_bytes()
    tb = Part(top, "GFOpenTitle", binary_version(tbb, "GFOpenTitle") or "0.7",
              "GFOpenTitle: the GFOS mark, free memory, a clock, the settings cog and the taskspace on the title bar")
    tb.add("C/GFOpenTitle", tbb, "SYS:C/GFOpenTitle")
    tb.line("startup Run >NIL: C:GFOpenTitle")
    # GFOpenNetwork (gfopenprefs Network/, 0.8, Dale 12 Nov 2026 "make the sound and wifi and network commodities"):
    # the LAN and Wi-Fi icons in GFOpenTitle's row, a commodity of its own. GFOpenTitle 0.8 leaves the network to it
    # while it runs and draws its own icons again when it ends, so the two go together in this part.
    tb.add("Tools/Commodities/GFOpenNetwork", (a.openamigaprefs / "Network/build/os3/GFOpenNetwork").read_bytes(),
           "SYS:Tools/Commodities/GFOpenNetwork")
    tb.line("startup Run >NIL: SYS:Tools/Commodities/GFOpenNetwork")
    tb.write()

    # GFOpenDateTime: the date, the time and the region (gfopenprefs DateTime). The time zone is a place, and
    # summer time follows it by itself (0.2); the clock is set through C:Date and C:SetClock with NIL: handles
    # (0.2.1). Since W48 a page of GFOpenPrefs, and since the no-ROM-volume design (24 Oct 2026) its icon is
    # SYS:Preferences/Date & Time (the GFOpenPrefs part): this part has no files and archives its old name's stub.
    dtp = Part(top, "GFOpenDateTime", opver,
               "GFOpenDateTime: the date, the time and the region, with summer time by itself (GFOpenPrefs's Date & Time page)")
    dtp.line('retire "SYS:Prefs/OpenDateTime"')
    dtp.write()

    # GFOpenBench: the Team's own Workbench (gfopenbench desktop/), with the dock and the Open editors built in.
    # It goes in SYS:System; the machine runs it where its Startup-Sequence starts SYS:System/GFOpenBench in
    # LoadWB's place (falling back to LoadWB with Shift held, or without RTG). GFOpenUp doesn't change the
    # Startup-Sequence yet, so elsewhere it is there to try. A copy already there is kept in SYS:Storage.
    # openicon.library (icon.library R2) and icon.library R3 stay in gfopenbench: neither is switched on.
    bench = None
    if a.openamigabench:
        obb = (a.openamigabench / "build/os3/GFOpenBench").read_bytes()
        bench = Part(top, "GFOpenBench", binary_version(obb, "GFOpenBench") or "0.1",
                     "GFOpenBench: the Team's own Workbench, with the dock and the Open editors built in (preview)")
        bench.add("System/GFOpenBench", obb, "SYS:System/GFOpenBench")
        # 0.8.13: and under its name before the GF rename, SYS:System/OpenBench, which is what Cradle's instances'
        # Startup-Sequence and the GFOpenKick ROM's workbench.library start (amicradle scripts/openkick_rom.py,
        # tests/test_boot_screen_line.py). Without it an Amiga upgraded from 0.8.11 booted the OpenBench the old
        # OpenUp had replaced (put back by its UNINSTALL), and a new install never started GFOpenBench (lab
        # t5-up0813-upgrade, 4 Nov 2026). The copy it replaces is kept in SYS:Storage, as ever.
        bench.add("System/OpenBench", obb, "SYS:System/OpenBench")
        bench.write()

    # Sound: GFOpenPrefs Sound (the volume, Paula's and AHI's levels, the beep)
    # in SYS:Preferences/Sound (the OS's Sound goes to SYS:Prefs/Classic:
    # GFOpenUpTool RECONCILE), and GFOpenSpeaker on the menu bar (gfopenprefs).
    so = a.openamigaprefs / "Sound/build/os3"
    snd = Part(top, "Sound", "0.2.4", "Sound: the volume, Paula's and AHI's levels, a speaker on the menu bar, and GFOpenAHI on AmiCradle")
    snd.add("C/GFOpenSpeaker", (so / "GFOpenSpeaker").read_bytes(), "SYS:C/GFOpenSpeaker")   # the editor: GFOpenPrefs's Sound (W48)
    # started at boot by its own line (Dale, 11 Nov 2026, "where did the volume control go?"): on the open ROM the
    # prefs task applies Sound's levels and GFOpenPrefs APPLY no longer runs, so nothing else started it. It quits
    # at once when Sound says not to show it, and a second copy only tells the first to read its settings again.
    snd.line("startup Run >NIL: C:GFOpenSpeaker")
    # 0.2.4: GFOpenSpeaker 0.2, the speaker an item in GFOpenTitle 0.8's row (gfopenprefs #193)
    # GFOpenAHI, folded into Sound (the user, 9 Oct 2026), is the Team's own AHI for the ACAHI board: ahi.device
    # (devices/ahi.device, MIT, no Aminet code), acaudio.audio and DEVS:AudioModes/ACAUDIO (amicradle's
    # vendor/amigachrome-guest/devices/acaudio.audio, built here) and AHI's settings on the board's Host mix
    # mode (mkahiprefs.py there; "default": a user's own stay). Never Aminet's AHI.
    acb = ROOT / "build/acaudio"
    ahb = ROOT / "build/ahidevice"
    sys.path.insert(0, str(a.amigachrome / "vendor/amigachrome-guest/devices/acaudio.audio"))
    import mkahiprefs
    # Each of these lines is "when AmiCradle" (GFOpenUpTool 0.6.3): the board is AmiCradle's, so nothing
    # lands on a real Amiga or a PiStorm, where Sound is installed too.
    n_before = len(snd.lines)
    snd.add("Devs/ahi.device", (ahb / "Devs/ahi.device").read_bytes(), "DEVS:ahi.device")
    snd.add("Devs/AHI/acaudio.audio", (acb / "AHI/acaudio.audio").read_bytes(), "DEVS:AHI/acaudio.audio")
    snd.add("Devs/AudioModes/ACAUDIO", (acb / "AudioModes/ACAUDIO").read_bytes(), "DEVS:AudioModes/ACAUDIO")
    prefs = mkahiprefs.prefs()
    snd.add("Prefs/ahi.prefs", prefs, "ENVARC:Sys/ahi.prefs", default=True)
    snd.add("Prefs/ahi.prefs", prefs, "ENV:Sys/ahi.prefs", default=True)
    # GFOpenAHI's licence note, with the other licences: ahi.device is MIT, the Team's own, written from AHI's
    # public interface, with no Aminet code in it
    lic = (a.amigachrome / "LICENSE").read_text(encoding="utf-8", errors="replace")
    note = ("GFOpenAHI: ahi.device, acaudio.audio and DEVS:AudioModes/ACAUDIO\n\n"
            "GFOpenAHI is the Team's own AHI for AmiCradle's ACAHI sound board. Its ahi.device is written by\n"
            "the Team from AHI's public interface (devices/ahi.h, libraries/ahi_sub.h and AHI's documented\n"
            "behaviour). No Aminet code or binaries are in it, and nothing from Aminet is needed. Programs that\n"
            "use AHI work unchanged. MIT licence, as follows.\n\n" + lic)
    snd.add("Utilities/GFOpenUp/Licences/GFOpenAHI.txt", note.encode("latin-1", "replace"), "SYS:Utilities/GFOpenUp/Licences/GFOpenAHI.txt")
    snd.lines[n_before:] = ["when AmiCradle " + l for l in snd.lines[n_before:]]
    # the boot's Sound USE is in GFOpenPrefs APPLY (the GFOpenPrefs part) since W48, and the ROM's own on the open ROM
    # (prefs.resource 47.101)
    snd.write()

    # GFOpenEdit: the text editor (gfopenedit), with its kinds of file in ENVARC:
    oe = a.openamigaedit / "build/os3"
    ed = Part(top, "GFOpenEdit", "0.1.2", "GFOpenEdit: a text editor with tabs, find and replace, and colours for scripts and code")
    ed.add("C/GFOpenEdit", (oe / "GFOpenEdit").read_bytes(), "SYS:C/GFOpenEdit")
    for k in sorted((oe / "Kinds").glob("*.kind")):
        ed.add(f"Kinds/{k.name}", k.read_bytes(), f"ENVARC:GFOpenEdit/Kinds/{k.name}")
    # its Tools icon: a script IconX runs without a window, so it opens GFOpenEdit with no file
    ed.add("Tools/GFOpenEdit", b"Run >NIL: C:GFOpenEdit\n", "SYS:Tools/GFOpenEdit")
    ed.add("Tools/GFOpenEdit.info", icons.icon(icons.WBPROJECT, "page", "C:IconX", ["WINDOW=NIL:"], stack=8192), "SYS:Tools/GFOpenEdit.info")
    ed.write()

    # GFOpenDrawer: the drawer windows of the Team's own Workbench (gfopengadtools workbench/), as a
    # program beside the OS's Workbench until the Team's workbench.library opens them itself. Its
    # icon opens SYS:, and drawers dropped on it or shift-selected with it open in windows of their
    # own; in a Shell, GFOpenDrawer [path ...] (SYS:Utilities is on the path). Off by default.
    odb = (a.opengadtools / "build/os3/GFOpenDrawer").read_bytes()
    od = Part(top, "GFOpenDrawer", binary_version(odb, "GFOpenDrawer") or "0.2",
              "GFOpenDrawer: drawer windows with an icon line, icons or a list, search and Snapshot")
    od.add("Utilities/GFOpenDrawer", odb, "SYS:Utilities/GFOpenDrawer")
    od.add("Utilities/GFOpenDrawer.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Utilities/GFOpenDrawer.info")
    od.write()

    # The GFOffice Suite (gfoffice): GFOffice, the shared files (the suite's four libraries, z.library, the
    # dictionary in SYS:Dictionaries, OWConvert), which GFOpenWrite, on owfwrite.library, and GFOpenCalc, on
    # owfcalc.library, need (gfoffice_parts)
    gfo, wr, gcalc = gfoffice_parts(top, a.gfoffice, (ROOT / "build/openzlib/libs/z.library").read_bytes(),
                                    (a.guest / "LICENSE").read_bytes(), icons)
    gpres = gfoffice_presentation_part(top, a.gfoffice, icons)
    for p in (gfo, wr, gcalc, gpres):
        if p:
            p.write()

    # GFOpenMUI: the clean-room muimaster.library and its examples (gfopenmui_part), or left out
    om = gfopenmui_part(top, a.openamigamui, icons)

    # GFOpenMulticore: gfopenmulticore.library runs a 68k job on another core of a
    # cores board (gfopenservice.device's cpu.m68k/1), or on the main CPU when
    # there is none, so programs that offload (openmodule.datatype) work on
    # every machine; OMCTest checks it, and OMCBench (0.3, a Shell command:
    # OMCBench [JOBS n] [ITEMS n] [WORK n] [STRICT]) measures what is worth
    # sending to the board.
    mcb = a.openamigamulticore / "build"
    mcv = binary_version((mcb / "gfopenmulticore.library").read_bytes(), "gfopenmulticore.library") or "0.2"
    with_bench = (mcb / "OMCBench").is_file()
    mc = Part(top, "GFOpenMulticore", mcv, "GFOpenMulticore: gfopenmulticore.library, jobs on the cores board's other cores, and OMCTest"
              + (" and OMCBench" if with_bench else ""))
    mc.add("Libs/gfopenmulticore.library", (mcb / "gfopenmulticore.library").read_bytes(), "LIBS:gfopenmulticore.library")
    mc.add("Tools/OMCTest", (mcb / "OMCTest").read_bytes(), "SYS:Tools/OMCTest")
    mc.add("Tools/OMCTest.info", icons.icon(icons.WBTOOL, "install", stack=16384), "SYS:Tools/OMCTest.info")
    if with_bench:                          # no icon: it prints its tables in the Shell it runs from
        mc.add("Tools/OMCBench", (mcb / "OMCBench").read_bytes(), "SYS:Tools/OMCBench")
    mc.write()

    # GFOpenMail: the mail reader (gfopenmail), in a drawer with its providers
    # and themes beside it (PROGDIR:Providers); GFOpenMailCheck in C:. It reaches
    # mail servers through GFOpenTLS (gfopentls.library), which the Installer picks with it.
    mb = a.openamigamail / "build/os3"
    vm = re.search(rb"\$VER: GFOpenMail ([0-9.]+)", (mb / "GFOpenMail").read_bytes())   # the program's own version
    mv = vm.group(1).decode() if vm else "0.3"
    ml = Part(top, "GFOpenMail", mv, "GFOpenMail: a mail reader for IMAP over TLS (through GFOpenTLS), and C:GFOpenMailCheck")
    ml.add("Utilities/GFOpenMail/GFOpenMail", (mb / "GFOpenMail").read_bytes(), "SYS:Utilities/GFOpenMail/GFOpenMail")
    mi = a.openamigamail / "package/icons"
    ml.add("Utilities/GFOpenMail/GFOpenMail.info", (mi / "GFOpenMail.info").read_bytes(), "SYS:Utilities/GFOpenMail/GFOpenMail.info")
    for sub in ("Providers", "package/Themes"):
        for f in sorted((a.openamigamail / sub).glob("*")):
            if f.is_file():
                d = f"Utilities/GFOpenMail/{Path(sub).name}/{f.name}"
                ml.add(d, f.read_bytes(), f"SYS:{d}")
    for name, src in (("LICENSE", "LICENSE"), ("GFOpenMail.readme", "gfopenmail.readme"), ("LICENCE.Boxie", "package/icons/LICENCE.Boxie")):
        if (a.openamigamail / src).is_file():
            ml.add(f"Utilities/GFOpenMail/{name}", (a.openamigamail / src).read_bytes(), f"SYS:Utilities/GFOpenMail/{name}")
    ml.add("Utilities/GFOpenMail.info", (mi / "Drawer.info").read_bytes(), "SYS:Utilities/GFOpenMail.info", own=True)
    ml.add("C/GFOpenMailCheck", (mb / "GFOpenMailCheck").read_bytes(), "SYS:C/GFOpenMailCheck")
    ml.write()

    # OpenBrowser Lite (gfopenbrowser's release, pinned: OPENBROWSERLITE): the small
    # browser, in a drawer in SYS:Utilities. https:// goes through GFOpenTLS, which the Installer
    # picks with it. It needs a 68040 (or 68060) with its FPU; the Installer leaves it out
    # elsewhere. Ticked on a new install, so the pack always has it: no archive, no pack.
    lf = openbrowserlite_files(a.openbrowserlite)
    nm = OPENBROWSERLITE["name"]
    bl = Part(top, nm, OPENBROWSERLITE["version"],
              "GFOpenBrowser Lite: a small web browser (no WebKit); https:// through GFOpenTLS")
    d = f"Utilities/{nm}"
    bl.add(f"{d}/{nm}", lf[nm], f"SYS:{d}/{nm}")
    bl.add(f"{d}/{nm}.info", icons.icon(icons.WBTOOL, "page", stack=16384), f"SYS:{d}/{nm}.info")
    for name in (f"{nm}.readme", "LICENSE"):
        bl.add(f"{d}/{name}", lf[name], f"SYS:{d}/{name}")
    bl.add(f"Utilities/{nm}.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 120)),
           f"SYS:Utilities/{nm}.info", own=True)
    bl.write()

    # OpenBrowser: too big for the pack (its engine is about 100 MB), so it
    # comes in its own archive, GFOpenUp-<version>-OpenBrowser.lha, which unpacks
    # into the same GFOpenUp drawer as Parts/OpenBrowser. The Installer offers it
    # when that part is there.
    browser_top = None
    if a.openbrowser:
        import tempfile
        src = a.openbrowser
        tmp = None
        if src.suffix.lower() == ".lha":
            tmp = Path(tempfile.mkdtemp(prefix="openbrowser-"))
            run([shutil.which("jlha") or "lha", "xq", src.resolve()], cwd=tmp)
            src = tmp
        ob_root = src / "OpenBrowser" if (src / "OpenBrowser" / "OpenBrowser").is_file() else src
        browser_top = a.out / "companion" / "GFOpenUp"
        bp = Part(browser_top, "OpenBrowser", OPENBROWSER["version"],
                  "OpenBrowser: a web browser on WebKit (needs an FPU; best on a 68040)")
        for f in sorted(ob_root.rglob("*")):
            if not f.is_file():
                continue
            rel = f.relative_to(ob_root).as_posix()
            if rel.startswith("Libs/"):
                bp.add(f"Libs/{rel[5:]}", f.read_bytes(), f"LIBS:{rel[5:]}")
            else:
                bp.add(f"Utilities/OpenBrowser/{rel}", f.read_bytes(), f"SYS:Utilities/OpenBrowser/{rel}")
        drawer_icon = a.openbrowser.parent / "OpenBrowser.info"
        if drawer_icon.is_file():
            bp.add("Utilities/OpenBrowser.info", drawer_icon.read_bytes(), "SYS:Utilities/OpenBrowser.info", own=True)
        bp.write()
        if tmp:
            shutil.rmtree(tmp, ignore_errors=True)

    # The drawer's own icon, and OS 3.2-style icons for the scripts
    (a.out / "GFOpenUp.info").write_bytes(icons.icon(icons.WBDRAWER, "drawer", window=(440, 170)))
    try:
        os32_icons(a.openamigaprint, icons, [(top / "Install-GFOpenUp.info", "install"), (top / "Uninstall-GFOpenUp.info", "install"),
                                             (core.dir / "Utilities/GFOpenUp/Uninstall-GFOpenUp.info", "install"),
                                             (opp.dir / "Preferences/GFOpenPrefs.info", "page"),
                                             (opp.dir / "Preferences.info", None),
                                             (setup.dir / "Prefs/GFOpenUp-Setup.info", "page"),
                                             (oc.dir / "Utilities/GFOpenCompress.info", archive_art),
                                             (of.dir / "Utilities/GFOpenFiles.info", files_art),
                                             (op.dir / "Utilities/GFOpenPlay.info", play_art),
                                             (od.dir / "Utilities/GFOpenDrawer.info", "page"),
                                             (sn.dir / "Tools/Commodities/GFOpenSnap.info", "picture"),
                                             (w3.dir / "Tools/W3DTest.info", "install"),
                                             (a.out / "GFOpenUp.info", None)]
                    + ([(w3.dir / "Tools/MGLTest.info", "install")] if (w3.dir / "Tools/MGLTest.info").is_file() else [])
                    + ([(dm.dir / f"Utilities/GFOpenDemos/{n}.info", "page") for n in ("Boing", "Chrome Gears", "Copper Tunnel")]
                       + [(dm.dir / "Utilities/GFOpenDemos.info", None)] if dm else [])
                    + ([(tg.dir / "Utilities/UnofficialTelegram.info", "page")] if tg else [])
                    # GFOpenCalc's own picture comes from icons/open (open_icons, below)
                    + ([(gcalc.dir / "Utilities/GFOpenCalc/GFOpenCalc.info", "page"), (gcalc.dir / "Utilities/GFOpenCalc.info", None)]
                       if gcalc else [])
                    + ([(gpres.dir / "Utilities/GFOpenPresentation/GFOpenPresentation.info", "page"),
                        (gpres.dir / "Utilities/GFOpenPresentation.info", None)] if gpres else [])
                    + ([(dev.dir / "Developer/GFOpenGPU-SDK.info", None), (dev.dir / "Developer.info", None)] if dev else [])
                    + ([(oin.dir / "Utilities/GFOpenUp/Docs.info", None)]
                       + [(oin.dir / f"Utilities/GFOpenUp/Docs/{n}.info", "page") for n in ("GFOpenInput.txt", "Design-GFOpenInput.txt", "Gamepads.txt")]
                       if oin else []))
        put("LICENCE.Boxie", (icons.BOXIE / "LICENCE").read_bytes())   # the drawer icon is Boxie's (MIT)
    except Exception as e:                  # node or amiga-icon.js missing: the classic icons stay
        print(f"  (OS 3.2-style icons not made: {e}; the classic ones stay)")
    try:
        open_icons(a.amigachrome, [
            (a.out / "GFOpenUp.info", "drawer-GFOpenUp"), (top / "Install-GFOpenUp.info", "Install-GFOpenUp"),
            (top / "Uninstall-GFOpenUp.info", "Uninstall-GFOpenUp"), (core.dir / "Utilities/GFOpenUp.info", "drawer-GFOpenUp"),
            (core.dir / "Utilities/GFOpenUp/Uninstall-GFOpenUp.info", "Uninstall-GFOpenUp"),
            (core.dir / "Utilities/GFOpenMonitor.info", "GFOpenMonitor"),
            (pr.dir / "Icons/GFOpenPrint.info", "drawer-GFOpenPrint"), (pr.dir / "Drawer/GFOpenView.info", "GFOpenView"),
            (pr.dir / "Drawer/Printers and Queue.info", "Printers and Queue"),
            (opp.dir / "Preferences/GFOpenPrefs.info", "GFOpenPrefs")]
            + [(opp.dir / f"Preferences/{page}.info", art) for page, _, art in PREFERENCES_PAGES]
            + [(setup.dir / "Prefs/GFOpenUp-Setup.info", "GFOpenUp-Setup"),
            (setup.dir / "WBStartup/GFOpenUp-Setup.info", "GFOpenUp-Setup"), (oc.dir / "Utilities/GFOpenCompress.info", "GFOpenCompress"),
            (of.dir / "Utilities/GFOpenFiles.info", "GFOpenFiles"), (op.dir / "Utilities/GFOpenPlay.info", "GFOpenPlay"),
            (ed.dir / "Tools/GFOpenEdit.info", "GFOpenEdit"),
            (wr.dir / "Utilities/GFOpenWrite/GFOpenWrite.info", "GFOpenWrite"),
            (wr.dir / "Utilities/GFOpenWrite.info", "drawer-GFOpenWrite"),
            (od.dir / "Utilities/GFOpenDrawer.info", "GFOpenDrawer")]
            # the dock's GFOpenCalc, GFOpenFTP and GFOpenPuTTY in the AOS drawer style (12 November 2026)
            + ([(gcalc.dir / "Utilities/GFOpenCalc/GFOpenCalc.info", "GFOpenCalc")] if gcalc else [])
            + ([(ftp.dir / "Utilities/GFOpenFTP/GFOpenFTP.info", "GFOpenFTP")] if ftp else [])
            + ([(pt.dir / "Utilities/GFOpenPuTTY/GFOpenPuTTY.info", "GFOpenPuTTY")] if pt else [])
            # the icon library's GFOpenTata and Telegram (#104), when those parts are in the pack
            + ([(tt.dir / "Prefs/GFOpenTata.info", "GFOpenTata")] if tt else [])
            + ([(tg.dir / "Utilities/UnofficialTelegram.info", "Telegram")] if tg else [])
            # GFOpenMUI's examples, when the part is in the pack
            + ([(om.dir / f"Tools/GFOpenMUI/{ex}.info", "MUIExample") for ex in ("MUIClasses", "MUIAddons", "MUITime")
                if (om.dir / f"Tools/GFOpenMUI/{ex}.info").is_file()] if om else []),
            drawers={a.out / "GFOpenUp.info": [top / "Install-GFOpenUp.info", top / "Uninstall-GFOpenUp.info"],
                     core.dir / "Utilities/GFOpenUp.info": [core.dir / "Utilities/GFOpenUp/Uninstall-GFOpenUp.info"],
                     opp.dir / "Preferences.info": [opp.dir / "Preferences/GFOpenPrefs.info"]
                     + [opp.dir / f"Preferences/{page}.info" for page, _, _ in PREFERENCES_PAGES],
                     pr.dir / "Icons/GFOpenPrint.info": sorted((pr.dir / "Drawer").glob("*.info")),
                     wr.dir / "Utilities/GFOpenWrite.info": [wr.dir / "Utilities/GFOpenWrite/GFOpenWrite.info"]},
            )
    except Exception as e:                  # an amicradle checkout without amiga_icon (before #153): the icons above stay
        print(f"  (the Open apps' icons not made: {e}; the ones above stay)")
    # GFOpenUp's own drawers as GFOpenIcons look B drawers with their program's motif (W78, 18 October 2026, Dale:
    # "we need to fix those older icons to our new style"), over the Gorilla-era drawer-NAME and Boxie drawers
    # above; each keeps its window and place. The apps' own animated icons stay as ICONS.md section 1 has them.
    if not a.no_openicons:
        import openicons_part
        n = openicons_part.own_drawers(a.guest, [a.out / "GFOpenUp.info", core.dir / "Utilities/GFOpenUp.info", opp.dir / "Preferences.info",
                                                 pr.dir / "Icons/GFOpenPrint.info", wr.dir / "Utilities/GFOpenWrite.info"]
                                       + ([dm.dir / "Utilities/GFOpenDemos.info"] if dm else [])
                                       + ([dev.dir / "Developer/GFOpenGPU-SDK.info", dev.dir / "Developer.info"] if dev else [])
                                       + ([oin.dir / "Utilities/GFOpenUp/Docs.info"] if oin else []))
        print(f"  GFOpenUp's own drawers in GFOpenIcons look B: {n}")
    print(f"{top}: {sum(1 for _ in top.rglob('*') if _.is_file())} files")
    check_part_versions(top / "Parts")
    if browser_top:
        check_part_versions(browser_top / "Parts")
    if a.lha:
        arc = a.out / f"GFOpenUp-{a.version}.lha"
        arc.unlink(missing_ok=True)
        # jlha makes archives; on some systems "lha" is lhasa, which only reads them
        run([shutil.which("jlha") or "lha", "aq", arc.name, "GFOpenUp", "GFOpenUp.info"], cwd=a.out)
        print(f"{arc}: {arc.stat().st_size} bytes")
        if browser_top:
            barc = a.out / f"GFOpenUp-{a.version}-OpenBrowser.lha"
            barc.unlink(missing_ok=True)
            run([shutil.which("jlha") or "lha", "aq", barc.resolve(), "GFOpenUp"], cwd=browser_top.parent)
            print(f"{barc}: {barc.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
