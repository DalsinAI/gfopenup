#!/usr/bin/env python3
"""The nightly's GFOpenUp pack (autobuild.md; LumaVAULT's nightly build runs this on bigguy).

    python3 tools/nightly_pack.py --base 0.8.31

It puts every repository the pack is built from beside this checkout, each at its main, as a cut on daletop has them
(tools/build_pack.py finds them there), then builds the pack as <base>.<date> with --lha into $NIGHTLY_OUT. Steps
whose repositories haven't moved since the last night are reused (tools/step_reuse.py), so a quiet night is short.
What each repository was at goes in $NIGHTLY_OUT/COMMITS.txt.

The environment is the nightly's: NIGHTLY (the date), NIGHTLY_OUT, STOVES. GitHub addresses are answered by the
machine's git mirror, so no login is needed for the clones. The parts in parts.lock come from their repositories'
releases and need GH_TOKEN; without it they are left out and named, as build_pack.py does for any part it lacks.
Copyright (c) 2026 AmiCradle Team. MIT licence."""
import argparse, os, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEV = ROOT.parent
ORG = "https://github.com/DalsinAI/"
# The checkouts build_pack.py looks for beside this one (its --options' defaults), by repository name.
REPOS = ("amicradle", "amicradle-guest", "gfopenprint", "gfopengadtools", "gfopenprefs", "gfopencompress", "gfopenservice",
         "gfopendatatypes", "gfopenrtg", "gfopenfilemanager", "gfopenmedia", "gfopenmediaplayer", "gfopenmui", "gfopenedit",
         "gfoffice", "gfopenmulticore", "gfopenmail", "gfopenlayout", "gfopenmesa", "gfopentls", "gfopenputty", "gfopentata",
         "gfopenftp", "gfopentelegram", "gfopeninput", "gfopensocket", "gfopenphysfs", "gfopenbench", "gfopenfont", "gfosfonts",
         "gfopennotifier")


def git(*a, cwd=None):
    return subprocess.run(["git", *a], cwd=cwd, capture_output=True, text=True)


def bring(repo: str) -> str:
    """The repository beside this checkout, at its main. Its commit, or '' with the reason printed."""
    d = DEV / repo
    if (d / ".git").is_dir():
        r = git("fetch", "--prune", "origin", cwd=d)
        if r.returncode == 0: r = git("reset", "--hard", "origin/HEAD", cwd=d)
    else:
        r = git("clone", ORG + repo, str(d))
    if r.returncode:
        print(f"nightly_pack: {repo}: {(r.stderr or r.stdout).strip().splitlines()[-1:]}", flush=True)
        return ""
    return git("rev-parse", "HEAD", cwd=d).stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", required=True, help="the version being worked towards (0.8.31): the pack is <base>.<date>")
    a = ap.parse_args()
    day = os.environ.get("NIGHTLY", "").split("-")[0] or __import__("time").strftime("%Y%m%d")
    out = Path(os.environ.get("NIGHTLY_OUT") or ROOT / "build" / "nightly")
    stoves = Path(os.environ.get("STOVES") or Path.home() / "AmiCradle/stoves")
    out.mkdir(parents=True, exist_ok=True)
    commits = [f"gfopenup\t{git('rev-parse', 'HEAD', cwd=ROOT).stdout.strip()}"]
    missing = []
    for repo in REPOS:
        sha = bring(repo)
        commits.append(f"{repo}\t{sha or 'MISSING'}")
        if not sha: missing.append(repo)
    env = dict(os.environ, STOVES=str(stoves), STOVE=str(stoves / "os32"), OS32_GCC16=str(stoves / "os32-gcc16"),
               STOVE16=str(stoves / "os32-gcc16"), SDL2_STOVE=str(stoves / "os32-gcc16"),
               OS32_GCC16_OPEN6=str(stoves / "os32-gcc16-open8"), GL_WORK=str(DEV / ".gl-work"))
    gcc = subprocess.run([str(stoves / "os32-gcc16/prefix/bin/m68k-amigaos-gcc"), "--version"], capture_output=True, text=True)
    old = subprocess.run([str(stoves / "os32/prefix/bin/m68k-amigaos-gcc"), "--version"], capture_output=True, text=True)
    (out / "COMMITS.txt").write_text("\n".join(commits) + "\n" + f"# OS32_GCC16: {gcc.stdout.splitlines()[0] if gcc.stdout else 'not found'}\n"
                                     f"# STOVE: {old.stdout.splitlines()[0] if old.stdout else 'not found'}\n")
    if missing: print("nightly_pack: not beside this checkout, so their parts will be left out: " + ", ".join(missing), flush=True)
    if os.environ.get("GH_TOKEN") or os.environ.get("PARTS_READ_TOKEN"):
        subprocess.run([sys.executable, str(ROOT / "tools/fetch_parts.py"), str(ROOT / "parts.lock"), "--dev", str(DEV), "--checkout"], env=env)
    else:
        print("nightly_pack: no GH_TOKEN, so parts.lock's parts (their own releases) are not fetched", flush=True)
    version = f"{a.base}.{day}"
    cmd = [sys.executable, str(ROOT / "tools/build_pack.py"), "--version", version, "--out", str(out / "pack"), "--lha",
           "--steps", str(DEV / ".gfopenup-steps")]
    print("nightly_pack: " + " ".join(cmd), flush=True)
    return subprocess.run(cmd, env=env, cwd=ROOT).returncode


if __name__ == "__main__":
    sys.exit(main())
