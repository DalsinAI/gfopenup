#!/usr/bin/env python3
"""GFAPT's Aminet index (tools/gfapt_bootstrap.py) on a small INDEX and readmes of Aminet's form, with no
network: the lines read and refused, the table filled and filled again, a cut-short INDEX refused whole, the
readme fields, the archive's SHA-256, and the pause between fetches. Run: python3 tests/test_gfapt_bootstrap.py

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import gzip
import hashlib
import io
import os
import sqlite3
import sys
import tempfile
import urllib.error
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gfapt_bootstrap as g  # noqa: E402

INDEX = """\
| This is the index of all files currently on Aminet.
| Name                Dir       Size Age Description
|-------------------- -------- ----- --- ------------------------------------
SysInfo.lha          util/moni  156K 300 System information and speed test
SysInfo.lha          util/moni  156K 300 System information and speed test
NDK3.2.lha           dev/misc   5.6M  90 Native Developer Kit for AmigaOS 3.2
fat95.v20260609.lha  disk/misc   86K  20 FAT12/16/32 filesystem handler
not a package line at all
gigalomania.i386-aros.lha game/strat 1.2M 400 Gigalomania for AROS x86
"""

README = """\
Short:        System information and speed test
Author:       Nic Wilson, Tobias Geijersson
Uploader:     sysinfo d0 se
Type:         util/moni
Version:      4.4
Replaces:     util/moni/SysInfo.lha
Architecture: m68k-amigaos >= 2.0.4
Requires:     68020+,
              reqtools.library
Distribution: Aminet

SysInfo shows what your Amiga has.
Version: 9.9 is in the text, not the header.
"""

fails = 0


def check(what: str, ok: bool, more: object = "") -> None:
    global fails
    if not ok:
        fails += 1
        print(f"FAIL {what} {more}")


class Net:
    """A stand-in for Aminet: what was asked, in order, and what each address gives."""

    def __init__(self, pages: dict[str, bytes]):
        self.pages, self.asked, self.slept = pages, [], []

    def fetch(self, url: str) -> bytes:
        self.asked.append(url)
        page = self.pages.get(url)
        if page is None:
            raise urllib.error.HTTPError(url, 404, "Not Found", None, None)
        return page

    def sleep(self, s: float) -> None:
        self.slept.append(s)


def run(net: Net, *argv: str) -> tuple[int, str]:
    out = io.StringIO()
    with redirect_stdout(out):
        rc = g.main(list(argv), fetch=net.fetch, sleep=net.sleep)
    return rc, out.getvalue()


def rows(db: str) -> dict[str, dict]:
    conn = sqlite3.connect(db)
    conn.row_factory = sqlite3.Row
    got = {r["filename"]: dict(r) for r in conn.execute("SELECT * FROM packages")}
    conn.close()
    return got


def test_lines() -> None:
    e = g.index_line("SysInfo.lha          util/moni  156K 300 System information and speed test")
    check("a package line", e is not None and e["dir"] == "util/moni" and e["name"] == "SysInfo"
          and e["size_bytes"] == 156 * 1024 and e["age_days"] == 300 and e["category"] == "util", e)
    check("archive, readme and page addresses",
          e["archive_url"] == "https://aminet.net/util/moni/SysInfo.lha"
          and e["readme_url"] == "https://aminet.net/util/moni/SysInfo.readme"
          and e["page_url"] == "https://aminet.net/package/util/moni/SysInfo", e)
    check("a heading is not a package", g.index_line("| Name                Dir       Size Age Description") is None)
    check("a line with no drawer is not a package", g.index_line("not a package line at all") is None)
    check("a name with dots keeps them", g.index_line(INDEX.splitlines()[6])["name"] == "fat95.v20260609")
    check("5.6M in bytes", g.size_bytes("5.6M") == int(5.6 * 1024 ** 2))
    check("a size that isn't one", g.size_bytes("big") is None)
    check("tar.gz ending", g.package_name("foo-1.0.tar.gz") == "foo-1.0")
    entries = g.index_entries(INDEX.splitlines())
    check("four packages: the repeat once, the stray line none", len(entries) == 4, [e["filename"] for e in entries])


def test_readme() -> None:
    f = g.parse_readme(README)
    check("version", f.get("version") == "4.4", f)
    check("architecture", f.get("architecture") == "m68k-amigaos >= 2.0.4", f)
    check("requires goes on to the next line", f.get("requires") == "68020+, reqtools.library", f)
    check("author, distribution, replaces", f.get("author", "").startswith("Nic Wilson")
          and f.get("distribution") == "Aminet" and f.get("replaces") == "util/moni/SysInfo.lha", f)
    check("the body after the blank line is not read", "9.9" not in f.values(), f)
    check("a readme with no header gives nothing", g.parse_readme("Just text.\n") == {})


def test_db(tmp: str) -> None:
    db = os.path.join(tmp, "gfapt.db")
    index_file = os.path.join(tmp, "INDEX.gz")
    Path(index_file).write_bytes(gzip.compress(INDEX.encode("latin-1")))
    net = Net({"https://aminet.net/INDEX.gz": gzip.compress(INDEX.encode("latin-1")),
               "https://aminet.net/util/moni/SysInfo.readme": README.encode("latin-1"),
               "https://aminet.net/util/moni/SysInfo.lha": b"LHA archive bytes"})

    rc, out = run(net, "--db", db, "--min-packages", "1")
    check("the INDEX from Aminet: one fetch", rc == 0 and net.asked == ["https://aminet.net/INDEX.gz"], (rc, net.asked, out))
    got = rows(db)
    check("four rows", len(got) == 4, list(got))
    first = got["SysInfo.lha"]["first_seen"]

    rc, out = run(net, "--db", db, "--min-packages", "5", "--index", index_file)
    check("too few packages: refused, return 10", rc == 10 and "left as it was" in out, out)
    check("refused: the rows are as they were", rows(db) == got)

    cut = gzip.compress(INDEX.encode("latin-1"))[:-20]
    Path(index_file).write_bytes(cut)
    rc, out = run(net, "--db", db, "--min-packages", "1", "--index", index_file)
    check("a cut-short INDEX.gz: refused, return 10", rc == 10 and "not a whole INDEX" in out, out)
    check("cut short: the rows are as they were", rows(db) == got)

    net.asked.clear()
    rc, out = run(net, "--db", db, "--readmes", "util", "--limit", "10")
    check("readmes for util: only SysInfo's, and no INDEX", rc == 0 and net.asked ==
          ["https://aminet.net/util/moni/SysInfo.readme"], (net.asked, out))
    s = rows(db)["SysInfo.lha"]
    check("readme fields kept", s["version"] == "4.4" and s["architecture"] == "m68k-amigaos >= 2.0.4"
          and s["requires"] == "68020+, reqtools.library" and s["readme_status"] == "ok", s)

    net.asked.clear()
    run(net, "--db", db, "--readmes", "util")
    check("a readme kept is not fetched again", net.asked == [], net.asked)

    net.asked.clear(), net.slept.clear()
    rc, out = run(net, "--db", db, "--readmes", "all", "--limit", "2", "--delay", "3")
    check("limit 2: two readmes asked", len(net.asked) == 2, net.asked)
    check("3 s between them, none before the first", net.slept == [3.0], net.slept)
    check("a missing readme noted, not asked again",
          sum(1 for r in rows(db).values() if r["readme_status"] == "HTTP 404") == 2, out)

    rc, out = run(net, "--db", db, "--hash", "sysinfo")
    s = rows(db)["SysInfo.lha"]
    check("SHA-256 and exact size of the archive", s["sha256"] == hashlib.sha256(b"LHA archive bytes").hexdigest()
          and s["size_bytes"] == len(b"LHA archive bytes"), s)
    rc, out = run(net, "--db", db, "--hash", "NoSuch.lha")
    check("a name not in gfapt.db is said", "NoSuch.lha" in out, out)

    newer = INDEX.replace("SysInfo.lha          util/moni  156K 300", "SysInfo.lha          util/moni  160K   2")
    newer = "\n".join(l for l in newer.splitlines() if not l.startswith("NDK3.2"))
    Path(index_file).write_bytes(newer.encode("latin-1"))
    rc, out = run(net, "--db", db, "--min-packages", "1", "--index", index_file)
    got = rows(db)
    s = got["SysInfo.lha"]
    check("a new upload: readme and SHA-256 dropped, size from the INDEX", s["readme_status"] is None
          and s["version"] == "4.4" and s["sha256"] is None and s["size_bytes"] == 160 * 1024 and s["age_days"] == 2, s)
    check("first_seen kept", s["first_seen"] == first, (first, s["first_seen"]))
    check("a package gone from the INDEX is kept, in_index 0", got["NDK3.2.lha"]["in_index"] == 0
          and got["fat95.v20260609.lha"]["in_index"] == 1, out)
    check("the run says one gone", "1 gone" in out, out)

    rc, out = run(net, "--db", db, "--stats")
    check("stats: three in the INDEX, one gone", rc == 0 and "packages 3 (gone from the INDEX 1)" in out, out)

    try:
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            g.main(["--db", db, "--delay", "0.1", "--stats"], fetch=net.fetch, sleep=net.sleep)
        check("a delay under 0.5 s is refused", False)
    except SystemExit as e:
        check("a delay under 0.5 s is refused", e.code == 2, e.code)


def test_user_agent() -> None:
    check("a plain user agent, no person in it", g.USER_AGENT == "GFAPT/0.1 (GFOS package index)", g.USER_AGENT)


def main() -> int:
    test_lines()
    test_readme()
    test_user_agent()
    with tempfile.TemporaryDirectory() as tmp:
        test_db(tmp)
    print(f"gfapt_bootstrap: {fails} failed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
