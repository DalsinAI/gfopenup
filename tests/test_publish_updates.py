#!/usr/bin/env python3
"""tools/publish_updates.py: what goes onto the release, and in what order (the index last).
MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import publish_updates as pu  # noqa: E402
import update_index as ui  # noqa: E402

fails = 0
checks = 0


def check(ok: bool, what: str) -> None:
    global fails, checks
    checks += 1
    if not ok:
        fails += 1
        print("FAIL:", what)


def stops(fn) -> bool:
    try:
        fn()
    except SystemExit:
        return True
    return False


with tempfile.TemporaryDirectory() as t:
    d = Path(t)
    (d / "GFOpenPlay-0.1.7.lha").write_bytes(b"x" * 10)
    (d / "GFOpenDock-0.7.lha").write_bytes(b"y" * 3)
    idx = d / ui.INDEX_NAME
    idx.write_text(ui.index_text([("GFOpenPlay", "0.1.7", 10, "a" * 64, "GFOpenPlay-0.1.7.lha"),
                                  ("GFOpenDock", "0.7", 3, "b" * 64, "GFOpenDock-0.7.lha")]))
    files = pu.plan(d)
    check([f.name for f in files] == ["GFOpenPlay-0.1.7.lha", "GFOpenDock-0.7.lha", ui.INDEX_NAME],
          "the archives in the index's order, the index last")
    cmds = pu.commands(files, "DalsinAI/gfopenup", "updates")
    check(cmds[0][:4] == ["gh", "release", "view", "updates"], "looks for the release first")
    check("--prerelease" in cmds[1], "a new release is a pre-release")
    check(all("--clobber" in c for c in cmds[2:]) and cmds[-1][4].endswith(ui.INDEX_NAME), "replaces, the index last")
    (d / "notes.txt").write_text("private")
    check(stops(lambda: pu.plan(d)), "a file the index doesn't name stops it")
    (d / "notes.txt").unlink()
    (d / "GFOpenDock-0.7.lha").write_bytes(b"y" * 4)
    check(stops(lambda: pu.plan(d)), "an archive of the wrong size stops it")
    (d / "GFOpenDock-0.7.lha").write_bytes(b"y" * 3)
    idx.write_text(idx.read_text().replace(ui.INDEX_END, ""))
    check(stops(lambda: pu.plan(d)), "an index not whole stops it")
    idx.unlink()
    check(stops(lambda: pu.plan(d)), "no index stops it")

print(f"{checks} checks, {fails} failed")
sys.exit(1 if fails else 0)
