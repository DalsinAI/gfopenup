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
    check({"GFOpenBrowser", "GFOffice", "GFOpenMakerSuite", "aMule", "KiCad"} <= set(by),
          "the browser, GFOffice, the maker suite, aMule and KiCad are in it")
    sys.path.insert(0, str(ROOT / "tools"))
    import build_pack
    check(not set(by) & set(build_pack.WORK_APPS),
          "no application is in both packs: what the GFOpenUp pack puts on Work: (WORK_APPS) is not in apps.lock")
    check(by["DOSBox"]["home"] == "Emulation" and all(a["home"] == "Work" for a in apps if a["drawer"] != "DOSBox"),
          "DOSBox, our emulator, is for the Emulation drive, and every other application for Work:")
    check(ap.version("v0.9.0-rc2") == "0.9.0" and ap.version("gfoffice-2.0.25-rc91") == "2.0.25"
          and ap.version("v5.14.2-amiga.2") == "5.14.2", "a part's version is its release's dotted number")
    refused("a release name with no version is refused", lambda: ap.version("latest"))
    # a line moved forward by half (the release's name, not its file, or the other way) would fetch nothing, or an old file
    check(ap.candidate("v0.82.2-amigaos3-rc3") == 3 and ap.candidate("GFAmigaKiCad-0.1.0-RC5.lha") == 5
          and ap.candidate("gfoffice-2.0.25-rc92") == 92 and ap.candidate("v1.1.0") is None and ap.candidate("4Pane-8.0.lha") is None
          and ap.candidate("src-arch.lha") is None, "a release candidate's number is read from a release's name and from a file's")
    check(ap.half_moved(apps) == [], "in apps.lock, a file that names a release candidate names its release's")
    check(ap.half_moved([dict(drawer="A", tag="v1.0-rc3", asset="A-1.0-rc1.lha"), dict(drawer="B", tag="v1.0-rc3", asset="B-1.0-RC3.lha"),
                         dict(drawer="C", tag="v1.0-rc4", asset="C-8.0.lha"), dict(drawer="D", tag="v1.0", asset="D-1.0-rc2.lha")]) == ["A"],
          "a file of another candidate than its release's is found; a file or a release that names none is let be")
    check(by["GFOffice"]["tag"] == "gfoffice-2.0.25-rc92" and by["GFOffice"]["asset"] == "GFOffice-2.0.25-rc92-AmigaOS3.lha"
          and by["DOSBox"]["tag"] == "v0.82.2-amigaos3-rc3" and by["DOSBox"]["asset"] == "dosbox-0.82.2-amigaos3-rc3-68040.lha",
          "GFOffice is at RC92 and DOSBox at RC3 (GF-45): the newest each has published")

    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        bad = t / "bad.lock"
        bad.write_text("repo v1.0 file.lha Drawer\n")
        refused("a lock line short of a word is refused", lambda: ap.read_lock(bad))
        bad.write_text("a v1.0 a.lha Same globe\nb v1.0 b.lha same globe\n")
        refused("a drawer named twice is refused", lambda: ap.read_lock(bad))
        bad.write_text("a v1.0 a.lha Games:Thing globe\n")
        refused("a volume that is not for applications is refused", lambda: ap.read_lock(bad))
        bad.write_text("a v1.0 a.lha ADrawerNameThatIsFarTooLongForFFS globe\n")
        refused("a drawer name FFS can't hold is refused", lambda: ap.read_lock(bad))

        lock = t / "apps.lock"
        lock.write_text("one v1.2-rc3 One-1.2.zip One globe   # the first\n"
                        "DalsinAI/two v0.3 Two.tar.gz Emulation:Two-App code\n")
        assets = t / "assets"
        zipped(assets / "one" / "One-1.2.zip", {"One/One": b"program one", "One/One.info": b"icon", "One/Docs/Read Me": b"text",
                                                 "One/Install": b"; installer", "One/INSTALL.txt": b"how to", "One.info": b"the release's drawer icon"})
        tgz = assets / "two" / "Two.tar.gz"
        tgz.parent.mkdir(parents=True)
        with tarfile.open(tgz, "w:gz") as tf:
            for name, data in {"Two": b"program two", "libs/two.library": b"library"}.items():
                info = tarfile.TarInfo(name)
                info.size = len(data)
                tf.addfile(info, io.BytesIO(data))
        tgz.with_name("Two.tar.gz.sha256").write_text(hashlib.sha256(tgz.read_bytes()).hexdigest() + "\n")

        icon = lambda mark: b"ICON:" + mark.encode()

        def info(kind: int, place: bytes = b"\x00\x00\x00\x10\x00\x00\x00\x20") -> bytes:      # an icon file as far as tidy_icon reads it
            return b"\xe3\x10" + bytes(46) + bytes([kind]) + bytes(9) + place + bytes(12) + b"old picture"

        face = lambda kind, old: b"\xe3\x10" + bytes(46) + bytes([{"drawer": 2, "tool": 3, "project": 4}[kind]]) + bytes(9) + \
            (old[58:66] if old else bytes(8)) + bytes(12) + b"S3D:" + kind.encode()
        tool = ap.tidy_icon(info(3), face)
        check(tool.endswith(b"S3D:tool") and tool[58:66] == ap.NO_PLACE and tool[48] == 3,
              "a release's tool icon gets the standard picture of its kind and no place of its own")
        check(ap.tidy_icon(info(2), face).endswith(b"S3D:drawer") and ap.tidy_icon(info(4), face).endswith(b"S3D:project"),
              "a drawer's and a project's icon likewise")
        check(ap.tidy_icon(b"icon", face) == b"icon" and ap.tidy_icon(info(1), face) == info(1),
              "a file that is no drawer, tool or project icon is left as it is")

        def picky(kind, old):
            if old:
                raise ValueError("an icon this writer can't read")
            return face(kind, None)
        check(ap.tidy_icon(info(3), picky).endswith(b"S3D:tool"), "an icon the writer can't read becomes the plain one of its kind")
        tidy = t / "tidy"
        (tidy / "Sub").mkdir(parents=True)
        (tidy / "Prog").write_bytes(ap.HUNK_HEADER + b"code")
        (tidy / "Other").write_bytes(ap.HUNK_HEADER + b"code")
        (tidy / "Other.INFO").write_bytes(info(3))
        (tidy / "ReadMe").write_bytes(b"text")
        (tidy / "Sub.info").write_bytes(info(2))
        (tidy / "Sub" / "helper").write_bytes(ap.HUNK_HEADER + b"code")
        row = ap.app_part(t / "tp", dict(repo="r", tag="v1.0", asset="a", drawer="Tidy", mark="globe", title="", home="Work"), tidy, icon, face)
        tp = t / "tp" / "Parts" / "AppTidy"
        lines = (tp / "Part").read_text().splitlines()
        check((tp / "Files/Prog.info").read_bytes().endswith(b"S3D:tool") and (tp / "Files/Prog.info").read_bytes()[58:66] == ap.NO_PLACE
              and 'when WorkDrive copy "Files/Prog.info" "Work:Tidy/Prog.info"' in lines,
              "a program at the drawer's top with no icon gets the tool icon")
        check(not (tp / "Files/ReadMe.info").exists() and not (tp / "Files/Sub/helper.info").exists()
              and sum("Other.INFO" in l for l in lines) == 1 and not any("Other.info" in l for l in lines),
              "a text, a program further in, and a program that has its icon get none added")
        check((tp / "Files/Other.INFO").read_bytes().endswith(b"S3D:tool") and (tp / "Files/Sub.info").read_bytes().endswith(b"S3D:drawer")
              and row["icons"] == 3, "every icon in the drawer is one style, and the row counts them")
        rows = ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: [], face=None)
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
        check('when EmulationDrive copy "Files/libs/two.library" "Emulation:Two-App/libs/two.library"' in two
              and two.startswith("part AppTwoApp 0.3\n") and "WorkDrive" not in two
              and 'when EmulationDrive add "Drawer.info" "Emulation:Two-App.info"' in two,
              "an emulator (Emulation:<Drawer> in the lock) goes on the Emulation drive; an archive with no top drawer is taken as it is")
        emu = (top / "Parts/EmulationDrawers/Part").read_text().splitlines()
        check(emu[0] == "part EmulationDrawers 1.0" and all(l.startswith("when EmulationDrive ") for l in emu[2:])
              and [l.split('"')[3] for l in emu[2:]] == [f"Emulation:{n}{s}" for n in ("Roms", "Disks", "HardDrives", "Optical Media")
                                                        for s in ("/ReadMe", ".info")]
              and sum(l.startswith("when EmulationDrive default ") for l in emu) == 4,
              "Emulation: gets Roms, Disks, HardDrives and Optical Media, each with a ReadMe (kept if there) and its folder icon")
        check(sorted(p.name for p in (top / "Parts/EmulationDrawers").iterdir()) ==
              sorted(["Part"] + [f"{k}{n}{e}" for n in range(4) for k, e in (("ReadMe", ""), ("Drawer", ".info"))])
              and (top / "Parts/EmulationDrawers/Drawer0.info").read_bytes() == b"ICON:chip",
              "the Emulation drawers ship a ReadMe and an icon each and nothing else: no ROM or disk image")
        check(all(m in marks.MOTIFS for m in ap.EMULATION_DRAWERS.values()), "the Emulation drawers' marks are drawn")
        check([r["installer"] for r in rows] == ["Install", ""] and rows[0]["files"] == 5 and rows[0]["bytes"] == 36,
              "the list names an app's own installer (a script, not a text about installing), its files and bytes")
        script = (top / "Install-GFOpenApps").read_text()
        check('C:GFOpenUpTool INSTALL "Work:GFOpenApps/Parts/AppOne"' in script
              and script.index("EmulationDrawers") < script.index("AppOne") < script.index("AppTwoApp") and "FailAt 21" in script,
              "the install script installs each part with GFOpenUpTool, in the lock's order")
        listing = (top / "APPS-LIST.txt").read_text().splitlines()
        check(len(listing) == 3 and rows[0]["sha256"] in listing[1] and "\tInstall\t" in listing[1], "APPS-LIST.txt has a row an app")
        check(not (t / "out" / "unpacked" / "one").exists(), "the unpacked copies are cleared away")

        # GF-103: a release that carries Install-Libraries gets its libraries into LIBS: through a part of their own
        check(all(r["libpart"] == "" and r["libs"] == 0 for r in rows) and not (top / "Parts/AppTwoAppLibs").exists()
              and "LIBS:" not in two, "a release with no Install-Libraries gets no libraries part, whatever library drawer it has")
        suite = t / "suite"
        (suite / "library").mkdir(parents=True)
        (suite / "Write").write_bytes(ap.HUNK_HEADER + b"code")
        for lib in ("gfopenlayout", "sqlite", "gfopendraw", "owfspell", "private"):
            (suite / "library" / f"{lib}.library").write_bytes(lib.encode())
        (suite / "Install-Libraries").write_text(
            "; Install-Libraries: copies the four libraries into LIBS:\nFailAt 21\n"
            "If NOT EXISTS library/gfopendraw.library\n    Quit 10\nEndIf\n"
            "Copy library/gfopenlayout.library LIBS: CLONE\nIf WARN\n    Echo \"gfopenlayout.library was not copied.\"\nEndIf\n"
            "Copy library/sqlite.library LIBS: CLONE\ncopy Library/GFOpenDraw.library LIBS: CLONE\n"
            'Copy "library/owfspell.library" "LIBS:"\nCopy library/private.library RAM: CLONE\n; Copy library/private.library LIBS:\n')
        office = dict(repo="gfoffice", tag="gfoffice-2.0.25-rc93", asset="a", drawer="GFOffice", mark="pages", title="", home="Work")
        check(ap.system_libraries(suite) == [f"library/{n}.library" for n in ("gfopenlayout", "sqlite", "gfopendraw", "owfspell")],
              "Install-Libraries' own Copy lines to LIBS: say which libraries go there, in its order, whatever their case; "
              "a copy elsewhere and a comment say nothing")
        row = ap.app_part(t / "sp", office, suite, icon)
        lp = t / "sp" / "Parts" / "AppGFOfficeLibs"
        ll = (lp / "Part").read_text().splitlines()
        check(row["libpart"] == "AppGFOfficeLibs" and row["libs"] == 4 and ll[0] == "part AppGFOfficeLibs 2.0.25"
              and ll[2:] == [f'when WorkDrive copy "Files/{n}.library" "LIBS:{n}.library"' for n in ("gfopenlayout", "sqlite", "gfopendraw", "owfspell")],
              "the libraries part copies each named library to LIBS:, where the application itself goes in")
        check(sorted(f.name for f in (lp / "Files").iterdir()) == sorted(f"{n}.library" for n in ("gfopenlayout", "sqlite", "gfopendraw", "owfspell"))
              and (lp / "Files/sqlite.library").read_bytes() == b"sqlite",
              "the part holds those libraries and no other")
        ol = (t / "sp" / "Parts" / "AppGFOffice" / "Part").read_text()
        check("LIBS:" not in ol and 'when WorkDrive copy "Files/library/sqlite.library" "Work:GFOffice/library/sqlite.library"' in ol,
              "the application's own part is as before: its drawer keeps the libraries, and it has no LIBS: line")
        emu_app = dict(office, drawer="Thing", home="Emulation")
        ap.app_part(t / "se", emu_app, suite, icon)
        check(all(l.startswith("when EmulationDrive copy ") for l in (t / "se/Parts/AppThingLibs/Part").read_text().splitlines()[2:]),
              "an Emulation drive application's libraries go in where that drive is")
        slock = t / "suite.lock"
        slock.write_text("first v1.0 First.zip First globe\ngfoffice gfoffice-2.0.25-rc93 Suite.zip GFOffice pages\n")
        zipped(t / "sassets" / "first" / "First.zip", {"First/First": b"program"})
        zipped(t / "sassets" / "gfoffice" / "Suite.zip", {"GFOffice/" + f.relative_to(suite).as_posix(): f.read_bytes()
                                                           for f in suite.rglob("*") if f.is_file()})
        srows = ap.build(slock, t / "sassets", t / "sout", icon=icon, check_private=lambda top: [], face=None)
        sscript = (t / "sout/GFOpenApps/Install-GFOpenApps").read_text()
        check(sscript.index('Parts/AppFirst"') < sscript.index('Parts/AppGFOfficeLibs"') < sscript.index('Parts/AppGFOffice"')
              and sscript.count("GFOpenUpTool INSTALL") == 4,
              "the install script installs an application's libraries just before the application")
        slist = (t / "sout/GFOpenApps/APPS-LIST.txt").read_text().splitlines()
        check(slist[0].endswith("\tlibraries to LIBS:") and slist[1].endswith("\t0") and slist[2].endswith("\t4")
              and [r["libs"] for r in srows] == [0, 4], "APPS-LIST.txt says how many libraries each app puts in LIBS:")
        (suite / "Install-Libraries").write_text("Copy library/missing.library LIBS: CLONE\n")
        refused("an Install-Libraries that names a library the release doesn't hold stops the build", lambda: ap.system_libraries(suite))
        (suite / "Install-Libraries").write_text("Echo nothing\n")
        refused("an Install-Libraries that copies nothing to LIBS: stops the build", lambda: ap.system_libraries(suite))
        (suite / "other").mkdir()
        (suite / "other" / "sqlite.library").write_bytes(b"twin")
        (suite / "Install-Libraries").write_text("Copy library/sqlite.library LIBS:\nCopy other/sqlite.library LIBS:\n")
        refused("two libraries of one name for LIBS: stop the build", lambda: ap.system_libraries(suite))

        refused("a file held back from release stops the build",
                lambda: ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: ["Parts/AppOne/Files/One: held"], face=None))
        (assets / "one" / "One-1.2.zip.sha256").write_text("0" * 64 + "\n")
        refused("a file that is not the one fetched stops the build",
                lambda: ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: [], face=None))
        (assets / "two" / "Two.tar.gz").unlink()
        zipped(assets / "one" / "One-1.2.zip", {"One/One": b"x"})
        refused("an app that isn't fetched stops the build",
                lambda: ap.build(lock, assets, t / "out", icon=icon, check_private=lambda top: [], face=None))

        long = t / "long"
        (long / "a").mkdir(parents=True)
        (long / "a" / ("n" * 108)).write_bytes(b"x")
        refused("a name longer than AmigaDOS takes stops the build",
                lambda: ap.app_part(t / "p", dict(repo="r", tag="v1.0", asset="a", drawer="D", mark="globe", title="", home="Work"), long, icon))
        refused("a release with no files stops the build",
                lambda: ap.app_part(t / "p", dict(repo="r", tag="v1.0", asset="a", drawer="D", mark="globe", title="", home="Work"), (t / "empty").mkdir() or t / "empty", icon))
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
        real = ap.tidy_icon(ap.soft3d_face("tool", None), ap.soft3d_face)
        check(ap.icon_kind(real) == "tool" and real[58:66] == ap.NO_PLACE and ap.icon_kind(ap.soft3d_face("project", None)) == "project",
              "the real Soft 3D tool and project icons are made, and read back as their kind")
        refused("a mark that isn't drawn is refused", lambda: ap.soft3d_icon("nothing"))
    print(f"test_apps_pack: {checks} checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
