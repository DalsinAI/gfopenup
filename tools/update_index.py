#!/usr/bin/env python3
"""GFOpenUpdate's side of a cut (docs/design/GFOPENUPDATE.md, step 2): each part of the pack as an archive of its
own, and the index that lists them, for the release GFOpenUpdate reads on the Amiga.

    python3 tools/update_index.py <pack>/Parts <out drawer>

    <out>/<Name>-<version>.lha     the part's drawer as it is in the pack (Parts/<Name>), "<Name>/..." inside; a
                                   part inside another's drawer (Parts/GFOpenLibs/rexx) has its own archive and
                                   is left out of the outer one
    <out>/GFOpenUpdate.index       "part <Name> <version> <bytes> <sha256> <file>" a line each, between
                                   "GFOpenUpdate 1" and "GFOpenUpdate end" (src/openupdate.h reads it)

tools/build_pack.py --lha runs this after its checks (no private material, no two parts on one path), so only
what the pack itself releases is here: built files, never source. Dale, 17 November 2026: "yes build parts from
public releases".

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import hashlib
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

INDEX_NAME = "GFOpenUpdate.index"
INDEX_HEAD = "GFOpenUpdate 1"
INDEX_END = "GFOpenUpdate end"
# what src/openupdate.h takes: a plain name of under 40 letters, a version of under 24 that starts with a digit
_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9._+-]{0,38}")
_VERSION = re.compile(r"[0-9][A-Za-z0-9._+-]{0,22}")


def part_of(part_file: Path) -> tuple[str, str] | None:
    """The name and version on a Part file's first line ("part <Name> <version>"), else None."""
    try:
        first = part_file.read_text(encoding="latin-1").splitlines()[0].split()
    except (OSError, IndexError):
        return None
    if len(first) < 3 or first[0] != "part":
        return None
    return first[1], first[2]


def pack_parts(parts: Path) -> list[tuple[str, str, Path]]:
    """Every part under the pack's Parts drawer, as (name, version, its drawer), by name. A name or a version the
    Amiga side would refuse, or a name used twice, stops the cut."""
    found: dict[str, tuple[str, str, Path]] = {}
    for pf in sorted(list(parts.glob("*/Part")) + list(parts.glob("*/*/Part"))):
        nv = part_of(pf)
        if not nv:
            continue
        name, version = nv
        if not _NAME.fullmatch(name):
            raise SystemExit(f"{pf}: the part's name {name!r} can't go in GFOpenUpdate's index")
        if not _VERSION.fullmatch(version):
            raise SystemExit(f"{pf}: the part's version {version!r} can't go in GFOpenUpdate's index")
        if name in found:
            raise SystemExit(f"{pf}: part {name} is also {found[name][2] / 'Part'}")
        found[name] = (name, version, pf.parent)
    return [found[k] for k in sorted(found)]


def lha_archive(archive: Path, folder: Path) -> None:
    """folder (a drawer in a scratch place) into archive, its own name first in each path."""
    # jlha makes archives; on some systems "lha" is lhasa, which only reads them
    subprocess.run([shutil.which("jlha") or "lha", "aq", str(archive.resolve()), folder.name],
                   cwd=folder.parent, check=True)


def _copy_part(drawer: Path, to: Path) -> None:
    """The part's drawer copied to `to`, without the drawers of parts inside it (they have their own archive)."""
    def inner(folder: str, names: list[str]) -> list[str]:
        here = Path(folder)
        return [n for n in names if (here / n).is_dir() and (here / n / "Part").is_file()]
    shutil.copytree(drawer, to, ignore=inner)


def index_text(rows: list[tuple[str, str, int, str, str]]) -> str:
    """The index's text from (name, version, bytes, sha256, file) rows."""
    lines = [INDEX_HEAD, "; part, version, bytes, SHA-256, file beside this index"]
    lines += [f"part {n} {v} {size} {sha} {file}" for n, v, size, sha, file in rows]
    return "\n".join(lines + [INDEX_END]) + "\n"


def write_updates(parts: Path, out: Path, archive=lha_archive) -> int:
    """Each part of the pack as <out>/<Name>-<version>.lha, and <out>/GFOpenUpdate.index listing them. The index is
    written last, whole or not at all. Returns how many parts. `archive(archive, folder)` makes one archive."""
    listed = pack_parts(parts)
    out.mkdir(parents=True, exist_ok=True)
    for old in list(out.glob("*.lha")) + [out / INDEX_NAME]:
        old.unlink(missing_ok=True)                     # an earlier cut's: the index lists only this cut's
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        for name, version, drawer in listed:
            file = f"{name}-{version}.lha"
            stage = Path(tmp) / name / drawer.name
            _copy_part(drawer, stage)
            archive(out / file, stage)
            data = (out / file).read_bytes()
            if not data:
                raise SystemExit(f"{out / file}: an empty archive")
            rows.append((name, version, len(data), hashlib.sha256(data).hexdigest(), file))
    whole = out / (INDEX_NAME + ".new")
    whole.write_text(index_text(rows), encoding="latin-1")
    whole.replace(out / INDEX_NAME)
    return len(rows)


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    n = write_updates(Path(sys.argv[1]), Path(sys.argv[2]))
    print(f"{Path(sys.argv[2]) / INDEX_NAME}: {n} parts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
