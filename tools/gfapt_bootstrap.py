#!/usr/bin/env python3
"""GFAPT's Aminet package index: gfapt.db, an SQLite file of every package on Aminet, for C:apt's Aminet source
(docs/design/GFOPENAPT.md, "Aminet later"; LumaVAULT GF-64).

Dale, 23 November 2026, pasted a first gfapt_bootstrap.py that opened Aminet's search pages one category at a
time; this is the same tool built on Aminet's own INDEX, which lists every package in one file (name, drawer,
size, age, short description). One fetch fills the table; a package's readme (Version, Author, Architecture,
Requires, Distribution...) and its archive's SHA-256 are fetched only when asked for, a few at a time, so
Aminet's mirrors see little of us.

    python3 tools/gfapt_bootstrap.py                         the whole INDEX into gfapt.db (one fetch)
    python3 tools/gfapt_bootstrap.py --index INDEX.gz        the same from a copy you have
    python3 tools/gfapt_bootstrap.py --readmes util/moni     readmes for one drawer (or util, or all); the
                                                             INDEX is read again only if --index is given
                                     --limit 200 --delay 2   ...at most 200 this run, 2 s apart
    python3 tools/gfapt_bootstrap.py --hash SysInfo.lha      fetch these archives and keep size and SHA-256
    python3 tools/gfapt_bootstrap.py --stats                 what the file holds

Re-running is safe: a package keeps its first_seen; one whose size changed has its readme and SHA-256 fetched
again when next asked; one no longer in the INDEX stays with in_index 0. An INDEX with fewer packages than
--min-packages (default 50000; Aminet has about 86,000) is refused whole and the file is left as it was, so a
cut-short download never marks Aminet as empty.

Python 3 standard library only. MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import re
import sqlite3
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from typing import Callable, Iterable

BASE_URL = "https://aminet.net"
INDEX_URL = BASE_URL + "/INDEX.gz"
USER_AGENT = "GFAPT/0.1 (GFOS package index)"
MIN_PACKAGES = 50000

Fetch = Callable[[str], bytes]

SCHEMA = """
CREATE TABLE IF NOT EXISTS packages (
    id INTEGER PRIMARY KEY,
    dir TEXT NOT NULL,              -- Aminet drawer, e.g. util/moni
    filename TEXT NOT NULL,         -- e.g. SysInfo.lha
    name TEXT NOT NULL,             -- filename less its archive ending, e.g. SysInfo
    category TEXT NOT NULL,         -- dir's first part, e.g. util
    size_text TEXT,                 -- as the INDEX gives it, e.g. 156K
    size_bytes INTEGER,             -- from size_text, exact once --hash has fetched it
    age_days INTEGER,               -- the INDEX's age column
    short TEXT,                     -- the INDEX's one-line description
    version TEXT, author TEXT, uploader TEXT, type TEXT, architecture TEXT,
    requires TEXT, distribution TEXT, replaces TEXT,
    readme_status TEXT,             -- NULL not fetched, 'ok', or why not ('HTTP 404', ...)
    readme_fetched TEXT,
    sha256 TEXT,                    -- of the archive, once --hash has fetched it
    archive_url TEXT NOT NULL, readme_url TEXT NOT NULL, page_url TEXT NOT NULL,
    source TEXT NOT NULL DEFAULT 'aminet',
    in_index INTEGER NOT NULL DEFAULT 1,
    first_seen TEXT NOT NULL,
    last_seen TEXT NOT NULL,
    UNIQUE(dir, filename)
);
CREATE INDEX IF NOT EXISTS idx_packages_name ON packages(name COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_packages_dir ON packages(dir);
CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT);
"""

README_FIELDS = ("short", "author", "uploader", "type", "version", "architecture", "requires",
                 "distribution", "replaces")
ARCHIVE_ENDING = re.compile(r"\.(lha|lzh|lzx|zip|tar\.gz|tgz|tar\.bz2|tar\.xz|tar|dms|adf|gz|bz2|xz|7z|run)$", re.I)
INDEX_LINE = re.compile(r"^(\S+)\s+([a-z]+/[a-z0-9]+)\s+([0-9.]+[KMG]?)\s+(\d+)\s?(.*)$")


def now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def http_fetch(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()


def size_bytes(text: str) -> int | None:
    m = re.fullmatch(r"([0-9.]+)([KMG]?)", text.strip())
    if not m:
        return None
    return int(float(m.group(1)) * {"": 1, "K": 1024, "M": 1024 ** 2, "G": 1024 ** 3}[m.group(2)])


def package_name(filename: str) -> str:
    return ARCHIVE_ENDING.sub("", filename)


def index_line(line: str) -> dict | None:
    """One package line of Aminet's INDEX, or None for a heading, a rule or anything else."""
    m = INDEX_LINE.match(line.rstrip("\r\n"))
    if not m:
        return None
    filename, d, size, age, short = m.groups()
    name = package_name(filename)
    return {"dir": d, "filename": filename, "name": name, "category": d.split("/")[0],
            "size_text": size, "size_bytes": size_bytes(size), "age_days": int(age), "short": short.strip(),
            "archive_url": f"{BASE_URL}/{d}/{filename}",
            "readme_url": f"{BASE_URL}/{d}/{name}.readme",
            "page_url": f"{BASE_URL}/package/{d}/{name}"}


def index_entries(lines: Iterable[str]) -> list[dict]:
    out, seen = [], set()
    for line in lines:
        e = index_line(line)
        if e and (e["dir"], e["filename"]) not in seen:
            seen.add((e["dir"], e["filename"]))
            out.append(e)
    return out


def decode_index(raw: bytes) -> str:
    """The INDEX's text; a gzip that doesn't unpack whole raises (EOFError, gzip.BadGzipFile)."""
    if raw[:2] == b"\x1f\x8b":
        raw = gzip.decompress(raw)
    return raw.decode("latin-1")


def parse_readme(text: str) -> dict:
    """The header fields of an Aminet readme (up to the first blank line); a line that starts with a space
    goes on with the field above it. Keys in lower case; only README_FIELDS are kept."""
    fields: dict[str, str] = {}
    last = None
    for line in text.splitlines():
        if not line.strip():
            break
        if line[:1] in " \t" and last:
            fields[last] = (fields[last] + " " + line.strip()).strip()
            continue
        m = re.match(r"^([A-Za-z][A-Za-z-]*):\s*(.*)$", line)
        if not m:
            break
        last = m.group(1).lower()
        fields[last] = m.group(2).strip()
    return {k: fields[k] for k in README_FIELDS if fields.get(k)}


def open_db(path: str) -> sqlite3.Connection:
    conn = sqlite3.connect(path)
    conn.executescript(SCHEMA)
    return conn


def load_index(conn: sqlite3.Connection, entries: list[dict], source: str) -> dict:
    """Every entry into the table in one transaction. A package whose size changed loses its readme and
    SHA-256 (they are of the old upload); a package not in this INDEX is kept with in_index 0."""
    stamp = now()
    with conn:
        before = conn.execute("SELECT COUNT(*) FROM packages").fetchone()[0]
        conn.execute("UPDATE packages SET in_index = 0")
        conn.executemany("""
            INSERT INTO packages(dir, filename, name, category, size_text, size_bytes, age_days, short,
                                 archive_url, readme_url, page_url, in_index, first_seen, last_seen)
            VALUES(:dir, :filename, :name, :category, :size_text, :size_bytes, :age_days, :short,
                   :archive_url, :readme_url, :page_url, 1, :stamp, :stamp)
            ON CONFLICT(dir, filename) DO UPDATE SET
                readme_status  = CASE WHEN size_text IS excluded.size_text THEN readme_status ELSE NULL END,
                readme_fetched = CASE WHEN size_text IS excluded.size_text THEN readme_fetched ELSE NULL END,
                sha256         = CASE WHEN size_text IS excluded.size_text THEN sha256 ELSE NULL END,
                size_bytes     = CASE WHEN size_text IS excluded.size_text AND sha256 IS NOT NULL
                                      THEN size_bytes ELSE excluded.size_bytes END,
                size_text = excluded.size_text, age_days = excluded.age_days, short = excluded.short,
                name = excluded.name, category = excluded.category, archive_url = excluded.archive_url,
                readme_url = excluded.readme_url, page_url = excluded.page_url,
                in_index = 1, last_seen = excluded.last_seen
            """, [dict(e, stamp=stamp) for e in entries])
        after = conn.execute("SELECT COUNT(*) FROM packages").fetchone()[0]
        gone = conn.execute("SELECT COUNT(*) FROM packages WHERE in_index = 0").fetchone()[0]
        conn.executemany("INSERT OR REPLACE INTO meta(key, value) VALUES(?, ?)",
                         [("index_source", source), ("index_loaded", stamp), ("index_packages", str(len(entries)))])
    return {"packages": len(entries), "new": after - before, "gone": gone}


def wanted(conn: sqlite3.Connection, where: str, limit: int, sql: str) -> list[tuple]:
    if where == "all":
        return conn.execute(sql + " LIMIT ?", (limit,)).fetchall()
    return conn.execute(sql.replace("WHERE", "WHERE (dir = ? OR category = ?) AND", 1) + " LIMIT ?",
                        (where, where, limit)).fetchall()


def fetch_readmes(conn: sqlite3.Connection, where: str, limit: int, delay: float, fetch: Fetch,
                  sleep: Callable[[float], None] = time.sleep) -> dict:
    """Readmes not yet fetched (or of an older upload) for a drawer (util/moni), a category (util) or all,
    at most limit of them, delay seconds apart. A readme that can't be had is noted and not asked again."""
    rows = wanted(conn, where, limit, "SELECT id, readme_url FROM packages WHERE in_index = 1 "
                                      "AND readme_status IS NULL ORDER BY dir, filename")
    got = failed = 0
    for n, (pid, url) in enumerate(rows):
        if n and delay:
            sleep(delay)
        try:
            fields = parse_readme(fetch(url).decode("latin-1"))
            status = "ok"
            got += 1
        except urllib.error.HTTPError as e:
            fields, status = {}, f"HTTP {e.code}"
            failed += 1
        except (urllib.error.URLError, OSError) as e:
            print(f"  {url}: {e}; left for the next run")
            failed += 1
            continue
        sets = {k: fields.get(k) for k in README_FIELDS if k != "short"}   # short: the INDEX's
        with conn:
            conn.execute("UPDATE packages SET " + ", ".join(f"{k} = :{k}" for k in sets) +
                         ", readme_status = :status, readme_fetched = :stamp WHERE id = :id",
                         dict(sets, status=status, stamp=now(), id=pid))
    return {"asked": len(rows), "got": got, "failed": failed}


def hash_archives(conn: sqlite3.Connection, names: list[str], delay: float, fetch: Fetch,
                  sleep: Callable[[float], None] = time.sleep) -> dict:
    """The archives named (filename, name, or dir/filename) fetched and weighed: exact size and SHA-256."""
    done, missing = 0, []
    for want in names:
        if "/" in want:
            d, f = want.rsplit("/", 1)
            rows = conn.execute("SELECT id, archive_url FROM packages WHERE dir = ? AND filename = ?", (d, f)).fetchall()
        else:
            rows = conn.execute("SELECT id, archive_url FROM packages WHERE in_index = 1 AND "
                                "(filename = ? COLLATE NOCASE OR name = ? COLLATE NOCASE)", (want, want)).fetchall()
        if not rows:
            missing.append(want)
            continue
        for pid, url in rows:
            if done:
                sleep(delay)
            data = fetch(url)
            with conn:
                conn.execute("UPDATE packages SET sha256 = ?, size_bytes = ? WHERE id = ?",
                             (hashlib.sha256(data).hexdigest(), len(data), pid))
            done += 1
    return {"hashed": done, "missing": missing}


def stats(conn: sqlite3.Connection) -> str:
    def q(where: str) -> int:
        return conn.execute("SELECT COUNT(*) FROM packages WHERE " + where).fetchone()[0]
    lines = ["packages %d (gone from the INDEX %d), readmes %d, SHA-256 %d" % (
        q("in_index = 1"), q("in_index = 0"), q("readme_status = 'ok'"), q("sha256 IS NOT NULL"))]
    for cat, n in conn.execute("SELECT category, COUNT(*) FROM packages WHERE in_index = 1 "
                               "GROUP BY category ORDER BY category"):
        lines.append(f"  {cat:8} {n}")
    return "\n".join(lines)


def main(argv: list[str] | None = None, fetch: Fetch = http_fetch,
         sleep: Callable[[float], None] = time.sleep) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--db", default="gfapt.db")
    ap.add_argument("--index", help="INDEX or INDEX.gz, a file or an address (default Aminet's); read when given, "
                                    "or when nothing else is asked for")
    ap.add_argument("--min-packages", type=int, default=MIN_PACKAGES)
    ap.add_argument("--readmes", metavar="DRAWER", help="util/moni, util, or all")
    ap.add_argument("--limit", type=int, default=100, help="readmes at most this run (default 100)")
    ap.add_argument("--delay", type=float, default=2.0, help="seconds between fetches from Aminet (default 2)")
    ap.add_argument("--hash", nargs="+", metavar="NAME", default=[])
    ap.add_argument("--stats", action="store_true")
    a = ap.parse_args(argv)
    if a.delay < 0.5:
        ap.error("--delay under 0.5 s is not polite to Aminet")

    conn = open_db(a.db)
    try:
        if a.index or not (a.readmes or a.hash or a.stats):
            src = a.index or INDEX_URL
            if re.match(r"^https?://", src):
                print(f"GET {src}")
                raw = fetch(src)
            else:
                with open(src, "rb") as f:
                    raw = f.read()
            try:
                text = decode_index(raw)
            except (EOFError, OSError) as e:
                print(f"{src}: not a whole INDEX ({e}); gfapt.db left as it was")
                return 10
            entries = index_entries(text.splitlines())
            if len(entries) < a.min_packages:
                print(f"{src}: {len(entries)} packages, fewer than {a.min_packages}; gfapt.db left as it was")
                return 10
            r = load_index(conn, entries, src)
            print(f"INDEX: {r['packages']} packages, {r['new']} new, {r['gone']} gone from it")
        if a.readmes:
            r = fetch_readmes(conn, a.readmes, a.limit, a.delay, fetch, sleep)
            print(f"readmes: {r['got']} read, {r['failed']} not, of {r['asked']} asked")
        if a.hash:
            r = hash_archives(conn, a.hash, a.delay, fetch, sleep)
            print(f"SHA-256: {r['hashed']} archives" + (f"; not in gfapt.db: {', '.join(r['missing'])}" if r["missing"] else ""))
        print(stats(conn))
    finally:
        conn.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
