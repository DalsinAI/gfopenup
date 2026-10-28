#!/usr/bin/env python3
"""Host checks of tools/build_pack.py's parts for the no-ROM-volume design (24 October 2026, sections 8 and 10):
OpenPrefs in SYS:Preferences, the old homes archived, the boot's APPLY line and OpenMonitor by the ROM, and
the Installer and OpenUpMenu with no old names left. And OpenSnap's part and pictures (tests/test_opensnap_pic.c),
and the GFOffice Suite's parts: GFOffice, the shared files, which OpenWrite and OpenCalc need ("needs part"), with
INSTALL and REMOVE in both orders read through OpenUpTool's own code (gfoffice_checks).

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
PAGES = ["Theme", "Screen Mode", "Pointer", "Fonts", "Colours", "Backdrops", "Workbench", "Icons", "File Requesters",
         "Windows", "Title Bar", "Menus", "IControl", "Dock", "Sound", "Input", "Gamepads", "Date & Time", "Serial",
         "Printing", "File Types", "System", "Blanker"]   # OpenPrefs 2.4's list order (openamigaprefs opframe.c lt_order)
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


# What Instance-32 had of OS 3.2.3's printing on 24 October 2026 (read from its disk: 61 files, 388,377 bytes),
# each of which the OpenPrinter part archives
INSTANCE32_PRINTING = (
    [f"SYS:Devs/Printers/{d}{i}" for d in bp.LEGACY_PRINTER_DRIVERS for i in ("", ".info")]
    + ["SYS:Devs/postscript_init.ps"]
    + [f"SYS:Prefs/Classic/{e}{i}" for e in ("Printer", "PrinterGfx", "PrinterPS") for i in ("", ".info")]
    + [f"SYS:Tools/{t}{i}" for t in ("CMD", "GraphicDump", "InitPrinter", "PrintFiles") for i in ("", ".info")]
    + [f"SYS:Locale/Help/{lang}/Sys/{h}" for lang in ("english", "english_british")
       for h in ("Prefs/printer.help", "Prefs/printergfx.help", "Prefs/printerps.help", "Tools/printfiles.help",
                 "Tools/graphicdump.help", "Tools/cmd.help", "Tools/initprinter.help")]
    + ["SYS:Locale/Catalogs/english_british/Sys/hp_printer.catalog", "SYS:Storage/Printers.info"])


def iff(data: bytes) -> dict:
    """A FORM PREF's chunks, read as printer.device reads them (FORM, its size, PREF, then id, size, body, pad)."""
    out = {}
    if data[:4] != b"FORM" or data[8:12] != b"PREF" or int.from_bytes(data[4:8], "big") != len(data) - 8:
        return out
    i = 12
    while i + 8 <= len(data):
        n = int.from_bytes(data[i + 4:i + 8], "big")
        out[data[i:i + 4].decode()] = data[i + 8:i + 8 + n]
        i += 8 + n + (n & 1)
    return out


def openprinter_checks() -> None:
    """The OpenPrinter part (retiring the legacy printing, 24 October 2026)."""
    with tempfile.TemporaryDirectory() as tmp:
        dev = b"\0" * 64 + b"printer.device 47.100 (13.10.2026) OpenPrinter, AmigaChrome Team\r\n\0" \
            + b"\0$VER: printer.device 47.100 (13.10.2026) OpenPrinter, AmigaChrome Team\0"
        op = bp.openprinter_part(Path(tmp), dev)
        op.write()
        lines = (op.dir / "Part").read_text(encoding="latin-1").splitlines()
        check("OpenPrinter: the part's version is its printer.device's", lines[0] == "part OpenPrinter 47.100", lines[0])
        check("OpenPrinter: AmigaChrome instances only (the user's decision)", "needs AmigaChrome" in lines)
        keep = [l for l in lines if l.startswith("keep ")]
        check("OpenPrinter: refused where DEVS:printer.device is neither OS 3.2's nor ours",
              keep == ['keep "SYS:Devs/printer.device" "printer.device 47." "OpenPrinter, AmigaChrome Team"'], str(keep))
        check("OpenPrinter: the OS 3.2.3 printer.device's id and ours each carry a marker",
              any(m in "printer.device 47.3 (23.5.2020)" for m in bp.OPENPRINTER_MARKERS)
              and any(m.encode() in dev for m in bp.OPENPRINTER_MARKERS))
        check("OpenPrinter: our printer.device in OS 3.2.3's place (the original kept by copy's rule)",
              'copy "Devs/printer.device" "SYS:Devs/printer.device"' in lines
              and (op.dir / "Devs/printer.device").read_bytes() == dev)
        for name in ("printer.prefs", "printergfx.prefs"):
            check(f"OpenPrinter: {name} only where there is none, in ENVARC:Sys and ENV:Sys",
                  f'default "Prefs/{name}" "ENVARC:Sys/{name}"' in lines and f'default "Prefs/{name}" "ENV:Sys/{name}"' in lines)
        txt = iff((op.dir / "Prefs/printer.prefs").read_bytes())
        gfx = iff((op.dir / "Prefs/printergfx.prefs").read_bytes())
        check("printer.prefs: PRHD, PTXT, PUNT and PDEV at the NDK 3.2 sizes",
              {k: len(v) for k, v in txt.items()} == {"PRHD": 6, "PTXT": 64, "PUNT": 56, "PDEV": 52}, str(txt.keys()))
        check("printergfx.prefs: PRHD and PGFX at the NDK 3.2 sizes",
              {k: len(v) for k, v in gfx.items()} == {"PRHD": 6, "PGFX": 38}, str(gfx.keys()))
        if "PTXT" in txt and "PUNT" in txt and "PGFX" in gfx:
            t, u, g = txt["PTXT"], txt["PUNT"], gfx["PGFX"]
            fields = [int.from_bytes(t[48 + 2 * i:50 + 2 * i], "big") for i in range(8)]
            check("PTXT: driver OpenPrint, port 0, single sheets, A4 (9), 66 lines, pica, 6 lpi, margins 5-75",
                  t[16:46].rstrip(b"\0") == b"OpenPrint" and t[46] == 0 and fields == [1, 9, 66, 0, 0, 5, 75, 1],
                  str((t[16:46], fields)))
            check("PUNT: oapspool (oapspool.device, as the editor writes it), unit 0",
                  int.from_bytes(u[16:20], "big") == 0 and u[24:56].rstrip(b"\0") == b"oapspool")
            check("PGFX: horizontal, colour, threshold 7, density 2",
                  [int.from_bytes(g[16 + 2 * i:18 + 2 * i], "big") for i in range(4)] == [0, 2, 0, 7] and g[30] == 2)
        retired = [l[8:-1] for l in lines if l.startswith('retire "')]
        check("OpenPrinter: every printing file Instance-32 has is archived (61)",
              not [p for p in INSTANCE32_PRINTING if p not in retired] and len(INSTANCE32_PRINTING) == 61,
              str([p for p in INSTANCE32_PRINTING if p not in retired]))
        check("OpenPrinter: the drivers where the real Installer keeps them, the editors at SYS:Prefs, 17 languages",
              "SYS:Storage/Printers/EpsonQ" in retired and "SYS:Prefs/Printer" in retired
              and "SYS:Locale/Help/deutsch/Sys/Prefs/printer.help" in retired
              and "SYS:Locale/Catalogs/fran\xe7ais/Sys/hp_printer.catalog" in retired and len(retired) == 218, str(len(retired)))
        stays = ("SYS:Devs/parallel.device", "SYS:Devs/serial.device", "SYS:Devs/Printers/OpenPrint", "SYS:Devs/Printers",
                 "SYS:Devs/Printers.info", "SYS:L/Port-Handler", "SYS:Devs/oapspool.device")
        check("OpenPrinter: the ports, PRT:'s handler, the Printers drawer and OpenPrint's own pieces stay",
              not [p for p in stays if p in retired], str([p for p in stays if p in retired]))
        check("OpenPrinter: nothing deleted, only archived (no delete line)", not [l for l in lines if l.startswith("delete")])
    check("OpenPrinter's version is read from its printer.device",
          bp.PART_MAIN.get("OpenPrinter") == ("Devs/printer.device", "printer.device"))
    inst = (ROOT / "pack/Install-OpenUp").read_text(encoding="latin-1")
    a, b = inst.find("INSTALL Parts/OpenPrint >>"), inst.find("INSTALL Parts/OpenPrinter >>")
    check("Install-OpenUp: OpenPrinter right after OpenPrint, under the same choice", 0 < a < b
          and inst.rfind("(if (bitand #parts 2)", 0, a) > inst.rfind("(if (bitand #parts", 0, a) - 1
          and inst.find("(if (bitand #parts", a) > b)


def manifest_run(filt: Path, how: str, part_file: Path, manifest: str, tmp: Path, *more: str) -> tuple[int, str]:
    """OpenUpTool's "needs part" check (tests/test_openup_lines.c install|remove) of a Part file against a manifest's
    text: its return code (0 done, 5 refused, 10 failed: with more "fail") and what it printed (the manifest after,
    or why it refused)."""
    m = tmp / "OpenUp.manifest"
    m.write_text(manifest, encoding="latin-1")
    r = subprocess.run([str(filt), how, str(part_file), str(m), *more], capture_output=True, text=True, encoding="latin-1")
    return r.returncode, r.stdout


def gfoffice_checks(tmp: Path, filt: Path) -> None:
    """The GFOffice Suite's parts (gfoffice docs/gfoffice/Design-Suite-Shared.md, section 12, 25 October 2026): the
    GFOffice part's shared files (the suite's four libraries since gfoffice docs/suite/owf-libraries.md), OpenWrite
    and OpenCalc needing it, and INSTALL and REMOVE in both orders through OpenUpTool's own code
    (src/openup_lines.h)."""
    src = tmp / "gfoffice"
    onlib = src / bp.GFO_LIBRARY
    files = {
        onlib / f"library/{name}.library": f"\0{name}.library 3.0 (25.10.2026) libowf 1.4.0, AmigaChrome Team\r\n\0"
                                           f"$VER: {name}.library 3.0 (25.10.2026) MIT\0".encode() + name.encode()
        for name in bp.GFO_LIBS
    }
    files.update({
        onlib / "OpenWrite": b"OpenWrite on owfwrite.library\0$VER: OpenWrite 2.0.23 (25.10.2026)\0",
        onlib / "OWConvert": b"OWConvert on the four libraries\0",
        onlib / "OpenCalc": b"\0$VER: OpenCalc 0.1 (25.10.2026) OpenCalc, the spreadsheet of the GFOffice Suite\0",
        src / "Dictionaries/en_GB.words": b"aardvark\nabacus\n",
        src / "LICENSE": b"MIT License\n\nCopyright (c) 2026 AmigaChrome Team\n",
        src / "THIRD_PARTY.md": b"# Third-party parts\n",
        src / "RELEASE_NOTES_1.0.md": b"1.0\n", src / "RELEASE_NOTES_1.0.2.md": b"1.0.2\n",
        src / "licenses/SCOWL-ASPELL-English.txt": b"SCOWL notice\n",
    })
    for f, data in files.items():
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_bytes(data)
    zlib = b"\0$VER: z.library 53.12 (13.10.2026) OpenZlib, AmigaChrome Team\0"
    icons = FakeIcons()
    top = tmp / "GF/OpenUp"
    gf, wr, oc = bp.gfoffice_parts(top, src, zlib, b"guest MIT licence\n", icons)
    for p in (gf, wr, oc):
        p.write()
    gl = (gf.dir / "Part").read_text(encoding="latin-1").splitlines()
    wl = (wr.dir / "Part").read_text(encoding="latin-1").splitlines()
    cl = (oc.dir / "Part").read_text(encoding="latin-1").splitlines()

    # GFOffice: the shared files, one owner each
    check("GFOffice: the part at owfwrite.library's version", gl[0] == "part GFOffice 3.0", gl[0])
    lic = "Utilities/OpenUp/Licences/"
    want = [f'copy "Libs/{name}.library" "LIBS:{name}.library"' for name in bp.GFO_LIBS] + [
            'copy "Libs/z.library" "LIBS:z.library"',
            'copy "Dictionaries/en_GB.words" "SYS:Dictionaries/en_GB.words"', 'copy "C/OWConvert" "SYS:C/OWConvert"',
            f'copy "{lic}GFOffice.txt" "SYS:{lic}GFOffice.txt"', f'copy "{lic}z.library.txt" "SYS:{lic}z.library.txt"',
            f'copy "{lic}GFOffice-Dictionaries.txt" "SYS:{lic}GFOffice-Dictionaries.txt"',
            'retire "LIBS:owf.library"']
    check("GFOffice: the four libraries, z.library, the dictionary in SYS:Dictionaries, OWConvert and their licences, "
          "owf.library retired, and nothing else (no needs, no startup line)", gl[2:] == want, str(gl[2:]))
    check("GFOffice: the libraries from the OWF=library build, z.library ours, the dictionary gfoffice's",
          all((gf.dir / f"Libs/{name}.library").read_bytes() == files[onlib / f"library/{name}.library"]
              for name in bp.GFO_LIBS)
          and (gf.dir / "Libs/z.library").read_bytes() == zlib
          and (gf.dir / "Dictionaries/en_GB.words").read_bytes() == files[src / "Dictionaries/en_GB.words"])
    check("GFOffice: OWConvert from the same build, on the four libraries",
          (gf.dir / "C/OWConvert").read_bytes() == files[onlib / "OWConvert"])
    check("GFOffice: the licences say what each covers (gfoffice's MIT, OpenZlib's, SCOWL's for the dictionary)",
          (gf.dir / lic / "GFOffice.txt").read_bytes().endswith(files[src / "LICENSE"])
          and b"z.library 53.12 is OpenZlib" in (gf.dir / lic / "z.library.txt").read_bytes()
          and (gf.dir / lic / "z.library.txt").read_bytes().endswith(b"guest MIT licence\n")
          and b"(en_GB.words)" in (gf.dir / lic / "GFOffice-Dictionaries.txt").read_bytes()
          and (gf.dir / lic / "GFOffice-Dictionaries.txt").read_bytes().endswith(b"SCOWL notice\n"))

    # OpenWrite: on owfwrite.library, needing GFOffice, with none of the shared files
    check("OpenWrite: 2.0.23, and it needs the GFOffice part", wl[0] == "part OpenWrite 2.0.23" and "needs part GFOffice" in wl,
          str(wl[:3]))
    check("OpenWrite: the program built on its library (OWF=library)",
          (wr.dir / "Utilities/OpenWrite/OpenWrite").read_bytes() == files[onlib / "OpenWrite"])
    check("OpenWrite: no z.library, OWConvert or dictionary of its own (GFOffice's now)",
          not [l for l in wl if l.startswith(("copy", "default", "add"))
               and ("z.library" in l or "OWConvert" in l or "Dictionaries" in l)], str(wl))
    check("OpenWrite: the dictionary an earlier part put beside it archived, and nothing else of its drawer",
          [l for l in wl if l.startswith("retire")] == ['retire "SYS:Utilities/OpenWrite/Dictionaries/en_GB.words"'])
    check("OpenWrite: its release notes are the newest", (wr.dir / "Utilities/OpenWrite/Release-Notes.txt").read_bytes() == b"1.0.2\n")
    check("OpenWrite: its drawer icon only where none is or OpenUp's",
          'add "Utilities/OpenWrite.info" "SYS:Utilities/OpenWrite.info"' in wl)

    # OpenCalc: the same way
    check("OpenCalc: its program's version, and it needs the GFOffice part",
          cl[0] == "part OpenCalc 0.1" and "needs part GFOffice" in cl, str(cl[:3]))
    check("OpenCalc: in SYS:Utilities/OpenCalc, with its icon, licence and drawer icon",
          'copy "Utilities/OpenCalc/OpenCalc" "SYS:Utilities/OpenCalc/OpenCalc"' in cl
          and 'copy "Utilities/OpenCalc/OpenCalc.info" "SYS:Utilities/OpenCalc/OpenCalc.info"' in cl
          and 'copy "Utilities/OpenCalc/LICENSE" "SYS:Utilities/OpenCalc/LICENSE"' in cl
          and 'add "Utilities/OpenCalc.info" "SYS:Utilities/OpenCalc.info"' in cl
          and (oc.dir / "Utilities/OpenCalc/OpenCalc").read_bytes() == files[onlib / "OpenCalc"], str(cl))
    tool = icon_of((oc.dir / "Utilities/OpenCalc/OpenCalc.info").read_bytes())
    check("OpenCalc.info: a tool icon with 32 KB of stack, as OpenWrite's", tool[0] == FakeIcons.WBTOOL and tool[4] == 32768)
    dests = {}
    for name, lines in (("GFOffice", gl), ("OpenWrite", wl), ("OpenCalc", cl)):
        for l in lines:
            m = re.match(r'(?:copy|default|add) "[^"]*" "([^"]*)"$', l)
            if m:
                dests.setdefault(m.group(1), []).append(name)
    check("no file is in two of the suite's parts (the shared ones have one owner)",
          not [d for d, ps in dests.items() if len(ps) > 1], str({d: ps for d, ps in dests.items() if len(ps) > 1}))
    check("the parts' versions are read from owfwrite.library and OpenCalc",
          bp.PART_MAIN.get("GFOffice") == ("Libs/owfwrite.library", "owfwrite.library")
          and bp.PART_MAIN.get("OpenCalc") == ("Utilities/OpenCalc/OpenCalc", "OpenCalc"))
    try:
        bp.check_part_versions(top / "Parts")
        check("check_part_versions: GFOffice, OpenWrite and OpenCalc match their binaries", True)
    except SystemExit as e:
        check("check_part_versions: GFOffice, OpenWrite and OpenCalc match their binaries", False, str(e))

    # OpenCalc's program not built: the part is left out, the others are made as before
    (onlib / "OpenCalc").unlink()
    top2 = tmp / "GF2/OpenUp"
    gf2, wr2, oc2 = bp.gfoffice_parts(top2, src, zlib, b"guest MIT licence\n", icons)
    check("without OpenCalc's program, no OpenCalc part, and GFOffice and OpenWrite as before",
          oc2 is None and not (top2 / "Parts/OpenCalc").exists() and gf2.lines == gf.lines and wr2.lines == wr.lines)

    # INSTALL and REMOVE through OpenUpTool's own code, from a lab with OpenUp's own part only
    fresh = "OpenUp\t0.1\npart\tOpenUp\t0.8.9\nnew\tSYS:C/OpenUpTool\n"
    gp, wp, cp = gf.dir / "Part", wr.dir / "Part", oc.dir / "Part"
    rc, out = manifest_run(filt, "install", wp, fresh, tmp)
    check("INSTALL OpenWrite alone: left out (5), and it says GFOffice is missing", rc == 5 and out == "missing GFOffice\n",
          f"{rc} {out!r}")
    rc, out = manifest_run(filt, "install", cp, fresh, tmp)
    check("INSTALL OpenCalc alone: left out too", rc == 5 and out == "missing GFOffice\n", f"{rc} {out!r}")
    rc, m1 = manifest_run(filt, "install", gp, fresh, tmp)
    check("INSTALL GFOffice: installed, needing nothing", rc == 0 and "part\tGFOffice\t3.0" in m1.splitlines()
          and OUL_NEEDPART not in m1, f"{rc} {m1!r}")
    rc, m2 = manifest_run(filt, "install", wp, m1, tmp)
    check("INSTALL OpenWrite after GFOffice: installed, its need noted in the manifest",
          rc == 0 and "part\tOpenWrite\t2.0.23" in m2.splitlines() and f"{OUL_NEEDPART}\tOpenWrite\tGFOffice" in m2.splitlines(),
          f"{rc} {m2!r}")
    rc, again = manifest_run(filt, "install", wp, m2, tmp)
    check("INSTALL OpenWrite again (an update): its need noted once", rc == 0 and again.count(f"{OUL_NEEDPART}\tOpenWrite\t") == 1,
          again)
    # 25 Oct 2026: an install with a file that couldn't be put in place isn't listed (Instance-32's calculator at
    # SYS:Utilities/OpenCalc, where OpenCalc's drawer goes, made INSTALL fail and list OpenCalc anyway)
    rc, mf = manifest_run(filt, "install", wp, m1, tmp, "fail")
    check("INSTALL OpenWrite that fails (10): not listed, its need not noted, the rest as it was",
          rc == 10 and mf.splitlines() == m1.splitlines(), f"{rc} {mf!r}")
    older = re.sub(r"(?m)^part\tOpenWrite\t.*$", "part\tOpenWrite\t2.0.0", m2)
    rc, mf = manifest_run(filt, "install", wp, older, tmp, "fail")
    check("an upgrade of OpenWrite that fails: the older version stays listed with its need, so the next one tries again",
          rc == 10 and mf.splitlines() == older.splitlines(), f"{rc} {mf!r}")
    rc, m3 = manifest_run(filt, "install", cp, m2, tmp)
    check("INSTALL OpenCalc after GFOffice: installed", rc == 0 and f"{OUL_NEEDPART}\tOpenCalc\tGFOffice" in m3.splitlines())
    rc, out = manifest_run(filt, "remove", gp, m3, tmp)
    check("REMOVE GFOffice while OpenWrite is installed: refused (5), nothing changed",
          rc == 5 and out == "needed by OpenWrite\n", f"{rc} {out!r}")
    rc, m4 = manifest_run(filt, "remove", wp, m3, tmp)
    check("REMOVE OpenWrite first: taken out, its need with it", rc == 0 and "OpenWrite" not in m4, m4)
    rc, out = manifest_run(filt, "remove", gp, m4, tmp)
    check("REMOVE GFOffice while OpenCalc is still installed: refused", rc == 5 and out == "needed by OpenCalc\n", f"{rc} {out!r}")
    rc, m5 = manifest_run(filt, "remove", cp, m4, tmp)
    rc2, m6 = manifest_run(filt, "remove", gp, m5, tmp)
    check("REMOVE GFOffice once no program needs it: taken out", rc == 0 and rc2 == 0
          and m6.splitlines() == fresh.splitlines(), m6)
    # Instance-32: an OpenWrite from an earlier OpenUp, with libowf linked in and no need noted, and no GFOffice
    old = fresh + "part\tOpenWrite\t2.0.1\nnew\tLIBS:z.library\nnew\tSYS:C/OWConvert\n"
    rc, out = manifest_run(filt, "install", wp, old, tmp)
    check("Instance-32 (an older OpenWrite, no GFOffice): the new OpenWrite waits for GFOffice", rc == 5 and out == "missing GFOffice\n")
    rc, out = manifest_run(filt, "remove", gp, manifest_run(filt, "install", gp, old, tmp)[1], tmp)
    check("Instance-32: REMOVE GFOffice under the older OpenWrite, which doesn't need it, is allowed", rc == 0, out)
    inst = (ROOT / "pack/Install-OpenUp").read_text(encoding="latin-1")
    a, b = inst.find("INSTALL Parts/GFOffice >>"), inst.find("INSTALL Parts/OpenWrite >>")
    check("Install-OpenUp: GFOffice right before OpenWrite, under OpenWrite's choice",
          0 < a < b and inst.rfind("(if (bitand #parts 1048576)", 0, a) > inst.rfind("(if (bitand #parts", 0, a) - 1
          and inst.find("(if (bitand #parts", a) > b and inst.count("INSTALL Parts/GFOffice") == 1)


OUL_NEEDPART = "needpart"     # src/openup_lines.h


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
        check("openkick101: OpenMonitor with its icon (an open ROM from before OpenMonitor 1.0 has 0.3.3 or 0.3.4, which the "
              "condition can't tell apart), CommandSwap's pins archived",
              eff[2:] == ['copy "Utilities/OpenMonitor" "SYS:Utilities/OpenMonitor"',
                          'copy "Utilities/OpenMonitor.info" "SYS:Utilities/OpenMonitor.info"',
                          'retire "ENVARC:Sys/CommandSwap.prefs"'], str(eff))
        mon = icon_of((core.dir / "Utilities/OpenMonitor.info").read_bytes())
        check("OpenMonitor.info: a tool icon with 32 KB of stack (OpenBench runs the ROM's resident from it)",
              mon[0] == FakeIcons.WBTOOL and mon[4] == 32768)

        # ACROMFix (amigachrome-guest tools/acromfix), SetPatch for the open ROM: the file and its QUIET startup line on
        # every ROM, as it changes nothing where there is nothing to fix and an install can move to another ROM
        core = bp.Part(top, "OpenUp", "0.9", "test")
        bp.acromfix_lines(core, b"ACROMFix body")
        core.write()
        for machine in ("os323", "openkick100-ac", "openkick101-ac"):
            eff = lines_on(core.dir / "Part", machine, filt)
            check(f"{machine}: C:ACROMFix and its startup line, QUIET",
                  eff[2:] == ['copy "C/ACROMFix" "SYS:C/ACROMFix"', "startup C:ACROMFix QUIET"], str(eff))
        check("ACROMFix's file as built", (core.dir / "C/ACROMFix").read_bytes() == b"ACROMFix body")

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
        stale = ["AmigaChrome-Prismatic-light-2560x1440.png", "AmigaChrome-Nebula-dark-3840x2160.png"]
        for n in stale:                                  # an export from before pack 1.1: sizes its names no longer list
            (bdir / n).write_bytes(n.encode())
        bd = bp.backdrops_part(top, bdir, ogt, tone=fake_tone)
        check("art an earlier export left behind, in sizes make_art_backdrops.py no longer makes, is left out too",
              sorted(p.name for p in (bd.dir / "Backdrops").iterdir() if p.suffix == ".png") == own)
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

        # OpenSnap: the commodity, its icon's tool types and its line in S:User-Startup, on every ROM
        sn = bp.opensnap_part(top, b"\0$VER: OpenSnap 1.0 (24.10.2026) MIT\0", icons)
        sn.write()
        sl = (sn.dir / "Part").read_text(encoding="latin-1").splitlines()
        check("OpenSnap: the part at its program's version", sl[0] == "part OpenSnap 1.0", sl[0])
        check("OpenSnap: in SYS:Tools/Commodities, its program replaced on an upgrade",
              'copy "Tools/Commodities/OpenSnap" "SYS:Tools/Commodities/OpenSnap"' in sl)
        check("OpenSnap: its icon only where none is, so a user's keys stay",
              'default "Tools/Commodities/OpenSnap.info" "SYS:Tools/Commodities/OpenSnap.info"' in sl)
        snap_icon = icon_of((sn.dir / "Tools/Commodities/OpenSnap.info").read_bytes())
        check("OpenSnap.info: a tool icon, 32 KB of stack, the keys Shift+Right Amiga+S and +W as tool types",
              snap_icon[0] == FakeIcons.WBTOOL and snap_icon[4] == 32768
              and "SCREENKEY=shift rcommand s" in snap_icon[3] and "WINDOWKEY=shift rcommand w" in snap_icon[3]
              and "DONOTWAIT" in snap_icon[3], str(snap_icon))
        for machine in ("os323", "openkick101-ac"):
            check(f"{machine}: OpenSnap started at boot",
                  "startup Run >NIL: SYS:Tools/Commodities/OpenSnap" in lines_on(sn.dir / "Part", machine, filt))
        check("OpenSnap's part version is read from its program", bp.PART_MAIN.get("OpenSnap") ==
              ("Tools/Commodities/OpenSnap", "OpenSnap"))

        # OpenSnap's pictures: the ILBM writer, HAM, Extra Half-Brite and the names (tests/test_opensnap_pic.c)
        pic = tmp / "test_opensnap_pic"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src/opensnap'}", str(ROOT / "tests/test_opensnap_pic.c"),
                        "-o", str(pic)], check=True)
        r = subprocess.run([str(pic)], capture_output=True, text=True)
        check("OpenSnap's ILBM, colours and names (tests/test_opensnap_pic.c)", r.returncode == 0, r.stdout[-600:])
        snap_src = (ROOT / "src/opensnap/opensnap.c").read_text(encoding="latin-1")
        keys = [t.split("=")[0] for t in bp.OPENSNAP_TOOLTYPES if "=" in t]
        check("every tool type the icon has is one OpenSnap reads", all(f'"{k}"' in snap_src for k in keys), str(keys))
        check("Install-OpenUp installs OpenSnap on every install",
              "INSTALL Parts/OpenSnap" in (ROOT / "pack/Install-OpenUp").read_text(encoding="latin-1"))

        gfoffice_checks(tmp, filt)

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

    openprinter_checks()

    print(f"{'all passed' if not fails else str(fails) + ' FAILED'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
