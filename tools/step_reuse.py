"""Step reuse for tools/build_pack.py: a cut builds again only the steps whose inputs moved.

Each build step (one run() in build_parts) gets a key made of:

  * its command, working folder and the environment it was given beyond this process's own;
  * the HEAD commit of every checkout it reads: the checkouts its command, folder or environment name by path,
    and the checkouts its build script (and the scripts beside it) names in its text, such as gfopengadtools,
    OGT or amicradle-guest, which a script reaches through a default or relative path no argument shows;
  * the compilers the step can reach (each stove's m68k-amigaos-gcc by size and time);
  * anything the caller adds (a pinned commit the step takes from git archive, say).

After a step runs, the files it made or changed in the checkouts it names by path are listed with their sizes in
its stamp (build/steps/<key>.json). On the next cut the step is skipped only when:

  * a stamp with the same key exists, so every checkout it reads is at the same commit as when it was built;
  * every checkout it reads has no changes to tracked files (a dirty tree is never reused);
  * every file listed in the stamp is still there at the same size;
  * no step that ran in this cut wrote a file under a path the step names (so a step that reads another step's
    output builds again when that output was made again).

Anything else builds as before. --fresh (STEPS_FRESH=1) builds everything. Each cut writes build/steps/report.tsv:
step, built or reused, seconds, the checkouts it read at their commits, and the command.

Copyright (c) 2026 AmiCradle Team. MIT licence."""
from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import time
from pathlib import Path

# Words a build script uses for a checkout besides the checkout folder's own name: the environment variables the
# scripts take for them, and the names before the renames (openamiga* to gfopen*, amigachrome to amicradle)
ALIASES = {
    "amicradle-guest": ("amicradle-guest", "amigachrome-guest", "GUEST"),
    "guest": ("amicradle-guest", "amigachrome-guest", "GUEST"),
    "gfopengadtools": ("gfopengadtools", "opengadtools", "OGT"),
    "gfopenprefs": ("gfopenprefs", "openamigaprefs", "OAP"),
    "gfopenfilemanager": ("gfopenfilemanager", "openamigafilemanager", "FILES"),
    "gfopencompress": ("gfopencompress", "openamigacompress", "OC"),
    "gfopenlayout": ("gfopenlayout", "openamigalayout", "OPENLAYOUT", "GFOPENLAYOUT"),
    "gfopentata": ("gfopentata", "openamigatata", "TATA"),
    "gfopentls": ("gfopentls", "openamigatls", "OPENCRYPTO"),
    "gfopenservice": ("gfopenservice", "openamigaservice", "OPENSERVICE"),
    "gfopenrtg": ("gfopenrtg", "openamigartg", "RTG"),
    "gfopendatatypes": ("gfopendatatypes", "opendatatypes", "ODT"),
    "gfopenftp": ("gfopenftp", "openamigaftp", "FTP"),
    "gfopenmedia": ("gfopenmedia", "openamigamedia", "MEDIA"),
    "amicradle": ("amicradle", "amigachrome"),
}
SCRIPT_SUFFIXES = (".sh", ".py", ".mk", ".mak", ".cmake", ".ini")
SCRIPT_MAX = 1 << 20      # a file bigger than this beside a script isn't read for names
GCC_PLACES = ("bin/m68k-amigaos-gcc", "prefix/bin/m68k-amigaos-gcc")
# The environment this process had: only what a step is given beyond it goes in the key, but these do too when
# set, as the scripts read them without being passed them
ENV_KEPT = ("STOVE", "STOVES", "STOVE16", "OS32_GCC16", "OS32_GCC16_OPEN6", "SDL2_STOVE", "GL_WORK", "CC", "AMIGA_CC",
            "CFLAGS", "LDFLAGS", "CPU")


def _git(repo: Path, *args: str) -> str:
    r = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True)
    return r.stdout.strip() if r.returncode == 0 else ""


def checkout_state(repo: Path) -> tuple[str, bool]:
    """(HEAD commit, whether a tracked file differs from it). No commit when it isn't a git checkout."""
    head = _git(repo, "rev-parse", "HEAD")
    dirty = bool(head) and bool(_git(repo, "status", "--porcelain", "--untracked-files=no"))
    return head, dirty


def read_freeze(path: Path) -> dict[str, str]:
    """freeze.tsv: one checkout per line, its folder name and the commit the cut pinned, by a tab or spaces.
    Lines starting with # are notes."""
    out = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, sha = line.split()[:2]
        out[name.rsplit("/", 1)[-1]] = sha
    return out


class StepCache:
    def __init__(self, stamps: Path, checkouts: dict[str, Path], fresh: bool = False, base_env=None):
        self.stamps = stamps
        self.fresh = fresh
        self.base_env = dict(os.environ if base_env is None else base_env)
        self.checkouts: dict[str, Path] = {}
        self.state: dict[str, tuple[str, bool]] = {}
        for name, path in checkouts.items():
            if not path:
                continue
            path = Path(path).resolve()
            head, dirty = checkout_state(path)
            if head and path not in self.checkouts.values():
                self.checkouts[name] = path
                self.state[name] = (head, dirty)
        self.rebuilt: list[Path] = []    # files the steps that ran in this cut wrote
        self.rows: list[list[str]] = []
        self.n = 0
        # each checkout's names as whole words: OC isn't DOCS, amicradle isn't amicradle-guest
        self._words = {n: re.compile("|".join(r"(?<![\w-])" + re.escape(w) + r"(?![\w-])" for w in self._names(n)))
                       for n in self.checkouts}

    def _names(self, name: str) -> tuple[str, ...]:
        return tuple(dict.fromkeys((name, self.checkouts[name].name) + ALIASES.get(name, ()) +
                                   ALIASES.get(self.checkouts[name].name, ())))

    def check_freeze(self, freeze: dict[str, str]) -> None:
        """Stops the cut unless every checkout the freeze list names is at its pinned commit with no tracked
        change: a reused step's stamp holds the commits it was built at, so a cut on the frozen commits takes
        only steps built at them."""
        bad = []
        by_folder = {p.name: n for n, p in self.checkouts.items()}
        for repo, sha in freeze.items():
            n = repo if repo in self.checkouts else by_folder.get(repo)
            if n is None:
                continue
            head, dirty = self.state[n]
            if not head.startswith(sha) and not sha.startswith(head):
                bad.append(f"{repo}: at {head[:12]}, the freeze list pins {sha[:12]}")
            elif dirty:
                bad.append(f"{repo}: tracked files changed since {head[:12]}")
        if bad:
            raise SystemExit("the checkouts aren't the freeze list's:\n  " + "\n  ".join(bad))

    # --- what a step reads ---------------------------------------------------------------------------------------
    def _owner(self, p: Path) -> str | None:
        best = None
        for n, root in self.checkouts.items():
            if p == root or root in p.parents:
                if best is None or len(str(root)) > len(str(self.checkouts[best])):
                    best = n
        return best

    def _paths(self, cmd, cwd, env_extra: dict) -> list[Path]:
        vals = [str(c) for c in cmd] + ([str(cwd)] if cwd else [])
        for v in env_extra.values():
            vals.extend(v.split(os.pathsep))
        out = []
        for v in vals:
            if v.startswith("/"):
                out.append(Path(os.path.abspath(v)))
        return out

    def _scripts(self, cmd, cwd) -> list[Path]:
        files = []
        for c in cmd:
            p = Path(str(c))
            if not p.is_absolute() and cwd:
                p = Path(cwd) / p
            if p.suffix in SCRIPT_SUFFIXES and p.is_file():
                files.append(p)
                for q in sorted(p.parent.iterdir()):
                    if q != p and q.is_file() and (q.suffix in SCRIPT_SUFFIXES or q.name.startswith("Makefile")):
                        files.append(q)
        return files

    def reads(self, cmd, cwd, env_extra: dict) -> list[str]:
        names = set()
        for p in self._paths(cmd, cwd, env_extra):
            n = self._owner(p)
            if n:
                names.add(n)
        text = ""
        for f in self._scripts(cmd, cwd):
            try:
                if f.stat().st_size <= SCRIPT_MAX:
                    text += f.read_text(encoding="latin-1") + "\n"
            except OSError:
                pass
        for n, words in self._words.items():
            if n not in names and words.search(text):
                names.add(n)
        return sorted(names)

    def _env_extra(self, env) -> dict:
        if env is None:
            return {}
        extra = {k: v for k, v in env.items() if self.base_env.get(k) != v and k != "PATH"}
        for k in ENV_KEPT:
            if k in env:
                extra[k] = env[k]
        if env.get("PATH") != self.base_env.get("PATH"):
            # only the folders put in front of this process's PATH (the stove's bin, say)
            base = self.base_env.get("PATH", "").split(os.pathsep)
            extra["PATH+"] = os.pathsep.join(d for d in env.get("PATH", "").split(os.pathsep) if d not in base)
        return extra

    def _compilers(self, cmd, env_extra: dict) -> dict:
        out = {}
        for p in self._paths(cmd, None, env_extra) + [Path(str(c)) for c in cmd[:1]]:
            for cand in [p] + [p / g for g in GCC_PLACES]:
                if cand.name == "m68k-amigaos-gcc" and cand.is_file():
                    st = cand.stat()
                    out[str(cand)] = [st.st_size, st.st_mtime_ns]
        return out

    def key(self, cmd, cwd=None, env=None, extra=None) -> tuple[str, dict]:
        env_extra = self._env_extra(env)
        reads = self.reads(cmd, cwd, env_extra)
        what = {
            "cmd": [str(c) for c in cmd],
            "cwd": str(cwd) if cwd else None,
            "env": dict(sorted(env_extra.items())),
            "reads": {n: self.state[n][0] for n in reads},
            "cc": self._compilers(cmd, env_extra),
            "extra": extra,
        }
        k = hashlib.sha256(json.dumps(what, sort_keys=True).encode()).hexdigest()[:32]
        return k, what

    # --- run or reuse ----------------------------------------------------------------------------------------------
    def _walk_roots(self, cmd, cwd, env_extra) -> list[Path]:
        roots = set()
        for p in self._paths(cmd, cwd, env_extra):
            n = self._owner(p)
            if n:
                roots.add(self.checkouts[n])
            elif p.is_dir() and str(p) in [str(c) for c in cmd]:
                roots.add(p)             # a folder outside the checkouts the command writes into (the GPU kit)
        return sorted(roots)

    @staticmethod
    def _changed(roots: list[Path], since_ns: int) -> list[Path]:
        out = []
        for root in roots:
            for d, dirs, files in os.walk(root):
                dirs[:] = [x for x in dirs if x != ".git"]
                for f in files:
                    p = Path(d) / f
                    try:
                        st = p.lstat()
                    except OSError:
                        continue
                    if st.st_mtime_ns >= since_ns:
                        out.append(p)
        return out

    def _why_not(self, what: dict, stamp: Path, cmd, cwd, env_extra) -> str | None:
        if self.fresh:
            return "fresh"
        dirty = [n for n in what["reads"] if self.state[n][1]]
        if dirty:
            return "changed files in " + ", ".join(dirty)
        if not stamp.is_file():
            return "new or moved"
        try:
            rec = json.loads(stamp.read_text())
        except (OSError, ValueError):
            return "unreadable stamp"
        for rel, size in rec.get("outputs", []):
            p = Path(rel)
            try:
                if p.stat().st_size != size:
                    return f"{p} changed"
            except OSError:
                return f"{p} gone"
        named = self._paths(cmd, cwd, env_extra)
        for out in self.rebuilt:
            for p in named:
                if out == p or p in out.parents:
                    return f"{out} made again in this cut"
        return None

    def would_reuse(self, cmd, cwd=None, env=None, extra=None) -> bool:
        k, what = self.key(cmd, cwd, env, extra)
        return self._why_not(what, self.stamps / f"{k}.json", cmd, cwd, self._env_extra(env)) is None

    def run(self, runner, cmd, cwd=None, env=None, extra=None) -> bool:
        """Runs the step through runner(cmd, cwd=, env=) unless it can be reused; True when it was reused."""
        self.n += 1
        k, what = self.key(cmd, cwd, env, extra)
        env_extra = self._env_extra(env)
        stamp = self.stamps / f"{k}.json"
        reads = " ".join(f"{n}@{h[:10]}" for n, h in what["reads"].items())
        why = self._why_not(what, stamp, cmd, cwd, env_extra)
        line = " ".join(str(c) for c in cmd)
        if why is None:
            print(f"  = step {self.n} reused ({reads})", flush=True)
            self.rows.append([str(self.n), "reused", "0", reads, line])
            return True
        print(f"  + step {self.n} builds: {why} ({reads})", flush=True)
        t0 = time.time()
        since = time.time_ns() - 2_000_000_000     # file systems that round times down
        runner(cmd, cwd=cwd, env=env)
        outs = self._changed(self._walk_roots(cmd, cwd, env_extra), since)
        self.rebuilt.extend(outs)
        self.stamps.mkdir(parents=True, exist_ok=True)
        rec = dict(what, key=k, built=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                   outputs=[[str(p), p.lstat().st_size] for p in outs if p.exists()])
        tmp = stamp.with_suffix(".tmp")
        tmp.write_text(json.dumps(rec, indent=1))
        tmp.replace(stamp)
        self.rows.append([str(self.n), "built", f"{time.time() - t0:.0f}", reads, line])
        return False

    def report(self, path: Path | None = None) -> str:
        path = path or self.stamps / "report.tsv"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("step\tdone\tseconds\treads\tcommand\n" + "".join("\t".join(r) + "\n" for r in self.rows))
        built = [r for r in self.rows if r[1] == "built"]
        secs = sum(float(r[2]) for r in built)
        return (f"{len(self.rows)} steps: {len(built)} built in {secs:.0f} s, "
                f"{len(self.rows) - len(built)} reused ({path})")

    def commits(self, path: Path) -> None:
        """The commits this cut was built from, one checkout per line, in freeze.tsv's form."""
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("".join(f"{n}\t{h}{'  (tracked changes)' if d else ''}\n"
                                for n, (h, d) in sorted(self.state.items())))
