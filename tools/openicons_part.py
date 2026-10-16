#!/usr/bin/env python3
"""The OpenIcons part: our own icon set, look B, as the system icons' new faces (ICONS.md section 8).

    python3 tools/openicons_part.py OUT_DIR --guest DIR

OpenIcons is the Team's own icon set (amigachrome-guest icons/openicons): every
picture drawn new from vector sources by its generator, under the MIT licence.
Look B is the look the OpenKick ROM's own default icons have (amigachrome-guest
#336), so the system's drawers, programs and file types match the ROM's disks,
drawers and tools.

The .info files are the generator's build output and are committed nowhere, so
the pack makes them here, at build time, from the amigachrome-guest checkout the
pack is built with (build_pack.py --guest): the same commit's look, manifest and
writer, nothing fetched from outside our repositories, and a pack that names the
set's source by its commit.

For each icon the system already has (a SYS: drawer, a program of OS 3.2.3's,
a prefs editor, a monitor, printer, keymap or DOS driver, a DefIcons file type's
icon in ENVARC:Sys), it writes OpenIcons' icon into OUT_DIR/Icons/<name>.info and
a Part file whose "face" lines give each system icon that picture: OpenUpTool
keeps the icon's type, default tool, tool types, stack and place, keeps the
original in SYS:Storage/OpenUp-Superseded, and UNINSTALL puts it back. An icon
the machine doesn't have is left alone.

Not faced: the default icons (ENVARC:Sys/def_RAM, def_harddisk, def_disk,
def_drawer, def_tool, def_project, def_trashcan, def_cd0, def_rad, def_card,
def_kickstart, def_sys, def_work) and the volumes' Disk.info. The ROM's own
icons are OpenIcons look B already, and OpenUpTool OLDLOOK takes those files out
(the user, 16 October 2026: "we should remove them and use ours").

OUT_DIR/Kinds holds OpenIcons' disk, drawer, tool, project and trashcan, with no
face line: OpenUpTool ICONS gives an old icon OpenIcons has no picture of by name
the one of its kind.

MIT, Copyright (c) 2026 Dalsin Limited.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

LOOK = "B"

# The default icons: the ROM's own show (OpenUpTool OLDLOOK takes these files out); never faced
DEFAULTS = {f"ENVARC:Sys/def_{k}".lower() for k in (
    "RAM", "harddisk", "disk", "drawer", "tool", "project", "trashcan", "cd0", "rad", "card", "kickstart", "sys", "work")}

# Where OS 3.2.3 keeps what the manifest names by another place (amigachrome's os32_host_install lays the
# disk out as the Installer does: Commodities in SYS:Tools, the Storage drawers' twins in SYS:Devs).
MOVED = {
    "SYS:Commodities": ["SYS:Tools/Commodities"],
    "SYS:Monitors": ["SYS:Devs/Monitors", "SYS:Storage/Monitors"],
    "SYS:Printers": ["SYS:Devs/Printers", "SYS:Storage/Printers"],
    "SYS:DOSDrivers": ["SYS:Devs/DOSDrivers", "SYS:Storage/DOSDrivers"],
    "SYS:Keymaps": ["SYS:Devs/Keymaps", "SYS:Storage/Keymaps"],
    "SYS:DataTypes": ["SYS:Devs/DataTypes", "SYS:Storage/DataTypes"],
    "SYS:Presets": ["SYS:Prefs/Presets"],
    "SYS:System/Installer": ["SYS:System/Installer", "SYS:Utilities/Installer"],
    "SYS:System/Help": ["SYS:System/Help", "SYS:Utilities/Help"],
}
# a drawer's things that live in two drawers of OS 3.2.3 (SYS:Devs/Monitors/PAL and SYS:Storage/Monitors/PAL...)
TWINS = ("Monitors", "Printers", "DOSDrivers", "Keymaps", "DataTypes")

# DefIcons' file types the manifest has no icon of its own for, and the one whose picture they take
TYPE_ALIASES = {
    "def_text": "def_ascii", "def_document": "def_officedocs", "def_html": "def_ascii", "def_ps": "def_pdf",
    "def_ilbm": "def_picture", "def_jpeg": "def_picture", "def_gif": "def_picture", "def_png": "def_picture",
    "def_8svx": "def_sound", "def_aiff": "def_sound", "def_wave": "def_sound", "def_mp3": "def_music",
    "def_mod": "def_music", "def_movie": "def_video", "def_lha": "def_archive", "def_lzx": "def_archive",
    "def_zip": "def_archive", "def_keymap": "def_key", "def_pref": "def_prefs",
}

# OpenUpTool ICONS: a picture by kind (the part's Kinds drawer), from these manifest entries
KINDS = {"Disk": "def_disk", "Drawer": "def_drawer", "Tool": "def_tool", "Project": "def_project", "Trashcan": "def_trashcan"}

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
    out, used = {}, set()
    for p in paths:
        n, k = names[p], 1
        while n.lower() in used:
            tail = f"_{k}"
            n, k = names[p][:room - len(tail)] + tail, k + 1
        used.add(n.lower())
        out[p] = n + suffix
    return out


def generator(guest: Path):
    """amigachrome-guest's OpenIcons generator: (load_manifest, spec_of, load_looks, render_icon, write_entry)."""
    root = guest / "icons" / "openicons"
    if not (root / "oi.py").is_file():
        raise SystemExit(f"{guest}: no icons/openicons/oi.py (OpenIcons is in amigachrome-guest from #329)")
    sys.path.insert(0, str(root))
    import oi
    from openicons.build import load_looks, render_icon, write_entry
    return oi.load_manifest, oi.spec_of, load_looks, render_icon, write_entry


def faces(rows: list[dict]) -> list[tuple[str, dict]]:
    """(the system icon without .info, its manifest entry) for every face: OS 3.2.3's icons and file types."""
    by_name = {e["name"]: e for e in rows if e["group"] == "os32"}
    out: list[tuple[str, dict]] = []
    seen: set[str] = set()

    def add(dest: str, e: dict) -> None:
        if dest.lower() in seen or dest.lower() in DEFAULTS:
            return
        seen.add(dest.lower())
        out.append((dest, e))

    for e in rows:
        if e["group"] != "os32" or e["wbtype"] == "disk" and not e["path"].startswith("ENVARC:"):
            continue                                            # the volumes: the ROM's disk icons show
        path = e["path"]
        dests = MOVED.get(path)
        if dests is None:
            head, _, rest = path.rpartition("/")
            parent = MOVED.get(head)
            dests = [f"{p}/{rest}" for p in parent] if parent and head.split(":")[-1] in TWINS else [path]
            if path.startswith(("SYS:Devs/", "SYS:Storage/")):
                for twin in TWINS:
                    for a, b in (("SYS:Devs/", "SYS:Storage/"), ("SYS:Storage/", "SYS:Devs/")):
                        if path.startswith(a + twin + "/"):
                            dests = [path, b + path[len(a):]]
        for d in dests:
            add(d, e)
    for alias, name in TYPE_ALIASES.items():
        if name in by_name:
            add(f"ENVARC:Sys/{alias}", by_name[name])
    return out


def build(out: Path, guest: Path) -> list[str]:
    """Writes OUT/Icons/*.info and OUT/Kinds/*.info; returns the Part's lines."""
    load_manifest, spec_of, load_looks, render_icon, write_entry = generator(guest)
    look = load_looks()[LOOK]
    rows = load_manifest()
    try:
        commit = subprocess.run(["git", "-C", str(guest), "rev-parse", "--short=12", "HEAD"], check=True,
                                capture_output=True, text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        commit = "unknown"
    (out / "Icons").mkdir(parents=True, exist_ok=True)
    (out / "Kinds").mkdir(parents=True, exist_ok=True)
    rendered: dict[str, dict] = {}

    def picture(e: dict) -> dict:
        key = e["name"] + "\0" + e["path"]
        if key not in rendered:
            rendered[key] = render_icon(spec_of(e), look)
        return rendered[key]

    lines = [f"; OpenIcons look {LOOK}, our own icons (amigachrome-guest {commit}, MIT): new faces for the system's icons"]
    todo = faces(rows)
    names = short_names([dest for dest, _ in todo], ".info")
    for dest, e in todo:
        write_entry(e, picture(e), str(out / "Icons" / names[dest]))
        lines.append(f'face "Icons/{names[dest]}" "{dest}.info"')
    by_name = {e["name"]: e for e in rows if e["group"] == "os32"}
    for kind, name in KINDS.items():
        if name in by_name:
            write_entry(by_name[name], picture(by_name[name]), str(out / "Kinds" / f"{kind}.info"))
    return lines


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    ap.add_argument("--guest", type=Path, required=True, help="the amigachrome-guest checkout (icons/openicons)")
    a = ap.parse_args(argv)
    lines = build(a.out, a.guest)
    (a.out / "Part").write_text("part OpenIcons 0.1\n" + "\n".join(lines) + "\n")
    print(f"{len(lines) - 1} faces in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
