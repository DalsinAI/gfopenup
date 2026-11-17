#!/usr/bin/env python3
"""Host checks of tools/apps_pack.py, the GFOpenApps pack that puts our applications on Work: (made-up archives
only; nothing is fetched).

    python3 tests/test_apps_pack.py
    AMICRADLE_GUEST=<amicradle-guest checkout> python3 tests/test_apps_pack.py     and the real drawer icons

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import hashlib
import io
import json
import os
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import apps_pack as ap  # noqa: E402
import soft3d_motifs as marks  # noqa: E402

checks = 0


def check(ok: bool, what: str) -> None:
    global checks
    checks += 1
    if not ok:
        raise SystemExit(f"FAIL: {what}")


def refused(what: str, f) -> None:
    try:
        f()
    except ap.AppsError:
        check(True, what)
    else:
        check(False, what)


def zipped(path: Path, files: dict[str, bytes]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "w") as z:
        for name, data in files.items():
            z.writestr(name, data)
    path.with_name(path.name + ".sha256").write_text(hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name + "\n")


def main() -> int:
    # the real lock: every line reads, drawers differ, each mark is drawn, each part has a name and a version
    apps = ap.read_lock(ROOT / "apps.lock")
    check(len(apps) >= 19, "apps.lock lists the applications that have a release")
    check(all(a["mark"] in marks.MOTIFS for a in apps), "every app's drawer mark is one that is drawn")
    check(len({ap.part_name(a) for a in apps}) == len(apps) and all(ap.version(a["tag"]) for a in apps),
          "every app has a part name of its own and a version")
    by = {a["drawer"]: a for a in apps}
    check({"GFOpenBrowser", "GFOffice", "GFOpenMakerSuite", "GFOpenPuTTY", "aMule", "KiCad"} <= set(by),
          "the browser, GFOffice, the maker suite, PuTTY, aMule and KiCad are in it")
    check(ap.version("v0.9.0-rc2") == "0.9.0" and ap.version("gfoffice-2.0.25-rc91") == "2.0.25"
          and ap.version("v5.14.2-amiga.2") == "5.14.2", "a part's version is its release's dotted number")
    refused("a release name with no version is refused", lambda: ap.version("latest"))

    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        bad = t / "bad.lock"
        bad.write_text("repo v1.0 file.lha Drawer\n")
        refused("a lock line short of a word is refused", lambda: ap.read_lock(bad))
        bad.write_text("a v1.0 a.lha Same globe\nb v1.0 b.lha same globe\n")
        refused("a drawer named twice is refused", lambda: ap.read_lock(bad))
        bad.write_text("a v1.0 a.lha ADrawerNameThatIsFarTooLongForFFS globe\n")
        refused("a drawer name FFS can't hold is refused", lambda: ap.read_lock(bad))

        lock = t / "apps.lock"
        lock.write_text("one v1.2-rc3 One-1.2.zip One globe   # the first\n"
                        "DalsinAI/two v0.3 Two.tar.gz Two-App code\n")
        assets = t / "assets"
        zipped(assets / "one" / "One-1.2.zip", {"One/One": b"program one", "One/One.info": b"icon", "One/Docs/Read Me": b"text",
                                                 "One/Install": b"; installer", "One.info": b"the release's drawer icon"})
        tgz = assets / "two" / "Two.tar.gz"
        tgz.parent.mkdir(parents=True)
        with tarfile.open(tgz, "w:gz") as tf:
            for name, data in {"Two": b"program two", "libs/two.library": b"library"}.items():
                info = tarfile.TarInfo(name)
                info.size = len(data)
                tf.addfile(info, io.BytesIO(data))
        tgz.with_name("Two.tar.gz.sha256").write_text(hashlib.sha256(tgz.read_bytes()).hexdigest() + "\n")

        icon = lambda mark: b"ICON:" + mark.encode()
        rows = ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: [])
        top = t / "out" / "GFOpenApps"
        one = (top / "Parts/AppOne/Part").read_text().splitlines()
        check(one[0] == "part AppOne 1.2", "the part is App<Drawer> at the release's version")
        check('when WorkDrive copy "Files/One" "Work:One/One"' in one
              and 'when WorkDrive copy "Files/Docs/Read Me" "Work:One/Docs/Read Me"' in one,
              "the archive's one top drawer is taken off, and its files go in Work:<Drawer>")
        check(one[-1] == 'when WorkDrive add "Drawer.info" "Work:One.info"'
              and (top / "Parts/AppOne/Drawer.info").read_bytes() == b"ICON:globe",
              "the drawer gets the standard folder with the app's mark, not the release's own drawer icon")
        check(all(l.startswith(("part ", "; ", "when WorkDrive ")) for l in one), "every line is only for a machine with Work:")
        check((top / "Parts/AppOne/Files/Docs/Read Me").read_bytes() == b"text", "the files are in the part")
        two = (top / "Parts/AppTwoApp/Part").read_text()
        check('when WorkDrive copy "Files/libs/two.library" "Work:Two-App/libs/two.library"' in two
              and two.startswith("part AppTwoApp 0.3\n"), "an archive with no top drawer is taken as it is")
        check([r["installer"] for r in rows] == ["Install", ""] and rows[0]["files"] == 4 and rows[0]["bytes"] == 30,
              "the list names an app's own installer, its files and bytes")
        script = (top / "Install-GFOpenApps").read_text()
        check('C:GFOpenUpTool INSTALL "Work:GFOpenApps/Parts/AppOne"' in script
              and script.index("AppOne") < script.index("AppTwoApp") and "FailAt 21" in script,
              "the install script installs each part with GFOpenUpTool, in the lock's order")
        listing = (top / "APPS-LIST.txt").read_text().splitlines()
        check(len(listing) == 3 and rows[0]["sha256"] in listing[1] and "\tInstall" in listing[1], "APPS-LIST.txt has a row an app")
        check(not (t / "out" / "unpacked" / "one").exists(), "the unpacked copies are cleared away")

        refused("a file held back from release stops the build",
                lambda: ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: ["Parts/AppOne/Files/One: held"]))
        (assets / "one" / "One-1.2.zip.sha256").write_text("0" * 64 + "\n")
        refused("a file that is not the one fetched stops the build",
                lambda: ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: []))
        (assets / "two" / "Two.tar.gz").unlink()
        zipped(assets / "one" / "One-1.2.zip", {"One/One": b"x"})
        refused("an app that isn't fetched stops the build",
                lambda: ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: []))

        long = t / "long"
        (long / "a").mkdir(parents=True)
        (long / "a" / ("n" * 31)).write_bytes(b"x")
        refused("a name longer than an FFS volume keeps stops the build",
                lambda: ap.app_part(t / "p", dict(repo="r", tag="v1.0", asset="a", drawer="D", mark="globe", title=""), long, icon))
        refused("a release with no files stops the build",
                lambda: ap.app_part(t / "p", dict(repo="r", tag="v1.0", asset="a", drawer="D", mark="globe", title=""), (t / "empty").mkdir() or t / "empty", icon))
        evil = t / "evil.zip"
        with zipfile.ZipFile(evil, "w") as z:
            z.writestr("../out.txt", b"x")
        refused("an archive member that leaves its drawer is refused", lambda: ap.unpack(evil, t / "e"))
        refused("a file that is no archive is refused", lambda: ap.unpack(bad, t / "e"))

        # fetch, with a made-up GitHub: the file is written with its sha256, a wrong digest is refused, a kept file isn't fetched again
        payload = b"a release's archive"
        calls = []
        named = hashlib.sha256(payload).hexdigest()

        def get(url, token, raw=False):
            calls.append(url)
            if raw:
                return payload
            return json.dumps({"assets": [{"name": "One-1.2.zip", "url": "asset://one",
                                           "digest": "sha256:" + named}]}).encode()

        solo = t / "solo.lock"
        solo.write_text("one v1.2-rc3 One-1.2.zip One globe\n")
        got = ap.fetch(solo, t / "fetched", get=get)
        f = t / "fetched" / "one" / "One-1.2.zip"
        check(got == ["One-1.2.zip"] and f.read_bytes() == payload
              and f.with_name(f.name + ".sha256").read_text().split()[0] == hashlib.sha256(payload).hexdigest()
              and calls[0].endswith("/repos/DalsinAI/one/releases/tags/v1.2-rc3"), "fetch takes the lock's release file and writes its sha256")
        calls.clear()
        check(ap.fetch(solo, t / "fetched", get=get) == [] and not calls, "a file already fetched is kept")
        payload = b"something else"
        f.unlink()
        refused("a file that is not the one its release names is refused", lambda: ap.fetch(solo, t / "fetched", get=get))
        solo.write_text("one v1.2-rc3 Other.zip One globe\n")
        refused("a file the release doesn't have is refused", lambda: ap.fetch(solo, t / "fetched", get=get))

    guest = os.environ.get("AMICRADLE_GUEST")
    if guest:
        data = ap.soft3d_icon("globe")
        check(data[:2] == b"\xe3\x10" and data[48] == 2, "the real drawer icon is a drawer's .info file")
        refused("a mark that isn't drawn is refused", lambda: ap.soft3d_icon("nothing"))
    print(f"test_apps_pack: {checks} checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
