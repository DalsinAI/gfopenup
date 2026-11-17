#!/usr/bin/env python3
"""Host checks of tools/update_index.py: a cut's part archives and GFOpenUpdate's index, read back through the
Amiga side's own reader (src/openupdate.h). Made-up parts; the archiver is a stand-in, so no LhA is needed.

    python3 tests/test_update_index.py

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import update_index as ui  # noqa: E402

checks = fails = 0


def check(ok: bool, what: str) -> None:
    global checks, fails
    checks += 1
    if not ok:
        fails += 1
        print("FAIL", what)


def fake_archive(archive: Path, folder: Path) -> None:
    """Stands in for LhA: the names and bytes of what would be archived, so a test can see what went in."""
    rows = [f"{f.relative_to(folder.parent).as_posix()}={f.read_bytes().hex()}"
            for f in sorted(p for p in folder.rglob("*") if p.is_file())]
    archive.write_text("\n".join(rows) + "\n")


def part(parts: Path, rel: str, first: str, files: dict[str, bytes]) -> None:
    d = parts / rel
    d.mkdir(parents=True)
    (d / "Part").write_text(first + '\ncopy "C/x" "C:x"\n', encoding="latin-1")
    for name, data in files.items():
        (d / name).parent.mkdir(parents=True, exist_ok=True)
        (d / name).write_bytes(data)


READER = r"""
#include <stdio.h>
#include "openupdate.h"
int main(int argc, char **argv)
{
    static char buf[65536];
    static struct oud_part p[64];
    FILE *f = fopen(argv[1], "rb");
    long len = f ? (long)fread(buf, 1, sizeof buf, f) : 0;
    int i, n = oud_index_parts(buf, len, p, 64);
    (void)argc; (void)ouv_ver; (void)ouv_keep_newer; (void)oud_offers; (void)oud_interval; (void)oud_due; (void)oud_url; (void)oud_offer_text; (void)oud_skip_text; (void)oud_fetch_cmd; (void)oud_unpack_cmd; (void)oud_install_cmd; (void)oud_last; (void)oud_rows; (void)oud_row_text; (void)oud_status; (void)oud_notice_cmd;
    printf("%d\n", n);
    for (i = 0; i < n; i++) printf("%s %s %ld %s %s\n", p[i].name, p[i].version, p[i].bytes, p[i].sha256, p[i].file);
    return 0;
}
"""


def stopped(fn) -> str:
    try:
        fn()
    except SystemExit as e:
        return str(e)
    return ""


def main() -> int:
    with tempfile.TemporaryDirectory() as t:
        tmp = Path(t)
        parts, out = tmp / "pack/Parts", tmp / "updates"
        part(parts, "GFOpenPlay", "part GFOpenPlay 0.1.7 GFOpenPlay, the player", {"C/x": b"play"})
        part(parts, "GFOpenLibs", "part GFOpenLibs 0.8.30-rc6 the libraries", {"C/x": b"libs", "Libs/a.library": b"a"})
        part(parts, "GFOpenLibs/rexx", "part GFOpenRexx 1.2 ARexx", {"C/x": b"rexx"})
        (parts / "NotAPart").mkdir()
        (parts / "NotAPart/readme").write_text("no Part file here")
        out.mkdir()
        (out / "Old-0.1.lha").write_text("an earlier cut's")

        n = ui.write_updates(parts, out, archive=fake_archive)
        check(n == 3, "three parts, the one inside another's drawer too")
        check(sorted(p.name for p in out.iterdir()) == ["GFOpenLibs-0.8.30-rc6.lha", "GFOpenPlay-0.1.7.lha",
                                                         "GFOpenRexx-1.2.lha", "GFOpenUpdate.index"],
              "an archive a part and the index; an earlier cut's archive gone")
        libs = (out / "GFOpenLibs-0.8.30-rc6.lha").read_text()
        check("GFOpenLibs/Libs/a.library=" in libs and "GFOpenLibs/Part=" in libs, "a part's drawer, its own name first")
        check("rexx" not in libs, "the part inside it is left out of the outer archive")
        check("rexx/C/x=" + b"rexx".hex() in (out / "GFOpenRexx-1.2.lha").read_text(), "and has its own, by its drawer")
        text = (out / "GFOpenUpdate.index").read_text()
        lines = text.splitlines()
        check(lines[0] == "GFOpenUpdate 1" and lines[-1] == "GFOpenUpdate end" and text.endswith("\n"), "head and end lines")
        data = (out / "GFOpenPlay-0.1.7.lha").read_bytes()
        check(f"part GFOpenPlay 0.1.7 {len(data)} {hashlib.sha256(data).hexdigest()} GFOpenPlay-0.1.7.lha" in lines,
              "a part's line: its archive's bytes and SHA-256")

        # the Amiga side's own reader takes it
        cc = os.environ.get("CC") or shutil.which("cc") or shutil.which("gcc")
        (tmp / "reader.c").write_text(READER)
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}", str(tmp / "reader.c"),
                        "-o", str(tmp / "reader")], check=True)
        r = subprocess.run([str(tmp / "reader"), str(out / "GFOpenUpdate.index")], capture_output=True, text=True)
        got = r.stdout.splitlines()
        check(got and got[0] == "3", "src/openupdate.h reads three parts from it")
        check(got[1:] == [ln[len("part "):] for ln in lines if ln.startswith("part ")], "the same fields, in the index's order")

        # what stops a cut
        bad = tmp / "bad1/Parts"
        part(bad, "A", "part A new x", {})
        check("version" in stopped(lambda: ui.write_updates(bad, tmp / "o1", archive=fake_archive)), "a version without a number")
        check(not (tmp / "o1" / "GFOpenUpdate.index").exists(), "and no index is left")
        bad = tmp / "bad2/Parts"
        part(bad, "A", "part Same 1.0 x", {})
        part(bad, "B", "part Same 1.1 x", {})
        check("is also" in stopped(lambda: ui.write_updates(bad, tmp / "o2", archive=fake_archive)), "a name used twice")
        bad = tmp / "bad3/Parts"
        part(bad, "A", "part A/B 1.0 x", {})
        check("name" in stopped(lambda: ui.write_updates(bad, tmp / "o3", archive=fake_archive)), "a name with a slash")
        bad = tmp / "bad4/Parts"
        part(bad, "A", "part A 1.0 x", {})
        check("empty" in stopped(lambda: ui.write_updates(bad, tmp / "o4", archive=lambda a, f: a.write_bytes(b""))),
              "an empty archive")
        check(not (tmp / "o4" / "GFOpenUpdate.index").exists(), "and no index is left then either")

        empty = tmp / "none/Parts"
        empty.mkdir(parents=True)
        check(ui.write_updates(empty, tmp / "o5", archive=fake_archive) == 0
              and (tmp / "o5/GFOpenUpdate.index").read_text().splitlines()[-1] == "GFOpenUpdate end", "no parts: a whole, empty index")

    print(f"{checks} checks, {fails} failed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
