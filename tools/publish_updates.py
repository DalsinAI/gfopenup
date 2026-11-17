#!/usr/bin/env python3
"""GFOpenUpdate's files onto the public release (docs/design/GFOPENUPDATE.md, "Where it is published"):

    python3 tools/publish_updates.py <out>/updates [--repo DalsinAI/gfopenup] [--tag updates] [--dry-run]

uploads every part archive and then GFOpenUpdate.index to the release `updates` (made, as a pre-release, when there
is none), replacing what is there. The index goes last, so an Amiga that reads it finds every archive it names.
Run after `tools/build_pack.py --lha` on the machine that cuts (it needs `gh`, signed in). Only what
update_index.py wrote is sent: its archives and the index, each named in the index; anything else stops it.

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import update_index as ui                                                       # noqa: E402

NOTES = "GFOpenUpdate's index and part archives (built files only), read by GFOpenUpdate on the Amiga."


def plan(updates: Path) -> list[Path]:
    """The files to upload, archives first and the index last; SystemExit when the drawer isn't update_index.py's."""
    index = updates / ui.INDEX_NAME
    if not index.is_file():
        raise SystemExit(f"{index}: no index (tools/build_pack.py --lha writes it)")
    lines = index.read_text(encoding="latin-1").splitlines()
    if not lines or lines[0] != ui.INDEX_HEAD or lines[-1] != ui.INDEX_END:
        raise SystemExit(f"{index}: not whole")
    named = []
    for line in lines[1:-1]:
        w = line.split()
        if not w or w[0].startswith(";"):
            continue
        if len(w) != 6 or w[0] != "part":
            raise SystemExit(f"{index}: a wrong line: {line!r}")
        f = updates / w[5]
        if not f.is_file() or f.stat().st_size != int(w[3]):
            raise SystemExit(f"{f}: missing, or not the size the index says")
        named.append(f)
    extra = sorted(p.name for p in updates.iterdir() if p.is_file() and p != index and p not in named)
    if extra:
        raise SystemExit(f"{updates}: files the index doesn't name: {', '.join(extra)}")
    return named + [index]


def commands(files: list[Path], repo: str, tag: str) -> list[list[str]]:
    view = ["gh", "release", "view", tag, "--repo", repo]
    create = ["gh", "release", "create", tag, "--repo", repo, "--prerelease", "--title", "GFOpenUpdate", "--notes", NOTES]
    upload = [["gh", "release", "upload", tag, str(f), "--repo", repo, "--clobber"] for f in files]
    return [view, create] + upload


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("updates", type=Path)
    ap.add_argument("--repo", default="DalsinAI/gfopenup")
    ap.add_argument("--tag", default="updates")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args(argv)
    files = plan(a.updates)
    view, create, *uploads = commands(files, a.repo, a.tag)
    if a.dry_run:
        for c in [create] + uploads:
            print(" ".join(c))
        return 0
    if subprocess.run(view, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
        subprocess.run(create, check=True)
    for c in uploads:
        subprocess.run(c, check=True)
    print(f"{len(files)} files on {a.repo} release {a.tag}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
