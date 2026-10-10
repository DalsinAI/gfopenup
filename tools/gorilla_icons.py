#!/usr/bin/env python3
"""The Gorilla icons: AROS's icon set as the system icons' new faces (ICONS.md section 8).

    python3 tools/gorilla_icons.py OUT_DIR --amigachrome DIR [--cache DIR]

Gorilla is AROS's icon set (images/IconSets/Gorilla in the AROS repository),
under the GNU General Public License (its LEGAL file). Chosen on 8 October
2026 ("ship gorilla"): complete for a Workbench, soft-shaded, at 60 pixels.
As the team's rule for third-party material says, it is never committed
here: it is fetched at build time from AROS's repository at a pinned commit,
and every file used is checked against its SHA-256 below.

For each icon the system already has (a SYS: drawer, a program of OS 3.2.3's,
a default icon in ENVARC:Sys), it writes a GlowIcon holding only the picture,
at 85 per cent of Gorilla's size (the look approved on the canvas), into
OUT_DIR/Icons/<name>.info, and a Part file whose "face" lines give each
system icon its new picture: OpenUpTool keeps the icon's type, default tool,
tool types, stack and drawer window, keeps the original in
SYS:Storage/OpenUp-Superseded, and UNINSTALL puts it back. An icon the
machine doesn't have is left alone.

GlowIcons have no partial transparency, so Gorilla's soft shadows end as a
solid edge or nothing (a pixel is solid at alpha 110 or more, as for our own
icons); the PNGs go in too (SYS:Storage/Icons/Gorilla), for PNG icons once
OpenUp draws them, and as the GPL's source, with Gorilla's LEGAL and the GPL.

MIT for this script, Copyright (c) 2026 Dalsin Limited. The icons are AROS's,
GPL.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import shutil
import subprocess
import sys
from pathlib import Path

AROS_URL = "https://github.com/aros-development-team/AROS"
AROS_COMMIT = "aa541964354368da8d26c04312c5570b483f5447"
SET = "images/IconSets/Gorilla"
MEDIUM = SET + "/Icons/Medium"
SCALE = 0.85                       # the approved look: 15 per cent smaller than Gorilla's own

# (the system icon, without .info; the Gorilla picture under Icons/Medium)
FACES = [
    # the boot volume and the second partition
    ("SYS:Disk", "AROS/Prefs/Env-Archive/SYS/def_Harddisk.png"),
    ("Work:Disk", "AROS/Prefs/Env-Archive/SYS/def_Harddisk.png"),
    # SYS:'s drawers
    ("SYS:Prefs", "AROS/Prefs.png"),
    ("SYS:Utilities", "AROS/Utilities.png"),
    ("SYS:Tools", "AROS/Tools.png"),
    ("SYS:System", "AROS/System.png"),
    ("SYS:Devs", "AROS/Devs.png"),
    ("SYS:Storage", "AROS/Storage.png"),
    ("SYS:WBStartup", "AROS/WBStartup.png"),
    ("SYS:Expansion", "AROS/Prefs/Env-Archive/SYS/def_Drawer.png"),
    ("SYS:Developer", "AROS/Developer.png"),                    # the user, 10 Oct 2026: Developer in our look
    ("SYS:Developer/OpenGPU-SDK", "AROS/Developer/Include.png"),
    ("SYS:Fonts", "AROS/Fonts.png"),
    ("SYS:Locale", "AROS/Locale.png"),
    ("SYS:Libs", "AROS/Libs.png"),
    ("SYS:Rexxc", "AROS/Rexxc.png"),
    ("SYS:Tools/Commodities", "AROS/Tools/Commodities.png"),
    ("SYS:Prefs/Presets", "AROS/Prefs/Presets.png"),
    ("SYS:Devs/DOSDrivers", "AROS/Devs/DOSDrivers.png"),
    ("SYS:Devs/DataTypes", "AROS/Devs/DataTypes.png"),
    ("SYS:Devs/Keymaps", "AROS/Devs/Keymaps.png"),
    ("SYS:Devs/Monitors", "AROS/Devs/Monitors.png"),
    ("SYS:Devs/Printers", "AROS/Devs/Printers.png"),
    ("SYS:Storage/DOSDrivers", "AROS/Devs/DOSDrivers.png"),
    ("SYS:Storage/DataTypes", "AROS/Devs/DataTypes.png"),
    ("SYS:Storage/Keymaps", "AROS/Devs/Keymaps.png"),
    ("SYS:Storage/Monitors", "AROS/Devs/Monitors.png"),
    ("SYS:Storage/Printers", "AROS/Devs/Printers.png"),
    # OS 3.2.3's programs
    ("SYS:System/Shell", "AROS/System/Shell.png"),
    ("SYS:System/Format", "AROS/System/Format.png"),
    ("SYS:System/FixFonts", "AROS/System/FixFonts.png"),
    ("SYS:Utilities/Clock", "AROS/Utilities/Clock.png"),
    ("SYS:Utilities/MultiView", "AROS/Utilities/MultiView.png"),
    ("SYS:Utilities/Installer", "AROS/Utilities/Installer.png"),
    ("SYS:Tools/Calculator", "AROS/Tools/Calculator.png"),
    ("SYS:Tools/HDToolBox", "AROS/Tools/HDToolBox.png"),
    ("SYS:Tools/GraphicDump", "AROS/Tools/GraphicDump.png"),
    ("SYS:Tools/InitPrinter", "AROS/Tools/InitPrinter.png"),
    ("SYS:Tools/KeyShow", "AROS/Tools/KeyShow.png"),
    ("SYS:Tools/PrintFiles", "AROS/Tools/PrintFiles.png"),
    ("SYS:Tools/ShowConfig", "AROS/Tools/ShowConfig.png"),
    ("SYS:Tools/TextEdit", "AROS/Tools/Editor.png"),
    ("SYS:Tools/Commodities/AutoPoint", "AROS/Tools/Commodities/AutoPoint.png"),
    ("SYS:Tools/Commodities/Blanker", "AROS/Tools/Commodities/Blanker.png"),
    ("SYS:Tools/Commodities/ClickToFront", "AROS/Tools/Commodities/ClickToFront.png"),
    ("SYS:Tools/Commodities/Exchange", "AROS/Tools/Commodities/Exchange.png"),
    ("SYS:Tools/Commodities/FKey", "AROS/Tools/Commodities/FKey.png"),
    ("SYS:Tools/Commodities/NoCapsLock", "AROS/Tools/Commodities/NoCapsLock.png"),
    # OS 3.2.3's prefs editors (Gorilla's carry its prefs mark)
    ("SYS:Prefs/Asl", "AROS/Prefs/Asl.png"),
    ("SYS:Prefs/Font", "AROS/Prefs/Font.png"),
    ("SYS:Prefs/IControl", "AROS/Prefs/IControl.png"),
    ("SYS:Prefs/Input", "AROS/Prefs/Input.png"),
    ("SYS:Prefs/Locale", "AROS/Prefs/Locale.png"),
    ("SYS:Prefs/Palette", "AROS/Prefs/Palette.png"),
    ("SYS:Prefs/Pointer", "AROS/Prefs/Pointer.png"),
    ("SYS:Prefs/Printer", "AROS/Prefs/Printer.png"),
    ("SYS:Prefs/PrinterGfx", "AROS/Prefs/Printer.png"),
    ("SYS:Prefs/PrinterPS", "AROS/Prefs/Printer.png"),
    ("SYS:Prefs/ScreenMode", "AROS/Prefs/ScreenMode.png"),
    ("SYS:Prefs/Serial", "AROS/Prefs/Serial.png"),
    ("SYS:Prefs/Time", "AROS/Prefs/Time.png"),
    ("SYS:Prefs/Workbench", "AROS/Prefs/Wanderer.png"),
    ("SYS:Prefs/ReqTools", "AROS/Prefs/ReqTools.png"),
    ("SYS:Prefs/AHI", "AROS/Prefs/AHI.png"),
    # the default icons (ENVARC:Sys; Workbench's names, in any case)
    ("ENVARC:Sys/def_Disk", "AROS/Prefs/Env-Archive/SYS/def_Disk.png"),
    ("ENVARC:Sys/def_Drawer", "AROS/Prefs/Env-Archive/SYS/def_Drawer.png"),
    ("ENVARC:Sys/def_Tool", "AROS/Prefs/Env-Archive/SYS/def_Tool.png"),
    ("ENVARC:Sys/def_Project", "AROS/Prefs/Env-Archive/SYS/def_Project.png"),
    ("ENVARC:Sys/def_Trashcan", "AROS/Prefs/Env-Archive/SYS/def_Trashcan.png"),
    ("ENVARC:Sys/def_Kick", "AROS/Prefs/Env-Archive/SYS/def_KickDisk.png"),
    ("ENVARC:Sys/def_RAM", "AROS/Prefs/Env-Archive/SYS/def_RAM.png"),
    ("ENVARC:Sys/def_Harddisk", "AROS/Prefs/Env-Archive/SYS/def_Harddisk.png"),
    ("ENVARC:Sys/def_CDROM", "AROS/Prefs/Env-Archive/SYS/def_CDROM.png"),
    ("ENVARC:Sys/def_ascii", "AROS/Prefs/Env-Archive/SYS/def_Text.png"),
    ("ENVARC:Sys/def_text", "AROS/Prefs/Env-Archive/SYS/def_Text.png"),
    ("ENVARC:Sys/def_document", "AROS/Prefs/Env-Archive/SYS/def_Document.png"),
    ("ENVARC:Sys/def_amigaguide", "AROS/Utilities/Help.png"),
    ("ENVARC:Sys/def_install", "AROS/Prefs/Env-Archive/SYS/def_Installer.png"),
    ("ENVARC:Sys/def_rexx", "AROS/Prefs/Env-Archive/SYS/def_REXX.png"),
    ("ENVARC:Sys/def_html", "AROS/Prefs/Env-Archive/SYS/def_HTML.png"),
    ("ENVARC:Sys/def_pdf", "AROS/Prefs/Env-Archive/SYS/def_PDF.png"),
    ("ENVARC:Sys/def_ps", "AROS/Prefs/Env-Archive/SYS/def_Postscript.png"),
    ("ENVARC:Sys/def_iff", "AROS/Prefs/Env-Archive/SYS/def_Picture.png"),
    ("ENVARC:Sys/def_ilbm", "AROS/Prefs/Env-Archive/SYS/def_Picture.png"),
    ("ENVARC:Sys/def_picture", "AROS/Prefs/Env-Archive/SYS/def_Picture.png"),
    ("ENVARC:Sys/def_jpeg", "AROS/Prefs/Env-Archive/SYS/def_Picture.png"),
    ("ENVARC:Sys/def_gif", "AROS/Prefs/Env-Archive/SYS/def_Picture.png"),
    ("ENVARC:Sys/def_png", "AROS/Prefs/Env-Archive/SYS/def_Picture.png"),
    ("ENVARC:Sys/def_8svx", "AROS/Prefs/Env-Archive/SYS/def_Sound.png"),
    ("ENVARC:Sys/def_aiff", "AROS/Prefs/Env-Archive/SYS/def_Sound.png"),
    ("ENVARC:Sys/def_wave", "AROS/Prefs/Env-Archive/SYS/def_Sound.png"),
    ("ENVARC:Sys/def_sound", "AROS/Prefs/Env-Archive/SYS/def_Sound.png"),
    ("ENVARC:Sys/def_mp3", "AROS/Prefs/Env-Archive/SYS/def_MP3.png"),
    ("ENVARC:Sys/def_mod", "AROS/Prefs/Env-Archive/SYS/def_Music.png"),
    ("ENVARC:Sys/def_music", "AROS/Prefs/Env-Archive/SYS/def_Music.png"),
    ("ENVARC:Sys/def_anim", "AROS/Prefs/Env-Archive/SYS/def_ANIM.png"),
    ("ENVARC:Sys/def_movie", "AROS/Prefs/Env-Archive/SYS/def_Movie.png"),
    ("ENVARC:Sys/def_lha", "AROS/Prefs/Env-Archive/SYS/def_LHA.png"),
    ("ENVARC:Sys/def_lzx", "AROS/Prefs/Env-Archive/SYS/def_Archive.png"),
    ("ENVARC:Sys/def_zip", "AROS/Prefs/Env-Archive/SYS/def_Archive.png"),
    ("ENVARC:Sys/def_archive", "AROS/Prefs/Env-Archive/SYS/def_Archive.png"),
    ("ENVARC:Sys/def_font", "AROS/Prefs/Env-Archive/SYS/def_Bitmap_Font.png"),
    ("ENVARC:Sys/def_catalog", "AROS/Prefs/Env-Archive/SYS/def_Catalog.png"),
    ("ENVARC:Sys/def_keymap", "AROS/Prefs/Env-Archive/SYS/def_Keymap.png"),
    ("ENVARC:Sys/def_pref", "AROS/Prefs/Env-Archive/SYS/def_Pref.png"),
    ("ENVARC:Sys/def_datatype", "AROS/Prefs/Env-Archive/SYS/def_DataType.png"),
]

# SHA-256 of every file used, at AROS_COMMIT (tools/gorilla_icons.py --lock writes this list)
LOCK: dict[str, str] = {}
LOCK_FILE = Path(__file__).resolve().parent / "gorilla.lock"


def run(cmd, cwd=None):
    subprocess.run(cmd, cwd=cwd, check=True, stdout=subprocess.DEVNULL)


def fetch(cache: Path) -> Path:
    """AROS's Gorilla set at AROS_COMMIT, with its LEGAL and the GPL, in a sparse checkout."""
    root = cache / f"aros-{AROS_COMMIT[:12]}"
    if not (root / SET / "LEGAL").is_file():
        if root.exists():
            shutil.rmtree(root)
        root.mkdir(parents=True)
        run(["git", "init", "-q"], cwd=root)
        run(["git", "remote", "add", "origin", AROS_URL], cwd=root)
        run(["git", "sparse-checkout", "set", "--no-cone", f"/{SET}/", "/LICENSE.GPL"], cwd=root)
        run(["git", "fetch", "-q", "--depth", "1", "--filter=blob:none", "origin", AROS_COMMIT], cwd=root)
        run(["git", "checkout", "-q", "FETCH_HEAD"], cwd=root)
    return root


DRAWER = "AROS/Prefs/Env-Archive/SYS/def_Drawer.png"     # our drawers' badges go on it (build_pack, open_icons)


def drawer_png(cache: Path) -> bytes:
    """Gorilla's drawer, fetched and checked against the lock."""
    root = fetch(cache)
    check(root)
    return (root / MEDIUM / DRAWER).read_bytes()


def used_files() -> list[str]:
    return sorted({f"{MEDIUM}/{p}" for _, p in FACES} | {f"{SET}/LEGAL", "LICENSE.GPL"})


def read_lock() -> dict[str, str]:
    lock = {}
    for line in LOCK_FILE.read_text().splitlines():
        if line.strip() and not line.startswith("#"):
            digest, path = line.split(None, 1)
            lock[path] = digest
    return lock


def check(root: Path) -> None:
    lock = read_lock()
    for rel in used_files():
        data = (root / rel).read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if lock.get(rel) != digest:
            raise SystemExit(f"gorilla: {rel} doesn't match tools/gorilla.lock (got {digest}): not the pinned Gorilla")


def write_lock(root: Path) -> None:
    lines = [f"# Gorilla at AROS {AROS_COMMIT}: SHA-256 of every file OpenUp uses (tools/gorilla_icons.py --lock)"]
    for rel in used_files():
        lines.append(f"{hashlib.sha256((root / rel).read_bytes()).hexdigest()}  {rel}")
    LOCK_FILE.write_text("\n".join(lines) + "\n")


def picture(png: bytes, amiga_icon) -> tuple[int, int, bytes, bytes]:
    """85 per cent of Gorilla's size, every pixel solid or see-through (a
    GlowIcon), dithered for a 16-bit screen: the rest frame and the chosen
    one with the OS 3.5 glow."""
    from PIL import Image
    im = Image.open(io.BytesIO(png)).convert("RGBA")
    im = im.resize((max(1, round(im.width * SCALE)), max(1, round(im.height * SCALE))), Image.LANCZOS)
    rgba = bytearray(im.tobytes())
    for i in range(0, len(rgba), 4):
        rgba[i:i + 4] = rgba[i:i + 3] + b"\xff" if rgba[i + 3] >= 110 else b"\0\0\0\0"
    w, h, rgba = im.width, im.height, bytes(rgba)
    return w, h, amiga_icon.dither_rgb565(w, h, rgba), amiga_icon.dither_rgb565(w, h, amiga_icon.glow_selected(w, h, rgba))


FFS_NAME = 30          # the longest name a standard FFS volume keeps; longer ones are cut short


def short_names(paths: list[str], suffix: str) -> dict[str, str]:
    """A short, unique file name for each path, for an FFS volume: the last
    part of the path, then the parts before it while two names are the same,
    then a number. With the suffix, no name is longer than FFS_NAME; names
    that differ only in case count as the same (AmigaDOS doesn't tell them apart).
    SYS:Tools/Commodities/ClickToFront -> ClickToFront."""
    def parts(p: str) -> list[str]:
        return [x for x in p.replace(":", "/").split("/") if x]
    room = FFS_NAME - len(suffix)
    depth = {p: 1 for p in paths}
    while True:
        names = {p: "_".join(parts(p)[-depth[p]:])[-room:] for p in paths}
        seen: dict[str, list[str]] = {}
        for p, n in names.items():
            seen.setdefault(n.lower(), []).append(p)
        clash = [ps for ps in seen.values() if len(ps) > 1]
        if not clash:
            return {p: n + suffix for p, n in names.items()}
        grew = False
        for ps in clash:
            for p in ps:
                if depth[p] < len(parts(p)) and len("_".join(parts(p)[-depth[p] - 1:])) <= room:
                    depth[p] += 1
                    grew = True
        if not grew:
            break
    # still the same after every part that fits: a number makes them differ
    out, used = {}, set()
    for p in paths:
        n, k = names[p], 1
        while n.lower() in used:
            tail = f"_{k}"
            n, k = names[p][:room - len(tail)] + tail, k + 1
        used.add(n.lower())
        out[p] = n + suffix
    return out


def build(out: Path, amigachrome: Path, cache: Path) -> list[str]:
    """Writes OUT/Icons/*.info, OUT/Sources/*.png, LEGAL and COPYING; returns the Part's lines."""
    sys.path.insert(0, str(amigachrome / "scripts"))
    import amiga_icon
    root = fetch(cache)
    check(root)
    (out / "Icons").mkdir(parents=True, exist_ok=True)
    (out / "Sources").mkdir(parents=True, exist_ok=True)
    lines = [f"; Gorilla, AROS's icon set (GPL), at AROS {AROS_COMMIT[:12]}: new faces for the system's icons"]
    done_src = set()
    # names an FFS volume keeps whole, unpacked and copied (30 characters)
    icon_names = short_names([dest for dest, _ in FACES], ".info")
    src_names = short_names(sorted({rel[:-4] for _, rel in FACES}), ".png")
    for dest, rel in FACES:
        png = (root / MEDIUM / rel).read_bytes()
        w, h, rest, chosen = picture(png, amiga_icon)
        meta = {"type": 4, "defaultTool": None, "toolTypes": [], "toolWindow": None, "stack": 4096,
                "x": amiga_icon.NO_POSITION, "y": amiga_icon.NO_POSITION, "drawer": None}
        name = icon_names[dest]
        (out / "Icons" / name).write_bytes(amiga_icon.write_os35(meta, [(w, h, rest), (w, h, chosen)]))
        lines.append(f'face "Icons/{name}" "{dest}.info"')
        src = src_names[rel[:-4]]
        if src not in done_src:
            (out / "Sources" / src).write_bytes(png)
            lines.append(f'copy "Sources/{src}" "SYS:Storage/Icons/Gorilla/{src}"')
            done_src.add(src)
    shutil.copyfile(root / SET / "LEGAL", out / "LEGAL")
    shutil.copyfile(root / "LICENSE.GPL", out / "COPYING")
    lines.append('copy "LEGAL" "SYS:Storage/Icons/Gorilla/LEGAL"')
    lines.append('copy "COPYING" "SYS:Storage/Icons/Gorilla/COPYING"')
    return lines


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    ap.add_argument("--amigachrome", type=Path, required=True)
    ap.add_argument("--cache", type=Path, default=Path(__file__).resolve().parents[1] / "build" / "cache")
    ap.add_argument("--lock", action="store_true", help="write tools/gorilla.lock from the fetched files")
    a = ap.parse_args(argv)
    if a.lock:
        write_lock(fetch(a.cache))
        print(f"wrote {LOCK_FILE}")
        return 0
    lines = build(a.out, a.amigachrome, a.cache)
    (a.out / "Part").write_text("part GorillaIcons 0.1\n" + "\n".join(lines) + "\n")
    print(f"{len(FACES)} faces in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
