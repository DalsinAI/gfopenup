#!/usr/bin/env python3
"""The repo list C:apt ships with (pack/apt.sources, docs/design/GFOPENAPT.md) against the two locks: every
repository in parts.lock and apps.lock has a line, GFOpenUpdate's own index is the first source and is on, and
each line is one src/openapt.h reads. Run: python3 tests/test_apt_sources.py

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OWNER = "DalsinAI"                      # tools/fetch_parts.py's and tools/apps_pack.py's
_REPO = r"[A-Za-z0-9_-][A-Za-z0-9._-]*/[A-Za-z0-9_-][A-Za-z0-9._-]*"
_LINE = re.compile(rf"(off\s+)?(github\s+({_REPO})(\s+[A-Za-z0-9][A-Za-z0-9._+-]*)?|index\s+https?://[!-~]+)")


def lock_repos(lock: Path) -> list[str]:
    """The repositories a lock names: the first word of each line that isn't a comment."""
    return [line.split()[0] for line in lock.read_text(encoding="latin-1").splitlines()
            if line.strip() and not line.lstrip().startswith("#")]


def sources(text: str) -> list[tuple[bool, str]]:
    """(on, the line without "off") for each source; a line the Amiga side would refuse stops the test."""
    out = []
    for n, line in enumerate(text.splitlines(), 1):
        s = line.strip()
        if not s or s[0] in ";#":
            continue
        if not _LINE.fullmatch(s):
            raise SystemExit(f"pack/apt.sources line {n}: not a source: {s!r}")
        off = s.split()[0] == "off"
        out.append((not off, " ".join(s.split()[1:] if off else s.split())))
    return out


def main() -> int:
    fails = 0

    def check(what: str, ok: bool, more: str = "") -> None:
        nonlocal fails
        if not ok:
            fails += 1
            print(f"FAIL {what} {more}")

    got = sources((ROOT / "pack/apt.sources").read_text(encoding="latin-1"))
    check("GFOpenUpdate's own index is the first source, and on", bool(got) and got[0] == (True, f"github {OWNER}/gfopenup updates"))
    listed = {line.split()[1] for _, line in got if line.startswith("github ")}
    for lock in ("parts.lock", "apps.lock"):
        for repo in lock_repos(ROOT / lock):
            check(f"{lock}'s {repo} is in the repo list", f"{OWNER}/{repo}" in listed)
    check("no source twice", len({line for _, line in got}) == len(got))
    check("no more sources than src/openapt.h holds (OUA_MAX_SOURCES)", len(got) <= 48, str(len(got)))
    print(f"{len(got)} sources, {fails} failed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
