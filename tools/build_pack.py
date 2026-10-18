#!/usr/bin/env python3
"""Builds OpenUp: a drawer to run the Installer from, and the same as an LhA
archive (DESIGN.md sections 3 and 4).

    python3 tools/build_pack.py [--version 0.1] [--out build/pack]
        [--amigachrome DIR] [--openamigaprint DIR] [--opengadtools DIR]
        [--guest DIR] [--openamigaservice DIR] [--openamigaimage DIR] [--openamigartg DIR]
        [--openamigafilemanager DIR] [--openamigamediaplayer DIR] [--openamigamui DIR]
        [--openamigaedit DIR] [--openamigawrite DIR] [--zlib DIR]
        [--openamigamesa DIR] [--openamigatls DIR] [--openamigaputty DIR] [--openamigatata DIR]
        [--openamigaftp DIR] [--openamigatelegram DIR] [--opengpu-sdk DIR] [--openamigainput DIR]
        [--openamigasocket DIR] [--openamigabench DIR] [--no-build] [--lha]

GL.module's build (openamigartg library/modules/gl) needs meson on PATH and
the os32-gcc16 stove; GL_WORK names its work directory (about 1 GB). OpenDemos
is built against it. The GCC 16 builds (OpenService, the datatypes, SDL 2, GL,
OpenDemos, opengfx.library, OpenCrypto, OpenPuTTY) take their stove from
OS32_GCC16 / STOVE16 / SDL2_STOVE; the 0.6.17 pack used a stove whose gcc adds
-fno-tree-loop-distribute-patterns (GCC 16.2.0b's loop-shift miscompile).

Each part is built by its own repository's scripts, from checkouts given on
the command line (or found beside this one), then laid out as it goes, with
a Part file that C/OpenUpTool reads:

  OpenUp/               OpenUpTool and the uninstaller, OpenUpMenu: the
                        Tools menu's OpenUp and Preferences sub-menus, and
                        OpenMonitor in SYS:Utilities (tasks, CPU, cores, memory, GPU);
                        LIBS:softfpu.library, the FPU in software for a 68LC040 or
                        68EC040 under the open ROM (amigachrome-guest libraries/opensoftfpu)
  OpenSocket/           bsdsocket.library 4.3 and its tools, built from openamigasocket
                        (--openamigasocket) and laid out by the manifest of amigachrome's
                        vendored payload (vendor/opensocket-os32); without the checkout,
                        that vendored payload as it is
  OpenPrint/            OpenPrint and OpenView (openamigaprint's package)
  AmigaChromeSharing/   accontrol.device, ACClip and ACDrop (amigachrome-guest)
  Backdrops/            the themes' desktops as Workbench backdrops (opengadtools)
  OpenIcons/            OpenIcons look B, the Team's own icon set (MIT), as new pictures for
                        the system's icons, made at build time by its generator from the
                        amigachrome-guest checkout (--guest; tools/openicons_part.py)
  OpenTypes/            which program opens each kind of file (openamigaprefs)
  OpenCompress/         OpenCompress, its window and command (openamigacompress)
  OpenService/          openservice.device and Nursery: services from boards and
                        paired Cradles (openamigaservice)
  OpenImage/            datatypes: PNG, Targa (TGA), WebP, WebM and music modules (MOD,
                        MED, XM...) on the 68k, and AVIF, HEIC, SVG, FLAC, MP4, DOCX and more
                        through OpenService (openamigaimage)
  OpenBlanker/          the screen blanker and its show, Gods and Angels: always installed (openamigaprefs)
  OpenPrefs/            OpenPrefs, every editor of ours as a section of one program with one icon
                        (SYS:Prefs/OpenPrefs), its small body C:OpenPrefs (the boot's one
                        OpenPrefs APPLY line), and a stub with no icon for each old editor's
                        name in SYS:Prefs; the old icons retired. Always installed (openamigaprefs)
  OpenWindows/          snapping, Amiga+Tab, double-click to front, the wheel under the
                        pointer and window places; its editor is OpenPrefs's Windows (openamigaprefs)
  OpenDock/             a dock, which takes over ToolManager's, AmiDock's or AmiStart's
                        buttons; its editor is OpenPrefs's Dock (openamigaprefs)
  OpenMenus/            OpenMenus, the Team's own menus (pull-down or pop-up); their editor is
                        OpenPrefs's Menus (openamigaprefs)
  OpenUpSetup/          OpenUp Setup: the first-start wizard and the startup switches
                        (openamigaprefs)
  OpenRTG/              Workbench on OpenRTG's own 16-bit screen, without Picasso96:
                        openrtg.library (Native modes that follow the host window from
                        0.17), its monitor driver and CyberGraphX compatibility, with
                        opengpu.library and ACRTG.gpu (openamigartg; AmigaChrome's ACRTG
                        board). No screen mode: its Mode/ drawer (part OpenRTGMode) has
                        Workbench's, which the Installer puts in only when a new install
                        on Kickstart 3.2.3 says Yes
  OpenGPU/              opengpu.library and LIBS:OpenGPU/: ACRTG.gpu (the ACRTG
                        board's ring, amigachrome-guest), SDL2.module and GL.module
                        (Mesa), loaded when a program first asks; C:OpenGPUCheck.
                        On the CPU where there is no driver (openamigartg)
  Warp3D/               Warp3D.library 5.0 over OpenGPU or the CPU, C:Warp3DPrefs,
                        W3DTest and W3DCheck; Wazp3D's settings are taken over
                        (openamigartg; needs a 68040 or 68060 with its FPU)
  OpenDemos/            Boing, Chrome Gears and Copper Tunnel on GL.module, off
                        by default (--openamigamesa)
  OpenCrypto/           opencrypto.library 2.0: SSH's and TLS's sums, on x86 or ARM64 cores through
                        AC090 on AmigaChrome, on the 68k elsewhere (--openamigatls)
  OpenTLS/              opentls.library (a TLS client on BearSSL, its maths through OpenCrypto)
                        and S:OpenTLS/: Mozilla's CA list (MPL-2.0) from this machine's
                        ca-certificates package, with its licence; off by default, picked with
                        OpenFTP (--openamigatls)
  OpenPuTTY/            OpenPuTTY 0.2, PuTTY on OpenCrypto, off by default (--openamigaputty)
  Tata/                 Tata, the file inspector: C:Tata (the hooks, the question and the
                        Shell command, started from S:User-Startup), its window
                        SYS:Prefs/Tata, and the list, S:Tata.known (the window's "Update
                        the list" reads Parts/Tata/S/Tata.known); off by default while
                        Tata is 0.x (--openamigatata)
  OpenFTP/              OpenFTP, FTP and FTPS in SYS:Utilities/OpenFTP, off by default;
                        FTPS works through OpenTLS (--openamigaftp)
  Telegram/             Unofficial Telegram for Amiga (Open), SYS:Utilities/UnofficialTelegram,
                        off by default; it needs setting up: the user's own Telegram API
                        key and the telegram.td/1 service (--openamigatelegram)
  OpenGPUDev/           the OpenGPU developer files (openamigartg tools/make_sdk.py's kit) in
                        SYS:Developer/OpenGPU-SDK with the OpenGPU-SDK: assign, off by
                        default (--opengpu-sdk DIR: the kit is made in, or read from, DIR)
  OpenInput/            openinput.library (game controllers for every program), C:OIList,
                        and the docs (Gamepads is OpenPrefs's); off by default; the boot's
                        Gamepads USE is in OpenPrefs APPLY (--openamigainput)
  OpenFiles/            OpenFiles, the two-pane file manager (openamigafilemanager)
  OpenPlay/             OpenPlay, the media player for every datatype (openamigamediaplayer)
  OpenTitle/            OpenTitle: logo, free memory, clock and the settings cog on the title bar (openamigaprefs)
  OpenDateTime/         OpenDateTime in SYS:Prefs (OpenPrefs's Date and time section since W48): the date,
                        the time and the region, a place rather than an offset, with summer time
                        by itself (openamigaprefs)
  OpenBench/            OpenBench, the Team's own Workbench, in SYS:System, off unless picked
                        (--openamigabench). It runs where the Startup-Sequence starts
                        SYS:System/OpenBench in LoadWB's place; OpenUp doesn't change the
                        Startup-Sequence yet
  OpenLibs/             the open libraries, one switch each, off by default (0.8.0): S:OpenLibs-Startup
                        (OS 3.2's LoadModule, from S:Pre-LoadModule-Startup), and a part of its own per library in
                        OpenLibs/<name> (utility.module, mathieeedoubbas.library, input.device
                        50.2, OpenRexx's ARexx libraries), which OpenUpTool REMOVE takes out alone (amigachrome-guest,
                        openamigainput; OPEN_LIBRARIES.md section 3)
  Sound/                OpenPrefs Sound and OpenSpeaker, the menu bar speaker (openamigaprefs), and
                        OpenAHI's files for the ACAHI sound board: acaudio.audio, its modes file
                        and AHI's settings on Host mix, with OpenAHI's own ahi.device
                        (amigachrome's vendor/amigachrome-guest; all "when AmigaChrome")
  OpenEdit/             OpenEdit, the text editor with tabs (openamigaedit)
  OpenDrawer/           OpenDrawer, the drawer windows of the Team's own Workbench, beside the OS's
                        Workbench, in SYS:Utilities; off by default (opengadtools workbench/)
  OpenWrite/            OpenWrite 1.0.2, the word processor, and C:OWConvert (openamigawrite)
  OpenMUI/              zunemaster.library, the muimaster.library face (only where no MUI is), and ZuneTest (openamigamui)
  OpenMulticore/        openmulticore.library: jobs on a cores board's other cores, or on
                        the main CPU without one, OMCTest and OMCBench (openamigamulticore)
  OpenMail/             OpenMail, the mail reader, and C:OpenMailCheck; needs OpenTLS (openamigamail)
  OpenBrowser/          OpenBrowser, the web browser, in its own archive beside the pack,
                        OpenUp-<version>-OpenBrowser.lha (--openbrowser; openamigabrowser)

Icons are OS 3.2-style, made with openamigaprint's icon writer (ACBuild's
amiga-icon.js). MIT, Copyright (c) 2026 AmigaChrome Team.
"""
from __future__ import annotations

import argparse
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
STOVE = Path(os.environ.get("STOVE", Path.home() / "AmigaChrome/stoves/os32"))
CC = STOVE / "prefix/bin/m68k-amigaos-gcc"
# soft float: -m68040 links libnix's libm020, not libm881 (specs/m68k-soft-m68040.specs)
SOFT_SPECS = Path(__file__).resolve().parent.parent / "specs/m68k-soft-m68040.specs"
# Every 68k compile here: address 0 is real memory on the Amiga, and without
# this GCC puts TRAP #7 (80000027) where it can prove a pointer null
# (amigachrome #320). The parts' own scripts carry it too.
NULL_OK = "-fno-delete-null-pointer-checks"

# OpenSocket's files that must be ours, or absent: never another stack's (DESIGN.md section 3).
OPENSOCKET_MARKERS = ["OpenSocket (AmigaChrome)", "ACNet (AmigaChrome)", "AmigaChrome ACNet"]


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


def first(*paths: Path) -> Path | None:
    for p in paths:
        if p and p.is_dir():
            return p
    return None


class Part:
    """A part's drawer: its files and its Part file's lines."""

    def __init__(self, top: Path, name: str, version: str, title: str):
        self.dir = top / "Parts" / name
        if self.dir.exists():
            shutil.rmtree(self.dir)
        self.dir.mkdir(parents=True)
        self.lines = [f"part {name} {version}", f'; {title}']

    def add(self, rel: str, data: bytes, dest: str, default: bool = False, own: bool = False):
        """default: only where nothing is (the user's settings); own: where
        nothing is or OpenUp put it, so an upgrade reaches it but another
        program's file stays."""
        p = self.dir / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        self.lines.append(f'{"add" if own else "default" if default else "copy"} "{rel}" "{dest}"')

    def line(self, text: str):
        self.lines.append(text)

    def write(self):
        (self.dir / "Part").write_text("\n".join(self.lines) + "\n", encoding="latin-1")


def icon_maker(openamigaprint: Path):
    """openamigaprint's icon writer (classic icons, then OS 3.2-style ones)."""
    spec = importlib.util.spec_from_file_location("oap_make_package", openamigaprint / "tools" / "make_package.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def os32_icons(oap: Path, icons_mod, jobs: list[tuple[Path, str | None]]) -> None:
    """Rewrites classic icons as OS 3.2-style ones (OS 3.5 colour icons with
    the classic picture as fallback), as openamigaprint's package does: art
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
    them (amigachrome's icon_theme, as Cradle's new icon set does).

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
    """OpenCompress's icon: an archive box with a green arrow coming out, drawn
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


def build_parts(a):
    """Each part's own build, into its own build folder."""
    env = dict(os.environ, AMIGA_CC=str(CC), PATH=f"{CC.parent}:{os.environ['PATH']}")
    (ROOT / "build/os3").mkdir(parents=True, exist_ok=True)   # a fresh checkout has no build folder
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-std=gnu99", "-Wall", "-Werror", "-O2", NULL_OK, "-fno-common",
         "-o", ROOT / "build/os3/OpenUpTool", ROOT / "src/openuptool.c"], env=env)
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-std=gnu99", "-Wall", "-Werror", "-O2", NULL_OK, "-fno-common",
         "-o", ROOT / "build/os3/OpenUpMenu", ROOT / "src/openupmenu.c"], env=env)
    ogt = a.opengadtools / "lib"                              # OpenMonitor draws in the theme's font, as the Open apps
    # OpenMonitor 0.3.3 is built resident (libnix -resident: each run its own variables), so one seglist
    # serves every run: Resident ... PURE, or the ROM's copy. One 68040 build (builds are 68040 only); the ROM
    # takes the same file from build/os3-040.
    (ROOT / "build/os3-040").mkdir(parents=True, exist_ok=True)
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-resident", "-std=gnu99", "-Wall", "-Werror", "-O2", NULL_OK, "-fno-common", f"-I{ogt}",
         "-o", ROOT / "build/os3/OpenMonitor", ROOT / "src/openmonitor.c",
         ogt / "ogt_look.c", ogt / "ogt_theme.c", ogt / "ogt_font.c"], env=env)
    shutil.copy2(ROOT / "build/os3/OpenMonitor", ROOT / "build/os3-040/OpenMonitor")
    run(["sh", a.opengadtools / "look/build.sh"], env=env)
    run(["sh", a.opengadtools / "workbench/build.sh"], env=env)   # OpenDrawer
    run([sys.executable, a.opengadtools / "tools/make_backdrops.py"], env=env)
    g = a.guest
    out = g / "build/guest/os32"
    run(["sh", g / "devices/accontrol.device/build.sh", out], env=env)
    inc = g / "devices/accontrol.device/include"
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-O2", NULL_OK, "-Wall", f"-I{inc}", g / "tools/acclip/acclip.c", "-o", out / "ACClip"], env=env)
    run([CC, "-noixemul", "-m68040", f"-specs={SOFT_SPECS}", "-O2", NULL_OK, "-Wall", f"-I{inc}", f"-I{g / 'common/amiga'}",
         g / "tools/acdrop/acdrop.c", g / "common/amiga/ac_stack.c", "-o", out / "ACDrop"], env=env)
    run(["sh", g / "libraries/acrtg.gpu/build.sh", out], env=env)        # OpenGPU's driver for the ACRTG board
    # opengfx.library 1.2: the stub that sends opengfx.library's calls to opengpu.library 0.6 (GCC 16)
    gcc16_root = os.environ.get("OS32_GCC16", str(Path.home() / "AmigaChrome/stoves/os32-gcc16"))
    run(["sh", g / "libraries/opengfx/build-amiga.sh"], env=dict(os.environ, STOVE=gcc16_root))
    run(["sh", a.openamigaprefs / "OpenTypes/build.sh"], env=env)
    # OpenPrefs: every editor of ours as a section of one program (W48), its small body (C:OpenPrefs) and the
    # old names' stubs
    run(["sh", a.openamigaprefs / "OpenPrefs/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Look/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Windows/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Blanker/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Dock/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Setup/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Menus/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Menus/engine/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "TitleBar/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "DateTime/build.sh"], env=env)     # OpenDateTime
    run(["sh", a.openamigaprefs / "Sound/build.sh"], env=env)
    # acaudio.audio, the AHI driver for the ACAHI board, and its modes file (amigachrome's vendored guest tree)
    run(["sh", a.amigachrome / "vendor/amigachrome-guest/devices/acaudio.audio/build.sh", ROOT / "build/acaudio"],
        env=dict(env, STOVE=str(STOVE)))
    # ahi.device, OpenAHI's own (MIT; amigachrome #388): the other half of AHI on the ACAHI board
    run(["sh", a.amigachrome / "vendor/amigachrome-guest/devices/ahi.device/build.sh", ROOT / "build/ahidevice"],
        env=dict(env, STOVE=str(STOVE)))
    # OpenSocket from its own repository's main, with its own scripts and the os32 stove (as the vendored
    # payload's manifest says it was built), httpget with OpenTLS's headers (its own copy). OpenUp 0.7.5 took the
    # vendored payload, last refreshed on 4 October, so it shipped OpenSocketControl 1.0 after 1.0.1 and
    # 1.0.2 (the addresses' byte order) were merged in openamigasocket.
    if a.openamigasocket:
        so_out = ROOT / "build/opensocket"
        run(["sh", a.openamigasocket / "guest/network/acnet/build.sh", so_out], env=dict(env, STOVE=str(STOVE)))
        run(["sh", a.openamigasocket / "tools/actcptools/build.sh", so_out], env=dict(env, STOVE=str(STOVE)))
    run(["sh", a.openamigacompress / "build-os3.sh"], env=env)
    run(["sh", a.openamigaservice / "build.sh"], env=dict(os.environ))   # its own compiler (os32-gcc16)
    run(["sh", a.openamigaimage / "Datatypes/build.sh"], env=dict(os.environ))   # os32-gcc16 too
    run(["sh", a.openamigartg / "library/build.sh"], env=env)   # also Warp3D and, with the GCC 16 stove, SDL2.module
    run(["sh", a.openamigartg / "tools/build.sh"], env=env)     # OpenGPUCheck
    # GL.module (Mesa) with the GCC 16 stove, which its script puts first on PATH; then OpenDemos on it
    gl_out = a.openamigartg / "build/gl"
    run(["sh", a.openamigartg / "library/modules/gl/module/build.sh", gl_out], env=dict(os.environ))
    if a.openamigamesa:                     # the GCC 16 stove first on PATH, as demos/README.md says
        stove16 = Path(os.environ.get("STOVE16", Path.home() / "AmigaChrome/stoves/os32-gcc16/prefix"))
        run(["sh", a.openamigamesa / "demos/build.sh"],
            env=dict(os.environ, RTG=str(a.openamigartg), CROSS="mesa/cross/m68k-amigaos.ini", GL_OUT=str(gl_out),
                     PATH=f"{stove16 / 'bin'}:{os.environ['PATH']}"))
    run(["sh", a.openamigafilemanager / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigamediaplayer / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigamui / "os3/build.sh"], env=env)
    run(["sh", a.openamigaedit / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    # OpenWrite carries its own OpenGadTools copy; DOCX and ODT need zlib's source (--zlib)
    run(["sh", a.openamigawrite / "build-os3.sh"], env=dict(env, ZLIB_SRC=str(a.zlib)))
    # OpenMail: GCC 16 (its build-os3.sh's own default), TLS through OpenTLS, whose headers it carries
    run(["sh", a.openamigamail / "build-os3.sh"], env=dict(env, STOVE=gcc16_root))
    # openmulticore.library with the os32 stove, as its build.sh says (built with
    # os32-gcc16 it passes jobs the wrong arguments, 8 October 2026)
    omc_env = {k: v for k, v in env.items() if k not in ("CC", "AMIGA_CC")}
    run(["sh", a.openamigamulticore / "library/build.sh"],
        env=dict(omc_env, STOVE=str(STOVE), OPENSERVICE=str(a.openamigaservice)))
    # OpenCrypto and OpenPuTTY (GCC 16), OpenPuTTY linked with this OpenCrypto's headers
    if a.openamigatls:
        run(["sh", a.openamigatls / "build-amiga.sh"],
            env=dict(os.environ, STOVE=gcc16_root, FPCR_CHECK=str(a.openamigartg / "tools/fpcr_check.py")))
        # OpenTLS: opentls.library (GCC 16), and its CA bundle from this
        # machine's ca-certificates package (Mozilla's list, MPL-2.0; only the
        # package's own files: make_ca_bundle.py refuses anything else)
        if (a.openamigatls / "build-amiga-tls.sh").is_file():
            run(["sh", a.openamigatls / "build-amiga-tls.sh"],
                env=dict(os.environ, STOVE=gcc16_root, FPCR_CHECK=str(a.openamigartg / "tools/fpcr_check.py")))
            run([sys.executable, a.openamigatls / "tools/make_ca_bundle.py", a.openamigatls / "build-ca-bundle"])
        if a.openamigaputty:
            run(["sh", a.openamigaputty / "build.sh"], env=dict(os.environ, OS32_GCC16=gcc16_root, CPU="020",
                                                                OPENCRYPTO=str(a.openamigatls)))
            run([sys.executable, a.openamigaputty / "tools/make_icons.py", a.amigachrome / "scripts/amiga_icon.py",
                 a.openamigaputty / "build/icons"])
    # Tata: its own build.sh (os32 stove; its window with this OpenGadTools' lib/)
    if a.openamigatata:
        run(["sh", a.openamigatata / "build.sh"], env=dict(os.environ, STOVE=str(STOVE), OGT=str(a.opengadtools)))
    # OpenFTP: GCC 16 and OpenGadTools' lib/. FTPS works through OpenTLS, whose
    # headers come with openamigaftp.
    if a.openamigaftp:
        run(["sh", a.openamigaftp / "build-os3.sh"], env=dict(os.environ, OS32_GCC16=gcc16_root, OGT=str(a.opengadtools),
                                                             FPCR_CHECK=str(a.openamigartg / "tools/fpcr_check.py")))
    # Unofficial Telegram for Amiga (Open): the Amiga program (os32 stove)
    if a.openamigatelegram:
        run(["sh", a.openamigatelegram / "build-os3.sh"], env=dict(os.environ, STOVE=str(STOVE)))
    # The OpenGPU developer files: openamigartg's kit, from this build's libraries
    # OpenInput (openamigainput): openinput.library and OIList (os32 stove), and
    # OpenPrefs Gamepads, its editor (openamigaprefs, with OpenInput's headers)
    if a.openamigainput:
        run(["sh", a.openamigainput / "library/build.sh", a.openamigainput / "build"], env=dict(env, STOVE=str(STOVE)))
        run(["sh", a.openamigaprefs / "Gamepads/build.sh"], env=dict(env, STOVE=str(STOVE)))
    # OpenLibs (0.8.0): the open libraries the Installer offers one switch each, off by default
    # (amigachrome-guest libraries/OPEN_LIBRARIES.md, section 3), each built with its own script as written:
    # utility.module and the IEEE maths (amigachrome-guest, their default os32-gcc16 stove), and input.device 50.2
    # (openamigainput device/build.sh, os32 stove). The ROM ones load with OS 3.2's own C:LoadModule (OPENLIBS_STARTUP).
    # Their own stove: STOVE here is the os32 one, so it is left out of their environment.
    ol = ROOT / "build/openlibs"
    g = a.guest / "libraries"
    genv = {k: v for k, v in os.environ.items() if k not in ("STOVE", "CC")}
    run(["sh", g / "openutility/build-amiga.sh", "module"], env=dict(genv, OUT=str(ol / "openutility")))
    run(["sh", g / "openmathieee/build-amiga.sh"], env=dict(genv, OUT=str(ol / "openmathieee")))
    # OpenRexx (amigachrome-guest libraries/openrexx, its default os32-gcc16 stove): rexxsyslib.library,
    # rexxsupport.library and RexxMast
    run(["sh", g / "openrexx/build-amiga.sh"], env=dict(genv, OUT=str(ol / "openrexx")))
    # softfpu.library (amigachrome-guest libraries/opensoftfpu, its default os32-gcc16 stove): the OpenUp part's
    run(["sh", g / "opensoftfpu/build-amiga.sh"], env=dict(genv, OUT=str(ROOT / "build/opensoftfpu")))
    if a.openamigainput:
        run(["sh", a.openamigainput / "device/build.sh", ol / "input"], env=dict(env, STOVE=str(STOVE)))
    if a.opengpu_sdk:
        # make_sdk.py won't write over a kit: a rebuild's old one goes to archive/ beside it
        for old in Path(a.opengpu_sdk).glob("OpenGPU-SDK-*/"):
            arch = Path(a.opengpu_sdk) / "archive"
            arch.mkdir(parents=True, exist_ok=True)
            old.rename(arch / f"{old.name}-{time.strftime('%Y%m%dT%H%M%S')}")
        run([sys.executable, a.openamigartg / "tools/make_sdk.py", "--build", a.openamigartg / "build",
             "--gl", gl_out, a.opengpu_sdk], env=dict(os.environ))
    # OpenBench (openamigabench desktop/build.sh, GCC 16 since openamigabench #31: it stops when given the
    # os32 stove's GCC 6.5): the desktop with the dock and the Open editors built in, from this build's
    # opengadtools, openamigaprefs, openamigatata and openamigafilemanager (its drag and drop)
    if a.openamigabench:
        bench_env = dict(env, STOVE=gcc16_root, OGT=str(a.opengadtools), OAP=str(a.openamigaprefs),
                         FILES=str(a.openamigafilemanager))
        if a.openamigatata:
            bench_env["TATA"] = str(a.openamigatata)
        run(["sh", a.openamigabench / "desktop/build.sh"], env=bench_env)
    # OpenPrint, OpenPrint's browser and OpenView with GCC 16 (OpenNDK's os32 libamiga.a, for GCC 6.5,
    # has no SetRexxVar, which OpenView calls); IPPS through OpenTLS, whose headers openamigaprint carries
    p = a.openamigaprint
    cc16 = Path(gcc16_root) / "prefix/bin/m68k-amigaos-gcc"
    print_env = dict(os.environ, AMIGA_CC=str(cc16), PATH=f"{cc16.parent}:{os.environ['PATH']}")
    for script in ("build-amiga.sh", "build-browser.sh", "build-viewer.sh"):
        run(["sh", p / script], cwd=p, env=print_env)
    run([sys.executable, p / "tools/make_package.py", "--out", p / "build/package"], cwd=p, env=print_env)


def files_art() -> bytes:
    """OpenFiles' icon: a window with two panes of files, a drawer and pages
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
    """OpenPlay's icon: a film frame with a green play button on it, drawn
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
    gives for name: "$VER: OpenDock 0.4 (...)" or "opengpu.library 0.9 (...)"."""
    n = re.escape(name.encode("latin-1"))
    m = re.search(rb"\$VER: ?" + n + rb" ([0-9][0-9.]*[0-9])", data) or re.search(rb"(?<![A-Za-z0-9_.])" + n + rb" ([0-9]+\.[0-9][0-9.]*) \(", data)
    return m.group(1).decode() if m else None


# The old editors' names: each a stub in SYS:Prefs that opens its section of OpenPrefs (W48)
OPENPREFS_NAMES = ("Look", "OpenLook", "Windows", "TitleBar", "Menus", "OpenDock", "OpenSound", "OpenInput",
                   "OpenGamepads", "OpenDateTime", "OpenSerial", "OpenTypes", "OpenSystem")


# Each part's main binary: the part's declared version (its Part file, the
# Installer's log, the manifest) must be that binary's $VER version.
PART_MAIN = {
    "OpenUp": ("Utilities/OpenUp/Uninstall-OpenUp", "Uninstall-OpenUp"),
    "OpenBlanker": ("Tools/Commodities/OpenBlanker", "OpenBlanker"),
    "OpenSocket": ("Libs/bsdsocket.library", "bsdsocket.library"),
    "OpenTypes": ("Prefs/OpenTypes", "OpenTypes"),
    "OpenPrefs": ("Prefs/OpenPrefs", "OpenPrefs"),
    "OpenCompress": ("Utilities/OpenCompress", "OpenCompress"),
    "OpenService": ("Devs/openservice.device", "openservice.device"),
    "OpenMulticore": ("Libs/openmulticore.library", "openmulticore.library"),
    "OpenRTG": ("Libs/openrtg.library", "openrtg.library"),
    "OpenGPU": ("Libs/opengpu.library", "opengpu.library"),
    "Warp3D": ("Libs/Warp3D.library", "Warp3D.library"),
    "OpenCrypto": ("Libs/opencrypto.library", "opencrypto.library"),
    "OpenTLS": ("Libs/opentls.library", "opentls.library"),
    "OpenPuTTY": ("Utilities/OpenPuTTY/OpenPuTTY", "OpenPuTTY"),
    "Tata": ("C/Tata", "Tata"),
    "OpenFTP": ("Utilities/OpenFTP/OpenFTP", "OpenFTP"),
    "Telegram": ("Utilities/UnofficialTelegram", "UnofficialTelegram"),
    "OpenInput": ("Libs/openinput.library", "openinput.library"),
    "OpenWindows": ("Tools/Commodities/OpenWindows", "OpenWindows"),
    "OpenDock": ("C/OpenDock", "OpenDock"),
    "OpenUpSetup": ("Prefs/OpenUp-Setup", "OpenUp-Setup"),
    "OpenMenus": ("C/OpenMenus", "OpenMenus"),
    "OpenFiles": ("Utilities/OpenFiles", "OpenFiles"),
    "OpenPlay": ("Utilities/OpenPlay", "OpenPlay"),
    "OpenTitle": ("C/OpenTitle", "OpenTitle"),
    "OpenDateTime": ("Prefs/OpenDateTime", "OpenDateTime"),
    "OpenBench": ("System/OpenBench", "OpenBench"),
    "OpenEdit": ("C/OpenEdit", "OpenEdit"),
    "OpenDrawer": ("Utilities/OpenDrawer", "OpenDrawer"),
    "OpenWrite": ("Utilities/OpenWrite/OpenWrite", "OpenWrite"),
    "OpenMail": ("Utilities/OpenMail/OpenMail", "OpenMail"),
    "OpenBrowser": ("Utilities/OpenBrowser/OpenBrowser", "OpenBrowser"),
}
# Parts with a version of their own: bundles of several programs, or files
# without a binary of their own to read. OpenPrint 0.4 is OpenPrint (0.3) with
# OpenView 0.5 and the copy at SYS:Utilities/MultiView; OpenMUI 0.2.1 is
# zunemaster.library (0.1, with the datatypes cache's Stricmp fix of 12 October 2026) with the
# muimaster.library face (19.1). Sound 0.2.2 is OpenSpeaker (0.1.4; the editor is OpenPrefs's since W48) with OpenAHI's
# ahi.device (6.1.1), acaudio.audio and modes (AmigaChrome machines only). OpenImage 0.4 adds
# opentga.datatype (47.1). A part's own version goes up when what it ships changes, so OpenUpTool UPDATE
# and the Installer's log name the change.
PART_OWN_VERSION = {"OpenLibs", "AmigaChromeSharing", "OpenImage", "Backdrops", "OpenIcons", "OpenDemos", "OpenGPUDev",
                    "OpenPrint", "OpenMUI", "Sound"}


def opentls_part(top: Path, tls_src: Path):
    """The OpenTLS part from an openamigatls checkout built by
    build-amiga-tls.sh and tools/make_ca_bundle.py (build-ca-bundle/), or
    None when it has no opentls.library."""
    lib = tls_src / "build-amiga-tls/lib/opentls.library"
    if not lib.is_file():
        return None
    otl = lib.read_bytes()
    bundle = tls_src / "build-ca-bundle"
    tls = Part(top, "OpenTLS", binary_version(otl, "opentls.library") or "1.2",
               "OpenTLS: opentls.library, secure connections for the Open programs, with Mozilla's CA list")
    tls.add("Libs/opentls.library", otl, "LIBS:opentls.library")
    # the bundle in S:, not ENVARC: (copied into ENV:, in RAM, at every boot);
    # "add": an upgrade replaces OpenUp's own copy, never someone else's
    lic = (bundle / "ca-bundle.LICENSE").read_bytes()
    tls.add("S/OpenTLS/ca-bundle.pem", (bundle / "ca-bundle.pem").read_bytes(), "S:OpenTLS/ca-bundle.pem", own=True)
    tls.add("S/OpenTLS/ca-bundle.LICENSE", lic, "S:OpenTLS/ca-bundle.LICENSE", own=True)
    # the licences with the others: Mozilla's list (MPL-2.0), and OpenTLS's
    # own (MIT) with BearSSL's (MIT) from its THIRD_PARTY_NOTICES
    tls.add("Utilities/OpenUp/Licences/OpenTLS-CA-bundle.txt", lic,
            "SYS:Utilities/OpenUp/Licences/OpenTLS-CA-bundle.txt")
    tls.add("Utilities/OpenUp/Licences/OpenTLS.txt",
            (tls_src / "LICENSE").read_bytes() + b"\n\n" + (tls_src / "THIRD_PARTY_NOTICES.md").read_bytes(),
            "SYS:Utilities/OpenUp/Licences/OpenTLS.txt")
    tls.write()
    return tls


# OpenLibs (0.8.0): the open libraries, one switch each in the Installer's sixth page, off by default
# (amigachrome-guest libraries/OPEN_LIBRARIES.md, section 3: "default off until its ABI test is clean"). Only a
# library whose ABI test against OS 3.2's own is clean, and which builds with the stoves as its script says, is
# here; the others, and why, are in docs/releases/0.8.0/RELEASE-NOTES.md. Each is a part of its own in
# Parts/OpenLibs/<name>, so OpenUpTool REMOVE takes it out alone and puts OS 3.2's own back.
#   name: (the file built, the part's file, where it goes, the name its version is read under, what it is)
OPENLIBS = {
    "utility": ("openutility/modules/utility.module", "Libs/OpenLibs/utility.module", "LIBS:OpenLibs/utility.module",
                "utility.library", "utility.library 47.100 in the ROM's place (a module, loaded by OS 3.2's LoadModule)"),
    "mathieeedoubbas": ("openmathieee/libs/mathieeedoubbas.library", "Libs/mathieeedoubbas.library",
                        "LIBS:mathieeedoubbas.library", "mathieeedoubbas.library",
                        "mathieeedoubbas.library 47.100 in LIBS:, for a 68040 or 68060 with its FPU"),
    "input": ("input/input.device", "Libs/OpenLibs/input.device", "LIBS:OpenLibs/input.device", "input.device",
              "input.device 50.2 in the ROM's place (openamigainput #9; a module, loaded by OS 3.2's LoadModule)"),
    # ARexx stays on the disk (Dale, 18 Oct 2026): OpenRexx's two libraries in OS 3.2's place in LIBS:, and its
    # RexxMast only where there is none (OS 3.2.3's RexxMast works with them, measured: amigachrome-guest
    # docs/design/openrexx.md, and under stock Kickstart 3.2.3: W75). Off by default, as every switch here is.
    "rexx": ("openrexx/libs/rexxsyslib.library", "Libs/rexxsyslib.library", "LIBS:rexxsyslib.library",
             "rexxsyslib.library", "OpenRexx, ARexx's rexxsyslib.library and rexxsupport.library in OS 3.2's place, "
             "for a 68040 or 68060 with its FPU"),
}

# S:OpenLibs-Startup, from S:Pre-LoadModule-Startup, which OS 3.2's Startup-Sequence runs first of all: the ROM
# libraries switched on, in one call of OS 3.2's own C:LoadModule, as openamigainput #9's install notes do for
# input.device. LoadModule reloads the set and restarts the machine when what is in memory isn't that set, so after
# a power-on the Amiga restarts once by itself, and a library switched off goes at the next start. One call only:
# two LoadModule calls with different sets would restart the machine for ever. The Team's C:OpenModule
# (amigachrome-guest #79) is not used yet: in a lab copy of Instance-32 its modules were gone at the second reset
# while OpenModule LIST still listed them, so it never loaded them again, and OpenModule REMOVE then brought
# Guru 81000005 (docs/releases/0.8.0/RELEASE-NOTES.md).
OPENLIBS_STARTUP = """\
; $VER: OpenLibs-Startup 1.0 (13.10.2026)
; OpenUp's open ROM libraries (Install-OpenUp, page 6): each module in LIBS:OpenLibs, loaded in one call of
; OS 3.2's own LoadModule, which restarts the Amiga once when they aren't in memory yet. Taking a file out of
; LIBS:OpenLibs puts the ROM's own back at the next start. MIT, Copyright (c) 2026 AmigaChrome Team.
FailAt 21
If EXISTS LIBS:OpenLibs/utility.module
  If EXISTS LIBS:OpenLibs/input.device
    C:LoadModule LIBS:OpenLibs/utility.module LIBS:OpenLibs/input.device >NIL:
  Else
    C:LoadModule LIBS:OpenLibs/utility.module >NIL:
  EndIf
Else
  If EXISTS LIBS:OpenLibs/input.device
    C:LoadModule LIBS:OpenLibs/input.device >NIL:
  EndIf
EndIf
FailAt 10
"""


def openlibs_parts(top: Path, a) -> list:
    """Parts/OpenLibs (the startup script and S:Pre-LoadModule-Startup, which the Installer adds with the first
    library switched on and takes out with the last; it never writes over another program's
    Pre-LoadModule-Startup) and Parts/OpenLibs/<name>, one per library."""
    ol = ROOT / "build/openlibs"
    base = Part(top, "OpenLibs", "1.0", "OpenLibs: the startup that loads the open ROM libraries with OS 3.2's LoadModule")
    base.add("S/OpenLibs-Startup", OPENLIBS_STARTUP.encode("latin-1"), "S:OpenLibs-Startup", own=True)
    base.add("S/Pre-LoadModule-Startup", b"; OpenUp's OpenLibs (Install-OpenUp, page 6)\nExecute S:OpenLibs-Startup\n",
             "S:Pre-LoadModule-Startup", own=True)
    base.write()
    made = []
    for name, (built, rel, dest, vername, title) in OPENLIBS.items():
        f = ol / built
        if not f.is_file():
            print(f"  OpenLibs: {name} left out ({f} wasn't built)")
            continue
        data = f.read_bytes()
        ver = binary_version(data, vername)
        if not ver:
            raise SystemExit(f"OpenLibs: {f} has no {vername} version")
        p = Part(top, f"OpenLibs/{name}", ver, f"OpenLibs: {title}")
        p.lines[0] = f"part OpenLibs-{name} {ver}"
        p.add(rel, data, dest)
        if name == "input":
            p.add("C/InputProbe", (ol / "input/InputProbe").read_bytes(), "SYS:C/InputProbe")
            p.add("Utilities/OpenUp/Licences/input.device.txt", (a.openamigainput / "LICENSE").read_bytes(),
                  "SYS:Utilities/OpenUp/Licences/input.device.txt")
        if name == "rexx":
            sup = (ol / "openrexx/libs/rexxsupport.library").read_bytes()
            if binary_version(sup, "rexxsupport.library") != ver:
                raise SystemExit(f"OpenLibs: rexxsupport.library isn't {ver} as rexxsyslib.library is")
            p.add("Libs/rexxsupport.library", sup, "LIBS:rexxsupport.library")
            p.add("System/RexxMast", (ol / "openrexx/C/RexxMast").read_bytes(), "SYS:System/RexxMast", default=True)
        p.write()
        made.append(name)
    print(f"  OpenLibs: {', '.join(made)}")
    return made


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
    # OpenLibs' libraries, one part each in Parts/OpenLibs/<name>: the version is the library's own
    for pf in sorted(parts.glob("OpenLibs/*/Part")):
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
    ap.add_argument("--version", default="0.1")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "pack")
    ap.add_argument("--amigachrome", type=Path, default=first(DEV / "amigachrome", DEV / "wt-deploy"))
    ap.add_argument("--openamigaprint", type=Path, default=first(DEV / "openamigaprint", DEV / "review/openamigaprint"))
    ap.add_argument("--opengadtools", type=Path, default=first(DEV / "opengadtools", DEV / "opengadtools-export"))
    ap.add_argument("--guest", type=Path, default=first(DEV / "amigachrome-guest", DEV / "guest"))
    ap.add_argument("--openamigaprefs", type=Path, default=first(DEV / "openamigaprefs"))
    ap.add_argument("--openamigacompress", type=Path, default=first(DEV / "openamigacompress"))
    ap.add_argument("--openamigaservice", type=Path, default=first(DEV / "openamigaservice", DEV.parent / "openamigaservice"))
    ap.add_argument("--openamigaimage", type=Path, default=first(DEV / "openamigaimage", DEV.parent / "openamigaimage"))
    ap.add_argument("--openamigartg", type=Path, default=first(DEV / "openamigartg", DEV.parent / "openamigartg"))
    ap.add_argument("--openamigafilemanager", type=Path,
                    default=first(DEV / "openamigafilemanager", DEV.parent / "openamigafilemanager"))
    ap.add_argument("--openamigamediaplayer", type=Path,
                    default=first(DEV / "openamigamediaplayer", DEV.parent / "openamigamediaplayer"))
    ap.add_argument("--openamigamui", type=Path, default=first(DEV / "openamigamui", DEV.parent / "openamigamui"))
    ap.add_argument("--openamigaedit", type=Path, default=first(DEV / "openamigaedit", DEV.parent / "openamigaedit"))
    ap.add_argument("--openamigawrite", type=Path, default=first(DEV / "openamigawrite", DEV.parent / "openamigawrite"))
    ap.add_argument("--openamigamulticore", type=Path, default=first(DEV / "openamigamulticore", DEV.parent / "openamigamulticore"))
    ap.add_argument("--openamigamail", type=Path, default=first(DEV / "openamigamail", DEV.parent / "openamigamail"))
    ap.add_argument("--openbrowser", type=Path, help="a built OpenBrowser release (.lha or its unpacked folder): "
                    "makes OpenUp-<version>-OpenBrowser.lha beside the pack")
    ap.add_argument("--zlib", type=Path, help="zlib 1.3 source for OpenWrite's DOCX and ODT (Chromium's, as the 1.0 release)",
                    default=first(DEV / "webkit-os32/deps/src/zlib", Path(os.environ.get("ZLIB_SRC", "/nonexistent"))))
    ap.add_argument("--openamigamesa", type=Path, default=first(DEV / "openamigamesa", DEV.parent / "openamigamesa"),
                    help="openamigamesa, for the OpenDemos part (left out without it)")
    ap.add_argument("--openamigatls", type=Path, default=first(DEV / "openamigatls", DEV.parent / "openamigatls"),
                    help="openamigatls, for the OpenCrypto and OpenTLS parts (left out without it)")
    ap.add_argument("--openamigaputty", type=Path, default=first(DEV / "openamigaputty", DEV.parent / "openamigaputty"),
                    help="openamigaputty, for the OpenPuTTY part (needs --openamigatls; left out without it)")
    ap.add_argument("--openamigatata", type=Path, default=first(DEV / "openamigatata", DEV.parent / "openamigatata"),
                    help="openamigatata, for the Tata part (left out without it)")
    ap.add_argument("--openamigaftp", type=Path, default=first(DEV / "openamigaftp", DEV.parent / "openamigaftp"),
                    help="openamigaftp, for the OpenFTP part (left out without it)")
    ap.add_argument("--openamigatelegram", type=Path, default=first(DEV / "openamigatelegram", DEV.parent / "openamigatelegram"),
                    help="openamigatelegram, for the Telegram part (left out without it)")
    ap.add_argument("--openamigainput", type=Path, default=first(DEV / "openamigainput", DEV.parent / "openamigainput"),
                    help="openamigainput, for the OpenInput part (left out without it)")
    ap.add_argument("--openamigasocket", type=Path, default=first(DEV / "openamigasocket", DEV.parent / "openamigasocket"),
                    help="openamigasocket, for the OpenSocket part's files, built here (without it, amigachrome's "
                         "vendored payload, which can be older than openamigasocket's main)")
    ap.add_argument("--openamigabench", type=Path, default=first(DEV / "openamigabench", DEV.parent / "openamigabench"),
                    help="openamigabench checkout: OpenBench, the Team's own Workbench (desktop/build.sh)")
    ap.add_argument("--opengpu-sdk", type=Path, default=None,
                    help="where the OpenGPU developer kit is made (or, with --no-build, read): the OpenGPUDev part (left out without it)")
    ap.add_argument("--no-build", action="store_true", help="use what the parts' builds already made")
    ap.add_argument("--no-openicons", action="store_true", help="leave out the OpenIcons part (made from --guest's icons/openicons)")
    ap.add_argument("--lha", action="store_true", help="also make OpenUp-<version>.lha with the host's lha")
    a = ap.parse_args()
    for name in ("amigachrome", "openamigaprint", "opengadtools", "guest", "openamigaprefs", "openamigacompress", "openamigaservice",
                 "openamigaimage", "openamigartg", "openamigafilemanager", "openamigamediaplayer",
                 "openamigamui", "openamigaedit", "openamigawrite", "zlib"):
        if not getattr(a, name) or not getattr(a, name).is_dir():
            ap.error(f"--{name}: give the checkout's folder")
    if not a.no_build:
        build_parts(a)
    icons = icon_maker(a.openamigaprint)
    date = time.strftime("%d.%m.%Y").lstrip("0").replace(".0", ".")
    top = a.out / "OpenUp"
    if top.exists():
        shutil.rmtree(top)
    top.mkdir(parents=True)
    files: list[tuple[str, bytes]] = []   # for the OS 3.2-style icon pass

    def put(rel: str, data: bytes):
        p = top / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        files.append((rel, data))

    tool = (ROOT / "build/os3/OpenUpTool").read_bytes()
    for script in ("Install-OpenUp", "Uninstall-OpenUp"):
        text = (ROOT / "pack" / script).read_text(encoding="latin-1").replace("@VERSION@", a.version).replace("@DATE@", date)
        put(script, text.encode("latin-1"))
        # NOLOG: the Installer's own install_log_file (and its icon) went in SYS: or Work: on every run, in no
        # list, so it stayed after Uninstall-OpenUp; what OpenUp does is in T:OpenUp.log and the manifest.
        # The Installer (47.19) lists NOLOG as a tooltype too, but in the labs a start from this icon still
        # wrote the log; NOLOG on the command line (OpenUpTool UPDATE) does stop it
        put(script + ".info", icons.icon(icons.WBPROJECT, "install", "Installer",
                                          [f"APPNAME=OpenUp", "MINUSER=NOVICE", "DEFUSER=NOVICE", "NOLOG"]))
    put("C/OpenUpTool", tool)
    readme = (ROOT / "openup.readme").read_bytes()
    put("OpenUp.readme", readme)

    # OpenUp itself: the tool and the uninstaller, where they can be found later,
    # and OpenUpMenu: Workbench's Tools menu gets an OpenUp sub-menu (the
    # programs) and a Preferences one (the settings editors), each showing only
    # what is installed (DESIGN.md section 7). Started at boot; it waits for
    # Workbench, as S:User-Startup runs before LoadWB.
    core = Part(top, "OpenUp", a.version, "OpenUpTool, the uninstaller, OpenUp's programs and settings in Workbench's Tools menu, and OpenMonitor")
    core.add("C/OpenUpTool", tool, "SYS:C/OpenUpTool")
    core.add("C/OpenUpMenu", (ROOT / "build/os3/OpenUpMenu").read_bytes(), "SYS:C/OpenUpMenu")
    core.line("startup Run >NIL: C:OpenUpMenu")
    # AmigaOS 3.2.3's own icons and their default tools (names and tool strings): FIXTOOLS puts back the ones
    # OpenUp 0.6.14 to 0.7.0 changed to OpenView by mistake, on the 0.7.1 upgrade and on UNINSTALL
    core.add("StockDefaultTools", (ROOT / "pack/StockDefaultTools").read_bytes(), "ENVARC:OpenAmiga/StockDefaultTools")
    un = (top / "Uninstall-OpenUp").read_bytes()
    core.add("Utilities/OpenUp/Uninstall-OpenUp", un, "SYS:Utilities/OpenUp/Uninstall-OpenUp")
    core.add("Utilities/OpenUp/Uninstall-OpenUp.info", (top / "Uninstall-OpenUp.info").read_bytes(),
             "SYS:Utilities/OpenUp/Uninstall-OpenUp.info")
    core.add("Utilities/OpenUp.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)), "SYS:Utilities/OpenUp.info", own=True)
    # OpenMonitor: tasks, CPU and the cores, memory and the GPU, as Task Manager or Resources show them
    core.add("Utilities/OpenMonitor", (ROOT / "build/os3/OpenMonitor").read_bytes(), "SYS:Utilities/OpenMonitor")
    core.add("Utilities/OpenMonitor.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenMonitor.info")
    # softfpu.library (amigachrome-guest libraries/opensoftfpu): the FPU in software, out of the 1M open ROM since
    # W61 (no room). On a 68LC040 or 68EC040 the open ROM's dos opens it from LIBS: before the Startup-Sequence
    # (the F-line emulator) and the open maths libraries open it by name; with an FPU, or under OS 3.2's own ROM,
    # nothing opens it. In every install, as the CPU can change under the same disk.
    sfl = ROOT / "build/opensoftfpu/libs/softfpu.library"
    if not sfl.is_file():
        raise SystemExit(f"{sfl} wasn't built (amigachrome-guest libraries/opensoftfpu/build-amiga.sh)")
    core.add("Libs/softfpu.library", sfl.read_bytes(), "LIBS:softfpu.library")
    core.write()

    # OpenSocket: the files and places amigachrome's vendored payload's manifest lists (the same list Cradle's own
    # installer uses), each from this build of openamigasocket (build_parts) when there is one, else the vendored
    # file. The version check reads only bsdsocket.library, so a stale OpenSocketControl in the vendored payload
    # went into 0.7.5 unseen; the line below says where the part's files came from.
    payload = a.amigachrome / "vendor/opensocket-os32"
    m = json.loads((payload / "manifest.json").read_text())
    so_out = ROOT / "build/opensocket" if a.openamigasocket else None
    sock = Part(top, "OpenSocket", "4.3", "bsdsocket.library 4.3 through the x86 or ARM64 cores (" + m.get("identity", "") + ")")
    sock.line("needs AmigaChrome")
    sock.line('keep "SYS:Libs/bsdsocket.library" ' + " ".join(f'"{x}"' for x in OPENSOCKET_MARKERS))
    for f in m["files"]:
        if f.get("installPath"):
            src = so_out / Path(f["path"]).name if so_out else payload / f["path"]
            if not src.is_file():
                raise SystemExit(f"OpenSocket: {f['path']} wasn't built ({src})")
            sock.add(f["path"], src.read_bytes(), "SYS:" + f["installPath"])
    sock.write()
    osc = (sock.dir / "Tools/Commodities/OpenSocketControl").read_bytes()
    print(f"  OpenSocket: {'built from ' + str(a.openamigasocket) if so_out else 'the vendored payload (openamigasocket ' + m.get('source', {}).get('commit', '?') + ')'}; "
          f"OpenSocketControl {binary_version(osc, 'OpenSocketControl') or '(no version)'}")

    # OpenPrint and OpenView: openamigaprint's own package, laid out as its Installer does
    pkg = a.openamigaprint / "build/package/OpenPrint"
    pr = Part(top, "OpenPrint", "0.4", "OpenPrint and OpenView: printing to PDF and IPP, and viewing; OpenView is MultiView's replacement")
    for p in sorted(pkg.rglob("*")):
        if not p.is_file():
            continue
        rel = p.relative_to(pkg).as_posix()
        if rel.startswith("C/"): dest = "SYS:C/" + rel[2:]
        elif rel.startswith("Devs/"): dest = "SYS:Devs/" + rel[5:]
        elif rel.startswith("WBStartup/"): dest = "SYS:WBStartup/" + rel[10:]
        elif rel.startswith("Drawer/"): dest = "SYS:Utilities/OpenPrint/" + rel[7:]
        elif rel == "Icons/OpenPrint.info": dest = "SYS:Utilities/OpenPrint.info"
        else: continue                    # its own Installer script and ReadMe
        pr.add(rel, p.read_bytes(), dest)
    # OpenView stands in MultiView's place (OpenUp 0.7.2): a copy at SYS:Utilities/MultiView. Everything that runs
    # MultiView (default tools, Help, programs, scripts) then runs OpenView, and no icon is edited. The original is
    # kept by the usual rule for a replaced file (SYS:Storage/OpenUp-Superseded) and Uninstall-OpenUp puts it back.
    # A copy, not a launcher or a link: it works on every file system, passes Workbench's and the Shell's arguments
    # untouched, and needs no second program.
    pr.add("Drawer/MultiView", (pkg / "Drawer/OpenView").read_bytes(), "SYS:Utilities/MultiView")
    pr.write()

    # AmigaChrome sharing: the control device, the clipboard and the files tools
    gout = a.guest / "build/guest/os32"
    icondir = a.amigachrome / "resources/icons/collections/amigachrome-guest/os3"
    sh = Part(top, "AmigaChromeSharing", "1.4", "accontrol.device, ACClip and ACDrop: the clipboard and files, in and out of the Amiga (Send Out icon optional)")
    sh.line("needs AmigaChrome")
    sh.add("Devs/accontrol.device", (gout / "accontrol.device").read_bytes(), "SYS:Devs/accontrol.device")
    for tool_name in ("ACClip", "ACDrop"):
        sh.add(f"C/{tool_name}", (gout / tool_name).read_bytes(), f"SYS:C/{tool_name}")
        sh.add(f"WBStartup/{tool_name}.info", (icondir / f"{tool_name}.info").read_bytes(), f"SYS:WBStartup/{tool_name}.info")
    sh.write()

    # OpenLook (C:OpenLook, its themes, OpenPrefs Look and its profiles) was a part up to OpenUp 0.8.1. Since 0.8.2
    # Workbench's look is the OS's own (the OpenKick ROM's: the user, 16 October 2026, "it should be default,
    # straight from the rom ... only the background will be the one that gets the attention"), and the
    # Installer's OLDLOOK takes out what an earlier OpenUp wrote for it.
    o = a.opengadtools

    # The background pack: each theme's desktop at four screen shapes (opengadtools)
    bd = Part(top, "Backdrops", "0.1", "The themes' desktops as Workbench backdrops")
    for png in sorted((o / "build/backdrops").glob("*.png")):
        bd.add(f"Backdrops/{png.name}", png.read_bytes(), f"SYS:Prefs/Presets/Backdrops/OpenUp/{png.name}")
    bd.write()

    # OpenIcons: the Team's own icon set (look B, MIT) as new pictures for the system's own icons
    # (ICONS.md section 8). Its .info files are the generator's build output, never committed: they are made
    # here from the amigachrome-guest checkout the pack is built with (--guest), so nothing is fetched from
    # outside our repositories and the pack's icons are that commit's.
    if not a.no_openicons:
        sys.path.insert(0, str(ROOT / "tools"))
        import openicons_part
        oi = Part(top, "OpenIcons", "0.2", "OpenIcons, the Team's own icon set: new pictures for the system's drawers, programs and kinds of file")
        for text in openicons_part.build(oi.dir, a.guest):
            oi.line(text)
        oi.write()

    # OpenPrefs (W48, Dale, 17 Oct 2026: "We should just combine them into a single app, prefs editors that is"):
    # every editor of ours as a section of one program with one icon, SYS:Prefs/OpenPrefs; C:OpenPrefs, its
    # small body, serves the boot's one OpenPrefs APPLY (in place of SYS:Prefs/OpenSound USE and
    # SYS:Prefs/OpenGamepads USE) without loading the app; and each old editor's name in SYS:Prefs is a stub
    # with no icon that runs OpenPrefs at that editor's section, with its arguments (OpenSound USE,
    # OpenDock SAVE). The old editors' icons are retired, so the Prefs drawer shows one OpenPrefs icon. With
    # every install, before the parts whose names it serves.
    opb = a.openamigaprefs / "OpenPrefs/build/os3"
    opapp = (opb / "OpenPrefs").read_bytes()
    opp = Part(top, "OpenPrefs", binary_version(opapp, "OpenPrefs") or "2.0",
               "OpenPrefs: every Open editor in one window with one icon; the old names open its sections")
    opp.add("Prefs/OpenPrefs", opapp, "SYS:Prefs/OpenPrefs")
    opp.add("Prefs/OpenPrefs.info", icons.icon(icons.WBTOOL, "page", stack=32768), "SYS:Prefs/OpenPrefs.info")
    opp.add("C/OpenPrefs", (opb / "OpenPrefs.small").read_bytes(), "SYS:C/OpenPrefs")
    for n in OPENPREFS_NAMES:
        opp.add(f"Prefs/{n}", (opb / "stubs" / n).read_bytes(), f"SYS:Prefs/{n}")
    for n in OPENPREFS_NAMES:
        opp.line(f'retire "SYS:Prefs/{n}.info"')
    opp.line("startup OpenPrefs APPLY")
    opp.write()

    # OpenTypes: which program opens each kind of file (openamigaprefs); since W48 a section of OpenPrefs, and
    # this part its name's stub
    tys = (opb / "stubs/OpenTypes").read_bytes()
    ty = Part(top, "OpenTypes", binary_version(tys, "OpenTypes") or "2.0", "OpenTypes: which program opens each kind of file (in OpenPrefs)")
    ty.add("Prefs/OpenTypes", tys, "SYS:Prefs/OpenTypes")
    ty.write()

    # OpenCompress: one program, the window from Workbench and the command in
    # the Shell (SYS:Utilities is in the Shell's path)
    oc = Part(top, "OpenCompress", "0.3.2", "OpenCompress: archives and disk images, from its window or the Shell")
    oc.add("Utilities/OpenCompress", (a.openamigacompress / "build/os3/OpenCompress").read_bytes(), "SYS:Utilities/OpenCompress")
    oc.add("Utilities/OpenCompress.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenCompress.info")
    oc.write()

    # OpenService: the services device and Nursery (openamigaservice). It
    # does nothing on the LAN until ENV:OpenService/Paired lists a Cradle.
    sv = Part(top, "OpenService", "1.1", "OpenService: services from AutoConfig boards and paired Cradles")
    sv.add("Devs/openservice.device", (a.openamigaservice / "bin/openservice.device").read_bytes(), "DEVS:openservice.device")
    sv.add("C/Nursery", (a.openamigaservice / "bin/Nursery").read_bytes(), "C:Nursery")
    sv.write()

    # OpenImage: datatypes (openamigaimage). PNG, Targa (opentga.datatype), WebP, WebM and music modules
    # (openmodule.datatype: MOD, MED, Oktalyzer, DigiBooster, XM, S3M, IT,
    # played by libxmp on the Amiga itself) decode on the 68k; the rest ask
    # media.decode/1 or doc.render/1 through OpenService. Every class and
    # descriptor Datatypes/build.sh makes goes in. A
    # datatype or descriptor already there (Aminet's flac.datatype, say)
    # stays: ours only go where nothing is or where OpenUp put them (so a
    # newer pack updates them), and a machine without a Cradle keeps what it
    # had. Their own classes have names nobody else uses.
    dt = a.openamigaimage / "Datatypes/out"
    img = Part(top, "OpenImage", "0.4", "OpenImage: datatypes for today's pictures (PNG and Targa on the Amiga itself), sounds, music modules, documents and video")
    for c in sorted((dt / "Classes/DataTypes").glob("*.datatype")):
        img.add(f"Classes/DataTypes/{c.name}", c.read_bytes(), f"SYS:Classes/DataTypes/{c.name}",
                own=c.name in ("webp.datatype", "webm.datatype"))
    for d in sorted((dt / "Devs/DataTypes").iterdir()):
        if d.is_file() and not d.name.endswith(".info"):
            img.add(f"Devs/DataTypes/{d.name}", d.read_bytes(), f"DEVS:DataTypes/{d.name}", own=True)
    img.write()

    # OpenWindows: the commodity, started at boot, and its editor (openamigaprefs)
    ow = Part(top, "OpenWindows", "0.8.4", "OpenWindows: snapping, Amiga+Tab, double-click to front, the wheel under the pointer, window places")
    ow.add("Tools/Commodities/OpenWindows", (a.openamigaprefs / "Windows/build/os3/OpenWindows").read_bytes(),
           "SYS:Tools/Commodities/OpenWindows")
    ow.line("startup Run >NIL: SYS:Tools/Commodities/OpenWindows")
    ow.write()

    # OpenBlanker: the screen blanker, started at boot, on every install
    # (openamigaprefs): after five minutes with no key or mouse it plays Gods
    # and Angels, AmigaChrome's homage to the Amiga's makers and keepers
    bl = Part(top, "OpenBlanker", "1.1.1", "OpenBlanker: the screen blanker, and its show, Gods and Angels")
    bl.add("Tools/Commodities/OpenBlanker", (a.openamigaprefs / "Blanker/build/os3/OpenBlanker").read_bytes(),
           "SYS:Tools/Commodities/OpenBlanker")
    bl.add("Tools/Commodities/OpenBlanker.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Tools/Commodities/OpenBlanker.info")
    bl.add("Settings/Blanker", b"300\n", "ENVARC:OpenPrefs/Blanker", default=True)
    bl.add("Settings/Blanker.env", b"300\n", "ENV:OpenPrefs/Blanker", default=True)
    bl.line("startup Run >NIL: SYS:Tools/Commodities/OpenBlanker")
    bl.write()

    # OpenDock: the dock and its editor (openamigaprefs). The Installer runs
    # "SYS:Prefs/OpenDock SAVE" once, on a first install, so the dock has the
    # buttons of a dock already there; that save starts it (its editor 0.5.3).
    # No User-Startup line (Dale, 17 Oct 2026): OpenBench 0.1.23 starts the dock
    # (the ROM's or this C:OpenDock, the newer by $VER) when ENV:OpenDock/Dock
    # is there, and OpenDock 0.6.6 ends at once when one runs already, so an
    # old "Run >NIL: C:OpenDock" line left by an earlier OpenUp does no harm.
    dock = Part(top, "OpenDock", "0.6.6", "OpenDock: a dock that takes over ToolManager's, AmiDock's or AmiStart's, and its editor")
    dock.add("C/OpenDock", (a.openamigaprefs / "Dock/build/os3/OpenDock").read_bytes(), "SYS:C/OpenDock")
    # its editor is OpenPrefs's Dock section (W48), SYS:Prefs/OpenDock its name's stub (the OpenPrefs part)
    # Workbench as a backdrop: OpenDock sits just above it, so Workbench's own
    # window would cover the dock. A stock 3.2.3 opens Workbench as a window;
    # the backdrop flag is the word at offset 16 of WBConfig.prefs' WBCF chunk
    # (Workbench writes it on Window > Snapshot, not when Backdrop is toggled).
    # OpenUpTool keeps the original in SYS:Storage and UNINSTALL puts it back.
    dock.add("Prefs/Env-Archive/Sys/WBConfig.prefs", wbconfig_backdrop(), "ENVARC:Sys/WBConfig.prefs")
    dock.write()

    # OpenMenus: the engine, started at boot, and its editor (openamigaprefs).
    # The Installer takes MagicMenu's settings over once, when there are any.
    menus = Part(top, "OpenMenus", "0.4.4", "OpenMenus: menus in the theme's look, the menu bar at any edge, right-click menus on Workbench, and their editor")
    menus.add("C/OpenMenus", (a.openamigaprefs / "Menus/build/os3/OpenMenus").read_bytes(), "SYS:C/OpenMenus")
    menus.line("startup Run >NIL: C:OpenMenus")
    menus.write()

    # OpenUp Setup: the first-start wizard (openamigaprefs). A project in
    # WBStartup starts it once; it does nothing after ENVARC:OpenAmiga/Setup.done.
    setup = Part(top, "OpenUpSetup", "0.1.3", "OpenUp Setup: the first-start wizard, with the startup switches")
    setup.add("Prefs/OpenUp-Setup", (a.openamigaprefs / "Setup/build/os3/OpenUp-Setup").read_bytes(), "SYS:Prefs/OpenUp-Setup")
    setup.add("Prefs/OpenUp-Setup.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/OpenUp-Setup.info")
    setup.add("WBStartup/OpenUp-Setup", b"", "SYS:WBStartup/OpenUp-Setup")
    setup.add("WBStartup/OpenUp-Setup.info", icons.icon(icons.WBPROJECT, "page", "SYS:Prefs/OpenUp-Setup", ["DONOTWAIT"], stack=16384),
              "SYS:WBStartup/OpenUp-Setup.info")
    setup.write()

    # OpenRTG: Workbench on OpenRTG's own screens, without Picasso96, on
    # AmigaChrome's ACRTG board. LoadMonDrvs runs its monitor driver; the
    # Picasso96 monitor for the same board is moved aside (UNINSTALL puts it
    # back), and Workbench's screen mode becomes OpenRTG.1 800x600 16-bit.
    # opengpu.library and ACRTG.gpu come with it, so OpenRTG's drawing goes
    # to the board's ring without the OpenGPU part; that part has the same
    # files, and whichever installs last leaves the same bytes.
    rtg = a.openamigartg / "build"
    acrtg_gpu = (a.guest / "build/guest/os32/ACRTG.gpu").read_bytes()
    # amigachrome-guest keeps an ACRTG.gpu in git, and on 12 October 2026 that one was 1.1 (8.10.2026) while the
    # source said 1.2.1: build_parts builds over it, but a --no-build pack from a fresh checkout would take the old
    # driver. Its version must be the source's.
    want = re.search(r"\$VER: ACRTG\.gpu ([0-9][0-9.]*) ",
                     (a.guest / "libraries/acrtg.gpu/acrtg_gpu.c").read_text(encoding="latin-1", errors="replace"))
    if want and binary_version(acrtg_gpu, "ACRTG.gpu") != want.group(1):
        raise SystemExit(f"ACRTG.gpu: build/guest/os32/ACRTG.gpu says {binary_version(acrtg_gpu, 'ACRTG.gpu')}, "
                         f"its source {want.group(1)}: build it (libraries/acrtg.gpu/build.sh) first")
    rtv = binary_version((rtg / "openrtg.library").read_bytes(), "openrtg.library") or "0.17"
    rt = Part(top, "OpenRTG", rtv, "OpenRTG: Workbench on OpenRTG's own screens, without Picasso96, and Native modes")
    rt.line("needs AmigaChrome")
    rt.line("needs ACRTG")
    rt.line('retire "DEVS:Monitors/acrtg"')
    rt.line('retire "DEVS:Monitors/acrtg.info"')
    rt.add("Libs/openrtg.library", (rtg / "openrtg.library").read_bytes(), "LIBS:openrtg.library")
    rt.add("Libs/opengpu.library", (rtg / "opengpu.library").read_bytes(), "LIBS:opengpu.library")
    rt.add("Libs/OpenGPU/ACRTG.gpu", acrtg_gpu, "LIBS:OpenGPU/ACRTG.gpu")
    rt.add("Libs/OpenRTG/cybergraphics.library", (rtg / "cybergraphics.library").read_bytes(), "LIBS:OpenRTG/cybergraphics.library")
    rt.add("C/OpenRTG", (rtg / "OpenRTG").read_bytes(), "C:OpenRTG")
    rt.add("Devs/Monitors/OpenRTG", (rtg / "OpenRTG-Monitor").read_bytes(), "DEVS:Monitors/OpenRTG")
    rt.write()
    # Workbench's screen mode is the user's (0.8.2; the user, 16 October 2026): the part writes none. On the OpenKick
    # ROM Workbench opens on OpenRTG's 16-bit screen by itself; on Kickstart 3.2.3 a new install asks, and only a Yes
    # installs Parts/OpenRTG/Mode. Width and Height on Default (0xFFFF, as ScreenMode prefs writes it): the screen is
    # the size of whichever mode is chosen (a fixed 800 x 600 kept Workbench at 800 x 600 after the user picked
    # 1920 x 1080, 10 Oct 2026). Up to 0.8.1 Parts/OpenRTG wrote it on every new install, and Parts/OpenRTG/Upgrade
    # (the same part without it) was for upgrades; since the part itself has none, that drawer is gone.
    mode = screenmode_prefs(0x61011100, 0xFFFF, 0xFFFF, 16)
    md = rt.dir / "Mode"
    (md / "Prefs").mkdir(parents=True)
    (md / "Prefs/ScreenMode.prefs").write_bytes(mode)
    (md / "Prefs/ScreenMode-ENV.prefs").write_bytes(mode)
    (md / "Part").write_text(f"part OpenRTGMode {rtv}\n; Workbench on OpenRTG's 16-bit Native mode, asked for on a new install\n"
                             'copy "Prefs/ScreenMode.prefs" "ENVARC:Sys/ScreenMode.prefs"\n'
                             'copy "Prefs/ScreenMode-ENV.prefs" "ENV:Sys/ScreenMode.prefs"\n', encoding="latin-1")

    # OpenGPU: opengpu.library (openamigartg) and its drivers and modules in
    # LIBS:OpenGPU/. ACRTG.gpu (amigachrome-guest) puts the work on the ACRTG
    # board's ring, so the PC draws it; with no board it steps aside and
    # opengpu.library draws on the CPU. SDL2.module and GL.module (Mesa) load
    # when a program linked with libSDL2.a or libGL.a first calls them.
    # Picasso96 screens too: it doesn't need the OpenRTG part.
    gl = rtg / "gl"
    gp = Part(top, "OpenGPU", "0.15", "OpenGPU: opengpu.library with OpenGfx, the ACRTG board's driver, and SDL 2 and GL (Mesa) for programs")
    gp.add("Libs/opengpu.library", (rtg / "opengpu.library").read_bytes(), "LIBS:opengpu.library")
    gp.add("Libs/OpenGPU/ACRTG.gpu", acrtg_gpu, "LIBS:OpenGPU/ACRTG.gpu")
    gp.add("Libs/OpenGPU/SDL2.module", (rtg / "SDL2.module").read_bytes(), "LIBS:OpenGPU/SDL2.module")
    # SDL2_image and SDL2_mixer (openamigartg #54): resident modules beside SDL2.module,
    # decoding on the x86 or ARM64 cores first; their licences go in OpenUp's drawer
    sats = [n for n in ("SDL2_image", "SDL2_mixer") if (rtg / f"{n}.module").is_file()]
    for n in sats:
        gp.add(f"Libs/OpenGPU/{n}.module", (rtg / f"{n}.module").read_bytes(), f"LIBS:OpenGPU/{n}.module")
    gp.add("Libs/OpenGPU/GL.module", (gl / "GL.module").read_bytes(), "LIBS:OpenGPU/GL.module")
    gp.add("C/OpenGPUCheck", (rtg / "OpenGPUCheck").read_bytes(), "C:OpenGPUCheck")
    # tinygl.library 53 (openamigartg library/tinygl, from #50): the stand-in TinyGL programs
    # open; their calls are in the program (libtinygl.a) and run on GL.module
    if (rtg / "tinygl.library").is_file():
        gp.add("Libs/tinygl.library", (rtg / "tinygl.library").read_bytes(), "LIBS:tinygl.library")
    # opengfx.library 1.2 (amigachrome-guest): OpenGfx is inside opengpu.library 0.6, and the
    # stub hands a program that opens opengfx.library the same calls there
    gp.add("Libs/opengfx.library", (a.guest / "libraries/opengfx/build-amiga/lib/opengfx.library").read_bytes(),
           "LIBS:opengfx.library")
    # The modules' third-party licences (Mesa: MIT; SDL 2 and its Amiga back ends:
    # Zlib), in OpenUp's drawer: not in LIBS:OpenGPU/, where the library looks for drivers
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
            raise SystemExit(f"OpenGPU: {name}'s licence text wasn't found (GL_WORK, {sdl})")
        gp.add(f"Utilities/OpenUp/Licences/{name}", f.read_bytes(), f"SYS:Utilities/OpenUp/Licences/{name}")
    gp.write()

    # Warp3D: Warp3D.library 5.0 (openamigartg), drawing through OpenGPU or
    # on the CPU. Another Warp3D.library (Wazp3D's, say) is kept in
    # SYS:Storage/OpenUp-Superseded, and the Installer takes Wazp3D's
    # settings over once with Warp3DPrefs FROMWAZP3D SAVE. Needs a 68040 or
    # 68060 with its FPU, as Warp3D always has: the Installer checks.
    w3 = Part(top, "Warp3D", "5.0", "Warp3D 5.0 on OpenGPU, with Warp3DPrefs, W3DTest and W3DCheck")
    w3.add("Libs/Warp3D.library", (rtg / "Warp3D.library").read_bytes(), "LIBS:Warp3D.library")
    w3.add("C/Warp3DPrefs", (rtg / "Warp3DPrefs").read_bytes(), "C:Warp3DPrefs")
    w3.add("C/W3DCheck", (rtg / "W3DCheck").read_bytes(), "C:W3DCheck")
    w3.add("Tools/W3DTest", (rtg / "W3DTest").read_bytes(), "SYS:Tools/W3DTest")
    w3.add("Tools/W3DTest.info", icons.icon(icons.WBTOOL, "install", stack=65536), "SYS:Tools/W3DTest.info")
    # MiniGL (openamigartg #37): minigl.library 29, OpenGL 1.1 on the same 3D
    # core, with its MGLTest demo, when the openamigartg checkout builds them
    if (rtg / "minigl.library").is_file():
        w3.lines[1] = "; Warp3D 5.0 and MiniGL on OpenGPU, with Warp3DPrefs, W3DTest, W3DCheck and MGLTest"
        w3.add("Libs/minigl.library", (rtg / "minigl.library").read_bytes(), "LIBS:minigl.library")
        if (rtg / "MGLTest").is_file():
            w3.add("Tools/MGLTest", (rtg / "MGLTest").read_bytes(), "SYS:Tools/MGLTest")
            w3.add("Tools/MGLTest.info", icons.icon(icons.WBTOOL, "install", stack=65536), "SYS:Tools/MGLTest.info")
    w3.write()

    # OpenDemos (openamigamesa): Boing, Chrome Gears and Copper Tunnel, linked
    # with libGL.a, so GL comes from the OpenGPU part's GL.module: on the PC's
    # graphics chip on AmigaChrome, on the 68k elsewhere. One icon a demo, a
    # script IconX runs without a window.
    dm = None
    if a.openamigamesa:
        demos = gl_work / "mesa-build-m68k-amigaos/OpenDemos.stripped"
        dm = Part(top, "OpenDemos", "0.1", "OpenDemos: Boing, Chrome Gears and Copper Tunnel, in GL on OpenGPU")
        dm.add("Utilities/OpenDemos/OpenDemos", demos.read_bytes(), "SYS:Utilities/OpenDemos/OpenDemos")
        for name, demo in (("Boing", "boing"), ("Chrome Gears", "gears"), ("Copper Tunnel", "tunnel")):
            dm.add(f"Utilities/OpenDemos/{name}", f"Run >NIL: SYS:Utilities/OpenDemos/OpenDemos {demo}\n".encode(),
                   f"SYS:Utilities/OpenDemos/{name}")
            dm.add(f"Utilities/OpenDemos/{name}.info", icons.icon(icons.WBPROJECT, "page", "C:IconX", ["WINDOW=NIL:"], stack=8192),
                   f"SYS:Utilities/OpenDemos/{name}.info")
        dm.add("Utilities/OpenDemos.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 120)), "SYS:Utilities/OpenDemos.info", own=True)
        dm.write()

    # OpenCrypto (openamigatls): opencrypto.library 2.0. On AmigaChrome its sums run
    # on the PC (AC090's magic functions); elsewhere its own 68k code runs.
    cr = None
    if a.openamigatls:
        ocl = (a.openamigatls / "build-amiga/lib/opencrypto.library").read_bytes()
        ocv = re.search(rb"opencrypto\.library ([0-9]+\.[0-9]+)", ocl)
        cr = Part(top, "OpenCrypto", ocv.group(1).decode() if ocv else "2.0",
                  "OpenCrypto: opencrypto.library, SSH's and TLS's sums, on x86 or ARM64 cores on AmigaChrome")
        cr.add("Libs/opencrypto.library", ocl, "LIBS:opencrypto.library")
        cr.write()

    # OpenTLS (openamigatls): opentls.library, the TLS client the Open programs
    # use (OpenFTP's FTPS first), with its maths through OpenCrypto; and the
    # CA bundle in S:OpenTLS/ with its licence (MPL-2.0), refreshed by each
    # OpenUp ("add": only OpenUp's own copy is replaced). The user's own CAs
    # go in ENVARC:OpenTLS/certs/, which OpenUp never touches.
    tls = opentls_part(top, a.openamigatls) if a.openamigatls else None

    # OpenPuTTY (openamigaputty), as its release lays it out, in a drawer in
    # SYS:Utilities; its opencrypto.library is the OpenCrypto part's
    pt = None
    if a.openamigatls and a.openamigaputty:
        pu = a.openamigaputty
        pv = re.search(r'OPENPUTTY_VERSION "([0-9.]+)"', (pu / "amiga/main.c").read_text(errors="replace"))
        stripped = pu / "build/os3-020/OpenPuTTY.stripped"
        gcc16 = Path(os.environ.get("OS32_GCC16", Path.home() / "AmigaChrome/stoves/os32-gcc16"))
        run([gcc16 / "prefix/bin/m68k-amigaos-strip", "-o", stripped, pu / "build/os3-020/OpenPuTTY"])
        pi = pu / "build/icons"
        pt = Part(top, "OpenPuTTY", pv.group(1) if pv else "0.2", "OpenPuTTY: PuTTY's SSH, Telnet and serial terminal, on OpenCrypto")
        d = "Utilities/OpenPuTTY"
        for rel, data in (("OpenPuTTY", stripped.read_bytes()), ("OpenPuTTY.info", (pi / "OpenPuTTY.info").read_bytes()),
                          ("ReadMe", (pu / "docs/ReadMe.txt").read_bytes()), ("ReadMe.info", (pi / "Document.info").read_bytes()),
                          ("LICENSE", (pu / "LICENSE").read_bytes()), ("LICENSE.info", (pi / "Document.info").read_bytes()),
                          ("Licences/PuTTY.txt", (pu / "upstream/putty/LICENCE").read_bytes()),
                          ("Licences/PuTTY.txt.info", (pi / "Document.info").read_bytes()),
                          ("Licences.info", (pi / "Drawer.info").read_bytes())):
            pt.add(f"{d}/{rel}", data, f"SYS:{d}/{rel}")
        pt.add("Utilities/OpenPuTTY.info", (pi / "Drawer.info").read_bytes(), "SYS:Utilities/OpenPuTTY.info", own=True)
        pt.write()

    # Tata (openamigatata): warns before a program or file known to fail on
    # AmigaOS 3.2.3 or with OpenUp loads. The list is S:Tata.known (the user,
    # 8 October 2026: S:, not LIBS:), replaced by each OpenUp, the old one
    # kept in SYS:Storage; the user's own ENVARC:Tata/Known and decisions are
    # never touched. The Installer leaves it unticked while Tata is 0.x.
    tt = None
    if a.openamigatata:
        tt_src = a.openamigatata
        tb_os3 = tt_src / "build/os3"
        tv = re.search(r'TATA_VERSION\s+"([0-9.]+)"', (tt_src / "src/tata_priv.h").read_text(errors="replace"))
        tt = Part(top, "Tata", tv.group(1) if tv else "0.2",
                  "Tata: warns before loading programs and files known to fail (off unless picked while 0.x)")
        tt.add("C/Tata", (tb_os3 / "Tata").read_bytes(), "SYS:C/Tata")
        tt.add("Prefs/OpenTata", (tb_os3 / "Tata-Prefs").read_bytes(), "SYS:Prefs/OpenTata")
        tt.add("Prefs/OpenTata.info", (tt_src / "icons/Tata.info").read_bytes(), "SYS:Prefs/OpenTata.info")
        tt.add("S/Tata.known", (tt_src / "data/Tata.known").read_bytes(), "S:Tata.known")
        tt.line("startup Run >NIL: C:Tata START")
        tt.write()

    # OpenFTP (openamigaftp), in a drawer in SYS:Utilities. FTPS works through
    # OpenTLS (opentls.library), which the Installer picks with it. Plain FTP
    # works without it, and OpenFTP says in words that FTPS needs OpenTLS.
    # Off by default.
    ftp = None
    if a.openamigaftp:
        fs = a.openamigaftp
        fv = re.search(r'#define VERSION_STRING "([0-9.]+)"', (fs / "app/openftp.c").read_text(errors="replace"))
        ftp = Part(top, "OpenFTP", fv.group(1) if fv else "0.1",
                   "OpenFTP: FTP and FTPS in two panes (FTPS works through OpenTLS)")
        d = "Utilities/OpenFTP"
        for rel, data in (("OpenFTP", (fs / "build/os3/OpenFTP").read_bytes()), ("OpenFTP.info", (fs / "package/OpenFTP.info").read_bytes()),
                          ("ReadMe", (fs / "README.md").read_bytes()), ("ReadMe.info", (fs / "package/ReadMe.info").read_bytes()),
                          ("LICENSE", (fs / "LICENSE").read_bytes()), ("LICENSE.info", (fs / "package/ReadMe.info").read_bytes())):
            ftp.add(f"{d}/{rel}", data, f"SYS:{d}/{rel}")
        ftp.add("Utilities/OpenFTP.info", (fs / "package/Drawer.info").read_bytes(), "SYS:Utilities/OpenFTP.info", own=True)
        ftp.write()

    # Unofficial Telegram for Amiga (Open) (openamigatelegram): the Amiga
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
        tg.add("Utilities/OpenUp/Licences/UnofficialTelegram.txt", (ts / "LICENSE").read_bytes(),
               "SYS:Utilities/OpenUp/Licences/UnofficialTelegram.txt")
        tg.write()

    # The OpenGPU developer files (openamigartg tools/make_sdk.py): the kit as
    # it is, in SYS:Developer/OpenGPU-SDK, with an assign (amigachrome
    # docs/design/Design-OpenUp-OpenGPU-Developer-Files.md). Off by default.
    dev = None
    if a.opengpu_sdk:
        kits = sorted(a.opengpu_sdk.glob("OpenGPU-SDK-*/"))
        if not kits:
            raise SystemExit(f"no OpenGPU-SDK-* kit in {a.opengpu_sdk}")
        kit = kits[-1]
        dev = Part(top, "OpenGPUDev", kit.name.rsplit("-", 1)[1],
                   "OpenGPU developer files: SDL 2, OpenGL and GLA headers and libraries for building programs")
        for f in sorted(p for p in kit.rglob("*") if p.is_file()):
            rel = f.relative_to(kit).as_posix()
            dev.add(f"Developer/OpenGPU-SDK/{rel}", f.read_bytes(), f"SYS:Developer/OpenGPU-SDK/{rel}")
        # only where there is none (as Developer.info): the OpenIcons part gives it OpenIcons' picture, and
        # "add" put the plain drawer back over that picture on every upgrade, so both parts wrote it each run
        dev.add("Developer/OpenGPU-SDK.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)),
                "SYS:Developer/OpenGPU-SDK.info", default=True)
        dev.add("Developer.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 120)), "SYS:Developer.info", default=True)
        dev.line("startup Assign OpenGPU-SDK: SYS:Developer/OpenGPU-SDK")
        dev.write()

    # OpenInput (openamigainput): openinput.library, every game controller in one
    # standard layout; C:OIList; OpenPrefs Gamepads (openamigaprefs) to test, map
    # and choose ports; the docs. "Gamepads USE" at boot applies the saved settings
    # (the ReadJoyPort patch too, when the user switched it on). Off by default.
    # On AmigaChrome the cores' pad window (amigachrome #352) is still to come, so
    # an instance sees only the Amiga's ports until it is there.
    oin = None
    if a.openamigainput:
        oib = a.openamigainput / "build"
        oil = (oib / "openinput.library").read_bytes()
        oiv = re.search(rb"openinput\.library ([0-9]+\.[0-9]+)", oil)
        oin = Part(top, "OpenInput", oiv.group(1).decode() if oiv else "1.2",
                   "OpenInput: game controllers for every program, and Gamepads in Prefs (on AmigaChrome: the Amiga's ports for now)")
        oin.add("Libs/openinput.library", oil, "LIBS:openinput.library")
        oin.add("C/OIList", (oib / "OIList").read_bytes(), "SYS:C/OIList")
        # Gamepads is a section of OpenPrefs (W48); its name's stub is in the OpenPrefs part
        d = "Utilities/OpenUp/Docs"
        for name, src in (("OpenInput.txt", a.openamigainput / "README.md"),
                          ("Design-OpenInput.txt", a.openamigainput / "Design-OpenInput.md"),
                          ("Gamepads.txt", a.openamigaprefs / "Gamepads/gamepads.readme")):
            oin.add(f"{d}/{name}", src.read_bytes(), f"SYS:{d}/{name}")
            oin.add(f"{d}/{name}.info", icons.icon(icons.WBPROJECT, "page", "SYS:Utilities/MultiView", stack=16384), f"SYS:{d}/{name}.info")
        oin.add("Utilities/OpenUp/Docs.info", icons.icon(icons.WBDRAWER, "drawer", window=(320, 120)), "SYS:Utilities/OpenUp/Docs.info", default=True)
        oin.add("Utilities/OpenUp/Licences/OpenInput.txt", (a.openamigainput / "LICENSE").read_bytes(),
                "SYS:Utilities/OpenUp/Licences/OpenInput.txt")
        # the boot's Gamepads USE is in OpenPrefs APPLY (the OpenPrefs part) since W48
        oin.write()

    openlibs_parts(top, a)

    # GCC 16's FPCR clash (openamigartg tools/fpcr_check.py) must not be in
    # anything the graphics parts ship, whichever stove built it.
    objdump = next((p for p in (Path.home() / "AmigaChrome/stoves/os32-gcc16/prefix/bin/m68k-amigaos-objdump",
                                STOVE / "prefix/bin/m68k-amigaos-objdump") if p.is_file()), None)
    if objdump:
        shipped = [f for part in (rt, gp, w3, dm, cr, tls, pt, ftp) if part for f in sorted(part.dir.rglob("*"))
                   if f.is_file() and f.name != "Part" and not f.name.endswith((".info", ".prefs")) and f.read_bytes()[:4] == b"\0\0\x03\xf3"]
        run([sys.executable, a.openamigartg / "tools/fpcr_check.py", objdump] + shipped)

    # OpenFiles: the file manager (openamigafilemanager). Double-click opens
    # files with their icon's program, else OpenView (the OpenPrint part).
    of = Part(top, "OpenFiles", "0.3.1", "OpenFiles: two panes with tabs, the drives bar, safe copies, a trashcan and undo")
    of.add("Utilities/OpenFiles", (a.openamigafilemanager / "build/os3/OpenFiles").read_bytes(), "SYS:Utilities/OpenFiles")
    of.add("Utilities/OpenFiles.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenFiles.info")
    of.write()

    # OpenPlay: the media player for every datatype (openamigamediaplayer).
    # Files can be shift-clicked with it or dropped on its window.
    op = Part(top, "OpenPlay", "0.1.6", "OpenPlay: plays everything the datatypes open, with a playlist")
    op.add("Utilities/OpenPlay", (a.openamigamediaplayer / "build/os3/OpenPlay").read_bytes(), "SYS:Utilities/OpenPlay")
    op.add("Utilities/OpenPlay.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenPlay.info")
    op.write()

    # OpenTitle: the title bar's logo, free memory, clock and settings cog, and its editor (openamigaprefs)
    tbo = a.openamigaprefs / "TitleBar/build/os3"
    tbb = (tbo / "OpenTitle").read_bytes()
    tb = Part(top, "OpenTitle", binary_version(tbb, "OpenTitle") or "0.7",
              "OpenTitle: AmigaChrome's logo, free memory, a clock, the settings cog and the taskspace on the title bar")
    tb.add("C/OpenTitle", tbb, "SYS:C/OpenTitle")
    tb.line("startup Run >NIL: C:OpenTitle")
    tb.write()

    # OpenDateTime: the date, the time and the region (openamigaprefs DateTime). The time zone is a place, and
    # summer time follows it by itself (0.2); the clock is set through C:Date and C:SetClock with NIL: handles
    # (0.2.1). A copy already there (put in SYS:Prefs by hand) is kept in SYS:Storage; its icon stays.
    # Since W48 a section of OpenPrefs: this part is its name's stub (SYS:Prefs/OpenDateTime)
    dtb = (a.openamigaprefs / "OpenPrefs/build/os3/stubs/OpenDateTime").read_bytes()
    dtp = Part(top, "OpenDateTime", binary_version(dtb, "OpenDateTime") or "2.0",
               "OpenDateTime: the date, the time and the region, with summer time by itself (in OpenPrefs)")
    dtp.add("Prefs/OpenDateTime", dtb, "SYS:Prefs/OpenDateTime")
    dtp.write()

    # OpenBench: the Team's own Workbench (openamigabench desktop/), with the dock and the Open editors built in.
    # It goes in SYS:System; the machine runs it where its Startup-Sequence starts SYS:System/OpenBench in
    # LoadWB's place (falling back to LoadWB with Shift held, or without RTG). OpenUp doesn't change the
    # Startup-Sequence yet, so elsewhere it is there to try. A copy already there is kept in SYS:Storage.
    # openicon.library (icon.library R2) and icon.library R3 stay in openamigabench: neither is switched on.
    bench = None
    if a.openamigabench:
        obb = (a.openamigabench / "build/os3/OpenBench").read_bytes()
        bench = Part(top, "OpenBench", binary_version(obb, "OpenBench") or "0.1",
                     "OpenBench: the Team's own Workbench, with the dock and the Open editors built in (preview)")
        bench.add("System/OpenBench", obb, "SYS:System/OpenBench")
        bench.write()

    # Sound: OpenPrefs Sound (the volume, Paula's and AHI's levels, the beep)
    # in SYS:Prefs/OpenSound (the OS's Sound goes to SYS:Prefs/Classic:
    # OpenUpTool RECONCILE), and OpenSpeaker on the menu bar (openamigaprefs).
    so = a.openamigaprefs / "Sound/build/os3"
    snd = Part(top, "Sound", "0.2.2", "Sound: the volume, Paula's and AHI's levels, a speaker on the menu bar, and OpenAHI on AmigaChrome")
    snd.add("C/OpenSpeaker", (so / "OpenSpeaker").read_bytes(), "SYS:C/OpenSpeaker")   # the editor: OpenPrefs's Sound (W48)
    # OpenAHI, folded into Sound (the user, 9 Oct 2026), is the Team's own AHI for the ACAHI board: ahi.device
    # (devices/ahi.device, MIT, no Aminet code), acaudio.audio and DEVS:AudioModes/ACAUDIO (amigachrome's
    # vendor/amigachrome-guest/devices/acaudio.audio, built here) and AHI's settings on the board's Host mix
    # mode (mkahiprefs.py there; "default": a user's own stay). Never Aminet's AHI.
    acb = ROOT / "build/acaudio"
    ahb = ROOT / "build/ahidevice"
    sys.path.insert(0, str(a.amigachrome / "vendor/amigachrome-guest/devices/acaudio.audio"))
    import mkahiprefs
    # Each of these lines is "when AmigaChrome" (OpenUpTool 0.6.3): the board is AmigaChrome's, so nothing
    # lands on a real Amiga or a PiStorm, where Sound is installed too.
    n_before = len(snd.lines)
    snd.add("Devs/ahi.device", (ahb / "Devs/ahi.device").read_bytes(), "DEVS:ahi.device")
    snd.add("Devs/AHI/acaudio.audio", (acb / "AHI/acaudio.audio").read_bytes(), "DEVS:AHI/acaudio.audio")
    snd.add("Devs/AudioModes/ACAUDIO", (acb / "AudioModes/ACAUDIO").read_bytes(), "DEVS:AudioModes/ACAUDIO")
    prefs = mkahiprefs.prefs()
    snd.add("Prefs/ahi.prefs", prefs, "ENVARC:Sys/ahi.prefs", default=True)
    snd.add("Prefs/ahi.prefs", prefs, "ENV:Sys/ahi.prefs", default=True)
    # OpenAHI's licence note, with the other licences: ahi.device is MIT, the Team's own, written from AHI's
    # public interface, with no Aminet code in it
    lic = (a.amigachrome / "LICENSE").read_text(encoding="utf-8", errors="replace")
    note = ("OpenAHI: ahi.device, acaudio.audio and DEVS:AudioModes/ACAUDIO\n\n"
            "OpenAHI is the Team's own AHI for AmigaChrome's ACAHI sound board. Its ahi.device is written by\n"
            "the Team from AHI's public interface (devices/ahi.h, libraries/ahi_sub.h and AHI's documented\n"
            "behaviour). No Aminet code or binaries are in it, and nothing from Aminet is needed. Programs that\n"
            "use AHI work unchanged. MIT licence, as follows.\n\n" + lic)
    snd.add("Utilities/OpenUp/Licences/OpenAHI.txt", note.encode("latin-1", "replace"), "SYS:Utilities/OpenUp/Licences/OpenAHI.txt")
    snd.lines[n_before:] = ["when AmigaChrome " + l for l in snd.lines[n_before:]]
    # the boot's Sound USE is in OpenPrefs APPLY (the OpenPrefs part) since W48
    snd.write()

    # OpenEdit: the text editor (openamigaedit), with its kinds of file in ENVARC:
    oe = a.openamigaedit / "build/os3"
    ed = Part(top, "OpenEdit", "0.1.2", "OpenEdit: a text editor with tabs, find and replace, and colours for scripts and code")
    ed.add("C/OpenEdit", (oe / "OpenEdit").read_bytes(), "SYS:C/OpenEdit")
    for k in sorted((oe / "Kinds").glob("*.kind")):
        ed.add(f"Kinds/{k.name}", k.read_bytes(), f"ENVARC:OpenEdit/Kinds/{k.name}")
    # its Tools icon: a script IconX runs without a window, so it opens OpenEdit with no file
    ed.add("Tools/OpenEdit", b"Run >NIL: C:OpenEdit\n", "SYS:Tools/OpenEdit")
    ed.add("Tools/OpenEdit.info", icons.icon(icons.WBPROJECT, "page", "C:IconX", ["WINDOW=NIL:"], stack=8192), "SYS:Tools/OpenEdit.info")
    ed.write()

    # OpenDrawer: the drawer windows of the Team's own Workbench (opengadtools workbench/), as a
    # program beside the OS's Workbench until the Team's workbench.library opens them itself. Its
    # icon opens SYS:, and drawers dropped on it or shift-selected with it open in windows of their
    # own; in a Shell, OpenDrawer [path ...] (SYS:Utilities is on the path). Off by default.
    odb = (a.opengadtools / "build/os3/OpenDrawer").read_bytes()
    od = Part(top, "OpenDrawer", binary_version(odb, "OpenDrawer") or "0.2",
              "OpenDrawer: drawer windows with an icon line, icons or a list, search and Snapshot")
    od.add("Utilities/OpenDrawer", odb, "SYS:Utilities/OpenDrawer")
    od.add("Utilities/OpenDrawer.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Utilities/OpenDrawer.info")
    od.write()

    # OpenWrite: the word processor (openamigawrite), as its 1.0 release lays it
    # out: a drawer, with the dictionary beside the program, where it looks
    # (PROGDIR:Dictionaries), and OWConvert in C: for scripts
    ow_src = a.openamigawrite
    ob = ow_src / "build/os3"
    wr = Part(top, "OpenWrite", "2.0.1", "OpenWrite: a word processor for DOCX, ODT and Amiga documents, with PDF and printing")
    wr.add("Utilities/OpenWrite/OpenWrite", (ob / "OpenWrite").read_bytes(), "SYS:Utilities/OpenWrite/OpenWrite")
    wr.add("Utilities/OpenWrite/OpenWrite.info", icons.icon(icons.WBTOOL, "page", stack=32768), "SYS:Utilities/OpenWrite/OpenWrite.info")
    for d in sorted((ow_src / "Dictionaries").glob("*.words")):
        wr.add(f"Utilities/OpenWrite/Dictionaries/{d.name}", d.read_bytes(), f"SYS:Utilities/OpenWrite/Dictionaries/{d.name}")
    # the newest release's notes, as the release archive carries them
    notes = sorted(ow_src.glob("RELEASE_NOTES_*.md"), key=lambda f: [int(n) for n in f.stem.split("_")[-1].split(".")])
    for name, src in (("LICENSE", "LICENSE"), ("THIRD_PARTY.txt", "THIRD_PARTY.md"), ("Release-Notes.txt", notes[-1].name if notes else "RELEASE_NOTES_1.0.md")):
        if (ow_src / src).is_file():
            wr.add(f"Utilities/OpenWrite/{name}", (ow_src / src).read_bytes(), f"SYS:Utilities/OpenWrite/{name}")
    for lic in sorted((ow_src / "licenses").glob("*")):
        wr.add(f"Utilities/OpenWrite/licenses/{lic.name}", lic.read_bytes(), f"SYS:Utilities/OpenWrite/licenses/{lic.name}")
    wr.add("Utilities/OpenWrite.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)), "SYS:Utilities/OpenWrite.info", own=True)
    wr.add("C/OWConvert", (ob / "OWConvert").read_bytes(), "SYS:C/OWConvert")
    wr.write()

    # OpenMUI: zunemaster.library, MUI from Zune for OS 3.x (openamigamui), a
    # preview beside MUI, which it doesn't replace; ZuneTest shows it working.
    # muimaster.library (0.2) is the face MUI programs open, a forwarder to
    # zunemaster.library at MUI 3.8's version. It goes only where
    # LIBS:muimaster.library is absent ("add": a real MUI, or any other
    # program's file, stays; OpenUp's own copy is kept up to date and the
    # uninstaller removes it).
    mui = a.openamigamui / "build/os3"
    om = Part(top, "OpenMUI", "0.2.1", "OpenMUI: zunemaster.library, a muimaster.library face where there is no MUI (preview), and ZuneTest")
    om.add("Libs/zunemaster.library", (mui / "zunemaster.library").read_bytes(), "LIBS:zunemaster.library")
    om.add("Libs/muimaster.library", (mui / "muimaster.library").read_bytes(), "LIBS:muimaster.library", own=True)
    om.add("Tools/ZuneTest", (mui / "ZuneTest").read_bytes(), "SYS:Tools/ZuneTest")
    om.add("Tools/ZuneTest.info", icons.icon(icons.WBTOOL, "install", stack=16384), "SYS:Tools/ZuneTest.info")
    om.write()

    # OpenMulticore: openmulticore.library runs a 68k job on another core of a
    # cores board (openservice.device's cpu.m68k/1), or on the main CPU when
    # there is none, so programs that offload (openmodule.datatype) work on
    # every machine; OMCTest checks it, and OMCBench (0.3, a Shell command:
    # OMCBench [JOBS n] [ITEMS n] [WORK n] [STRICT]) measures what is worth
    # sending to the board.
    mcb = a.openamigamulticore / "build"
    mcv = binary_version((mcb / "openmulticore.library").read_bytes(), "openmulticore.library") or "0.2"
    with_bench = (mcb / "OMCBench").is_file()
    mc = Part(top, "OpenMulticore", mcv, "OpenMulticore: openmulticore.library, jobs on the cores board's other cores, and OMCTest"
              + (" and OMCBench" if with_bench else ""))
    mc.add("Libs/openmulticore.library", (mcb / "openmulticore.library").read_bytes(), "LIBS:openmulticore.library")
    mc.add("Tools/OMCTest", (mcb / "OMCTest").read_bytes(), "SYS:Tools/OMCTest")
    mc.add("Tools/OMCTest.info", icons.icon(icons.WBTOOL, "install", stack=16384), "SYS:Tools/OMCTest.info")
    if with_bench:                          # no icon: it prints its tables in the Shell it runs from
        mc.add("Tools/OMCBench", (mcb / "OMCBench").read_bytes(), "SYS:Tools/OMCBench")
    mc.write()

    # OpenMail: the mail reader (openamigamail), in a drawer with its providers
    # and themes beside it (PROGDIR:Providers); OpenMailCheck in C:. It reaches
    # mail servers through OpenTLS (opentls.library), which the Installer picks with it.
    mb = a.openamigamail / "build/os3"
    vm = re.search(rb"\$VER: OpenMail ([0-9.]+)", (mb / "OpenMail").read_bytes())   # the program's own version
    mv = vm.group(1).decode() if vm else "0.3"
    ml = Part(top, "OpenMail", mv, "OpenMail: a mail reader for IMAP over TLS (through OpenTLS), and C:OpenMailCheck")
    ml.add("Utilities/OpenMail/OpenMail", (mb / "OpenMail").read_bytes(), "SYS:Utilities/OpenMail/OpenMail")
    mi = a.openamigamail / "package/icons"
    ml.add("Utilities/OpenMail/OpenMail.info", (mi / "OpenMail.info").read_bytes(), "SYS:Utilities/OpenMail/OpenMail.info")
    for sub in ("Providers", "package/Themes"):
        for f in sorted((a.openamigamail / sub).glob("*")):
            if f.is_file():
                d = f"Utilities/OpenMail/{Path(sub).name}/{f.name}"
                ml.add(d, f.read_bytes(), f"SYS:{d}")
    for name, src in (("LICENSE", "LICENSE"), ("OpenMail.readme", "openmail.readme"), ("LICENCE.Boxie", "package/icons/LICENCE.Boxie")):
        if (a.openamigamail / src).is_file():
            ml.add(f"Utilities/OpenMail/{name}", (a.openamigamail / src).read_bytes(), f"SYS:Utilities/OpenMail/{name}")
    ml.add("Utilities/OpenMail.info", (mi / "Drawer.info").read_bytes(), "SYS:Utilities/OpenMail.info", own=True)
    ml.add("C/OpenMailCheck", (mb / "OpenMailCheck").read_bytes(), "SYS:C/OpenMailCheck")
    ml.write()

    # OpenBrowser: too big for the pack (its engine is about 100 MB), so it
    # comes in its own archive, OpenUp-<version>-OpenBrowser.lha, which unpacks
    # into the same OpenUp drawer as Parts/OpenBrowser. The Installer offers it
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
        bv = re.search(r"OpenBrowser-([0-9.]+)", a.openbrowser.name)
        browser_top = a.out / "companion" / "OpenUp"
        bp = Part(browser_top, "OpenBrowser", bv.group(1) if bv else "0.5",
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
    (a.out / "OpenUp.info").write_bytes(icons.icon(icons.WBDRAWER, "drawer", window=(440, 170)))
    try:
        os32_icons(a.openamigaprint, icons, [(top / "Install-OpenUp.info", "install"), (top / "Uninstall-OpenUp.info", "install"),
                                             (core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info", "install"),
                                             (opp.dir / "Prefs/OpenPrefs.info", "page"),
                                             (setup.dir / "Prefs/OpenUp-Setup.info", "page"),
                                             (oc.dir / "Utilities/OpenCompress.info", archive_art),
                                             (of.dir / "Utilities/OpenFiles.info", files_art),
                                             (op.dir / "Utilities/OpenPlay.info", play_art),
                                             (od.dir / "Utilities/OpenDrawer.info", "page"),
                                             (w3.dir / "Tools/W3DTest.info", "install"),
                                             (a.out / "OpenUp.info", None)]
                    + ([(w3.dir / "Tools/MGLTest.info", "install")] if (w3.dir / "Tools/MGLTest.info").is_file() else [])
                    + ([(dm.dir / f"Utilities/OpenDemos/{n}.info", "page") for n in ("Boing", "Chrome Gears", "Copper Tunnel")]
                       + [(dm.dir / "Utilities/OpenDemos.info", None)] if dm else [])
                    + ([(tg.dir / "Utilities/UnofficialTelegram.info", "page")] if tg else [])
                    + ([(dev.dir / "Developer/OpenGPU-SDK.info", None), (dev.dir / "Developer.info", None)] if dev else [])
                    + ([(oin.dir / "Utilities/OpenUp/Docs.info", None)]
                       + [(oin.dir / f"Utilities/OpenUp/Docs/{n}.info", "page") for n in ("OpenInput.txt", "Design-OpenInput.txt", "Gamepads.txt")]
                       if oin else []))
        put("LICENCE.Boxie", (icons.BOXIE / "LICENCE").read_bytes())   # the drawer icon is Boxie's (MIT)
    except Exception as e:                  # node or amiga-icon.js missing: the classic icons stay
        print(f"  (OS 3.2-style icons not made: {e}; the classic ones stay)")
    try:
        open_icons(a.amigachrome, [
            (a.out / "OpenUp.info", "drawer-OpenUp"), (top / "Install-OpenUp.info", "Install-OpenUp"),
            (top / "Uninstall-OpenUp.info", "Uninstall-OpenUp"), (core.dir / "Utilities/OpenUp.info", "drawer-OpenUp"),
            (core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info", "Uninstall-OpenUp"),
            (core.dir / "Utilities/OpenMonitor.info", "OpenMonitor"),
            (pr.dir / "Icons/OpenPrint.info", "drawer-OpenPrint"), (pr.dir / "Drawer/OpenView.info", "OpenView"),
            (pr.dir / "Drawer/Printers and Queue.info", "Printers and Queue"),
            (opp.dir / "Prefs/OpenPrefs.info", "OpenPrefs"), (setup.dir / "Prefs/OpenUp-Setup.info", "OpenUp-Setup"),
            (setup.dir / "WBStartup/OpenUp-Setup.info", "OpenUp-Setup"), (oc.dir / "Utilities/OpenCompress.info", "OpenCompress"),
            (of.dir / "Utilities/OpenFiles.info", "OpenFiles"), (op.dir / "Utilities/OpenPlay.info", "OpenPlay"),
            (om.dir / "Tools/ZuneTest.info", "ZuneTest"), (ed.dir / "Tools/OpenEdit.info", "OpenEdit"),
            (wr.dir / "Utilities/OpenWrite/OpenWrite.info", "OpenWrite"),
            (wr.dir / "Utilities/OpenWrite.info", "drawer-OpenWrite")]
            # the icon library's OpenTata and Telegram (#104), when those parts are in the pack
            + ([(tt.dir / "Prefs/OpenTata.info", "OpenTata")] if tt else [])
            + ([(tg.dir / "Utilities/UnofficialTelegram.info", "Telegram")] if tg else []),
            drawers={a.out / "OpenUp.info": [top / "Install-OpenUp.info", top / "Uninstall-OpenUp.info"],
                     core.dir / "Utilities/OpenUp.info": [core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info"],
                     pr.dir / "Icons/OpenPrint.info": sorted((pr.dir / "Drawer").glob("*.info")),
                     wr.dir / "Utilities/OpenWrite.info": [wr.dir / "Utilities/OpenWrite/OpenWrite.info"]},
            )
    except Exception as e:                  # an amigachrome checkout without amiga_icon (before #153): the icons above stay
        print(f"  (the Open apps' icons not made: {e}; the ones above stay)")
    # OpenUp's own drawers as OpenIcons look B drawers with their program's motif (W78, 18 October 2026, Dale:
    # "we need to fix those older icons to our new style"), over the Gorilla-era drawer-NAME and Boxie drawers
    # above; each keeps its window and place. The apps' own animated icons stay as ICONS.md section 1 has them.
    if not a.no_openicons:
        import openicons_part
        n = openicons_part.own_drawers(a.guest, [a.out / "OpenUp.info", core.dir / "Utilities/OpenUp.info",
                                                 pr.dir / "Icons/OpenPrint.info", wr.dir / "Utilities/OpenWrite.info"]
                                       + ([dm.dir / "Utilities/OpenDemos.info"] if dm else [])
                                       + ([dev.dir / "Developer/OpenGPU-SDK.info", dev.dir / "Developer.info"] if dev else [])
                                       + ([oin.dir / "Utilities/OpenUp/Docs.info"] if oin else []))
        print(f"  OpenUp's own drawers in OpenIcons look B: {n}")
    print(f"{top}: {sum(1 for _ in top.rglob('*') if _.is_file())} files")
    check_part_versions(top / "Parts")
    if browser_top:
        check_part_versions(browser_top / "Parts")
    if a.lha:
        arc = a.out / f"OpenUp-{a.version}.lha"
        arc.unlink(missing_ok=True)
        # jlha makes archives; on some systems "lha" is lhasa, which only reads them
        run([shutil.which("jlha") or "lha", "aq", arc.name, "OpenUp", "OpenUp.info"], cwd=a.out)
        print(f"{arc}: {arc.stat().st_size} bytes")
        if browser_top:
            barc = a.out / f"OpenUp-{a.version}-OpenBrowser.lha"
            barc.unlink(missing_ok=True)
            run([shutil.which("jlha") or "lha", "aq", barc.resolve(), "OpenUp"], cwd=browser_top.parent)
            print(f"{barc}: {barc.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
