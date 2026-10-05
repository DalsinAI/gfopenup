#!/usr/bin/env python3
"""Builds OpenUp: a drawer to run the Installer from, and the same as an LhA
archive (DESIGN.md sections 3 and 4).

    python3 tools/build_pack.py [--version 0.1] [--out build/pack]
        [--amigachrome DIR] [--openamigaprint DIR] [--opengadtools DIR]
        [--guest DIR] [--no-build] [--lha]

Each part is built by its own repository's scripts, from checkouts given on
the command line (or found beside this one), then laid out as it goes, with
a Part file that C/OpenUpTool reads:

  OpenUp/               OpenUpTool and the uninstaller
  OpenSocket/           bsdsocket.library 4.2 and its tools (amigachrome's
                        vendored payload, vendor/opensocket-os32)
  OpenPrint/            OpenPrint and OpenView (openamigaprint's package)
  AmigaChromeSharing/   accontrol.device, ACClip and ACDrop (amigachrome-guest)
  OpenLook/             the look and its themes (opengadtools)
  Backdrops/            the themes' desktops as Workbench backdrops (opengadtools)
  OpenTypes/            which program opens each kind of file (openamigaprefs)
  OpenCompress/         C:OpenCompress, archives and disk images (openamigacompress)

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

    def add(self, rel: str, data: bytes, dest: str, default: bool = False):
        p = self.dir / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        self.lines.append(f'{"default" if default else "copy"} "{rel}" "{dest}"')

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
            rgba = Path(tmp) / f"{art}.rgba"
            if not rgba.exists():
                rgba.write_bytes(icon_art.draw(art))
            spec.append({"info": str(info), "rgba": str(rgba), "w": icon_art.W, "h": icon_art.H})
        (Path(tmp) / "jobs.json").write_text(json.dumps(spec))
        subprocess.run(["node", str(oap / "tools" / "os32_icons.js"), str(icons_mod.ACBUILD_ICON_JS), str(Path(tmp) / "jobs.json")],
                       check=True, stdout=subprocess.DEVNULL)


def build_parts(a):
    """Each part's own build, into its own build folder."""
    env = dict(os.environ, AMIGA_CC=str(CC), PATH=f"{CC.parent}:{os.environ['PATH']}")
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
    run(["sh", a.openamigacompress / "build-os3.sh"], env=env)
    p = a.openamigaprint
    for script in ("build-amiga.sh", "build-browser.sh", "build-viewer.sh"):
        run(["sh", p / script], cwd=p, env=env)
    run([sys.executable, p / "tools/make_package.py", "--out", p / "build/package"], cwd=p, env=env)


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
    ap.add_argument("--no-build", action="store_true", help="use what the parts' builds already made")
    ap.add_argument("--lha", action="store_true", help="also make OpenUp-<version>.lha with the host's lha")
    a = ap.parse_args()
    for name in ("amigachrome", "openamigaprint", "opengadtools", "guest", "openamigaprefs", "openamigacompress"):
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
    pr = Part(top, "OpenPrint", "0.3", "OpenPrint and OpenView: printing to PDF and IPP, and viewing")
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
    look = Part(top, "OpenLook", "0.1", "OpenLook: the OpenGadTools look for every GadTools program")
    look.add("C/OpenLook", (o / "build/os3/OpenLook").read_bytes(), "SYS:C/OpenLook")
    for t in sorted((o / "themes").glob("*.theme")):
        look.add(f"Themes/{t.name}", t.read_bytes(), f"SYS:Prefs/Presets/Themes/{t.name}")
    look.line("startup Run >NIL: C:OpenLook")
    look.write()

    # The background pack: each theme's desktop at four screen shapes (opengadtools)
    bd = Part(top, "Backdrops", "0.1", "The themes' desktops as Workbench backdrops")
    for png in sorted((o / "build/backdrops").glob("*.png")):
        bd.add(f"Backdrops/{png.name}", png.read_bytes(), f"SYS:Prefs/Presets/Backdrops/OpenUp/{png.name}")
    bd.write()

    # OpenTypes: which program opens each kind of file (openamigaprefs)
    ty = Part(top, "OpenTypes", "0.1", "OpenTypes: which program opens each kind of file")
    ty.add("Prefs/OpenTypes", (a.openamigaprefs / "OpenTypes/build/os3/OpenTypes").read_bytes(), "SYS:Prefs/OpenTypes")
    ty.add("Prefs/OpenTypes.info", icons.icon(icons.WBTOOL, "page", stack=16384), "SYS:Prefs/OpenTypes.info")
    ty.write()

    # OpenCompress: the command (the window comes in its phase 2)
    oc = Part(top, "OpenCompress", "0.1", "C:OpenCompress: list, test and extract ADF, ADZ, zip, tar and gzip")
    oc.add("C/OpenCompress", (a.openamigacompress / "build/os3/OpenCompress").read_bytes(), "SYS:C/OpenCompress")
    oc.write()

    # The drawer's own icon, and OS 3.2-style icons for the scripts
    (a.out / "OpenUp.info").write_bytes(icons.icon(icons.WBDRAWER, "drawer", window=(440, 170)))
    try:
        os32_icons(a.openamigaprint, icons, [(top / "Install-OpenUp.info", "install"), (top / "Uninstall-OpenUp.info", "install"),
                                             (core.dir / "Utilities/OpenUp/Uninstall-OpenUp.info", "install"),
                                             (ty.dir / "Prefs/OpenTypes.info", "page"),
                                             (a.out / "OpenUp.info", None)])
        put("LICENCE.Boxie", (icons.BOXIE / "LICENCE").read_bytes())   # the drawer icon is Boxie's (MIT)
    except Exception as e:                  # node or amiga-icon.js missing: the classic icons stay
        print(f"  (OS 3.2-style icons not made: {e}; the classic ones stay)")
    print(f"{top}: {sum(1 for _ in top.rglob('*') if _.is_file())} files")
    if a.lha:
        arc = a.out / f"OpenUp-{a.version}.lha"
        arc.unlink(missing_ok=True)
        run(["lha", "aq", arc.name, "OpenUp", "OpenUp.info"], cwd=a.out)
        print(f"{arc}: {arc.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
