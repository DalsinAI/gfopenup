#!/usr/bin/env python3
"""Host checks of tools/build_pack.py's parts for the no-ROM-volume design (24 October 2026, sections 8 and 10):
OpenPrefs in SYS:Preferences, the old homes archived, the boot's APPLY line and OpenMonitor by the ROM, and
the Installer and OpenUpMenu with no old names left.

    python3 tests/test_build_pack.py

The Part lines are read through OpenUpTool's own condition code (src/openup_lines.h, built here from
tests/test_openup_lines.c with the host's cc) for each kind of machine: OS 3.2.3's ROM, an open ROM with
prefs.resource 47.100, and one with 47.101 (the ROM that applies the boot's settings). With an openamigaprefs
checkout beside this one (or OPENAMIGAPREFS=dir), each page's SECTION word is checked against OpenPrefs's.
No pack is built and no stove is needed.

MIT, Copyright (c) 2026 AmigaChrome Team."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_pack as bp  # noqa: E402

# The pages by the user's plain names (the design's section 8, Q9), in OpenPrefs's order
PAGES = ["Theme", "Screen Mode", "Pointer", "Fonts", "Colours", "Backdrops", "Workbench", "File Requesters", "Windows",
         "Title Bar", "Menus", "IControl", "Dock", "Sound", "Input", "Gamepads", "Date & Time", "Serial", "File Types",
         "System", "Blanker"]
# The stubs archived: the 13 old ones and the 9 OpenUp added on 21 October (section 10)
OLD13 = ["Look", "OpenLook", "Windows", "TitleBar", "Menus", "OpenDock", "OpenSound", "OpenInput", "OpenGamepads",
         "OpenDateTime", "OpenSerial", "OpenTypes", "OpenSystem"]
NEW9 = ["OpenScreenMode", "OpenPointer", "OpenFonts", "OpenColours", "OpenBackdrops", "OpenWorkbench",
        "OpenFileRequesters", "OpenBlanker", "OpenIControl"]

fails = 0


def check(what: str, ok: bool, more: str = "") -> None:
    global fails
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else f": {more}"))
    fails += not ok


class FakeIcons:
    """openamigaprint's icon writer as far as the parts call it: each icon's bytes say what it was asked for."""
    WBDISK, WBDRAWER, WBTOOL, WBPROJECT = 1, 2, 3, 4

    def icon(self, kind, art, default_tool=None, tool_types=None, stack=4096, window=(400, 200)):
        return repr((kind, art, default_tool, tool_types or [], stack)).encode()


def icon_of(data: bytes):
    return eval(data.decode())                 # our own repr, above


def lines_on(part_file: Path, machine: str, filt: Path) -> list[str]:
    """The Part file's lines as OpenUpTool's INSTALL acts on them on that machine."""
    out = subprocess.run([str(filt), str(part_file), machine], check=True, capture_output=True, text=True).stdout
    return out.splitlines()


def main() -> int:
    cc = shutil.which("cc") or shutil.which("gcc")
    if not cc:
        print("FAIL no host C compiler (cc) for tests/test_openup_lines.c")
        return 1
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        filt = tmp / "test_openup_lines"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}", str(ROOT / "tests/test_openup_lines.c"),
                        "-o", str(filt)], check=True)
        r = subprocess.run([str(filt)], capture_output=True, text=True)
        check("OpenUpTool's conditions (tests/test_openup_lines.c)", r.returncode == 0, r.stdout[-400:])

        icons = FakeIcons()
        top = tmp / "OpenUp"
        opp = bp.openprefs_part(top, b"OpenPrefs body", icons, "2.3")
        opp.write()
        pf = opp.dir / "Part"
        text = pf.read_text(encoding="latin-1")
        lines = text.splitlines()
        check("the part is OpenPrefs at its program's version", lines[0] == "part OpenPrefs 2.3", lines[0])

        # SYS:Preferences: the drawer, the program and its icon, an icon per page
        check("SYS:Preferences.info, a drawer icon OpenUp keeps up to date",
              'add "Preferences.info" "SYS:Preferences.info"' in lines
              and icon_of((opp.dir / "Preferences.info").read_bytes())[0] == FakeIcons.WBDRAWER)
        check("SYS:Preferences/OpenPrefs", 'copy "Preferences/OpenPrefs" "SYS:Preferences/OpenPrefs"' in lines
              and (opp.dir / "Preferences/OpenPrefs").read_bytes() == b"OpenPrefs body")
        tool = icon_of((opp.dir / "Preferences/OpenPrefs.info").read_bytes())
        check("SYS:Preferences/OpenPrefs.info, a tool icon with 32 KB of stack",
              'copy "Preferences/OpenPrefs.info" "SYS:Preferences/OpenPrefs.info"' in lines
              and tool[0] == FakeIcons.WBTOOL and tool[4] == 32768, str(tool))
        pages = [m.group(1) for m in re.finditer(r'^copy "Preferences/([^"/]+)\.info" "SYS:Preferences/\1\.info"$', text, re.M)
                 if m.group(1) != "OpenPrefs"]
        check("a project icon per page, by the plain names, in OpenPrefs's order", pages == PAGES, str(pages))
        words = {}
        for page in PAGES:
            f = opp.dir / f"Preferences/{page}.info"
            kind, _, deftool, tts, stack = icon_of(f.read_bytes()) if f.is_file() else (0, "", "", [], 0)
            ok = kind == FakeIcons.WBPROJECT and deftool == "SYS:Preferences/OpenPrefs" and len(tts) == 1 \
                and tts[0].startswith("SECTION=") and stack == 32768
            words[page] = tts[0][8:] if ok else ""
            if not ok:
                check(f"{page}.info: a project of SYS:Preferences/OpenPrefs with SECTION=", False, str((kind, deftool, tts, stack)))
        check("each page icon: a project of SYS:Preferences/OpenPrefs, SECTION=<its word>, 32 KB of stack",
              all(words.values()))
        check("no page has a file of its own (Q8: icons, no stubs)",
              not [p for p in PAGES if (opp.dir / f"Preferences/{p}").exists()])
        check("nothing goes into SYS:Prefs or C: any more",
              not re.search(r'^(copy|add|default) "[^"]*" "(SYS:Prefs/|SYS:C/|C:)', text, re.M))

        # the old homes, archived
        for n in OLD13 + NEW9:
            if f'retire "SYS:Prefs/{n}"' not in lines or f'retire "SYS:Prefs/{n}.info"' not in lines:
                check(f"SYS:Prefs/{n} and its icon archived", False, "no retire line")
        check(f"the {len(OLD13)} old stubs and the {len(NEW9)} of 21 October archived, with their icons",
              all(f'retire "SYS:Prefs/{n}"' in lines and f'retire "SYS:Prefs/{n}.info"' in lines for n in OLD13 + NEW9))
        check("SYS:Prefs/OpenPrefs and its icon archived",
              'retire "SYS:Prefs/OpenPrefs"' in lines and 'retire "SYS:Prefs/OpenPrefs.info"' in lines)
        check("C:OpenPrefs archived on every ROM (it looks for the program in SYS:Prefs and ROM:Prefs only)",
              'retire "SYS:C/OpenPrefs"' in lines)
        check("the stubs' list is the 22 names", list(bp.OPENPREFS_NAMES) == OLD13 + NEW9, str(bp.OPENPREFS_NAMES))

        # the lines by ROM, through OpenUpTool's own reading
        for machine, apply_line in (("os323", True), ("os323-ac", True), ("openkick100-ac", True), ("openkick101-ac", False),
                                    ("openkick101", False)):
            eff = lines_on(pf, machine, filt)
            check(f"{machine}: SYS:Preferences on the Shell's path", "startup Path >NIL: SYS:Preferences ADD" in eff)
            check(f"{machine}: the boot's APPLY {'by its full path' if apply_line else 'left to the ROM'}",
                  ("startup SYS:Preferences/OpenPrefs APPLY" in eff) == apply_line
                  and not any(l.startswith("startup") and "APPLY" in l and "SYS:Preferences/" not in l for l in eff),
                  str([l for l in eff if l.startswith("startup")]))
            check(f"{machine}: the archived files are the same on every ROM",
                  sum(l.startswith("retire") for l in eff) == 2 * len(OLD13 + NEW9) + 3)
        rm = lines_on(pf, "remove", filt)
        check("REMOVE reads the APPLY line with its condition off", "startup SYS:Preferences/OpenPrefs APPLY" in rm)

        # OpenMonitor and CommandSwap by the ROM
        core = bp.Part(top, "OpenUp", "0.9", "test")
        bp.openup_rom_lines(core, b"OpenMonitor body", icons.icon(icons.WBTOOL, "install", stack=32768))
        core.write()
        cf = core.dir / "Part"
        for machine in ("os323", "openkick100-ac"):
            eff = lines_on(cf, machine, filt)
            check(f"{machine}: OpenMonitor with its icon, nothing archived",
                  'copy "Utilities/OpenMonitor" "SYS:Utilities/OpenMonitor"' in eff
                  and 'copy "Utilities/OpenMonitor.info" "SYS:Utilities/OpenMonitor.info"' in eff
                  and not any(l.startswith("retire") for l in eff), str(eff))
        eff = lines_on(cf, "openkick101-ac", filt)
        check("openkick101: OpenMonitor with its icon (the disk's task manager is newer than the ROM's 0.3.3 until "
              "1.0 goes into the ROM), CommandSwap's pins archived",
              eff[2:] == ['copy "Utilities/OpenMonitor" "SYS:Utilities/OpenMonitor"',
                          'copy "Utilities/OpenMonitor.info" "SYS:Utilities/OpenMonitor.info"',
                          'retire "ENVARC:Sys/CommandSwap.prefs"'], str(eff))
        mon = icon_of((core.dir / "Utilities/OpenMonitor.info").read_bytes())
        check("OpenMonitor.info: a tool icon with 32 KB of stack (OpenBench runs the ROM's resident from it)",
              mon[0] == FakeIcons.WBTOOL and mon[4] == 32768)

        # Backdrops: OpenUp's own desktops, each with its .tone; the AmigaChrome Background Pack's art (which
        # opengadtools' make_backdrops.py also exports) is an optional install, not OpenUp's (24 October 2026)
        bdir = tmp / "backdrops"
        bdir.mkdir()
        ogt = tmp / "opengadtools"
        (ogt / "tools").mkdir(parents=True)
        (ogt / "tools/make_art_backdrops.py").write_text(
            "from PIL import Image\nSIZES = [(640, 480), (1920, 1200)]\n"
            "ART = [('Prismatic-light', 'light'), ('Nebula-dark', 'dark')]\n", encoding="utf-8")
        own = ["Clear-light-chrome-logo-1920x1080.png", "Open-dark-1920x1200.png", "Open-light-strip-1080.png"]
        art = [f"AmigaChrome-{d}-{size}.png" for d in ("Prismatic-light", "Nebula-dark") for size in ("640x480", "1920x1200")]
        for n in own + art:
            (bdir / n).write_bytes(n.encode())
        check("make_art_backdrops.py's names read from its source, without PIL", bp.art_backdrop_names(ogt) == set(art),
              str(bp.art_backdrop_names(ogt)))
        fake_tone = lambda png: "dark" if "-dark-" in png.name else "light"
        bd = bp.backdrops_part(top, bdir, ogt, tone=fake_tone)
        bd.write()
        lines = (bd.dir / "Part").read_text(encoding="latin-1").splitlines()
        check("the Backdrops part is 0.2 (its tones are new)", lines[0] == "part Backdrops 0.2", lines[0])
        want = []
        for n in own:
            want += [f'copy "Backdrops/{n}" "SYS:Prefs/Presets/Backdrops/OpenUp/{n}"',
                     f'copy "Backdrops/{n}.tone" "SYS:Prefs/Presets/Backdrops/OpenUp/{n}.tone"']
        check("each of OpenUp's own pictures goes in with its .tone, in name order", lines[2:] == want, str(lines[2:]))
        check("each .tone says the picture's tone",
              all((bd.dir / f"Backdrops/{n}.tone").read_bytes() == (fake_tone(Path(n)) + "\n").encode() for n in own))
        check("none of the Background Pack's art is in the part",
              not [p for p in (bd.dir / "Backdrops").iterdir() if p.name.startswith("AmigaChrome-")]
              and not [l for l in lines if "AmigaChrome-" in l])
        (ogt / "tools/make_art_backdrops.py").rename(ogt / "tools/gone.py")
        bd = bp.backdrops_part(top, bdir, ogt, tone=fake_tone)
        check("without make_art_backdrops.py, the art is still left out by its AmigaChrome-<design>-<W>x<H>.png names",
              sorted(p.name for p in (bd.dir / "Backdrops").iterdir() if p.suffix == ".png") == own)
        try:
            from PIL import Image
        except ImportError:
            Image = None
        if Image is None:
            print("skipped picture_tone on real pictures: no PIL")
        else:
            # OpenUpTool's rule: the mean Rec. 709 luma of twelve rows, every eighth pixel; under 115 is dark
            for grey, tone in ((114, "dark"), (115, "light"), (30, "dark"), (220, "light")):
                f = tmp / f"grey{grey}.png"
                Image.new("RGB", (1920, 1200), (grey, grey, grey)).save(f)
                check(f"picture_tone: a grey of {grey} is {tone}", bp.picture_tone(f) == tone, bp.picture_tone(f))
            f = tmp / "halves.png"
            im = Image.new("RGB", (64, 48), (0, 0, 0))
            im.paste((255, 255, 255), (0, 0, 64, 20))     # 5 of the 12 rows sampled are white: a mean of 106, dark
            im.save(f)
            check("picture_tone samples twelve rows, top to bottom", bp.picture_tone(f) == "dark", bp.picture_tone(f))

    # OpenTypes and OpenDateTime: pages of OpenPrefs, parts with no files that archive their stubs
    src = (ROOT / "tools/build_pack.py").read_text(encoding="utf-8")
    for name, stub in (("OpenTypes", "OpenTypes"), ("OpenDateTime", "OpenDateTime")):
        check(f"the {name} part installs no stub, and archives SYS:Prefs/{stub}",
              f'"Prefs/{stub}", ' not in src and f"retire \"SYS:Prefs/{stub}\"" in src
              and name in bp.PART_OWN_VERSION and name not in bp.PART_MAIN)
    check("OpenPrefs's part version is read from SYS:Preferences/OpenPrefs",
          bp.PART_MAIN["OpenPrefs"] == ("Preferences/OpenPrefs", "OpenPrefs"))

    # the SECTION words are OpenPrefs's (openamigaprefs OpenPrefs/src/openprefs.c)
    oap = Path(os.environ.get("OPENAMIGAPREFS", ROOT.parent / "openamigaprefs"))
    op = oap / "OpenPrefs/src/openprefs.c"
    if op.is_file():
        known = set(re.findall(r'\{ "([A-Z]+)", LT_[A-Z]+ \}', op.read_text(encoding="latin-1")))
        bad = [(p, w) for p, w, _ in bp.PREFERENCES_PAGES if w not in known]
        check(f"every page's SECTION word is one OpenPrefs knows ({op})", not bad, str(bad))
    else:
        print(f"skipped the SECTION words: no {op} (OPENAMIGAPREFS=)")
    check("the pages' pictures are in icons/open",
          all((ROOT / "icons/open" / f"{art}.png").is_file() for _, _, art in bp.PREFERENCES_PAGES))

    # the Installer, the uninstaller and OpenUpMenu name none of the old stubs to run
    stubs = "|".join(OLD13 + NEW9 + ["OpenPrefs"])
    for f in ("pack/Install-OpenUp", "pack/Uninstall-OpenUp"):
        t = (ROOT / f).read_text(encoding="latin-1")
        guarded = re.findall(rf'\(run "SYS:Prefs/({stubs})\b', t)
        # an old stub is run only as the fallback where SYS:Preferences/OpenPrefs isn't there (Uninstall's OpenTypes)
        check(f"{f}: runs OpenPrefs from SYS:Preferences, an old name only as a fallback",
              len(guarded) <= (1 if "Uninstall" in f else 0), str(guarded))
    menu = (ROOT / "src/openupmenu.c").read_text(encoding="latin-1")
    check("OpenUpMenu: no entry runs an old stub", not re.search(rf'"SYS:Prefs/({stubs})"', menu))
    entries = re.findall(r'HOW_PAGE, "([^"]+)", +"SYS:Preferences/([^"]+)", +"([A-Z]+)"', menu)
    pagewords = {p: w for p, w, _ in bp.PREFERENCES_PAGES}
    check("OpenUpMenu's page entries: each a page icon by its plain name, with its page's SECTION word",
          entries and all(label == page and pagewords.get(page) == word for label, page, word in entries), str(entries))

    print(f"{'all passed' if not fails else str(fails) + ' FAILED'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
