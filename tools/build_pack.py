#!/usr/bin/env python3
"""Builds OpenUp: a drawer to run the Installer from, and the same as an LhA
archive (DESIGN.md sections 3 and 4).

    python3 tools/build_pack.py [--version 0.1] [--out build/pack]
        [--amigachrome DIR] [--openamigaprint DIR] [--opengadtools DIR]
        [--guest DIR] [--openamigaservice DIR] [--openamigaimage DIR] [--openamigartg DIR]
        [--openamigafilemanager DIR] [--openamigamediaplayer DIR] [--openamigamui DIR]
        [--no-build] [--lha]

Each part is built by its own repository's scripts, from checkouts given on
the command line (or found beside this one), then laid out as it goes, with
a Part file that C/OpenUpTool reads:

  OpenUp/               OpenUpTool and the uninstaller
  OpenSocket/           bsdsocket.library 4.2 and its tools (amigachrome's
                        vendored payload, vendor/opensocket-os32)
  OpenPrint/            OpenPrint and OpenView (openamigaprint's package)
  AmigaChromeSharing/   accontrol.device, ACClip and ACDrop (amigachrome-guest)
  OpenLook/             the look and its themes (opengadtools), and its editor,
                        OpenPrefs Look, with the desktop profiles (openamigaprefs)
  Backdrops/            the themes' desktops as Workbench backdrops (opengadtools)
  OpenTypes/            which program opens each kind of file (openamigaprefs)
  OpenCompress/         OpenCompress, its window and command (openamigacompress)
  OpenService/          openservice.device and Nursery: services from boards and
                        paired Cradles (openamigaservice)
  OpenImage/            datatypes: WebP and WebM on the 68k, and AVIF, HEIC, SVG,
                        FLAC, MP4, DOCX and more through OpenService (openamigaimage)
  OpenWindows/          snapping, Amiga+Tab, the wheel under the pointer and window
                        places, with its editor, OpenPrefs Windows (openamigaprefs)
  OpenDock/             a dock, which takes over ToolManager's, AmiDock's or AmiStart's
                        buttons, and its editor, OpenPrefs Dock (openamigaprefs)
  OpenMenus/            OpenMenus, our own menus (pull-down or pop-up), and their editor,
                        OpenPrefs Menus (openamigaprefs)
  OpenUpSetup/          OpenUp Setup: the first-start wizard and the startup switches
                        (openamigaprefs)
  OpenRTG/              Workbench on OpenRTG's own 16-bit screen, without Picasso96:
                        openrtg.library, its monitor driver and CyberGraphX
                        compatibility (openamigartg; AmigaChrome's ACRTG board)
  OpenFiles/            OpenFiles, the two-pane file manager (openamigafilemanager)
  OpenPlay/             OpenPlay, the media player for every datatype (openamigamediaplayer)
  OpenTitle/            OpenTitle and OpenPrefs Title bar: logo, free memory and clock on the title bar (openamigaprefs)
  Sound/                OpenPrefs Sound and OpenSpeaker, the menu bar speaker (openamigaprefs)
  OpenEdit/             OpenEdit, the text editor with tabs (openamigaedit)
  OpenMUI/              zunemaster.library, MUI from Zune for OS 3.x, and ZuneTest (openamigamui)

Icons are OS 3.2-style, made with openamigaprint's icon writer (ACBuild's
amiga-icon.js). MIT, Copyright (c) 2026 AmigaChrome Team.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEV = ROOT.parent
STOVE = Path(os.environ.get("STOVE", Path.home() / "AmigaChrome/stoves/os32"))
CC = STOVE / "prefix/bin/m68k-amigaos-gcc"

# OpenSocket's files that must be ours, or absent: never another stack's (DESIGN.md section 3).
OPENSOCKET_MARKERS = ["OpenSocket (AmigaChrome)", "ACNet (AmigaChrome)", "AmigaChrome ACNet"]


def run(cmd, cwd=None, env=None):
    print("  $", " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], cwd=cwd, env=env, check=True)


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


def open_icons(amigachrome: Path, jobs: list[tuple[Path, str]], drawers: dict[Path, list[Path]]) -> None:
    """The Open apps' own icons: each job's icon keeps what it is (type,
    default tool, tool types, stack) and gets its picture from icons/open as
    a GlowIcon, dithered for a 16-bit screen. Icons in drawers we share with
    the system (Prefs, Utilities, Tools) are left unplaced, so Workbench puts
    them where there is room; `drawers` maps each drawer icon of our own to
    the icons in it, which are laid out on a grid with the window sized to
    them (amigachrome's icon_theme, as Cradle's new icon set does)."""
    sys.path.insert(0, str(amigachrome / "scripts"))
    import amiga_icon
    import icon_theme

    def frame(path: Path) -> tuple[int, int, bytes]:
        w, h, rgba = amiga_icon.png_decode(path.read_bytes())
        hard = bytearray(rgba)
        for i in range(0, len(hard), 4):              # GlowIcons: every pixel solid or see-through
            hard[i:i + 4] = hard[i:i + 3] + b"\xff" if hard[i + 3] >= 110 else b"\0\0\0\0"
        return w, h, bytes(hard)

    def art(name: str) -> list[tuple[int, int, bytes]]:
        """The icon's two frames (docs/ICONS.md): NAME.png at rest, and NAME-selected.png,
        its second frame, with the glow round it, shown while the icon is chosen."""
        w, h, rest = frame(OPEN_ART / f"{name}.png")
        chosen = OPEN_ART / f"{name}-selected.png"
        sw, sh, sel = frame(chosen) if chosen.exists() else (w, h, rest)
        if (sw, sh) != (w, h):
            raise SystemExit(f"{chosen.name}: {sw}x{sh}, but {name}.png is {w}x{h}")
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
    run([CC, "-noixemul", "-m68020", "-std=gnu99", "-Wall", "-Werror", "-O2", "-fno-common",
         "-o", ROOT / "build/os3/OpenUpTool", ROOT / "src/openuptool.c"], env=env)
    run(["sh", a.opengadtools / "look/build.sh"], env=env)
    run([sys.executable, a.opengadtools / "tools/make_backdrops.py"], env=env)
    g = a.guest
    out = g / "build/guest/os32"
    run(["sh", g / "devices/accontrol.device/build.sh", out], env=env)
    inc = g / "devices/accontrol.device/include"
    run([CC, "-noixemul", "-m68020", "-O2", "-Wall", f"-I{inc}", g / "tools/acclip/acclip.c", "-o", out / "ACClip"], env=env)
    run([CC, "-noixemul", "-m68020", "-O2", "-Wall", f"-I{inc}", f"-I{g / 'common/amiga'}",
         g / "tools/acdrop/acdrop.c", g / "common/amiga/ac_stack.c", "-o", out / "ACDrop"], env=env)
    run(["sh", a.openamigaprefs / "OpenTypes/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Look/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Windows/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Dock/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Setup/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Menus/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "Menus/engine/build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigaprefs / "TitleBar/build.sh"], env=env)
    run(["sh", a.openamigaprefs / "Sound/build.sh"], env=env)
    run(["sh", a.openamigacompress / "build-os3.sh"], env=env)
    run(["sh", a.openamigaservice / "build.sh"], env=dict(os.environ))   # its own compiler (os32-gcc16)
    run(["sh", a.openamigaimage / "Datatypes/build.sh"], env=dict(os.environ))   # os32-gcc16 too
    run(["sh", a.openamigartg / "library/build.sh"], env=env)
    run(["sh", a.openamigafilemanager / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigamediaplayer / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    run(["sh", a.openamigamui / "os3/build.sh"], env=env)
    run(["sh", a.openamigaedit / "build.sh"], env=dict(env, OGT=str(a.opengadtools)))
    p = a.openamigaprint
    for script in ("build-amiga.sh", "build-browser.sh", "build-viewer.sh"):
        run(["sh", p / script], cwd=p, env=env)
    run([sys.executable, p / "tools/make_package.py", "--out", p / "build/package"], cwd=p, env=env)


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
    ap.add_argument("--no-build", action="store_true", help="use what the parts' builds already made")
    ap.add_argument("--lha", action="store_true", help="also make OpenUp-<version>.lha with the host's lha")
    a = ap.parse_args()
    for name in ("amigachrome", "openamigaprint", "opengadtools", "guest", "openamigaprefs", "openamigacompress", "openamigaservice",
                 "openamigaimage", "openamigartg", "openamigafilemanager", "openamigamediaplayer",
                 "openamigamui", "openamigaedit"):
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
        put(script + ".info", icons.icon(icons.WBPROJECT, "install", "Installer",
                                          [f"APPNAME=OpenUp", "MINUSER=NOVICE", "DEFUSER=NOVICE"]))
    put("C/OpenUpTool", tool)
    readme = (ROOT / "openup.readme").read_bytes()
    put("OpenUp.readme", readme)

    # OpenUp itself: the tool and the uninstaller, where they can be found later
    core = Part(top, "OpenUp", a.version, "OpenUpTool and the uninstaller")
    core.add("C/OpenUpTool", tool, "SYS:C/OpenUpTool")
    un = (top / "Uninstall-OpenUp").read_bytes()
    core.add("Utilities/OpenUp/Uninstall-OpenUp", un, "SYS:Utilities/OpenUp/Uninstall-OpenUp")
    core.add("Utilities/OpenUp/Uninstall-OpenUp.info", (top / "Uninstall-OpenUp.info").read_bytes(),
             "SYS:Utilities/OpenUp/Uninstall-OpenUp.info")
    core.add("Utilities/OpenUp.info", icons.icon(icons.WBDRAWER, "drawer", window=(300, 150)), "SYS:Utilities/OpenUp.info", own=True)
    core.write()

    # OpenSocket: amigachrome's vendored payload, by its manifest
    payload = a.amigachrome / "vendor/opensocket-os32"
    m = json.loads((payload / "manifest.json").read_text())
    sock = Part(top, "OpenSocket", "4.2", "bsdsocket.library 4.2 through this PC (" + m.get("identity", "") + ")")
    sock.line("needs AmigaChrome")
    sock.line('keep "SYS:Libs/bsdsocket.library" ' + " ".join(f'"{x}"' for x in OPENSOCKET_MARKERS))
    for f in m["files"]:
        if f.get("installPath"):
            sock.add(f["path"], (payload / f["path"]).read_bytes(), "SYS:" + f["installPath"])
    sock.write()

    # OpenPrint and OpenView: openamigaprint's own package, laid out as its Installer does
    pkg = a.openamigaprint / "build/package/OpenPrint"
    pr = Part(top, "OpenPrint", "0.4", "OpenPrint and OpenView: printing to PDF and IPP, and viewing")
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
    pr.write()

    # AmigaChrome sharing: the control device, the clipboard and the files tools
    gout = a.guest / "build/guest/os32"
    icondir = a.amigachrome / "resources/icons/collections/amigachrome-guest/os3"
    sh = Part(top, "AmigaChromeSharing", "1.2", "accontrol.device, ACClip and ACDrop: the clipboard and files, with this PC")
    sh.line("needs AmigaChrome")
    sh.add("Devs/accontrol.device", (gout / "accontrol.device").read_bytes(), "SYS:Devs/accontrol.device")
    for tool_name in ("ACClip", "ACDrop"):
        sh.add(f"C/{tool_name}", (gout / tool_name).read_bytes(), f"SYS:C/{tool_name}")
        sh.add(f"WBStartup/{tool_name}.info", (icondir / f"{tool_name}.info").read_bytes(), f"SYS:WBStartup/{tool_name}.info")
    sh.write()

    # OpenLook and its themes
    o = a.opengadtools
    look = Part(top, "OpenLook", "0.4", "OpenLook: the OpenGadTools look for every GadTools program, frames and title bars too, and its editor")
    look.add("C/OpenLook", (o / "build/os3/OpenLook").read_bytes(), "SYS:C/OpenLook")
    for t in sorted((o / "themes").glob("*.theme")):
        look.add(f"Themes/{t.name}", t.read_bytes(), f"SYS:Prefs/Presets/Themes/{t.name}")
    # OpenPrefs Look, OpenLook's editor, and the desktop profiles it offers
    look.add("Prefs/Look", (a.openamigaprefs / "Look/build/os3/Look").read_bytes(), "SYS:Prefs/Look")
    look.add("Prefs/Look.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/Look.info")
    for prof in sorted((a.openamigaprefs / "Look/profiles").glob("*.profile")):
        look.add(f"Profiles/{prof.name}", prof.read_bytes(), f"SYS:Prefs/Presets/OpenPrefs/{prof.name}")
    look.line("startup Run >NIL: C:OpenLook")
    look.write()

    # The background pack: each theme's desktop at four screen shapes (opengadtools)
    bd = Part(top, "Backdrops", "0.1", "The themes' desktops as Workbench backdrops")
    for png in sorted((o / "build/backdrops").glob("*.png")):
        bd.add(f"Backdrops/{png.name}", png.read_bytes(), f"SYS:Prefs/Presets/Backdrops/OpenUp/{png.name}")
    bd.write()

    # OpenTypes: which program opens each kind of file (openamigaprefs)
    ty = Part(top, "OpenTypes", "0.2", "OpenTypes: which program opens each kind of file")
    ty.add("Prefs/OpenTypes", (a.openamigaprefs / "OpenTypes/build/os3/OpenTypes").read_bytes(), "SYS:Prefs/OpenTypes")
    ty.add("Prefs/OpenTypes.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/OpenTypes.info")
    ty.write()

    # OpenCompress: one program, the window from Workbench and the command in
    # the Shell (SYS:Utilities is in the Shell's path)
    oc = Part(top, "OpenCompress", "0.3", "OpenCompress: archives and disk images, from its window or the Shell")
    oc.add("Utilities/OpenCompress", (a.openamigacompress / "build/os3/OpenCompress").read_bytes(), "SYS:Utilities/OpenCompress")
    oc.add("Utilities/OpenCompress.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenCompress.info")
    oc.write()

    # OpenService: the services device and Nursery (openamigaservice). It
    # does nothing on the LAN until ENV:OpenService/Paired lists a Cradle.
    sv = Part(top, "OpenService", "0.1", "OpenService: services from AutoConfig boards and paired Cradles")
    sv.add("Devs/openservice.device", (a.openamigaservice / "bin/openservice.device").read_bytes(), "DEVS:openservice.device")
    sv.add("C/Nursery", (a.openamigaservice / "bin/Nursery").read_bytes(), "C:Nursery")
    sv.write()

    # OpenImage: datatypes (openamigaimage). WebP and WebM decode on the 68k;
    # the rest ask media.decode/1 or doc.render/1 through OpenService. A
    # datatype or descriptor already there (Aminet's flac.datatype, say)
    # stays: ours only go where nothing is or where OpenUp put them (so a
    # newer pack updates them), and a machine without a Cradle keeps what it
    # had. Their own classes have names nobody else uses.
    dt = a.openamigaimage / "Datatypes/out"
    img = Part(top, "OpenImage", "0.1", "OpenImage: datatypes for today's pictures, sounds, documents and video")
    for c in sorted((dt / "Classes/DataTypes").glob("*.datatype")):
        img.add(f"Classes/DataTypes/{c.name}", c.read_bytes(), f"SYS:Classes/DataTypes/{c.name}",
                own=c.name in ("webp.datatype", "webm.datatype"))
    for d in sorted((dt / "Devs/DataTypes").iterdir()):
        if d.is_file() and not d.name.endswith(".info"):
            img.add(f"Devs/DataTypes/{d.name}", d.read_bytes(), f"DEVS:DataTypes/{d.name}", own=True)
    img.write()

    # OpenWindows: the commodity, started at boot, and its editor (openamigaprefs)
    ow = Part(top, "OpenWindows", "0.3", "OpenWindows: snapping, Amiga+Tab, the wheel under the pointer, window places")
    ow.add("Prefs/Windows", (a.openamigaprefs / "Windows/build/os3/Windows").read_bytes(), "SYS:Prefs/Windows")
    ow.add("Prefs/Windows.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/Windows.info")
    ow.add("Tools/Commodities/OpenWindows", (a.openamigaprefs / "Windows/build/os3/OpenWindows").read_bytes(),
           "SYS:Tools/Commodities/OpenWindows")
    ow.line("startup Run >NIL: SYS:Tools/Commodities/OpenWindows")
    ow.write()

    # OpenDock: the dock, started at boot, and its editor (openamigaprefs). The
    # Installer runs "SYS:Prefs/Dock SAVE" once, on a first install, so the
    # dock starts with the buttons of a dock already there.
    dock = Part(top, "OpenDock", "0.1", "OpenDock: a dock that takes over ToolManager's, AmiDock's or AmiStart's, and its editor")
    dock.add("C/OpenDock", (a.openamigaprefs / "Dock/build/os3/OpenDock").read_bytes(), "SYS:C/OpenDock")
    dock.add("Prefs/Dock", (a.openamigaprefs / "Dock/build/os3/Dock").read_bytes(), "SYS:Prefs/Dock")
    dock.add("Prefs/Dock.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/Dock.info")
    dock.line("startup Run >NIL: C:OpenDock")
    dock.write()

    # OpenMenus: the engine, started at boot, and its editor (openamigaprefs).
    # The Installer takes MagicMenu's settings over once, when there are any.
    menus = Part(top, "OpenMenus", "0.5", "OpenMenus: our own menus, the menu bar at any edge, right-click menus on Workbench, and their editor")
    menus.add("C/OpenMenus", (a.openamigaprefs / "Menus/build/os3/OpenMenus").read_bytes(), "SYS:C/OpenMenus")
    menus.add("Prefs/Menus", (a.openamigaprefs / "Menus/build/os3/Menus").read_bytes(), "SYS:Prefs/Menus")
    menus.add("Prefs/Menus.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/Menus.info")
    menus.line("startup Run >NIL: C:OpenMenus")
    menus.write()

    # OpenUp Setup: the first-start wizard (openamigaprefs). A project in
    # WBStartup starts it once; it does nothing after ENVARC:OpenAmiga/Setup.done.
    setup = Part(top, "OpenUpSetup", "0.1", "OpenUp Setup: the first-start wizard, with the startup switches")
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
    rtg = a.openamigartg / "build"
    rt = Part(top, "OpenRTG", "0.10", "OpenRTG: Workbench on OpenRTG's own screens, without Picasso96")
    rt.line("needs AmigaChrome")
    rt.line("needs ACRTG")
    rt.line('retire "DEVS:Monitors/acrtg"')
    rt.line('retire "DEVS:Monitors/acrtg.info"')
    rt.add("Libs/openrtg.library", (rtg / "openrtg.library").read_bytes(), "LIBS:openrtg.library")
    rt.add("Libs/opengpu.library", (rtg / "opengpu.library").read_bytes(), "LIBS:opengpu.library")
    rt.add("Libs/OpenRTG/cybergraphics.library", (rtg / "cybergraphics.library").read_bytes(), "LIBS:OpenRTG/cybergraphics.library")
    rt.add("C/OpenRTG", (rtg / "OpenRTG").read_bytes(), "C:OpenRTG")
    rt.add("Devs/Monitors/OpenRTG", (rtg / "OpenRTG-Monitor").read_bytes(), "DEVS:Monitors/OpenRTG")
    mode = screenmode_prefs(0x61011100, 800, 600, 16)
    rt.add("Prefs/ScreenMode.prefs", mode, "ENVARC:Sys/ScreenMode.prefs")
    rt.add("Prefs/ScreenMode-ENV.prefs", mode, "ENV:Sys/ScreenMode.prefs")
    rt.write()

    # OpenFiles: the file manager (openamigafilemanager). Double-click opens
    # files with their icon's program, else OpenView (the OpenPrint part).
    of = Part(top, "OpenFiles", "0.2", "OpenFiles: two panes with tabs, safe copies, a trashcan and undo")
    of.add("Utilities/OpenFiles", (a.openamigafilemanager / "build/os3/OpenFiles").read_bytes(), "SYS:Utilities/OpenFiles")
    of.add("Utilities/OpenFiles.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenFiles.info")
    of.write()

    # OpenPlay: the media player for every datatype (openamigamediaplayer).
    # Files can be shift-clicked with it or dropped on its window.
    op = Part(top, "OpenPlay", "0.1", "OpenPlay: plays everything the datatypes open, with a playlist")
    op.add("Utilities/OpenPlay", (a.openamigamediaplayer / "build/os3/OpenPlay").read_bytes(), "SYS:Utilities/OpenPlay")
    op.add("Utilities/OpenPlay.info", icons.icon(icons.WBTOOL, "install", stack=32768), "SYS:Utilities/OpenPlay.info")
    op.write()

    # OpenTitle: the title bar's logo, free memory and clock, and its editor (openamigaprefs)
    tbo = a.openamigaprefs / "TitleBar/build/os3"
    tb = Part(top, "OpenTitle", "0.4", "OpenTitle: AmigaChrome's logo, free memory and a clock on the title bar")
    tb.add("C/OpenTitle", (tbo / "OpenTitle").read_bytes(), "SYS:C/OpenTitle")
    tb.add("Prefs/TitleBar", (tbo / "TitleBar").read_bytes(), "SYS:Prefs/TitleBar")
    tb.add("Prefs/TitleBar.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/TitleBar.info")
    tb.line("startup Run >NIL: C:OpenTitle")
    tb.write()

    # Sound: OpenPrefs Sound (the volume, Paula's and AHI's levels, the beep)
    # in SYS:Prefs/Sound, whose old editor the Installer copies to
    # SYS:Prefs/Classic first, and OpenSpeaker on the menu bar (openamigaprefs).
    so = a.openamigaprefs / "Sound/build/os3"
    snd = Part(top, "Sound", "0.1", "Sound: the volume, Paula's and AHI's levels, and a speaker on the menu bar")
    snd.add("Prefs/Sound", (so / "Sound").read_bytes(), "SYS:Prefs/Sound")
    snd.add("C/OpenSpeaker", (so / "OpenSpeaker").read_bytes(), "SYS:C/OpenSpeaker")
    snd.add("Prefs/Sound.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/Sound.info")
    snd.line("startup SYS:Prefs/Sound USE")
    snd.write()

    # OpenEdit: the text editor (openamigaedit), with its kinds of file in ENVARC:
    oe = a.openamigaedit / "build/os3"
    ed = Part(top, "OpenEdit", "0.1", "OpenEdit: a text editor with tabs, find and replace, and colours for scripts and code")
    ed.add("C/OpenEdit", (oe / "OpenEdit").read_bytes(), "SYS:C/OpenEdit")
    for k in sorted((oe / "Kinds").glob("*.kind")):
        ed.add(f"Kinds/{k.name}", k.read_bytes(), f"ENVARC:OpenEdit/Kinds/{k.name}")
    # its Tools icon: a script IconX runs without a window, so it opens OpenEdit with no file
    ed.add("Tools/OpenEdit", b"Run >NIL: C:OpenEdit\n", "SYS:Tools/OpenEdit")
    ed.add("Tools/OpenEdit.info", icons.icon(icons.WBPROJECT, "page", "C:IconX", ["WINDOW=NIL:"], stack=8192), "SYS:Tools/OpenEdit.info")
    ed.write()

    # OpenMUI: zunemaster.library, MUI from Zune for OS 3.x (openamigamui), a
    # preview beside MUI, which it doesn't replace; ZuneTest shows it working.
    mui = a.openamigamui / "build/os3"
    om = Part(top, "OpenMUI", "0.1", "OpenMUI: zunemaster.library, MUI from Zune (preview), and ZuneTest")
    om.add("Libs/zunemaster.library", (mui / "zunemaster.library").read_bytes(), "LIBS:zunemaster.library")
    om.add("Tools/ZuneTest", (mui / "ZuneTest").read_bytes(), "SYS:Tools/ZuneTest")
    om.add("Tools/ZuneTest.info", icons.icon(icons.WBTOOL, "install", stack=16384), "SYS:Tools/ZuneTest.info")
    om.write()

    # The drawer's own icon, and OS 3.2-style icons for the scripts
    (a.out / "OpenUp.info").write_bytes(icons.icon(icons.WBDRAWER, "drawer", window=(440, 170)))
    try:
        os32_icons(a.openamigaprint, icons, [(top / "Install-OpenUp.info", "install"), (top / "Uninstall-OpenUp.info", "install"),
                                             (core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info", "install"),
                                             (ty.dir / "Prefs/OpenTypes.info", "page"),
                                             (look.dir / "Prefs/Look.info", "page"),
                                             (ow.dir / "Prefs/Windows.info", "page"),
                                             (dock.dir / "Prefs/Dock.info", "page"),
                                             (menus.dir / "Prefs/Menus.info", "page"),
                                             (tb.dir / "Prefs/TitleBar.info", "page"),
                                             (setup.dir / "Prefs/OpenUp-Setup.info", "page"),
                                             (oc.dir / "Utilities/OpenCompress.info", archive_art),
                                             (of.dir / "Utilities/OpenFiles.info", files_art),
                                             (op.dir / "Utilities/OpenPlay.info", play_art),
                                             (a.out / "OpenUp.info", None)])
        put("LICENCE.Boxie", (icons.BOXIE / "LICENCE").read_bytes())   # the drawer icon is Boxie's (MIT)
    except Exception as e:                  # node or amiga-icon.js missing: the classic icons stay
        print(f"  (OS 3.2-style icons not made: {e}; the classic ones stay)")
    try:
        open_icons(a.amigachrome, [
            (a.out / "OpenUp.info", "drawer-OpenUp"), (top / "Install-OpenUp.info", "Install-OpenUp"),
            (top / "Uninstall-OpenUp.info", "Install-OpenUp"), (core.dir / "Utilities/OpenUp.info", "drawer-OpenUp"),
            (core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info", "Install-OpenUp"),
            (pr.dir / "Icons/OpenPrint.info", "drawer-OpenPrint"), (pr.dir / "Drawer/OpenView.info", "OpenView"),
            (pr.dir / "Drawer/Printers and Queue.info", "Printers and Queue"),
            (ty.dir / "Prefs/OpenTypes.info", "OpenTypes"), (look.dir / "Prefs/Look.info", "Look"),
            (ow.dir / "Prefs/Windows.info", "Windows"), (dock.dir / "Prefs/Dock.info", "Dock"),
            (menus.dir / "Prefs/Menus.info", "Menus"), (tb.dir / "Prefs/TitleBar.info", "TitleBar"),
            (snd.dir / "Prefs/Sound.info", "Sound"), (setup.dir / "Prefs/OpenUp-Setup.info", "OpenUp-Setup"),
            (setup.dir / "WBStartup/OpenUp-Setup.info", "OpenUp-Setup"), (oc.dir / "Utilities/OpenCompress.info", "OpenCompress"),
            (of.dir / "Utilities/OpenFiles.info", "OpenFiles"), (op.dir / "Utilities/OpenPlay.info", "OpenPlay"),
            (om.dir / "Tools/ZuneTest.info", "ZuneTest"), (ed.dir / "Tools/OpenEdit.info", "OpenEdit")],
            drawers={a.out / "OpenUp.info": [top / "Install-OpenUp.info", top / "Uninstall-OpenUp.info"],
                     core.dir / "Utilities/OpenUp.info": [core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info"],
                     pr.dir / "Icons/OpenPrint.info": sorted((pr.dir / "Drawer").glob("*.info"))})
    except Exception as e:                  # an amigachrome checkout without amiga_icon (before #153): the icons above stay
        print(f"  (the Open apps' icons not made: {e}; the ones above stay)")
    print(f"{top}: {sum(1 for _ in top.rglob('*') if _.is_file())} files")
    if a.lha:
        arc = a.out / f"OpenUp-{a.version}.lha"
        arc.unlink(missing_ok=True)
        # jlha makes archives; on some systems "lha" is lhasa, which only reads them
        run([shutil.which("jlha") or "lha", "aq", arc.name, "OpenUp", "OpenUp.info"], cwd=a.out)
        print(f"{arc}: {arc.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
