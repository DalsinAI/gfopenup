#!/usr/bin/env python3
"""GFOpenApps: the pack that puts our applications on Work: (Dale, 17 Nov 2026: "on the work volume of all
appliances and instances we should include all our open apps. so putty , bitrorrent , etc etc"). It takes each
application's own release as it is (apps.lock: repository, release, file, drawer, mark), and makes a pack of GFOpenUp
parts beside GFOpenUp's own, so the GFOpenUp pack stays its size:

    python3 tools/apps_pack.py fetch [--lock apps.lock] [--assets DIR]     the releases' files, each with its sha256
    python3 tools/apps_pack.py build [--lock apps.lock] [--assets DIR] [--out DIR]

    GFOpenApps/Parts/App<Drawer>/Part       part App<Drawer> <version>, a "when WorkDrive copy" line for every file
    GFOpenApps/Parts/App<Drawer>/Files/     the release's files, as its archive holds them
    GFOpenApps/Parts/App<Drawer>/Drawer.info  the drawer's icon: the Soft 3D folder with the app's mark (ICONS.md)
    GFOpenApps/Install-GFOpenApps           Execute Work:GFOpenApps/Install-GFOpenApps (GFOpenUp must be installed)
    GFOpenApps/APPS-LIST.txt                each app: part, release, file, sha256, files, bytes, its own installer

Every line is "when WorkDrive" (GFOpenUpTool 0.8.23): an application goes in Work:<Drawer> where a volume named
Work is mounted, and nowhere else. An emulator's drawer is written Emulation:<Drawer> in the lock: it goes on the
Emulation drive ("when EmulationDrive"), and the part EmulationDrawers gives that drive its Roms, Disks, HardDrives
and Optical Media drawers, each with a ReadMe and nothing else. GFOpenUpTool records each part, so UPDATE and REMOVE work as for any part. An
application that carries an installer of its own (Install, Install-<name>) keeps it in its drawer, and APPS-LIST.txt
names it: a part has no line that runs a program, so that installer is still the user's to run.

Copyright (c) 2026 AmiCradle Team. MIT licence."""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "apps.lock"
OWNER = "DalsinAI"
API = "https://api.github.com"
PACK = "GFOpenApps"
INSTALL = "Install-GFOpenApps"
HOME = "Work"                 # the volume: the standard build's DH1:, which is for applications
HOMES = {"Work": "WorkDrive", "Emulation": "EmulationDrive"}       # a volume an app may live on, and its condition
# Emulation:'s own drawers (Dale, 17 Nov 2026: "make sure our own emulaton applications are in emulation, with two
# or three folders , Roms , Disks , HardDrives , Optical Media"), each with its mark. They ship with a ReadMe and
# nothing else: no ROM, disk or disc image is ours to give.
EMULATION_DRAWERS = {"Roms": "chip", "Disks": "mount", "HardDrives": "archive", "Optical Media": "film"}
EMULATION_PART = "EmulationDrawers"
DEST_MAX = 180                # GFOpenUpTool's path buffers are 200
NAME_MAX = 30                 # the longest name a standard FFS volume keeps


class AppsError(Exception):
    pass


def read_lock(path: Path) -> list[dict]:
    """apps.lock's lines: repo, tag, asset, drawer, mark and the title after "#". A line short of five words, a
    drawer named twice or a drawer no FFS volume can hold is an error."""
    apps, seen = [], set()
    for n, raw in enumerate(Path(path).read_text().splitlines(), 1):
        line, _, title = raw.partition("#")
        w = line.split()
        if not w:
            continue
        if len(w) != 5:
            raise AppsError(f"{path.name} line {n}: wants repository, release, file, drawer and mark")
        repo, tag, asset, drawer, mark = w
        home, sep, rest = drawer.partition(":")
        if sep:
            if home not in HOMES:
                raise AppsError(f"{path.name} line {n}: {home}: is no volume an application goes on")
            drawer = rest
        else:
            home = HOME
        if drawer.lower() in seen:
            raise AppsError(f"{path.name} line {n}: the drawer {drawer} is there twice")
        if len(drawer) + len(".info") > NAME_MAX or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._+-]*", drawer):
            raise AppsError(f"{path.name} line {n}: {drawer} is no drawer name")
        seen.add(drawer.lower())
        apps.append(dict(repo=repo.rsplit("/", 1)[-1], tag=tag, asset=asset, drawer=drawer, mark=mark, title=title.strip(), home=home))
    return apps


def part_name(app: dict) -> str:
    return "App" + re.sub(r"[^A-Za-z0-9]", "", app["drawer"])


def version(tag: str) -> str:
    """A part's version from its release's name: the first dotted number in it (v0.9.0-rc2: 0.9.0; a release
    candidate's number is not part of it, so a new candidate is taken by its files' own versions)."""
    m = re.search(r"\d+(?:\.\d+)+", tag)
    if not m:
        raise AppsError(f"{tag}: no version in the release's name")
    return m.group(0)


def unpack(archive: Path, to: Path) -> None:
    """The archive's files under to: .lha by the host's lha, .zip and .tar.gz by Python. A member that would
    land outside to is refused."""
    to.mkdir(parents=True, exist_ok=True)
    name = archive.name.lower()
    if name.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            for m in z.namelist():
                if m.startswith("/") or ".." in Path(m).parts:
                    raise AppsError(f"{archive.name}: {m} leaves the archive's drawer")
            z.extractall(to)
    elif name.endswith((".tar.gz", ".tgz")):
        with tarfile.open(archive) as t:
            t.extractall(to, filter="data")
    elif name.endswith(".lha"):
        lha = shutil.which("lha") or shutil.which("lhasa")
        if not lha:
            raise AppsError("no lha on this machine to unpack " + archive.name)
        subprocess.run([lha, "xqf", str(archive.resolve())], cwd=to, check=True)
        for f in to.rglob("*"):
            if not f.resolve().is_relative_to(to.resolve()):
                raise AppsError(f"{archive.name}: {f} leaves the archive's drawer")
    else:
        raise AppsError(f"{archive.name}: not an archive this tool unpacks")


def app_root(tree: Path) -> Path:
    """Where the application's own files start: inside the archive's one top drawer when that is all it holds
    (with or without that drawer's icon beside it), else the archive's top."""
    tops = [p for p in tree.iterdir() if not (p.is_file() and p.suffix.lower() == ".info")]
    if len(tops) == 1 and tops[0].is_dir():
        return tops[0]
    return tree


def own_installer(root: Path) -> str:
    """The application's own installer, when its release has one at its top: Install, Install-<name>, Install.sh..."""
    for f in sorted(root.iterdir()):
        if f.is_file() and f.suffix.lower() != ".info" and re.match(r"install", f.name, re.I):
            return f.name
    return ""


def app_part(top: Path, app: dict, tree: Path, icon) -> dict:
    """Parts/App<Drawer> from an unpacked release (tree): every file to Work:<Drawer>/<its path>, where a Work
    volume is mounted, and the drawer's icon (icon(mark): the .info file's bytes) as Work:<Drawer>.info. The row
    for APPS-LIST.txt. A path GFOpenUpTool or the volume can't hold stops the build."""
    name, root = part_name(app), app_root(tree)
    cond = "when " + HOMES[app.get("home", HOME)]
    home = f"{app.get('home', HOME)}:{app['drawer']}"
    pdir = top / "Parts" / name
    if pdir.exists():
        shutil.rmtree(pdir)
    (pdir / "Files").mkdir(parents=True)
    lines = [f"part {name} {version(app['tag'])}", f"; {app['drawer']}: {app['title'] or app['repo']} ({app['repo']} {app['tag']})"]
    files = sorted(p for p in root.rglob("*") if p.is_file())
    if not files:
        raise AppsError(f"{app['drawer']}: the release holds no files")
    size = 0
    for f in files:
        rel = f.relative_to(root).as_posix()
        dest = f"{home}/{rel}"
        if '"' in rel or len(dest) > DEST_MAX or any(len(c) > NAME_MAX for c in rel.split("/")):
            raise AppsError(f"{app['drawer']}: {rel} is a path the Amiga side can't hold")
        (pdir / "Files" / rel).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(f, pdir / "Files" / rel)
        size += f.stat().st_size
        lines.append(f'{cond} copy "Files/{rel}" "{dest}"')
    (pdir / "Drawer.info").write_bytes(icon(app["mark"]))
    lines.append(f'{cond} add "Drawer.info" "{home}.info"')
    (pdir / "Part").write_text("\n".join(lines) + "\n", encoding="latin-1")
    return dict(part=name, drawer=app["drawer"], repo=app["repo"], tag=app["tag"], asset=app["asset"],
                files=len(files), bytes=size, installer=own_installer(root))


def emulation_part(top: Path, icon) -> str:
    """Parts/EmulationDrawers: Emulation:'s Roms, Disks, HardDrives and Optical Media, each made by its ReadMe
    (only where there is none) and given the Soft 3D folder with its mark, where a volume named Emulation is
    mounted. The part's name."""
    pdir = top / "Parts" / EMULATION_PART
    if pdir.exists():
        shutil.rmtree(pdir)
    pdir.mkdir(parents=True)
    lines = [f"part {EMULATION_PART} 1.0", "; Emulation: its Roms, Disks, HardDrives and Optical Media drawers, empty but for a ReadMe"]
    for n, (name, mark) in enumerate(EMULATION_DRAWERS.items()):
        (pdir / f"ReadMe{n}").write_text(f"Emulation:{name}\n\nThis drawer is for your own files for the emulators on this drive.\n"
                                         "None are supplied: use only files you have the right to use.\n", encoding="latin-1")
        (pdir / f"Drawer{n}.info").write_bytes(icon(mark))
        lines += [f'when EmulationDrive default "ReadMe{n}" "Emulation:{name}/ReadMe"',
                  f'when EmulationDrive add "Drawer{n}.info" "Emulation:{name}.info"']
    (pdir / "Part").write_text("\n".join(lines) + "\n", encoding="latin-1")
    return EMULATION_PART


def install_script(names: list[str]) -> str:
    lines = ["; GFOpenApps: our applications, each installed on Work: by GFOpenUpTool (GFOpenUp must be installed first).",
             f";   Execute {HOME}:{PACK}/{INSTALL}", "FailAt 21"]
    for n in names:
        lines += [f'Echo "GFOpenApps: {n}"', f'C:GFOpenUpTool INSTALL "{HOME}:{PACK}/Parts/{n}"']
    return "\n".join(lines) + "\n"


def soft3d_icon(mark: str) -> bytes:
    """The Soft 3D folder with the mark on it (tools/openicons_part.py; needs the amicradle-guest checkout's icon
    writer, AMICRADLE_GUEST or ../amicradle-guest)."""
    sys.path.insert(0, str(ROOT / "tools"))
    import openicons_part as part
    import soft3d_motifs as marks
    if mark not in marks.MOTIFS:
        raise AppsError(f"{mark}: no such mark for a drawer")
    guest = Path(os.environ.get("AMICRADLE_GUEST") or ROOT.parent / "amicradle-guest")
    part.generator(guest)
    return part.aos_icon("S3D:drawer:" + mark, "drawer")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def build(lock: Path, assets: Path, out: Path, icon=soft3d_icon, check_private=None) -> list[dict]:
    """The pack in out/GFOpenApps from the fetched files in assets. Every app in the lock must be there, with the
    sha256 fetch wrote beside it; nothing held back from release may be in it (build_pack.check_private)."""
    apps = read_lock(lock)
    top = out / PACK
    if top.exists():
        shutil.rmtree(top)
    top.mkdir(parents=True)
    rows = []
    for app in apps:
        archive = assets / app["repo"] / app["asset"]
        mark = archive.with_name(archive.name + ".sha256")
        if not archive.is_file() or not mark.is_file():
            raise AppsError(f"{app['drawer']}: {app['asset']} isn't fetched (apps_pack.py fetch)")
        got = sha256(archive)
        if got != mark.read_text().split()[0]:
            raise AppsError(f"{app['drawer']}: {app['asset']} is not the file that was fetched")
        tree = out / "unpacked" / app["repo"]
        if tree.exists():
            shutil.rmtree(tree)
        unpack(archive, tree)
        row = app_part(top, app, tree, icon)
        row["sha256"] = got
        rows.append(row)
        shutil.rmtree(tree)
    (top / INSTALL).write_text(install_script([emulation_part(top, icon)] + [r["part"] for r in rows]), encoding="latin-1")
    listing = ["# part\tdrawer\trepository\trelease\tfile\tsha256\tfiles\tbytes\town installer"]
    listing += ["\t".join(str(r[k]) for k in ("part", "drawer", "repo", "tag", "asset", "sha256", "files", "bytes", "installer"))
                for r in rows]
    (top / "APPS-LIST.txt").write_text("\n".join(listing) + "\n")
    if check_private is None:
        sys.path.insert(0, str(ROOT / "tools"))
        import build_pack
        check_private = build_pack.check_private
    problems = check_private(top)
    if problems:
        raise AppsError("not for release:\n  " + "\n  ".join(problems))
    return rows


def _get(url: str, token: str | None, raw: bool = False) -> bytes:
    req = urllib.request.Request(url, headers={"Accept": "application/octet-stream" if raw else "application/vnd.github+json",
                                               "User-Agent": "gfopenup-apps-pack"})
    if token:
        req.add_header("Authorization", "Bearer " + token)
    with urllib.request.urlopen(req, timeout=600) as r:
        return r.read()


def fetch(lock: Path, assets: Path, get=_get) -> list[str]:
    """Each app's release file into assets/<repo>/, with <file>.sha256 beside it. A file already there with its
    sha256 is kept. Where the release publishes the file's digest, it must match. The token is GH_TOKEN, else
    PARTS_READ_TOKEN."""
    token = os.environ.get("GH_TOKEN") or os.environ.get("PARTS_READ_TOKEN")
    done = []
    for app in read_lock(lock):
        dest = assets / app["repo"] / app["asset"]
        mark = dest.with_name(dest.name + ".sha256")
        if dest.is_file() and mark.is_file() and sha256(dest) == mark.read_text().split()[0]:
            continue
        rel = json.loads(get(f"{API}/repos/{OWNER}/{app['repo']}/releases/tags/{app['tag']}", token))
        found = [a for a in rel.get("assets", []) if a["name"] == app["asset"]]
        if not found:
            raise AppsError(f"{app['repo']} {app['tag']}: no file {app['asset']}")
        data = get(found[0]["url"], token, raw=True)
        got = hashlib.sha256(data).hexdigest()
        want = (found[0].get("digest") or "").removeprefix("sha256:")
        if want and want != got:
            raise AppsError(f"{app['repo']} {app['asset']}: not the file its release names")
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        mark.write_text(f"{got}  {app['asset']}\n")
        done.append(app["asset"])
    return done


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("what", choices=("fetch", "build"))
    ap.add_argument("--lock", type=Path, default=LOCK)
    ap.add_argument("--assets", type=Path, default=ROOT / "build" / "apps-assets")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "apps")
    a = ap.parse_args(argv)
    try:
        if a.what == "fetch":
            for name in fetch(a.lock, a.assets):
                print("fetched", name)
        else:
            rows = build(a.lock, a.assets, a.out)
            for r in rows:
                print(f"{r['part']}: {r['files']} files, {r['bytes']} bytes" + (f", own installer {r['installer']}" if r["installer"] else ""))
            print(f"{a.out / PACK}: {len(rows)} applications, {sum(r['bytes'] for r in rows)} bytes")
    except AppsError as e:
        print(f"apps_pack: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
