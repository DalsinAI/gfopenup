#!/usr/bin/env python3
"""Fetches the GFOpenUp parts their repositories built when they merged (docs/design/PARTS-ON-MERGE.md) and unpacks
each into its checkout, so build_pack.py --prebuilt prebuilt.tsv --assemble-only puts the pack together without
compiling.

    python3 tools/fetch_parts.py LIST [--dev DIR] [--out prebuilt.tsv] [--checkout]

LIST has one part repository per line, then what to take:
    gfopenbench  latest          the newest build in its "parts" release
    gfopenprefs  3f2a9c1d0e4b    the build of that commit, from "parts"
    acgame       v1.0-rc2        that release of the part's own (its RC)
A freeze.tsv (repository and commit) is such a list. parts.lock is the pack's own.

Each tarball's PART.json must name the repository and a commit; the checkout (DIR/<repo>) must be at that commit
(--checkout puts it there first), else the part isn't taken. The token is GH_TOKEN, else PARTS_READ_TOKEN.

Copyright (c) 2026 AmiCradle Team. MIT licence."""
from __future__ import annotations

import argparse
import io
import json
import os
import subprocess
import sys
import tarfile
import urllib.request
from pathlib import Path

OWNER = "DalsinAI"
API = "https://api.github.com"


def read_list(path: Path) -> list[tuple[str, str]]:
    out = []
    for line in path.read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            repo, ref = (line.split() + ["latest"])[:2]
            out.append((repo.rsplit("/", 1)[-1], ref))
    return out


def _get(url: str, token: str | None, raw: bool = False) -> bytes:
    req = urllib.request.Request(url, headers={"Accept": "application/octet-stream" if raw else "application/vnd.github+json",
                                               "User-Agent": "gfopenup-fetch-parts"})
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    with urllib.request.urlopen(req, timeout=120) as r:
        return r.read()


def pick_asset(assets: list[dict], repo: str, ref: str, latest: dict | None) -> dict | None:
    """The tarball for ref among a release's assets: a commit's (or latest.json's commit's) <repo>-<commit12>.tar.gz,
    or, on a part's own release, its only <repo>-*.tar.gz."""
    tars = [x for x in assets if x["name"].startswith(repo + "-") and x["name"].endswith(".tar.gz")]
    if ref.startswith("v"):
        return tars[0] if len(tars) == 1 else None
    commit = latest["commit"] if ref == "latest" and latest else ref
    want = f"{repo}-{commit[:12]}.tar.gz"
    return next((x for x in tars if x["name"] == want), None)


def unpack(blob: bytes, repo: str, checkout: Path) -> dict:
    """Unpacks a part tarball into the checkout; its PART.json, after checking it names this repository and only
    paths inside the checkout."""
    with tarfile.open(fileobj=io.BytesIO(blob), mode="r:gz") as t:
        part = json.load(t.extractfile("PART.json"))
        if part.get("repo") != repo or not part.get("commit"):
            raise SystemExit(f"{repo}: PART.json names {part.get('repo')!r} at {part.get('commit')!r}")
        for m in t.getmembers():
            if m.name.startswith("/") or ".." in Path(m.name).parts or not (m.isfile() or m.isdir()):
                raise SystemExit(f"{repo}: {m.name} isn't a plain file inside the checkout")
        t.extractall(checkout)
    return part


def head(checkout: Path) -> str:
    r = subprocess.run(["git", "-C", str(checkout), "rev-parse", "HEAD"], capture_output=True, text=True)
    return r.stdout.strip()


def fetch(repo: str, ref: str, checkout: Path, token: str | None, do_checkout: bool, get=_get) -> str:
    tag = ref if ref.startswith("v") else "parts"
    rel = json.loads(get(f"{API}/repos/{OWNER}/{repo}/releases/tags/{tag}", token))
    latest = None
    if ref == "latest":
        lj = next((x for x in rel["assets"] if x["name"] == "latest.json"), None)
        latest = json.loads(get(lj["url"], token, raw=True)) if lj else None
    a = pick_asset(rel["assets"], repo, ref, latest)
    if a is None:
        raise SystemExit(f"{repo}: no built part for {ref} in its {tag!r} release (its part.yml hasn't published it)")
    blob = get(a["url"], token, raw=True)
    with tarfile.open(fileobj=io.BytesIO(blob), mode="r:gz") as t:
        commit = json.load(t.extractfile("PART.json"))["commit"]
    if do_checkout and head(checkout) != commit:
        subprocess.run(["git", "-C", str(checkout), "fetch", "-q", "origin", commit], check=True)
        subprocess.run(["git", "-C", str(checkout), "checkout", "-q", "--detach", commit], check=True)
    if head(checkout) != commit:
        raise SystemExit(f"{repo}: the checkout is at {head(checkout)[:12]}, the part was built at {commit[:12]} "
                         f"(--checkout puts it there)")
    unpack(blob, repo, checkout)
    print(f"{repo}: {a['name']} ({commit[:12]})", flush=True)
    return commit


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("list", type=Path)
    ap.add_argument("--dev", type=Path, default=Path(__file__).resolve().parents[2])
    ap.add_argument("--out", type=Path, default=Path("prebuilt.tsv"))
    ap.add_argument("--checkout", action="store_true", help="put each checkout at its part's commit first")
    a = ap.parse_args()
    token = os.environ.get("GH_TOKEN") or os.environ.get("PARTS_READ_TOKEN")
    rows = []
    for repo, ref in read_list(a.list):
        rows.append(f"{repo}\t{fetch(repo, ref, a.dev / repo, token, a.checkout)}\n")
    a.out.write_text("".join(rows))
    print(f"{len(rows)} parts; build_pack.py --prebuilt {a.out} --assemble-only")
    return 0


if __name__ == "__main__":
    sys.exit(main())
