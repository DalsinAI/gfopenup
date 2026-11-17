#!/usr/bin/env python3
"""Host checks of tools/step_reuse.py, the step reuse build_pack.py's build_parts runs through: a step is reused only
when every checkout it reads is at the commit it was built at with no tracked change, its files are still there and
no step of this cut made what it reads again. Two small git checkouts stand in for the part repositories.

    python3 tests/test_step_reuse.py

Copyright (c) 2026 AmiCradle Team. MIT licence."""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import step_reuse  # noqa: E402

FAILS = []


def check(cond, what):
    print(("ok   " if cond else "FAIL ") + what)
    if not cond:
        FAILS.append(what)


def git(repo: Path, *args):
    subprocess.run(["git", "-C", str(repo), "-c", "user.name=t", "-c", "user.email=t@t", *args], check=True,
                   capture_output=True)


def make_repo(d: Path, files: dict) -> Path:
    d.mkdir(parents=True)
    git(d, "init", "-q")
    for k, v in files.items():
        (d / k).parent.mkdir(parents=True, exist_ok=True)
        (d / k).write_text(v)
    git(d, "add", "-A")
    git(d, "commit", "-qm", "one")
    return d


def main() -> int:
    with tempfile.TemporaryDirectory() as t:
        dev = Path(t)
        # partlib: a library another part's script reaches by a relative path no argument names
        lib = make_repo(dev / "gfopenpartlib", {"lib.h": "#define V 1\n", ".gitignore": "build/\n"})
        # app: its script reads ../gfopenpartlib, and writes build/app
        app = make_repo(dev / "gfopenapp", {
            "build.sh": 'set -e\nd=$(dirname "$0")\nmkdir -p "$d/build"\n'
                        'cat "$d/../gfopenpartlib/lib.h" > "$d/build/app"\necho x >> "$d/build/app"\n',
            ".gitignore": "build/\n"})
        # leaf: names nothing else
        leaf = make_repo(dev / "gfopenleaf", {
            "build.sh": 'set -e\nd=$(dirname "$0")\nmkdir -p "$d/build"\necho leaf > "$d/build/leaf"\n',
            ".gitignore": "build/\n"})
        # user: takes app's build output by path
        user = make_repo(dev / "gfopenuser", {
            "build.sh": 'set -e\nd=$(dirname "$0")\nmkdir -p "$d/build"\ncat "$IN/app" > "$d/build/user"\n',
            ".gitignore": "build/\n"})
        stamps = dev / "steps"
        ran = []

        def runner(cmd, cwd=None, env=None):
            ran.append(Path(str(cmd[1])).parent.name)
            subprocess.run([str(c) for c in cmd], cwd=cwd, env=env, check=True)

        def cut(fresh=False):
            ran.clear()
            c = step_reuse.StepCache(stamps, {p.name: p for p in (lib, app, leaf, user)}, fresh=fresh)
            env = dict(os.environ)
            c.run(runner, ["sh", app / "build.sh"], env=env)
            c.run(runner, ["sh", leaf / "build.sh"], env=env)
            c.run(runner, ["sh", user / "build.sh"], env=dict(env, IN=str(app / "build")))
            print("     " + c.report())
            return c

        c = cut()
        check(ran == ["gfopenapp", "gfopenleaf", "gfopenuser"], "first cut builds every step")
        check(c.reads(["sh", app / "build.sh"], None, {}) == ["gfopenapp", "gfopenpartlib"],
              "a checkout the script names only by a relative path is read")
        cut()
        check(ran == [], "second cut with nothing moved reuses every step")

        (lib / "lib.h").write_text("#define V 2\n")
        git(lib, "commit", "-qam", "two")
        cut()
        check(ran == ["gfopenapp", "gfopenuser"],
              "a commit in the library rebuilds the part that reads it and the step that takes its output, not the leaf")
        check((user / "build/user").read_text().startswith("#define V 2"), "the output that was taken is the new one")
        cut()
        check(ran == [], "and the cut after that reuses them again")

        (leaf / "build/leaf").unlink()
        cut()
        check(ran == ["gfopenleaf"], "a step whose file is gone builds again")

        (leaf / "build.sh").write_text((leaf / "build.sh").read_text() + "# changed, not committed\n")
        cut()
        check(ran == ["gfopenleaf"], "a checkout with a tracked change is never reused")
        cut()
        check(ran == ["gfopenleaf"], "nor on the next cut while the change is there")
        git(leaf, "checkout", "-q", "--", "build.sh")

        c = cut(fresh=True)
        check(ran == ["gfopenapp", "gfopenleaf", "gfopenuser"], "--fresh builds every step")

        head = subprocess.run(["git", "-C", str(lib), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()
        fz = dev / "freeze.tsv"
        fz.write_text(f"# rc\ngfopenpartlib\t{head}\ngfopenapp {head}\n")
        try:
            c.check_freeze(step_reuse.read_freeze(fz))
            check(False, "a checkout at another commit than the freeze list's stops the cut")
        except SystemExit as e:
            print("     " + str(e).replace("\n", " / "))
            check("gfopenapp" in str(e) and "gfopenpartlib" not in str(e),
                  "a checkout at another commit than the freeze list's stops the cut, and only it is named")
        c.commits(dev / "out/commits.tsv")
        check(f"gfopenpartlib\t{head}" in (dev / "out/commits.tsv").read_text(), "the cut's commits are written")
    prebuilt_checks()
    fetch_checks()
    print("all passed" if not FAILS else f"{len(FAILS)} failed")
    return 1 if FAILS else 0


def prebuilt_checks():
    """--prebuilt: a part fetched at its checkout's commit isn't built; --assemble-only: nothing else is either."""
    with tempfile.TemporaryDirectory() as t:
        dev = Path(t)
        a = make_repo(dev / "gfopena", {"build.sh": "exit 1\n"})
        b = make_repo(dev / "gfopenb", {"build.sh": "true\n"})
        ha = subprocess.run(["git", "-C", str(a), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()
        ran = []

        def runner(cmd, cwd=None, env=None):
            ran.append(Path(str(cmd[1])).parent.name)

        c = step_reuse.StepCache(dev / "steps", {"gfopena": a, "gfopenb": b}, prebuilt={"gfopena": ha[:12]},
                                 no_compile=True)
        c.run(runner, ["sh", a / "build.sh"], env=dict(os.environ))
        c.run(runner, ["sh", b / "build.sh"], env=dict(os.environ))
        check(ran == [] and c.rows[0][1] == "prebuilt", "a prebuilt part at its checkout's commit isn't built")
        check(len(c.missing) == 1 and "gfopenb" in c.missing[0], "--assemble-only names the step with no built part")
        c2 = step_reuse.StepCache(dev / "steps", {"gfopena": a}, prebuilt={"gfopena": "0" * 12})
        c2.run(runner, ["sh", a / "build.sh"], env=dict(os.environ))
        check(ran == ["gfopena"], "a part built at another commit than the checkout's is built here")


def fetch_checks():
    """fetch_parts.py: the asset for a commit, latest or a part's own RC; the checkout must be at its commit."""
    import io
    import json
    import tarfile
    import fetch_parts as fp
    with tempfile.TemporaryDirectory() as t:
        dev = Path(t)
        r = make_repo(dev / "gfopenx", {"a.c": "x\n"})
        h = subprocess.run(["git", "-C", str(r), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()

        def tarball(commit, repo="gfopenx", extra=None):
            bio = io.BytesIO()
            with tarfile.open(fileobj=bio, mode="w:gz") as tf:
                for name, data in [("PART.json", json.dumps({"repo": repo, "commit": commit}).encode()),
                                   ("build/os3/X", b"built")] + (extra or []):
                    ti = tarfile.TarInfo(name)
                    ti.size = len(data)
                    tf.addfile(ti, io.BytesIO(data))
            return bio.getvalue()

        store = {"https://api.github.com/repos/DalsinAI/gfopenx/releases/tags/parts":
                 json.dumps({"assets": [{"name": f"gfopenx-{h[:12]}.tar.gz", "url": "T1"},
                                        {"name": "gfopenx-000000000000.tar.gz", "url": "T0"},
                                        {"name": "latest.json", "url": "L"}]}).encode(),
                 "https://api.github.com/repos/DalsinAI/gfopenx/releases/tags/v1.0-rc1":
                 json.dumps({"assets": [{"name": "gfopenx-1.0-rc1.tar.gz", "url": "T1"}]}).encode(),
                 "L": json.dumps({"commit": h}).encode(), "T1": tarball(h), "T0": tarball("0" * 40)}

        def get(url, token, raw=False):
            return store[url]

        for ref in ("latest", h, "v1.0-rc1"):
            (r / "build/os3/X").unlink(missing_ok=True)
            got = fp.fetch("gfopenx", ref, r, None, False, get=get)
            check(got == h and (r / "build/os3/X").read_bytes() == b"built", f"fetch_parts takes {ref[:12]}")
        try:
            fp.fetch("gfopenx", "000000000000", r, None, False, get=get)
            check(False, "a part built at another commit than the checkout's isn't taken")
        except SystemExit:
            check(True, "a part built at another commit than the checkout's isn't taken")
        try:
            fp.unpack(tarball(h, extra=[("../evil", b"x")]), "gfopenx", r)
            check(False, "a tarball with a path outside the checkout isn't unpacked")
        except SystemExit:
            check(True, "a tarball with a path outside the checkout isn't unpacked")


if __name__ == "__main__":
    sys.exit(main())
