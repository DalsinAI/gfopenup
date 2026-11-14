#!/usr/bin/env python3
"""Host checks of tools/build_pack.py's parts for the no-ROM-volume design (24 October 2026, sections 8 and 10):
GFOpenPrefs in SYS:Preferences, the old homes archived, the boot's APPLY line and GFOpenMonitor by the ROM, and
the Installer and GFOpenUpMenu with no old names left. And GFOpenSnap's part and pictures (tests/test_opensnap_pic.c),
and the GFOffice Suite's parts: GFOffice, the shared files, which GFOpenWrite and GFOpenCalc need ("needs part"), with
INSTALL and REMOVE in both orders read through GFOpenUpTool's own code (gfoffice_checks).

    python3 tests/test_build_pack.py

The Part lines are read through GFOpenUpTool's own condition code (src/openup_lines.h, built here from
tests/test_openup_lines.c with the host's cc) for each kind of machine: OS 3.2.3's ROM, an open ROM with
prefs.resource 47.100, and one with 47.101 (the ROM that applies the boot's settings). With a gfopenprefs
checkout beside this one (or OPENAMIGAPREFS=dir), each page's SECTION word is checked against GFOpenPrefs's.
No pack is built and no stove is needed.

MIT, Copyright (c) 2026 AmiCradle Team."""
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

# The pages by the user's plain names (the design's section 8, Q9), in GFOpenPrefs's order
PAGES = ["Theme", "Screen Mode", "Pointer", "Fonts", "Colours", "Backdrops", "Workbench", "Icons", "File Requesters",
         "Windows", "Title Bar", "Menus", "IControl", "Dock", "Sound", "Input", "Gamepads", "Date & Time", "Serial",
         "Printing", "File Types", "System", "Blanker"]   # GFOpenPrefs 2.4's list order (gfopenprefs opframe.c lt_order)
# The stubs archived: the 13 old ones and the 9 GFOpenUp added on 21 October (section 10)
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
    """gfopenprint's icon writer as far as the parts call it: each icon's bytes say what it was asked for."""
    WBDISK, WBDRAWER, WBTOOL, WBPROJECT = 1, 2, 3, 4

    def icon(self, kind, art, default_tool=None, tool_types=None, stack=4096, window=(400, 200)):
        return repr((kind, art, default_tool, tool_types or [], stack)).encode()


def icon_of(data: bytes):
    return eval(data.decode())                 # our own repr, above


def lines_on(part_file: Path, machine: str, filt: Path) -> list[str]:
    """The Part file's lines as GFOpenUpTool's INSTALL acts on them on that machine."""
    out = subprocess.run([str(filt), str(part_file), machine], check=True, capture_output=True, text=True).stdout
    return out.splitlines()


# What Instance-32 had of OS 3.2.3's printing on 24 October 2026 (read from its disk: 61 files, 388,377 bytes),
# each of which the GFOpenPrinter part archives
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
    """The GFOpenPrinter part (retiring the legacy printing, 24 October 2026)."""
    with tempfile.TemporaryDirectory() as tmp:
        dev = b"\0" * 64 + b"printer.device 47.100 (13.10.2026) GFOpenPrinter, AmiCradle Team\r\n\0" \
            + b"\0$VER: printer.device 47.100 (13.10.2026) GFOpenPrinter, AmiCradle Team\0"
        op = bp.openprinter_part(Path(tmp), dev)
        op.write()
        lines = (op.dir / "Part").read_text(encoding="latin-1").splitlines()
        check("GFOpenPrinter: the part's version is its printer.device's", lines[0] == "part GFOpenPrinter 47.100", lines[0])
        check("GFOpenPrinter: AmiCradle instances only (the user's decision)", "needs AmiCradle" in lines)
        keep = [l for l in lines if l.startswith("keep ")]
        check("GFOpenPrinter: refused where DEVS:printer.device is neither OS 3.2's nor ours",
              keep == ['keep "SYS:Devs/printer.device" "printer.device 47." "GFOpenPrinter, AmiCradle Team" '
                       '"GFOpenPrinter, AmigaChrome Team"'], str(keep))
        check("GFOpenPrinter: the OS 3.2.3 printer.device's id and ours each carry a marker",
              any(m in "printer.device 47.3 (23.5.2020)" for m in bp.OPENPRINTER_MARKERS)
              and any(m.encode() in dev for m in bp.OPENPRINTER_MARKERS))
        check("GFOpenPrinter: our printer.device in OS 3.2.3's place (the original kept by copy's rule)",
              'copy "Devs/printer.device" "SYS:Devs/printer.device"' in lines
              and (op.dir / "Devs/printer.device").read_bytes() == dev)
        for name in ("printer.prefs", "printergfx.prefs"):
            check(f"GFOpenPrinter: {name} only where there is none, in ENVARC:Sys and ENV:Sys",
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
            check("PTXT: driver GFOpenPrint, port 0, single sheets, A4 (9), 66 lines, pica, 6 lpi, margins 5-75",
                  t[16:46].rstrip(b"\0") == b"GFOpenPrint" and t[46] == 0 and fields == [1, 9, 66, 0, 0, 5, 75, 1],
                  str((t[16:46], fields)))
            check("PUNT: oapspool (oapspool.device, as the editor writes it), unit 0",
                  int.from_bytes(u[16:20], "big") == 0 and u[24:56].rstrip(b"\0") == b"oapspool")
            check("PGFX: horizontal, colour, threshold 7, density 2",
                  [int.from_bytes(g[16 + 2 * i:18 + 2 * i], "big") for i in range(4)] == [0, 2, 0, 7] and g[30] == 2)
        retired = [l[8:-1] for l in lines if l.startswith('retire "')]
        check("GFOpenPrinter: every printing file Instance-32 has is archived (61)",
              not [p for p in INSTANCE32_PRINTING if p not in retired] and len(INSTANCE32_PRINTING) == 61,
              str([p for p in INSTANCE32_PRINTING if p not in retired]))
        check("GFOpenPrinter: the drivers where the real Installer keeps them, the editors at SYS:Prefs, 17 languages",
              "SYS:Storage/Printers/EpsonQ" in retired and "SYS:Prefs/Printer" in retired
              and "SYS:Locale/Help/deutsch/Sys/Prefs/printer.help" in retired
              and "SYS:Locale/Catalogs/fran\xe7ais/Sys/hp_printer.catalog" in retired and len(retired) == 218, str(len(retired)))
        stays = ("SYS:Devs/parallel.device", "SYS:Devs/serial.device", "SYS:Devs/Printers/GFOpenPrint", "SYS:Devs/Printers",
                 "SYS:Devs/Printers.info", "SYS:L/Port-Handler", "SYS:Devs/oapspool.device")
        check("GFOpenPrinter: the ports, PRT:'s handler, the Printers drawer and GFOpenPrint's own pieces stay",
              not [p for p in stays if p in retired], str([p for p in stays if p in retired]))
        check("GFOpenPrinter: nothing deleted, only archived (no delete line)", not [l for l in lines if l.startswith("delete")])
    check("GFOpenPrinter's version is read from its printer.device",
          bp.PART_MAIN.get("GFOpenPrinter") == ("Devs/printer.device", "printer.device"))
    inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
    a, b = inst.find("INSTALL Parts/GFOpenPrint >>"), inst.find("INSTALL Parts/GFOpenPrinter >>")
    check("Install-GFOpenUp: GFOpenPrinter right after GFOpenPrint, under the same choice", 0 < a < b
          and inst.rfind("(if (bitand #parts 2)", 0, a) > inst.rfind("(if (bitand #parts", 0, a) - 1
          and inst.find("(if (bitand #parts", a) > b)
    a = inst.find("INSTALL Parts/GFOpenImage >>")
    end = inst.find("(if (bitand #parts", a)
    check("Install-GFOpenUp: with GFOpenImage, the old OpenPNG, OpenTGA and OpenGIF descriptors are RETIREd "
          "(Uninstall puts them back)", a > 0 and all(
              0 < inst.find(f'(if (exists "DEVS:DataTypes/{d}") (run (cat #tool " RETIRE DEVS:DataTypes/{d} >>', a) < end
              for d in ("OpenPNG", "OpenTGA", "OpenGIF")))


def manifest_run(filt: Path, how: str, part_file: Path, manifest: str, tmp: Path, *more: str) -> tuple[int, str]:
    """GFOpenUpTool's "needs part" check (tests/test_openup_lines.c install|remove) of a Part file against a manifest's
    text: its return code (0 done, 5 refused, 10 failed: with more "fail") and what it printed (the manifest after,
    or why it refused)."""
    m = tmp / "GFOpenUp.manifest"
    m.write_text(manifest, encoding="latin-1")
    r = subprocess.run([str(filt), how, str(part_file), str(m), *more], capture_output=True, text=True, encoding="latin-1")
    return r.returncode, r.stdout


def entries(manifest: str) -> list[str]:
    """A manifest's lines but its first ("GFOpenUp", a tab, the form) and its end ("GFOpenUp", a tab, end: GFOpenUpTool
    writes it whole or not at all since 1 November 2026, src/openup_lines.h)."""
    return [x for x in manifest.splitlines() if not x.startswith("GFOpenUp\t")]


def gfoffice_checks(tmp: Path, filt: Path) -> None:
    """The GFOffice Suite's parts (gfoffice docs/gfoffice/Design-Suite-Shared.md, section 12, 25 October 2026): the
    GFOffice part's shared files (the suite's four libraries since gfoffice docs/suite/owf-libraries.md), GFOpenWrite
    and GFOpenCalc needing it, and INSTALL and REMOVE in both orders through GFOpenUpTool's own code
    (src/openup_lines.h)."""
    src = tmp / "gfoffice"
    onlib = src / bp.GFO_LIBRARY
    files = {
        onlib / f"library/{name}.library": f"\0{name}.library 3.0 (25.10.2026) libowf 1.4.0, AmiCradle Team\r\n\0"
                                           f"$VER: {name}.library 3.0 (25.10.2026) MIT\0".encode() + name.encode()
        for name in bp.GFO_LIBS
    }
    files.update({
        onlib / "GFOpenWrite": b"GFOpenWrite on owfwrite.library\0$VER: GFOpenWrite 2.0.26 (25.10.2026)\0",
        onlib / "OWConvert": b"OWConvert on the four libraries\0",
        onlib / "GFOpenCalc": b"\0$VER: GFOpenCalc 0.1 (25.10.2026) GFOpenCalc, the spreadsheet of the GFOffice Suite\0",
        src / "Dictionaries/en_GB.words": b"aardvark\nabacus\n",
        src / "LICENSE": b"MIT License\n\nCopyright (c) 2026 AmiCradle Team\n",
        src / "THIRD_PARTY.md": b"# Third-party parts\n",
        src / "RELEASE_NOTES_1.0.md": b"1.0\n", src / "RELEASE_NOTES_1.0.2.md": b"1.0.2\n",
        src / "licenses/SCOWL-ASPELL-English.txt": b"SCOWL notice\n",
    })
    for f, data in files.items():
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_bytes(data)
    zlib = b"\0$VER: z.library 53.12 (13.10.2026) GFOpenZlib, AmiCradle Team\0"
    icons = FakeIcons()
    top = tmp / "GF/GFOpenUp"
    gf, wr, oc = bp.gfoffice_parts(top, src, zlib, b"guest MIT licence\n", icons)
    for p in (gf, wr, oc):
        p.write()
    gl = (gf.dir / "Part").read_text(encoding="latin-1").splitlines()
    wl = (wr.dir / "Part").read_text(encoding="latin-1").splitlines()
    cl = (oc.dir / "Part").read_text(encoding="latin-1").splitlines()

    # GFOffice: the shared files, one owner each
    check("GFOffice: the part at owfwrite.library's version", gl[0] == "part GFOffice 3.0", gl[0])
    lic = "Utilities/GFOpenUp/Licences/"
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
    check("GFOffice: the licences say what each covers (gfoffice's MIT, GFOpenZlib's, SCOWL's for the dictionary)",
          (gf.dir / lic / "GFOffice.txt").read_bytes().endswith(files[src / "LICENSE"])
          and b"z.library 53.12 is GFOpenZlib" in (gf.dir / lic / "z.library.txt").read_bytes()
          and (gf.dir / lic / "z.library.txt").read_bytes().endswith(b"guest MIT licence\n")
          and b"(en_GB.words)" in (gf.dir / lic / "GFOffice-Dictionaries.txt").read_bytes()
          and (gf.dir / lic / "GFOffice-Dictionaries.txt").read_bytes().endswith(b"SCOWL notice\n"))

    # GFOpenWrite: on owfwrite.library, needing GFOffice, with none of the shared files
    check("GFOpenWrite: 2.0.26, and it needs the GFOffice part", wl[0] == "part GFOpenWrite 2.0.26" and "needs part GFOffice" in wl,
          str(wl[:3]))
    check("GFOpenWrite: the program built on its library (OWF=library)",
          (wr.dir / "Utilities/GFOpenWrite/GFOpenWrite").read_bytes() == files[onlib / "GFOpenWrite"])
    check("GFOpenWrite: no z.library, OWConvert or dictionary of its own (GFOffice's now)",
          not [l for l in wl if l.startswith(("copy", "default", "add"))
               and ("z.library" in l or "OWConvert" in l or "Dictionaries" in l)], str(wl))
    check("GFOpenWrite: the dictionary an earlier part put beside it archived, and nothing else of its drawer",
          [l for l in wl if l.startswith("retire")] == ['retire "SYS:Utilities/GFOpenWrite/Dictionaries/en_GB.words"'])
    check("GFOpenWrite: its release notes are the newest", (wr.dir / "Utilities/GFOpenWrite/Release-Notes.txt").read_bytes() == b"1.0.2\n")
    check("GFOpenWrite: its drawer icon only where none is or GFOpenUp's",
          'add "Utilities/GFOpenWrite.info" "SYS:Utilities/GFOpenWrite.info"' in wl)

    # GFOpenCalc: the same way
    check("GFOpenCalc: its program's version, and it needs the GFOffice part",
          cl[0] == "part GFOpenCalc 0.1" and "needs part GFOffice" in cl, str(cl[:3]))
    check("GFOpenCalc: in SYS:Utilities/GFOpenCalc, with its icon, licence and drawer icon",
          'copy "Utilities/GFOpenCalc/GFOpenCalc" "SYS:Utilities/GFOpenCalc/GFOpenCalc"' in cl
          and 'copy "Utilities/GFOpenCalc/GFOpenCalc.info" "SYS:Utilities/GFOpenCalc/GFOpenCalc.info"' in cl
          and 'copy "Utilities/GFOpenCalc/LICENSE" "SYS:Utilities/GFOpenCalc/LICENSE"' in cl
          and 'add "Utilities/GFOpenCalc.info" "SYS:Utilities/GFOpenCalc.info"' in cl
          and (oc.dir / "Utilities/GFOpenCalc/GFOpenCalc").read_bytes() == files[onlib / "GFOpenCalc"], str(cl))
    tool = icon_of((oc.dir / "Utilities/GFOpenCalc/GFOpenCalc.info").read_bytes())
    check("GFOpenCalc.info: a tool icon with 32 KB of stack, as GFOpenWrite's", tool[0] == FakeIcons.WBTOOL and tool[4] == 32768)
    dests = {}
    for name, lines in (("GFOffice", gl), ("GFOpenWrite", wl), ("GFOpenCalc", cl)):
        for l in lines:
            m = re.match(r'(?:copy|default|add) "[^"]*" "([^"]*)"$', l)
            if m:
                dests.setdefault(m.group(1), []).append(name)
    check("no file is in two of the suite's parts (the shared ones have one owner)",
          not [d for d, ps in dests.items() if len(ps) > 1], str({d: ps for d, ps in dests.items() if len(ps) > 1}))
    check("the parts' versions are read from owfwrite.library and GFOpenCalc",
          bp.PART_MAIN.get("GFOffice") == ("Libs/owfwrite.library", "owfwrite.library")
          and bp.PART_MAIN.get("GFOpenCalc") == ("Utilities/GFOpenCalc/GFOpenCalc", "GFOpenCalc"))
    try:
        bp.check_part_versions(top / "Parts")
        check("check_part_versions: GFOffice, GFOpenWrite and GFOpenCalc match their binaries", True)
    except SystemExit as e:
        check("check_part_versions: GFOffice, GFOpenWrite and GFOpenCalc match their binaries", False, str(e))

    # GFOpenPresentation: from the OWF=linked build, needing GFOffice (z.library), with its toolbar pictures
    linked = src / bp.GFO_LINKED
    pfiles = {linked / "GFOpenPresentation": b"\0$VER: GFOpenPresentation 0.6.23 (11.11.2026)\0",
              linked / "Images/Toolbar/Toolbar.ilbm": b"FORM....ILBM"}
    for f, data in pfiles.items():
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_bytes(data)
    pr = bp.gfoffice_presentation_part(top, src, icons)
    pr.write()
    pl = (pr.dir / "Part").read_text(encoding="latin-1").splitlines()
    d = "Utilities/GFOpenPresentation/"
    check("GFOpenPresentation: its program's version, and it needs the GFOffice part",
          pl[0] == "part GFOpenPresentation 0.6.23" and "needs part GFOffice" in pl, str(pl[:3]))
    check("GFOpenPresentation: in SYS:Utilities/GFOpenPresentation with its icon, toolbar pictures, licence and drawer icon",
          f'copy "{d}GFOpenPresentation" "SYS:{d}GFOpenPresentation"' in pl
          and f'copy "{d}GFOpenPresentation.info" "SYS:{d}GFOpenPresentation.info"' in pl
          and f'copy "{d}Images/Toolbar/Toolbar.ilbm" "SYS:{d}Images/Toolbar/Toolbar.ilbm"' in pl
          and f'copy "{d}LICENSE" "SYS:{d}LICENSE"' in pl
          and 'add "Utilities/GFOpenPresentation.info" "SYS:Utilities/GFOpenPresentation.info"' in pl
          and (pr.dir / d / "GFOpenPresentation").read_bytes() == pfiles[linked / "GFOpenPresentation"], str(pl))
    check("GFOpenPresentation's version is read from its program",
          bp.PART_MAIN.get("GFOpenPresentation") == (d + "GFOpenPresentation", "GFOpenPresentation"))
    try:
        bp.check_part_versions(top / "Parts")
        check("check_part_versions: GFOpenPresentation matches its binary", True)
    except SystemExit as e:
        check("check_part_versions: GFOpenPresentation matches its binary", False, str(e))
    rc, out = manifest_run(filt, "install", pr.dir / "Part", "GFOpenUp\t0.1\npart\tGFOpenUp\t0.8.9\nnew\tSYS:C/GFOpenUpTool\n", tmp)
    check("INSTALL GFOpenPresentation alone: left out (5), GFOffice missing", rc == 5 and out == "missing GFOffice\n", f"{rc} {out!r}")
    (linked / "GFOpenPresentation").unlink()
    check("without the linked build's GFOpenPresentation, no part", bp.gfoffice_presentation_part(tmp / "GF3/GFOpenUp", src, icons) is None)

    # GFOpenCalc's program not built: the part is left out, the others are made as before
    (onlib / "GFOpenCalc").unlink()
    top2 = tmp / "GF2/GFOpenUp"
    gf2, wr2, oc2 = bp.gfoffice_parts(top2, src, zlib, b"guest MIT licence\n", icons)
    check("without GFOpenCalc's program, no GFOpenCalc part, and GFOffice and GFOpenWrite as before",
          oc2 is None and not (top2 / "Parts/GFOpenCalc").exists() and gf2.lines == gf.lines
          and (wr2.write() or wr2.lines == wr.lines))

    # INSTALL and REMOVE through GFOpenUpTool's own code, from a lab with GFOpenUp's own part only
    fresh = "GFOpenUp\t0.1\npart\tGFOpenUp\t0.8.9\nnew\tSYS:C/GFOpenUpTool\n"
    gp, wp, cp = gf.dir / "Part", wr.dir / "Part", oc.dir / "Part"
    rc, out = manifest_run(filt, "install", wp, fresh, tmp)
    check("INSTALL GFOpenWrite alone: left out (5), and it says GFOffice is missing", rc == 5 and out == "missing GFOffice\n",
          f"{rc} {out!r}")
    rc, out = manifest_run(filt, "install", cp, fresh, tmp)
    check("INSTALL GFOpenCalc alone: left out too", rc == 5 and out == "missing GFOffice\n", f"{rc} {out!r}")
    rc, m1 = manifest_run(filt, "install", gp, fresh, tmp)
    check("INSTALL GFOffice: installed, needing nothing", rc == 0 and "part\tGFOffice\t3.0" in m1.splitlines()
          and OUL_NEEDPART not in m1, f"{rc} {m1!r}")
    rc, m2 = manifest_run(filt, "install", wp, m1, tmp)
    check("INSTALL GFOpenWrite after GFOffice: installed, its need noted in the manifest",
          rc == 0 and "part\tGFOpenWrite\t2.0.26" in m2.splitlines() and f"{OUL_NEEDPART}\tGFOpenWrite\tGFOffice" in m2.splitlines(),
          f"{rc} {m2!r}")
    rc, again = manifest_run(filt, "install", wp, m2, tmp)
    check("INSTALL GFOpenWrite again (an update): its need noted once", rc == 0 and again.count(f"{OUL_NEEDPART}\tGFOpenWrite\t") == 1,
          again)
    # 25 Oct 2026: an install with a file that couldn't be put in place isn't listed (Instance-32's calculator at
    # SYS:Utilities/GFOpenCalc, where GFOpenCalc's drawer goes, made INSTALL fail and list GFOpenCalc anyway)
    rc, mf = manifest_run(filt, "install", wp, m1, tmp, "fail")
    check("INSTALL GFOpenWrite that fails (10): not listed, its need not noted, the rest as it was",
          rc == 10 and mf.splitlines() == m1.splitlines(), f"{rc} {mf!r}")
    older = re.sub(r"(?m)^part\tGFOpenWrite\t.*$", "part\tGFOpenWrite\t2.0.0", m2)
    rc, mf = manifest_run(filt, "install", wp, older, tmp, "fail")
    check("an upgrade of GFOpenWrite that fails: the older version stays listed with its need, so the next one tries again",
          rc == 10 and mf.splitlines() == older.splitlines(), f"{rc} {mf!r}")
    rc, m3 = manifest_run(filt, "install", cp, m2, tmp)
    check("INSTALL GFOpenCalc after GFOffice: installed", rc == 0 and f"{OUL_NEEDPART}\tGFOpenCalc\tGFOffice" in m3.splitlines())
    rc, out = manifest_run(filt, "remove", gp, m3, tmp)
    check("REMOVE GFOffice while GFOpenWrite is installed: refused (5), nothing changed",
          rc == 5 and out == "needed by GFOpenWrite\n", f"{rc} {out!r}")
    rc, m4 = manifest_run(filt, "remove", wp, m3, tmp)
    check("REMOVE GFOpenWrite first: taken out, its need with it", rc == 0 and "GFOpenWrite" not in m4, m4)
    rc, out = manifest_run(filt, "remove", gp, m4, tmp)
    check("REMOVE GFOffice while GFOpenCalc is still installed: refused", rc == 5 and out == "needed by GFOpenCalc\n", f"{rc} {out!r}")
    rc, m5 = manifest_run(filt, "remove", cp, m4, tmp)
    rc2, m6 = manifest_run(filt, "remove", gp, m5, tmp)
    check("REMOVE GFOffice once no program needs it: taken out", rc == 0 and rc2 == 0
          and entries(m6) == entries(fresh), m6)
    check("the manifest written whole: its first line and its end line", m6.splitlines()[0] == "GFOpenUp\t0.2"
          and m6.splitlines()[-1] == "GFOpenUp\tend", m6)
    # Instance-32: a GFOpenWrite from an earlier GFOpenUp, with libowf linked in and no need noted, and no GFOffice
    old = fresh + "part\tGFOpenWrite\t2.0.1\nnew\tLIBS:z.library\nnew\tSYS:C/OWConvert\n"
    rc, out = manifest_run(filt, "install", wp, old, tmp)
    check("Instance-32 (an older GFOpenWrite, no GFOffice): the new GFOpenWrite waits for GFOffice", rc == 5 and out == "missing GFOffice\n")
    rc, out = manifest_run(filt, "remove", gp, manifest_run(filt, "install", gp, old, tmp)[1], tmp)
    check("Instance-32: REMOVE GFOffice under the older GFOpenWrite, which doesn't need it, is allowed", rc == 0, out)
    inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
    a, b = inst.find("INSTALL Parts/GFOffice >>"), inst.find("INSTALL Parts/GFOpenWrite >>")
    check("Install-GFOpenUp: GFOffice right before GFOpenWrite, under GFOpenWrite's choice",
          0 < a < b and inst.rfind("(if (bitand #parts 1048576)", 0, a) > inst.rfind("(if (bitand #parts", 0, a) - 1
          and inst.find("(if (bitand #parts", a) > b and inst.count("INSTALL Parts/GFOffice") == 1)


OUL_NEEDPART = "needpart"     # src/openup_lines.h


def amisans_checks() -> None:
    """The GFOpenFont part's AmiSans, TeX Gyre Heros (9 November 2026): from a gfopenfont checkout's fonts/outline
    (OPENAMIGAFONT=dir, or one beside this one), its .font an outline one listing the bitmap sizes 12 and 13, which
    come with it, the .otag naming _otf/texgyreheros-regular.otf with CGTriumvirate's OT_YSizeFactor, the file
    unmodified and the GUST Font License."""
    import struct
    src = Path(os.environ.get("OPENAMIGAFONT", ROOT.parent / "gfopenfont"))
    o = src / "fonts/outline"
    if not (o / "AmiSans.font").is_file():
        print(f"skipped: AmiSans (no {o}/AmiSans.font)")
        return
    with tempfile.TemporaryDirectory() as t:
        p = bp.Part(Path(t), "GFOpenFont", "0.9", "test")
        check("AmiSans: taken from gfopenfont", bp.amisans(p, src))
        d = p.dir
        otag = (d / "Fonts/AmiSans.otag").read_bytes()
        tags = {}
        for i in range(0, len(otag) - 8, 8):
            tag, val = struct.unpack(">II", otag[i:i + 8])
            if tag == 0:
                break
            tags[tag] = val
        spec = otag[tags.get(0x80009101, 0):].split(b"\0")[0]
        font = (d / "Fonts/AmiSans.font").read_bytes()
        check("AmiSans: an outline .font (OFCH_ID) listing AmiSans/12 and AmiSans/13, both in the part",
              font[:4] == b"\x0f\x03\x00\x02" and font[4:14] == b"AmiSans/12" and font[264:274] == b"AmiSans/13"
              and (d / "Fonts/AmiSans/12").is_file() and (d / "Fonts/AmiSans/13").is_file()
              and 'add "Fonts/AmiSans/13" "FONTS:AmiSans/13"' in p.lines)
        check("AmiSans: the .otag's engine, file, size and OT_YSizeFactor 8782/10519",
              tags.get(0x80001001) == len(otag) and spec == b"_otf/texgyreheros-regular.otf"
              and tags.get(0x80001011) == (8782 << 16 | 10519), str(tags))
        check("AmiSans: TeX Gyre Heros's file unmodified",
              (d / "Fonts/_otf/texgyreheros-regular.otf").read_bytes() ==
              (o / "TeXGyreHeros/texgyreheros-regular.otf").read_bytes())
        lic = (d / "Utilities/GFOpenUp/Licences/AmiSans.txt").read_text("latin-1")
        check("AmiSans: its licence, the GUST Font License, with the font's notice",
              "GUST Font License" in lic and "LaTeX Project Public License" in lic and "Jackowski" in lic)
        check("AmiSans: the Part lines add it where nothing is or GFOpenUp put it",
              'add "Fonts/AmiSans.otag" "FONTS:AmiSans.otag"' in p.lines
              and 'add "Fonts/_otf/texgyreheros-regular.otf" "FONTS:_otf/texgyreheros-regular.otf"' in p.lines)


def gfos_fonts_checks() -> None:
    """The GFOSFonts part (14 November 2026): from a gfosfonts checkout (GFOSFONTS=dir, or one beside this one), each
    face's .font and its 11, 13 and 16 built by the checkout's amigafont.py and added where nothing is or GFOpenUp put
    them; GFOS Mono not proportional; the licence; and the part left out, not half made, when a grid is missing."""
    import struct
    src = Path(os.environ.get("GFOSFONTS", ROOT.parent / "gfosfonts"))
    if not (src / "src/gfos-mono-16.txt").is_file():
        print(f"skipped: GFOSFonts (no {src}/src/gfos-mono-16.txt)")
        return
    with tempfile.TemporaryDirectory() as t:
        p = bp.gfos_fonts_part(Path(t), src)
        check("GFOSFonts: built from gfosfonts", p is not None)
        if p is None:
            return
        d = p.dir
        for face in ("GFOSSans", "GFOSSansBold", "GFOSMono"):
            font = (d / f"Fonts/{face}.font").read_bytes()
            fch_id, n = struct.unpack(">HH", font[:4])
            names = [font[4 + 260 * i:4 + 260 * i + 256].split(b"\0")[0].decode() for i in range(n)]
            check(f"GFOSFonts: {face}.font a bitmap contents file listing 11, 13 and 16",
                  fch_id == 0x0F00 and sorted(names) == sorted(f"{face}/{s}" for s in (11, 13, 16)), f"{fch_id:#x} {names}")
            for s in (11, 13, 16):
                check(f"GFOSFonts: FONTS:{face}/{s} in the part, added as GFOpenUp's own",
                      (d / f"Fonts/{face}/{s}").stat().st_size > 100
                      and f'add "Fonts/{face}/{s}" "FONTS:{face}/{s}"' in p.lines)
            check(f"GFOSFonts: FONTS:{face}.font added as GFOpenUp's own", f'add "Fonts/{face}.font" "FONTS:{face}.font"' in p.lines)

        def tf(face, s):                       # tf_YSize, tf_Flags, tf_XSize: the TextFont at 58 in the code hunk
            b = (d / f"Fonts/{face}/{s}").read_bytes()
            t = 32 + 58                         # the hunk's words start at 32 (header 24, HUNK_CODE and its length)
            return struct.unpack_from(">H", b, t + 20)[0], b[t + 23], struct.unpack_from(">H", b, t + 24)[0]
        check("GFOSFonts: Mono not proportional, 7, 8 and 10 wide; Sans proportional",
              all(not (tf("GFOSMono", s)[1] & 0x20) and tf("GFOSMono", s)[2] == w for s, w in ((11, 7), (13, 8), (16, 10)))
              and tf("GFOSSans", 13)[1] & 0x20 and [tf("GFOSSans", s)[0] for s in (11, 13, 16)] == [11, 13, 16],
              str([tf("GFOSMono", s) for s in (11, 13, 16)] + [tf("GFOSSans", 13)]))
        check("GFOSFonts: its licence", (d / "Utilities/GFOpenUp/Licences/GFOSFonts.txt").read_bytes()
              == (src / "LICENSE-FONTS").read_bytes()
              and 'copy "Utilities/GFOpenUp/Licences/GFOSFonts.txt" "SYS:Utilities/GFOpenUp/Licences/GFOSFonts.txt"' in p.lines)
        inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
        check("Install-GFOpenUp: GFOSFonts on every install, right after the Shell's font",
              0 < inst.index("INSTALL Parts/ShellFont") < inst.index("INSTALL Parts/GFOSFonts")
              and '(if (exists "Parts/GFOSFonts/Part")' in inst)
        check("GFOSFonts: its own version, 1.0", p.lines[0] == "part GFOSFonts 1.0" and "GFOSFonts" in bp.PART_OWN_VERSION)
    with tempfile.TemporaryDirectory() as t:
        half = Path(t) / "gfosfonts"
        shutil.copytree(src, half, ignore=shutil.ignore_patterns(".git", "specimens", "gfos-mono-16.txt"))
        check("GFOSFonts: left out, no drawer made, when a grid is missing",
              bp.gfos_fonts_part(Path(t), half) is None and not (Path(t) / "Parts/GFOSFonts").exists())


def kickstartset_checks(tmp: Path) -> None:
    """The slim ROM's disk part (9 November 2026): Parts/KickstartSet from a thin build's thin-support/: the set's
    modules copied, its Manifest last, the files on demand added (a disk's own stays), the ROM's version; a module
    that isn't its Manifest's stops the build."""
    import hashlib
    support = tmp / "out" / "thin-support"
    (support / "Kickstart/default").mkdir(parents=True)
    (support / "Libs").mkdir()
    (support / "L").mkdir()
    mods = {"icon.module": b"\0\0\x03\xf3icon", "workbench.module": b"\0\0\x03\xf3wb"}
    man = ["# GFOpenKickLoad manifest 1"]
    for name, data in mods.items():
        (support / "Kickstart/default" / name).write_bytes(data)
        man.append(f"{name} x.library 47.1 {len(data)} {hashlib.sha256(data).hexdigest()}")
    (support / "Kickstart/default/Manifest").write_text("\n".join(man) + "\n")
    (support / "Libs/asl.library").write_bytes(b"asl")
    (support / "L/CDFileSystem").write_bytes(b"cdfs")
    (support / "MANIFEST.txt").write_text("# the thin ROM's disk part\n"
                                          "Kickstart/default/icon.module\t8\tx\t47.5\tm/icon.module\n"
                                          "Libs/asl.library\t3\tx\t47.100\tb/asl/libs/asl.library\n"
                                          "L/CDFileSystem\t4\tx\t1.1\tb/cdfs/l/CDFileSystem\n")
    (tmp / "out" / "SOURCES.txt").write_text("# GFOpenKick trial ROM\n# id \"GFOpenKick 47.100 full41-thin\" (the ROM's version string)\n")
    top = tmp / "KS"
    check("no --thinsupport: no KickstartSet part", bp.kickstartset_part(top, None) is None)
    ks = bp.kickstartset_part(top, support)
    lines = (ks.dir / "Part").read_text(encoding="latin-1").splitlines()
    check("KickstartSet at the ROM's version", lines[0] == "part KickstartSet 47.100" and "full41-thin" in lines[1], lines[0] + lines[1])
    check("the set's modules copied whole into SYS:Kickstart/default/",
          'copy "Kickstart/default/icon.module" "SYS:Kickstart/default/icon.module"' in lines
          and 'copy "Kickstart/default/workbench.module" "SYS:Kickstart/default/workbench.module"' in lines)
    check("its Manifest last", lines[-1] == 'copy "Kickstart/default/Manifest" "SYS:Kickstart/default/Manifest"', lines[-1])
    check("the files on demand added where the disk has none (or GFOpenUp's)",
          'add "Libs/asl.library" "SYS:Libs/asl.library"' in lines and 'add "L/CDFileSystem" "SYS:L/CDFileSystem"' in lines)
    check("no needs line: the set goes on any machine that asks (KICKSET)", not any(l.startswith("needs") for l in lines))
    # Manifest 2: the pack holds every CPU's body. KICKSET copies one of them (tests/test_openup_kickset.c).
    support2 = tmp / "out2" / "thin-support"
    (support2 / "Kickstart/default/.bodies/62").mkdir(parents=True)
    (support2 / "Kickstart/default/.bodies/183").mkdir(parents=True)
    (support2 / "Kickstart/default/.bodies/20").mkdir(parents=True)
    (support2 / "Libs").mkdir()
    body = {4: b"\0\0\x03\xf3m68", 62: b"x86-64", 183: b"arm64", 20: b"ppc"}
    man2 = ["# GFOpenKickLoad manifest 2", "manifest 2",
            f"icon.module icon.library 47.5 {len(body[4])} {hashlib.sha256(body[4]).hexdigest()} 4"]
    (support2 / "Kickstart/default/icon.module").write_bytes(body[4])
    for cpu, data in ((62, body[62]), (183, body[183]), (20, body[20])):
        rel = f".bodies/{cpu}/icon.module"
        (support2 / "Kickstart/default" / rel).write_bytes(data)
        man2.append(f"{rel} icon.library 47.5 {len(data)} {hashlib.sha256(data).hexdigest()} {cpu}")
    (support2 / "Kickstart/default/Manifest").write_text("\n".join(man2) + "\n")
    (support2 / "Libs/asl.library").write_bytes(b"asl")
    (support2 / "MANIFEST.txt").write_text("# the thin ROM's disk part\nLibs/asl.library\t3\tx\t47.100\tb/asl\n")
    (tmp / "out2" / "SOURCES.txt").write_text("# id \"GFOpenKick 47.100 full41-thin\"\n")
    ks2 = bp.kickstartset_part(tmp / "KS2", support2)
    lines2 = (ks2.dir / "Part").read_text(encoding="latin-1").splitlines()
    check("a version 2 set lists every CPU's body in the pack",
          all(f'.bodies/{cpu}/icon.module' in "\n".join(lines2) for cpu in (62, 183, 20))
          and 'copy "Kickstart/default/icon.module"' in "\n".join(lines2),
          "\n".join(lines2))
    check("its Manifest is still last", lines2[-1].endswith('SYS:Kickstart/default/Manifest"'), lines2[-1])
    (support / "Kickstart/default/icon.module").write_bytes(b"changed")
    try:
        bp.kickstartset_part(top, support)
        check("a module that isn't its Manifest's stops the build", False)
    except SystemExit as exc:
        check("a module that isn't its Manifest's stops the build", "icon.module" in str(exc), str(exc))
    check("the Installer installs it with KICKSET on the slim ROM only",
          'KICKSET Parts/KickstartSet' in (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
          and '(= (getenv "GFOpenUp/SlimROM") "yes")' in (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1"))


def versions_stoves_checks(tmp: Path) -> None:
    """Corwin's 14 Nov build (teams/team3/inbox/20261114-1222): versions from the programs, an ID string after a
    letter, and the guest scripts' and Telegram's stoves."""
    # an ID string with a letter right before it is still read; the clean one wins where both are there
    check("binary_version: an ID string right after a letter is read (gfopeninput.library 1.3)",
          bp.binary_version(b"\0\0Xgfopeninput.library 1.3 (17.10.2026)\r\n", "gfopeninput.library") == "1.3")
    check("binary_version: a clean ID string is taken before one glued to a letter",
          bp.binary_version(b"Xgfopeninput.library 9.9 (1.1.2026)\0gfopeninput.library 1.3 (17.10.2026)",
                            "gfopeninput.library") == "1.3")
    check("binary_version: $VER first, as before",
          bp.binary_version(b"$VER: GFOpenFiles 0.5 (13.11.2026)\0", "GFOpenFiles") == "0.5")
    # the seven parts that had fallen behind take their program's version
    src = (ROOT / "tools/build_pack.py").read_text(encoding="latin-1")
    for name in ("GFOpenWrite", "GFOpenSocket", "GFOpenCompress", "GFOpenWindows", "GFOpenBlanker", "GFOpenUpSetup",
                 "GFOpenFiles"):
        check(f"{name}: the part's version is its program's (Part(..., None, ...)) and it has a PART_MAIN entry",
              f'Part(top, "{name}", None,' in src and name in bp.PART_MAIN)
    rel, vername = bp.PART_MAIN["GFOpenFiles"]
    pf = bp.Part(tmp, "GFOpenFiles", None, "GFOpenFiles: test")
    pf.add(rel, b"\0\0\x03\xf3$VER: GFOpenFiles 0.5 (13.11.2026)\0", "SYS:" + rel)
    pf.write()
    first = (tmp / "Parts/GFOpenFiles/Part").read_text(encoding="latin-1").split("\n")[0]
    check("a part with no version given is written with its program's ($VER 0.5)", first == "part GFOpenFiles 0.5", first)
    bp.check_part_versions(tmp / "Parts")
    check("check_part_versions agrees with it", True)
    nv = bp.Part(tmp, "GFOpenFiles", None, "GFOpenFiles: test")
    nv.add(rel, b"\0\0\x03\xf3no version here", "SYS:" + rel)
    try:
        nv.write()
        stopped = False
    except SystemExit:
        stopped = True
    check("a part whose program has no version stops the build", stopped)
    # the guest scripts: a STOVE default that this host lacks and names open6 -> GUEST_STOVE; else left alone
    home = tmp / "home"
    (home / "AmiCradle/stoves/os32-gcc16").mkdir(parents=True)
    s1 = tmp / "a.sh"
    s1.write_text('#!/bin/sh\nSTOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-16.2.0-open6"}\n')
    s2 = tmp / "b.sh"
    s2.write_text('#!/bin/sh\nSTOVE=${STOVE:-$HOME/AmiCradle/stoves/os32-gcc16}\n')
    s3 = tmp / "c.sh"
    s3.write_text('#!/bin/sh\nCC=m68k-amigaos-gcc\n')
    g0 = tmp / "g0"
    g0.mkdir()
    check("guest_stove: a missing open6 default gets GUEST_STOVE",
          bp.guest_stove(s1, {"OUT": "x"}, home, g0).get("STOVE") == str(g0))
    check("guest_stove: a default the host has is left to the script", "STOVE" not in bp.guest_stove(s2, {}, home, g0))
    check("guest_stove: a script with no STOVE default is left alone", "STOVE" not in bp.guest_stove(s3, {}, home, g0))
    check("guest_stove: a STOVE given stays", bp.guest_stove(s1, {"STOVE": "/x"}, home)["STOVE"] == "/x")
    # Londo's CHANGES on #254 (006e5a32): the guest scripts' real form names $STOVES, and GUEST_STOVE may be absent
    s4 = tmp / "d.sh"
    s4.write_text('#!/bin/sh\nSTOVES=${STOVES:-$HOME/AmiCradle/stoves}\n[ -d "$STOVES/os32-gcc16-open6" ] && '
                  'STOVE=${STOVE:-$STOVES/os32-gcc16-open6}\nSTOVE=${STOVE:-$STOVES/os32-gcc16-16.2.0-open6}\n')
    gstove = home / "AmiCradle/stoves/os32-gcc16-open6"
    check("guest_stove: no GUEST_STOVE on this host: the script's own default stays",
          "STOVE" not in bp.guest_stove(s1, {}, home, gstove) and "STOVE" not in bp.guest_stove(s4, {}, home, gstove))
    gstove.mkdir(parents=True)
    check("guest_stove: $STOVES form, its open6 default missing, GUEST_STOVE there: GUEST_STOVE",
          bp.guest_stove(s4, {}, home, gstove).get("STOVE") == str(gstove))
    (home / "AmiCradle/stoves/os32-gcc16-16.2.0-open6").mkdir(parents=True)
    check("guest_stove: $STOVES form whose default this host has: left to the script",
          "STOVE" not in bp.guest_stove(s4, {}, home, gstove))
    other = tmp / "elsewhere"
    (other / "os32-gcc16-16.2.0-open6").mkdir(parents=True)
    gstove2 = tmp / "g2"
    gstove2.mkdir()
    check("guest_stove: $STOVES from env, where the default is: left to the script",
          "STOVE" not in bp.guest_stove(s4, {"STOVES": str(other)}, tmp / "nohome", gstove2))
    check("guest_stove: $STOVES from env, default missing there: GUEST_STOVE",
          bp.guest_stove(s4, {"STOVES": str(tmp / "empty")}, home, gstove2).get("STOVE") == str(gstove2))
    gs = (ROOT / "tools/build_pack.py").read_text(encoding="latin-1")
    check("every amicradle-guest script run with genv or accontrol/acrtg.gpu's env goes through guest_stove or names a STOVE",
          not re.search(r'run\(\["sh", (?:g|a\.guest) [^\n]*env=(?:env\)|dict\(genv, (?!STOVE)(?![^\n]*guest_stove))', gs))
    check("UnofficialTelegram is built with the GCC 16 stove (OS32_GCC16)",
          re.search(r'openamigatelegram / "build-os3.sh"\], env=dict\(os.environ, STOVE=gcc16_root, OS32_GCC16=gcc16_root\)', gs)
          is not None)


def compress_files_checks(tmp: Path) -> None:
    """Legion's 14 Nov note: GFOpenCompress's two run-time libraries in its part; GFOpenFiles' build given the
    checkouts (OC, FTP, MEDIA) this build has."""
    oc = tmp / "gfopencompress"
    b = oc / "build/os3"
    b.mkdir(parents=True)
    (b / "GFOpenCompress").write_bytes(b"\0\0\x03\xf3$VER: GFOpenCompress 0.4.0 (13.11.2026)\0")
    for lib in bp.COMPRESS_LIBS:
        (b / lib).write_bytes(b"\0\0\x03\xf3" + lib.encode() + b" 1.0 (6.11.2026)\0")
    top = tmp / "top"
    part = bp.compress_part(top, oc, FakeIcons())
    lines = (part.dir / "Part").read_text(encoding="latin-1").splitlines()
    check("GFOpenCompress: its program's version (0.4.0)", lines[0] == "part GFOpenCompress 0.4.0", lines[0])
    check("GFOpenCompress: both run-time libraries go to LIBS: (add)",
          all(f'add "Libs/{lib}" "LIBS:{lib}"' in lines for lib in bp.COMPRESS_LIBS)
          and bp.COMPRESS_LIBS == ("gfopencompress_archive.library", "gfopencompress_crunch.library"), str(lines))
    check("GFOpenCompress: the program in SYS:Utilities as before",
          'copy "Utilities/GFOpenCompress" "SYS:Utilities/GFOpenCompress"' in lines)
    (b / bp.COMPRESS_LIBS[1]).unlink()
    try:
        bp.compress_part(tmp / "top2", oc, FakeIcons())
        stopped = False
    except SystemExit:
        stopped = True
    check("GFOpenCompress: a library not built stops the build", stopped)

    class A:
        pass
    a = A()
    a.opengadtools, a.guest, a.openamigacompress = Path("/g"), Path("/u"), Path("/oc")
    a.openamigaftp, a.openamigamedia = Path("/ftp"), Path("/media")
    e = bp.filemanager_env(a, {"PATH": "x"})
    check("GFOpenFiles' build gets OGT, GUEST, OC, FTP and MEDIA from this build's checkouts",
          (e["OGT"], e["GUEST"], e["OC"], e["FTP"], e["MEDIA"], e["PATH"]) == ("/g", "/u", "/oc", "/ftp", "/media", "x"), str(e))
    a.openamigaftp = a.openamigamedia = None
    e = bp.filemanager_env(a, {})
    check("without gfopenftp or gfopenmedia the build's own defaults stay", "FTP" not in e and "MEDIA" not in e)
    src = (ROOT / "tools/build_pack.py").read_text(encoding="latin-1")
    check("build_pack runs GFOpenFiles' build with filemanager_env and has --openamigamedia",
          'openamigafilemanager / "build.sh"], env=filemanager_env(a, env))' in src and '"--openamigamedia"' in src)


def icon_step_checks() -> None:
    """Ivanova's pack lab (bundle A, 14 Nov): after UPDATE 7 of 12 dock icons were generic. compress_part's part was
    never kept (oc.dir: NameError), MUIExample.png was missing, and both icon steps caught the error and went on."""
    src = (ROOT / "tools/build_pack.py").read_text(encoding="latin-1")
    check("the GFOpenCompress part is kept for the icon steps (oc = compress_part)", "oc = compress_part(" in src)
    block = src[src.index("        open_icons(a.amigachrome, ["):]
    block = block[:block.index("drawers=")]
    keys = set(re.findall(r'\.info", "([^"]+)"\)', block)) | {art for _, _, art in bp.PREFERENCES_PAGES}
    missing = sorted(k for k in keys if not (bp.OPEN_ART / f"{k}.png").is_file())
    check("every picture the Open apps' icons ask for is in icons/open", not missing and len(keys) > 20, str(missing))
    check("an icon step that fails stops the build (no 'icons not made' and carry on)",
          "icons not made" not in src and "the ones above stay" not in src and "the classic ones stay)" not in src)


def rc_version_checks() -> None:
    """Corwin's item 10 and the daily release cycle's numbering: a pack can be 0.8.29-rc1."""
    import argparse
    check("pack_version: 0.8.29 and 0.8.29-rc1 are taken",
          bp.pack_version("0.8.29") == "0.8.29" and bp.pack_version("0.8.29-rc1") == "0.8.29-rc1")
    bad = []
    for v in ("0.8.29-t5build", "rc1", "0.8", "0.8.29-rc", "0.8.29 rc1", "0.8.29-rc0", "0.8.29-rc01"):
        try:
            bp.pack_version(v)
            bad.append(v)
        except argparse.ArgumentTypeError:
            pass
    # -rc0 compares equal to the release in GFOpenUpTool (ouv_cmp: no rc number is 0), so it is refused (Team 5,
    # 14 Nov 16:12); -rc01 too, which ouv_cmp reads as rc1
    check("pack_version: other suffixes, -rc0, -rc01, a lone number and spaces are refused (0.8 is taken: two numbers)",
          bad == ["0.8"], str(bad))
    check("binary_version reads a release candidate's $VER whole",
          bp.binary_version(b"; $VER: Uninstall-GFOpenUp 0.8.29-rc1 (14.11.2026)\n", "Uninstall-GFOpenUp") == "0.8.29-rc1")
    check("binary_version: a plain $VER as before, and no -rc taken from a following word",
          bp.binary_version(b"$VER: GFOpenFiles 0.5 (13.11.2026)", "GFOpenFiles") == "0.5"
          and bp.binary_version(b"$VER: X 1.2-beta (1.1.2026)", "X") == "1.2")


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
        check("GFOpenUpTool's conditions (tests/test_openup_lines.c)", r.returncode == 0, r.stdout[-400:])
        sseq = tmp / "test_openup_startupseq"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}",
                        str(ROOT / "tests/test_openup_startupseq.c"), "-o", str(sseq)], check=True)
        r = subprocess.run([str(sseq)], capture_output=True, text=True)
        check("GFOpenUpTool STARTUPSEQ (tests/test_openup_startupseq.c)", r.returncode == 0, r.stdout[-400:])
        ks = tmp / "test_openup_kickset"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}",
                        str(ROOT / "tests/test_openup_kickset.c"), "-o", str(ks)], check=True)
        r = subprocess.run([str(ks)], capture_output=True, text=True)
        check("GFOpenUpTool KICKSET bodies (tests/test_openup_kickset.c)", r.returncode == 0, r.stdout[-600:])
        # GFOpenMUI from gfopenmui main's clean-room muimaster.library (muimaster/build-amiga.sh; Zune is gone from
        # main since 9bf57d2): the library and its examples, or the part left out, said (Corwin, 14 Nov 2026)
        mb = tmp / "gfopenmui" / "muimaster" / "build-amiga"
        (mb / "lib").mkdir(parents=True)
        (mb / "tests").mkdir(parents=True)
        (mb / "lib/muimaster.library").write_bytes(b"\0\x4a\xfcmuimaster.library\0\0$VER: muimaster.library 19.0 (10.11.2026) "
                                                   b"GFOpenMUI, AmiCradle Team\0")
        for ex in ("MUIClasses", "MUIAddons", "MUITime"):
            (mb / "tests" / ex).write_bytes(b"\0\0\x03\xf3" + ex.encode())
        # Ivanova's lab, bundle C row 10 (Team 5's 16:57 note): muimaster.library 19.0 opens gfopenlayout.library,
        # which nothing packed. The GFOpenLayout part brings it, GFOpenMUI needs that part, and a build without it stops
        try:
            bp.gfopenlayout_part(tmp / "olpack0", tmp / "gfopenmui")
            no_oll = None
        except SystemExit as e:
            no_oll = str(e)
        check("a GFOpenMUI build without gfopenlayout.library stops the pack build, said",
              no_oll is not None and "gfopenlayout.library" in no_oll, no_oll)
        check("no gfopenmui build: no GFOpenLayout part either", bp.gfopenlayout_part(tmp / "olpack1", tmp / "nothing") is None)
        (mb / "deps/gfopenlayout/build-amiga/lib").mkdir(parents=True)
        (mb / "deps/gfopenlayout/build-amiga/lib/gfopenlayout.library").write_bytes(
            b"\0\x4a\xfcgfopenlayout.library\0\0$VER: gfopenlayout.library 18.0 (10.11.2026) GFOpenLayout 0.18\0")
        ol = bp.gfopenlayout_part(tmp / "muipack", tmp / "gfopenmui")
        ol_lines = (ol.dir / "Part").read_text(encoding="latin-1").splitlines() if ol else []
        check("the GFOpenLayout part: gfopenlayout.library 18.0 to LIBS: (copy: an older one kept, a newer one stays)",
              ol_lines[:1] == ["part GFOpenLayout 18.0"] and 'copy "Libs/gfopenlayout.library" "LIBS:gfopenlayout.library"' in ol_lines
              and bp.PART_MAIN.get("GFOpenLayout") == ("Libs/gfopenlayout.library", "gfopenlayout.library"), "\n".join(ol_lines))
        om = bp.gfopenmui_part(tmp / "muipack", tmp / "gfopenmui", None)
        lines = (om.dir / "Part").read_text(encoding="latin-1").splitlines() if om else []
        check("GFOpenMUI needs the GFOpenLayout part", "needs part GFOpenLayout" in lines, "\n".join(lines))
        inst0 = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
        check("the Installer puts GFOpenLayout in before GFOpenMUI",
              0 < inst0.find("INSTALL Parts/GFOpenLayout") < inst0.find("INSTALL Parts/GFOpenMUI"))
        check("the packed libraries' gf* libraries: muimaster.library's gfopenlayout.library is in the pack",
              bp.unpacked_opened_libraries(tmp / "muipack") == [], bp.unpacked_opened_libraries(tmp / "muipack"))
        lone = tmp / "lonepack" / "GFOpenMUI" / "Libs"
        lone.mkdir(parents=True)
        (lone / "muimaster.library").write_bytes(b"\0\0$VER: muimaster.library 19.0\0gfopenlayout.library\0exec.library\0"
                                                 b"xgfnot.library\0")
        check("a packed library opening a gf* library nothing packs is named; OS and other names aren't ours",
              bp.unpacked_opened_libraries(tmp / "lonepack") == ["GFOpenMUI/Libs/muimaster.library opens gfopenlayout.library"],
              bp.unpacked_opened_libraries(tmp / "lonepack"))
        check("the ROM's own count as there", bp.unpacked_opened_libraries(tmp / "lonepack", {"gfopenlayout.library"}) == [])
        try:
            bp.check_opened_libraries(tmp / "lonepack")
            stopped = None
        except SystemExit as e:
            stopped = str(e)
        check("the pack build stops on it", stopped is not None and "opens gfopenlayout.library" in stopped, stopped)
        check("the pack build runs that check", "check_opened_libraries(top / \"Parts\")" in (ROOT / "tools/build_pack.py").read_text(encoding="utf-8"))
        oenv = bp.gfopenlayout_build_env(tmp / "mb", {"PATH": "/bin"}, tmp / "nogadtools")
        check("gfopenlayout.library is built from the muimaster script's own deps checkout, into its build-amiga",
              oenv.get("OUT") == str(tmp / "mb/deps/gfopenlayout/build-amiga") and "GFOPENLAYOUT" not in oenv
              and "OGT_ROOT" not in oenv and oenv.get("STOVE") == bp.gfopenmui_build_env(tmp / "mb", {})["STOVE"], oenv)
        bsrc = (ROOT / "tools/build_pack.py").read_text(encoding="utf-8")
        check("build_parts builds gfopenlayout.library after muimaster.library",
              bsrc.find('"deps/gfopenlayout/build-library.sh"') > bsrc.find('"muimaster/build-amiga.sh"], env=gfopenmui_build_env') > 0)
        check("GFOpenMUI: the clean-room muimaster.library 19.0, where no MUI is (add), and its examples",
              lines[:1] == ["part GFOpenMUI 19.0"] and 'add "Libs/muimaster.library" "LIBS:muimaster.library"' in lines
              and 'copy "Tools/GFOpenMUI/MUIClasses" "SYS:Tools/GFOpenMUI/MUIClasses"' in lines
              and not any("zune" in l.lower() for l in lines), "\n".join(lines))
        check("GFOpenMUI: left out, and said, when gfopenmui has no build",
              bp.gfopenmui_part(tmp / "muipack2", tmp / "nothing", None) is None)
        # Koenig's CHANGES on #253 (2ff22b70): the packed readme says what the part is, and a release build of
        # gfopenmui never builds the lab's AROS programs (LAB_PROGRAMS=0; they are APL and lab-only)
        rd = (ROOT / "gfopenup.readme").read_text(encoding="latin-1")
        mui = rd[rd.find("- GFOpenMUI"):]
        mui = mui[:mui.find("\n- ", 1)]
        check("the readme: GFOpenMUI is the clean-room muimaster.library and its examples, no Zune",
              "zune" not in mui.lower() and "muimaster.library" in mui and "MUIClasses" in mui
              and "zune" not in (ROOT / "README.md").read_text(encoding="utf-8").lower(), mui)
        env = bp.gfopenmui_build_env(tmp / "mb", {"PATH": "/bin"})
        check("a pack build of gfopenmui leaves the lab's AROS programs out (LAB_PROGRAMS=0)",
              env.get("LAB_PROGRAMS") == "0" and env.get("OUT") == str(tmp / "mb")
              and env.get("GFOPENLAYOUT") == str(tmp / "mb" / "deps/gfopenlayout") and env.get("PATH") == "/bin", env)
        check("no icon art key or art named ZuneTest is left",
              not any("zunetest" in (ROOT / f).read_text(encoding="utf-8").lower()
                      for f in ("tools/build_pack.py", "tools/icon_library_art.py", "tools/open_icons_art.py")))
        inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
        check("the Installer: no Zune left, and GFOpenMUI installed only when its part is in the pack",
              "AROS's Zune" not in inst and "MUI from Zune" not in inst and "ZuneTest" not in inst
              and '(and (bitand #parts 65536) (exists "Parts/GFOpenMUI/Part"))' in inst)
        sr = tmp / "test_openup_slimrom"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}",
                        str(ROOT / "tests/test_openup_slimrom.c"), "-o", str(sr)], check=True)
        r = subprocess.run([str(sr)], capture_output=True, text=True)
        check("GFOpenUpTool DETECT SlimROM (tests/test_openup_slimrom.c)", r.returncode == 0, r.stdout[-600:])
        vs = tmp / "test_openup_vers"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}",
                        str(ROOT / "tests/test_openup_vers.c"), "-o", str(vs)], check=True)
        r = subprocess.run([str(vs)], capture_output=True, text=True)
        check("GFOpenUpTool keeps a newer file (tests/test_openup_vers.c)", r.returncode == 0, r.stdout[-600:])
        lf = tmp / "test_openup_left"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}",
                        str(ROOT / "tests/test_openup_left.c"), "-o", str(lf)], check=True)
        r = subprocess.run([str(lf)], capture_output=True, text=True)
        check("GFOpenUpTool REINSTALL's list of parts the pack hasn't got (tests/test_openup_left.c)", r.returncode == 0,
              r.stdout[-600:])
        # Ivanova's pack lab b (14 Nov): a part the pack hasn't got was only named in T:GFOpenUp.log; REINSTALL sets
        # ENV:GFOpenUp/Left and the Installer's last screen names those parts
        tl = (ROOT / "src/openuptool.c").read_text(encoding="latin-1")
        inst2 = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
        check("REINSTALL sets GFOpenUp/Left, and the Installer's reinstall screen names the parts in it",
              'oul_left_add(' in tl and '"GFOpenUp/Left"' in tl and '(getenv "GFOpenUp/Left")' in inst2
              # Talkie Toaster's nit 1: cleared first, so an early return never leaves an earlier run's list
              and 'set_env("GFOpenUp/Left", "");' in tl[tl.index("static int reinstall("):][:tl[tl.index("static int reinstall("):].index("manifest_load();")]
              and "aren't in this pack" in inst2, "")
        un = tmp / "test_openup_unattended"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}",
                        str(ROOT / "tests/test_openup_unattended.c"), "-o", str(un)], check=True)
        r = subprocess.run([str(un)], capture_output=True, text=True)
        check("GFOpenUpTool UPDATE unattended (tests/test_openup_unattended.c)", r.returncode == 0, r.stdout[-600:])
        inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
        check("the Installer offers the ROM's Startup-Sequence, never replacing it unasked",
              'STARTUPSEQ CHECK' in inst and 'STARTUPSEQ ROM' in inst and 'STARTUPSEQ KEEP' in inst)

        icons = FakeIcons()
        top = tmp / "GFOpenUp"
        opp = bp.openprefs_part(top, b"GFOpenPrefs body", icons, "2.3", [("Open.theme", b"theme Open")],
                                [("Modern.profile", b"profile Modern")])
        opp.write()
        pf = opp.dir / "Part"
        text = pf.read_text(encoding="latin-1")
        lines = text.splitlines()
        check("the part is GFOpenPrefs at its program's version", lines[0] == "part GFOpenPrefs 2.3", lines[0])
        # the pieces its pages list (Instance-40, 4 November 2026: none on a new install, the Theme page's lists empty)
        check("the themes in SYS:Prefs/Presets/Themes, kept up to date, a user's own beside them",
              'add "Themes/Open.theme" "SYS:Prefs/Presets/Themes/Open.theme"' in lines
              and (opp.dir / "Themes/Open.theme").read_bytes() == b"theme Open")
        check("the desktop profiles in SYS:Prefs/Presets/GFOpenPrefs",
              'add "Profiles/Modern.profile" "SYS:Prefs/Presets/GFOpenPrefs/Modern.profile"' in lines
              and (opp.dir / "Profiles/Modern.profile").read_bytes() == b"profile Modern")

        # SYS:Preferences: the drawer, the program and its icon, an icon per page
        check("SYS:Preferences.info, a drawer icon GFOpenUp keeps up to date",
              'add "Preferences.info" "SYS:Preferences.info"' in lines
              and icon_of((opp.dir / "Preferences.info").read_bytes())[0] == FakeIcons.WBDRAWER)
        check("SYS:Preferences/GFOpenPrefs", 'copy "Preferences/GFOpenPrefs" "SYS:Preferences/GFOpenPrefs"' in lines
              and (opp.dir / "Preferences/GFOpenPrefs").read_bytes() == b"GFOpenPrefs body")
        tool = icon_of((opp.dir / "Preferences/GFOpenPrefs.info").read_bytes())
        check("SYS:Preferences/GFOpenPrefs.info, a tool icon with 32 KB of stack",
              'copy "Preferences/GFOpenPrefs.info" "SYS:Preferences/GFOpenPrefs.info"' in lines
              and tool[0] == FakeIcons.WBTOOL and tool[4] == 32768, str(tool))
        pages = [m.group(1) for m in re.finditer(r'^copy "Preferences/([^"/]+)\.info" "SYS:Preferences/\1\.info"$', text, re.M)
                 if m.group(1) != "GFOpenPrefs"]
        check("a project icon per page, by the plain names, in GFOpenPrefs's order", pages == PAGES, str(pages))
        words = {}
        for page in PAGES:
            f = opp.dir / f"Preferences/{page}.info"
            kind, _, deftool, tts, stack = icon_of(f.read_bytes()) if f.is_file() else (0, "", "", [], 0)
            ok = kind == FakeIcons.WBPROJECT and deftool == "SYS:Preferences/GFOpenPrefs" and len(tts) == 1 \
                and tts[0].startswith("SECTION=") and stack == 32768
            words[page] = tts[0][8:] if ok else ""
            if not ok:
                check(f"{page}.info: a project of SYS:Preferences/GFOpenPrefs with SECTION=", False, str((kind, deftool, tts, stack)))
        check("each page icon: a project of SYS:Preferences/GFOpenPrefs, SECTION=<its word>, 32 KB of stack",
              all(words.values()))
        check("no page has a file of its own (Q8: icons, no stubs)",
              not [p for p in PAGES if (opp.dir / f"Preferences/{p}").exists()])
        check("nothing goes into SYS:Prefs or C: any more, but the presets SYS:Prefs keeps (Q13)",
              not re.search(r'^(copy|add|default) "[^"]*" "(SYS:Prefs/(?!Presets/)|SYS:C/|C:)', text, re.M))

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

        # the lines by ROM, through GFOpenUpTool's own reading
        for machine, apply_line in (("os323", True), ("os323-ac", True), ("openkick100-ac", True), ("openkick101-ac", False),
                                    ("openkick101", False)):
            eff = lines_on(pf, machine, filt)
            check(f"{machine}: SYS:Preferences on the Shell's path", "startup Path >NIL: SYS:Preferences ADD" in eff)
            check(f"{machine}: the boot's APPLY {'by its full path' if apply_line else 'left to the ROM'}",
                  ("startup SYS:Preferences/GFOpenPrefs APPLY" in eff) == apply_line
                  and not any(l.startswith("startup") and "APPLY" in l and "SYS:Preferences/" not in l for l in eff),
                  str([l for l in eff if l.startswith("startup")]))
            check(f"{machine}: the archived files are the same on every ROM",
                  sum(l.startswith("retire") for l in eff) == 2 * len(OLD13 + NEW9) + 3)
        rm = lines_on(pf, "remove", filt)
        check("REMOVE reads the APPLY line with its condition off", "startup SYS:Preferences/GFOpenPrefs APPLY" in rm)

        # GFOpenMonitor and CommandSwap by the ROM
        core = bp.Part(top, "GFOpenUp", "0.9", "test")
        bp.openup_rom_lines(core, b"GFOpenMonitor body", icons.icon(icons.WBTOOL, "install", stack=32768))
        core.write()
        cf = core.dir / "Part"
        for machine in ("os323", "openkick100-ac"):
            eff = lines_on(cf, machine, filt)
            check(f"{machine}: GFOpenMonitor with its icon, nothing archived",
                  'copy "Utilities/GFOpenMonitor" "SYS:Utilities/GFOpenMonitor"' in eff
                  and 'copy "Utilities/GFOpenMonitor.info" "SYS:Utilities/GFOpenMonitor.info"' in eff
                  and not any(l.startswith("retire") for l in eff), str(eff))
        eff = lines_on(cf, "openkick101-ac", filt)
        check("openkick101: GFOpenMonitor with its icon (an open ROM from before GFOpenMonitor 1.0 has 0.3.3 or 0.3.4, which the "
              "condition can't tell apart), CommandSwap's pins archived",
              eff[2:] == ['copy "Utilities/GFOpenMonitor" "SYS:Utilities/GFOpenMonitor"',
                          'copy "Utilities/GFOpenMonitor.info" "SYS:Utilities/GFOpenMonitor.info"',
                          'retire "ENVARC:Sys/CommandSwap.prefs"'], str(eff))
        mon = icon_of((core.dir / "Utilities/GFOpenMonitor.info").read_bytes())
        check("GFOpenMonitor.info: a tool icon with 32 KB of stack (GFOpenBench runs the ROM's resident from it)",
              mon[0] == FakeIcons.WBTOOL and mon[4] == 32768)

        # ACROMFix (amicradle-guest tools/acromfix), SetPatch for the open ROM: the file and its QUIET startup line on
        # every ROM, as it changes nothing where there is nothing to fix and an install can move to another ROM
        core = bp.Part(top, "GFOpenUp", "0.9", "test")
        bp.acromfix_lines(core, b"ACROMFix body")
        core.write()
        for machine in ("os323", "openkick100-ac", "openkick101-ac"):
            eff = lines_on(core.dir / "Part", machine, filt)
            check(f"{machine}: C:ACROMFix and its startup line, QUIET",
                  eff[2:] == ['copy "C/ACROMFix" "SYS:C/ACROMFix"', "startup C:ACROMFix QUIET"], str(eff))
        check("ACROMFix's file as built", (core.dir / "C/ACROMFix").read_bytes() == b"ACROMFix body")

        # What the GFOpenKick ROM leaves to a disk (Instance-40, 4 November 2026: no iffparse.library, and nine of
        # GFOpenPrefs's pages ended): on the open ROM, each where none is; under OS 3.2.3's ROM nothing (its disk has them)
        built = tmp / "openkickdisk"
        for name, (_, out, _) in bp.OPENKICK_DISK.items():
            (built / name / out).parent.mkdir(parents=True, exist_ok=True)
            (built / name / out).write_bytes(f"{name} body".encode())
        core = bp.Part(top, "GFOpenUp", "0.9", "test")
        bp.openkick_disk_lines(core, built)
        core.write()
        want = sorted(f'default "OpenKick/{out}" "{dest}"' for _, out, dest in bp.OPENKICK_DISK.values())
        for machine in ("openkick100-ac", "openkick101-ac", "openkick101"):
            eff = lines_on(core.dir / "Part", machine, filt)
            if machine != "openkick100-ac":
                check(f"{machine}: the disk libraries and devices, each where none is", sorted(eff[2:]) == want, str(eff))
        eff = lines_on(core.dir / "Part", "os323", filt)
        check("os323: none of them (OS 3.2.3's disk has its own)", eff[2:] == [], str(eff))
        check("iffparse, diskfont, commodities, lowlevel, clipboard, serial and parallel among them",
              {"LIBS:iffparse.library", "LIBS:diskfont.library", "LIBS:commodities.library", "LIBS:lowlevel.library",
               "DEVS:clipboard.device", "DEVS:serial.device", "DEVS:parallel.device"}
              <= {d for _, _, d in bp.OPENKICK_DISK.values()})
        check("each file as built", all((core.dir / "OpenKick" / out).read_bytes() == f"{n} body".encode()
                                         for n, (_, out, _) in bp.OPENKICK_DISK.items()))

        # GFOS had no SYS:Classes/arexx.class, so no program's ARexx port opened (Team 5, 10 November 2026): ours
        # (GFOpenReaction) on the open ROM where none is, with its licence; under OS 3.2.3's ROM nothing (ReAction's)
        built = tmp / "openkickclasses"
        for name, (_, out, _) in bp.OPENKICK_CLASSES.items():
            (built / name / out).parent.mkdir(parents=True, exist_ok=True)
            (built / name / out).write_bytes(f"{name}.class 47.100 (10.11.2026) body".encode())
        core = bp.Part(top, "GFOpenUp", "0.9", "test")
        bp.openkick_classes_lines(core, built, b"MIT licence text")
        core.write()
        want = sorted(['default "OpenKick/classes/arexx.class" "SYS:Classes/arexx.class"',
                       'copy "Utilities/GFOpenUp/Licences/GFOpenReaction.txt" '
                       '"SYS:Utilities/GFOpenUp/Licences/GFOpenReaction.txt"'])
        for machine in ("openkick100-ac", "openkick101-ac", "openkick101"):
            eff = lines_on(core.dir / "Part", machine, filt)
            if machine != "openkick100-ac":
                check(f"{machine}: arexx.class where none is, and its licence", sorted(eff[2:]) == want, str(eff))
        eff = lines_on(core.dir / "Part", "os323", filt)
        check("os323: no arexx.class (ReAction's is there)", eff[2:] == [], str(eff))
        check("arexx.class as built", (core.dir / "OpenKick/classes/arexx.class").read_bytes().endswith(b"body"))
        check("GFOpenReaction's licence", (core.dir / "Utilities/GFOpenUp/Licences/GFOpenReaction.txt")
              .read_bytes().endswith(b"MIT licence text"))

        # Backdrops: GFOpenUp's own desktops, each with its .tone; the AmiCradle Background Pack's art (which
        # gfopengadtools' make_backdrops.py also exports) is an optional install, not GFOpenUp's (24 October 2026)
        bdir = tmp / "backdrops"
        bdir.mkdir()
        ogt = tmp / "gfopengadtools"
        (ogt / "tools").mkdir(parents=True)
        (ogt / "tools/make_art_backdrops.py").write_text(
            "from PIL import Image\nSIZES = [(640, 480), (1920, 1200)]\n"
            "ART = [('Prismatic-light', 'light'), ('Nebula-dark', 'dark')]\n", encoding="utf-8")
        own = ["Clear-light-chrome-logo-1920x1080.png", "Open-dark-1920x1200.png", "Open-light-strip-1080.png"]
        art = [f"AmiCradle-{d}-{size}.png" for d in ("Prismatic-light", "Nebula-dark") for size in ("640x480", "1920x1200")]
        for n in own + art:
            (bdir / n).write_bytes(n.encode())
        check("make_art_backdrops.py's names read from its source, without PIL", bp.art_backdrop_names(ogt) == set(art),
              str(bp.art_backdrop_names(ogt)))
        fake_tone = lambda png: "dark" if "-dark-" in png.name else "light"
        bd = bp.backdrops_part(top, bdir, ogt, tone=fake_tone)
        bd.write()
        lines = (bd.dir / "Part").read_text(encoding="latin-1").splitlines()
        check("the Backdrops part is 0.3 (the Ethereal backdrops are new)", lines[0] == "part Backdrops 0.3", lines[0])
        want = []
        for n in own:
            want += [f'copy "Backdrops/{n}" "SYS:Prefs/Presets/Backdrops/GFOpenUp/{n}"',
                     f'copy "Backdrops/{n}.tone" "SYS:Prefs/Presets/Backdrops/GFOpenUp/{n}.tone"']
        check("each of GFOpenUp's own pictures goes in with its .tone, in name order", lines[2:] == want, str(lines[2:]))
        check("each .tone says the picture's tone",
              all((bd.dir / f"Backdrops/{n}.tone").read_bytes() == (fake_tone(Path(n)) + "\n").encode() for n in own))
        check("none of the Background Pack's art is in the part",
              not [p for p in (bd.dir / "Backdrops").iterdir() if p.name.startswith("AmiCradle-")]
              and not [l for l in lines if "AmiCradle-" in l])
        stale = ["AmiCradle-Prismatic-light-2560x1440.png", "AmiCradle-Nebula-dark-3840x2160.png"]
        for n in stale:                                  # an export from before pack 1.1: sizes its names no longer list
            (bdir / n).write_bytes(n.encode())
        bd = bp.backdrops_part(top, bdir, ogt, tone=fake_tone)
        check("art an earlier export left behind, in sizes make_art_backdrops.py no longer makes, is left out too",
              sorted(p.name for p in (bd.dir / "Backdrops").iterdir() if p.suffix == ".png") == own)
        (ogt / "tools/make_art_backdrops.py").rename(ogt / "tools/gone.py")
        bd = bp.backdrops_part(top, bdir, ogt, tone=fake_tone)
        check("without make_art_backdrops.py, the art is still left out by its AmiCradle-<design>-<W>x<H>.png names",
              sorted(p.name for p in (bd.dir / "Backdrops").iterdir() if p.suffix == ".png") == own)
        try:
            from PIL import Image
        except ImportError:
            Image = None
        if Image is None:
            print("skipped picture_tone on real pictures: no PIL")
        else:
            # GFOpenUpTool's rule: the mean Rec. 709 luma of twelve rows, every eighth pixel; under 115 is dark
            for grey, tone in ((114, "dark"), (115, "light"), (30, "dark"), (220, "light")):
                f = tmp / f"grey{grey}.png"
                Image.new("RGB", (1920, 1200), (grey, grey, grey)).save(f)
                check(f"picture_tone: a grey of {grey} is {tone}", bp.picture_tone(f) == tone, bp.picture_tone(f))
            f = tmp / "halves.png"
            im = Image.new("RGB", (64, 48), (0, 0, 0))
            im.paste((255, 255, 255), (0, 0, 64, 20))     # 5 of the 12 rows sampled are white: a mean of 106, dark
            im.save(f)
            check("picture_tone samples twelve rows, top to bottom", bp.picture_tone(f) == "dark", bp.picture_tone(f))

        # GFOpenSnap: the commodity, its icon's tool types and its line in S:User-Startup, on every ROM
        sn = bp.opensnap_part(top, b"\0$VER: GFOpenSnap 1.0 (24.10.2026) MIT\0", icons)
        sn.write()
        sl = (sn.dir / "Part").read_text(encoding="latin-1").splitlines()
        check("GFOpenSnap: the part at its program's version", sl[0] == "part GFOpenSnap 1.0", sl[0])
        check("GFOpenSnap: in SYS:Tools/Commodities, its program replaced on an upgrade",
              'copy "Tools/Commodities/GFOpenSnap" "SYS:Tools/Commodities/GFOpenSnap"' in sl)
        check("GFOpenSnap: its icon only where none is, so a user's keys stay",
              'default "Tools/Commodities/GFOpenSnap.info" "SYS:Tools/Commodities/GFOpenSnap.info"' in sl)
        snap_icon = icon_of((sn.dir / "Tools/Commodities/GFOpenSnap.info").read_bytes())
        check("GFOpenSnap.info: a tool icon, 32 KB of stack, the keys Shift+Right Amiga+S and +W as tool types",
              snap_icon[0] == FakeIcons.WBTOOL and snap_icon[4] == 32768
              and "SCREENKEY=shift rcommand s" in snap_icon[3] and "WINDOWKEY=shift rcommand w" in snap_icon[3]
              and "DONOTWAIT" in snap_icon[3], str(snap_icon))
        for machine in ("os323", "openkick101-ac"):
            check(f"{machine}: GFOpenSnap started at boot",
                  "startup Run >NIL: SYS:Tools/Commodities/GFOpenSnap" in lines_on(sn.dir / "Part", machine, filt))
        check("GFOpenSnap's part version is read from its program", bp.PART_MAIN.get("GFOpenSnap") ==
              ("Tools/Commodities/GFOpenSnap", "GFOpenSnap"))

        # GFOpenSnap's pictures: the ILBM writer, HAM, Extra Half-Brite and the names (tests/test_opensnap_pic.c)
        pic = tmp / "test_opensnap_pic"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src/opensnap'}", str(ROOT / "tests/test_opensnap_pic.c"),
                        "-o", str(pic)], check=True)
        r = subprocess.run([str(pic)], capture_output=True, text=True)
        check("GFOpenSnap's ILBM, colours and names (tests/test_opensnap_pic.c)", r.returncode == 0, r.stdout[-600:])
        snap_src = (ROOT / "src/opensnap/opensnap.c").read_text(encoding="latin-1")
        keys = [t.split("=")[0] for t in bp.OPENSNAP_TOOLTYPES if "=" in t]
        check("every tool type the icon has is one GFOpenSnap reads", all(f'"{k}"' in snap_src for k in keys), str(keys))
        check("Install-GFOpenUp installs GFOpenSnap on every install",
              "INSTALL Parts/GFOpenSnap" in (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1"))

        # GFOpenUpMenu's hotkeys: Ctrl+Alt+Help and Ctrl+Alt+F12 open GFOpenMonitor (tests/test_openup_hotkeys.c)
        hk = tmp / "test_openup_hotkeys"
        subprocess.run([cc, "-std=c99", "-Wall", "-Werror", f"-I{ROOT / 'src'}", str(ROOT / "tests/test_openup_hotkeys.c"),
                        "-o", str(hk)], check=True)
        r = subprocess.run([str(hk)], capture_output=True, text=True)
        check("GFOpenUpMenu's hotkeys and ENV:GFOpenPrefs/Hotkeys (tests/test_openup_hotkeys.c)", r.returncode == 0, r.stdout[-600:])
        menu_src = (ROOT / "src/openupmenu.c").read_text(encoding="latin-1")
        check("GFOpenUpMenu holds both hotkeys, reads the switch again on Ctrl-F, and starts GFOpenMonitor with WAIT",
              '#include "openup_hotkeys.h"' in menu_src and "OUH_RAW_HELP, OUH_RAW_F12" in menu_src
              and "SIGBREAKF_CTRL_F" in menu_src and "GFOpenMonitor WAIT" in menu_src)
        check("GFOpenUpMenu, which holds the hotkeys, is started at boot by the GFOpenUp part",
              'core.line("startup Run >NIL: C:GFOpenUpMenu")' in (ROOT / "tools/build_pack.py").read_text(encoding="utf-8"))
        check("GFOpenSpeaker, the menu bar's volume control, is started at boot by the Sound part (open ROM: no APPLY)",
              'snd.line("startup Run >NIL: C:GFOpenSpeaker")' in (ROOT / "tools/build_pack.py").read_text(encoding="utf-8"))
        bp_src = (ROOT / "tools/build_pack.py").read_text(encoding="utf-8")
        check("GFOpenNetwork, the LAN and Wi-Fi icons in GFOpenTitle's row, is built and started at boot by the GFOpenTitle part",
              '"Network/build.sh"' in bp_src
              and 'tb.add("Tools/Commodities/GFOpenNetwork"' in bp_src
              and 'tb.line("startup Run >NIL: SYS:Tools/Commodities/GFOpenNetwork")' in bp_src
              and bp_src.index('tb.line("startup Run >NIL: C:GFOpenTitle")') < bp_src.index('tb.line("startup Run >NIL: SYS:Tools/Commodities/GFOpenNetwork")'))
        check("the Sound part is 0.2.4 (GFOpenSpeaker 0.2 in GFOpenTitle's row)", 'Part(top, "Sound", "0.2.4",' in bp_src)

        gfoffice_checks(tmp, filt)

    # GFOpenTypes and GFOpenDateTime: pages of GFOpenPrefs, parts with no files that archive their stubs
    src = (ROOT / "tools/build_pack.py").read_text(encoding="utf-8")
    for name, stub in (("GFOpenTypes", "OpenTypes"), ("GFOpenDateTime", "OpenDateTime")):
        check(f"the {name} part installs no stub, and archives SYS:Prefs/{stub}",
              f'"Prefs/{stub}", ' not in src and f"retire \"SYS:Prefs/{stub}\"" in src
              and name in bp.PART_OWN_VERSION and name not in bp.PART_MAIN)
    # GFOpenService 1.2 (0.8.17): the device with MEDIA:'s handler, its mountlist and ServiceTest, a version of its own
    check("the GFOpenService part brings MEDIA: (L:media-handler, DEVS:DOSDrivers/MEDIA) and C:ServiceTest",
          '"L:media-handler"' in src and '"DEVS:DOSDrivers/MEDIA", own=True' in src and '"C:ServiceTest"' in src
          and "GFOpenService" in bp.PART_OWN_VERSION and "GFOpenService" not in bp.PART_MAIN)
    # GFOpenService 1.4: the stub needs mitchy.device, so the part brings both
    check("the GFOpenService part is 1.4 and brings mitchy.device with gfopenservice.device, its old name",
          'Part(top, "GFOpenService", "1.4",' in src and '"DEVS:mitchy.device"' in src and '"DEVS:gfopenservice.device"' in src)
    # GFOpenGPU 0.29 (0.8.22): gfopengpu.library 0.29 (256 fence results, c2p through the core), ACRTG.gpu 1.6, C:GLQuery
    check("the GFOpenGPU part is 0.29 (gfopengpu.library 0.29)", 'Part(top, "GFOpenGPU", "0.29",' in src)
    check("the GFOpenGPU part brings C:GLQuery when the build has it", '"C:GLQuery"' in src)
    # 0.8.27: the CPU floor (9 November 2026), and OpenAL.module beside SDL2.module and GL.module
    check("the GFOpenGPU part needs a 68040 or 68060 with its FPU and MMU", 'gp.line("needs 68040")' in src)
    check("the GFOpenGPU part brings OpenAL.module, and copies it to LIBS:OpenGPU",
          '"LIBS:GFOpenGPU/OpenAL.module"' in src and '"GL", "OpenAL"]' in src)
    # 0.8.28: LIBS:physfs.library 1.1 (gfopenphysfs, pinned by commit) with LIBS:z.library in the GFOpenGPU part
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        built = t / "physfs"
        (built / "Libs").mkdir(parents=True)
        (built / "Libs/physfs.library").write_bytes(b"\0physfs.library 1.1 (9.11.2026) PhysicsFS 3.2.0\0")
        (built / "LICENSE.txt").write_text("zlib License\n")
        (built / "COMMIT").write_text(bp.GFOPENPHYSFS["commit"] + "\n")
        gp = bp.Part(t / "top", "GFOpenGPU", "0.29", "x")
        bp.physfs_into_gpu(gp, built, b"\0z.library 53.15 (10.11.2026)\0")
        gp.write()
        lines = (t / "top/Parts/GFOpenGPU/Part").read_text(encoding="latin-1").splitlines()
        check("the GFOpenGPU part brings LIBS:physfs.library and the z.library it inflates zip members through",
              'copy "Libs/physfs.library" "LIBS:physfs.library"' in lines and 'copy "Libs/z.library" "LIBS:z.library"' in lines
              and (t / "top/Parts/GFOpenGPU/Utilities/GFOpenUp/Licences/PhysicsFS.txt").is_file(), str(lines))
        (built / "COMMIT").write_text("0" * 40 + "\n")
        try:
            bp.physfs_into_gpu(bp.Part(t / "top", "GFOpenGPU", "0.29", "x"), built, b"")
            refused = False
        except SystemExit:
            refused = True
        check("physfs.library from another gfopenphysfs commit than GFOPENPHYSFS is refused", refused)
        # The next pack: physfs.library at gfopenphysfs d3a0ad7 (main after #5) wants z.library 53.15 (the hostlib inflate)
        (built / "COMMIT").write_text(bp.GFOPENPHYSFS["commit"] + "\n")
        for zl, want in ((b"\0z.library 53.14 (7.11.2026)\0", True), (b"no version", True),
                         (b"\0z.library 53.16 (1.12.2026)\0", False), (b"\0z.library 54.1 (1.1.2027)\0", False)):
            try:
                bp.physfs_into_gpu(bp.Part(t / "top2", "GFOpenGPU", "0.29", "x"), built, zl)
                refused = False
            except SystemExit:
                refused = True
            check(f"physfs.library with {zl[1:22]!r}: {'refused' if want else 'taken'}", refused == want)
    check("the GFOpenGPU part takes physfs.library when it was built", "physfs_into_gpu(gp, " in src)
    rtg_sdk = ROOT.parent / "gfopenrtg/tools/make_sdk.py"
    if rtg_sdk.is_file() and "PHYSFS_COMMIT" in rtg_sdk.read_text(encoding="utf-8"):
        check("gfopenrtg's make_sdk.py pins the same gfopenphysfs commit",
              f'PHYSFS_COMMIT = "{bp.GFOPENPHYSFS["commit"]}"' in rtg_sdk.read_text(encoding="utf-8"))
    # GFOpenWindows 0.12.2 (0.8.23): the snapshot pin is a toggle (gfopenprefs #184); 0.12.1, none on system requesters
    check("the GFOpenWindows part takes its program's version", 'Part(top, "GFOpenWindows", None,' in src)
    # OGLES3 (0.8.20): ogles3.library on GFOpenGPU (gfopenrtg #190), as Install-ogles3 has it: AmiCradle only, with
    # the GFOpenGPU part, never over another ogles3.library; its version is the library's
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        (t / "built/libs").mkdir(parents=True)
        (t / "built/libs/ogles3.library").write_bytes(b"\0$VER: ogles3.library 1.1 (6.11.2026) on GFOpenGPU, x\0")
        (t / "built/OGLES3Check").write_bytes(b"\0$VER: OGLES3Check 1.1 (6.11.2026)\0")
        (t / "src").mkdir()
        (t / "src/README.md").write_text("# ogles3.library on GFOpenGPU\n\nladislawv's ogles3 \u2014 credits\n", encoding="utf-8")
        (t / "src/ogles3_api.txt").write_text("# table\n#   Copyright (c) 2026 R-Typer and Ladislawv\nagl|x|y|z\n", encoding="utf-8")
        og = bp.ogles3_part(t / "top", t / "built", t / "src")
        lines = (t / "top/Parts/OGLES3/Part").read_text(encoding="latin-1").splitlines()
        check("the OGLES3 part is the library's version, AmiCradle only, after GFOpenGPU, and keeps another ogles3.library",
              og is not None and lines[0] == "part OGLES3 1.1" and "needs AmiCradle" in lines
              and "needs part GFOpenGPU" in lines and 'keep "LIBS:ogles3.library" "on GFOpenGPU"' in lines
              and 'copy "Libs/ogles3.library" "LIBS:ogles3.library"' in lines and 'copy "C/OGLES3Check" "C:OGLES3Check"' in lines,
              str(lines))
        credits = (t / "top/Parts/OGLES3/Utilities/GFOpenUp/Licences/ogles3.txt").read_bytes()
        check("ogles3.txt carries the README and ogles3's SDK notice, in Latin-1",
              b"ladislawv" in credits and b"R-Typer and Ladislawv" in credits and b"agl|" not in credits)
        bp.check_part_versions(t / "top/Parts")
    # AkikoPlus 2.0 (0.8.27): akikoplus.library 2 from a built drawer, its version the library's, no needs line,
    # installed with GFOpenService
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for rel, _ in bp.AKIKOPLUS_FILES:
            (t / "ak" / rel).parent.mkdir(parents=True, exist_ok=True)
            (t / "ak" / rel).write_bytes(b"x")
        (t / "ak/Libs/akikoplus.library").write_bytes(b"\0$VER: akikoplus.library 2.0 (9.11.2026) board and 68k\0")
        ak = bp.akikoplus_part(t / "top", t / "ak")
        lines = (t / "top/Parts/AkikoPlus/Part").read_text(encoding="latin-1").splitlines()
        check("the AkikoPlus part is the library's version, with every file and no needs line",
              ak is not None and lines[0] == "part AkikoPlus 2.0"
              and 'copy "Libs/akikoplus.library" "LIBS:akikoplus.library"' in lines
              and 'copy "C/AkikoTest" "C:AkikoTest"' in lines and 'copy "C/APTest" "C:APTest"' in lines
              and sum(1 for x in lines if x.startswith("copy ")) == len(bp.AKIKOPLUS_FILES)
              and not any(x.startswith("needs") for x in lines), str(lines))
        bp.check_part_versions(t / "top/Parts")
        check("without --akikoplus the AkikoPlus part is left out", bp.akikoplus_part(t / "top2", None) is None)
    inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
    sv_block = inst[inst.index('(working "Installing GFOpenService")'):inst.index("(if (bitand #parts 8388608)")]
    check("Install-GFOpenUp installs Parts/AkikoPlus with GFOpenService", "INSTALL Parts/AkikoPlus" in sv_block)
    inst = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
    gpu_block = inst[inst.index('(working "Installing GFOpenGPU")'):inst.index("; Warp3D, as it always has")]
    check("Install-GFOpenUp installs Parts/OGLES3 with GFOpenGPU", "INSTALL Parts/OGLES3" in gpu_block)
    check("the GFOpenCompress part takes its program's version", 'Part(top, "GFOpenCompress", None,' in src)
    check("the GFOpenImage part is 0.6 (gfopensound-decoders.library)", 'Part(top, "GFOpenImage", "0.6",' in src)
    check("GFOpenImage without the pack builder: Datatypes/out/Libs's libraries go to LIBS:",
          'img.add(f"Libs/{lib.name}", lib.read_bytes(), f"LIBS:{lib.name}")' in src)
    check("GFOpenPrefs's part version is read from SYS:Preferences/GFOpenPrefs",
          bp.PART_MAIN["GFOpenPrefs"] == ("Preferences/GFOpenPrefs", "GFOpenPrefs"))

    # the SECTION words are GFOpenPrefs's (gfopenprefs OpenPrefs/src/openprefs.c)
    oap = Path(os.environ.get("OPENAMIGAPREFS", ROOT.parent / "gfopenprefs"))
    op = oap / "OpenPrefs/src/openprefs.c"
    if op.is_file():
        known = set(re.findall(r'\{ "([A-Z]+)", LT_[A-Z]+ \}', op.read_text(encoding="latin-1")))
        bad = [(p, w) for p, w, _ in bp.PREFERENCES_PAGES if w not in known]
        check(f"every page's SECTION word is one GFOpenPrefs knows ({op})", not bad, str(bad))
    else:
        print(f"skipped the SECTION words: no {op} (OPENAMIGAPREFS=)")
    check("the pages' pictures are in icons/open",
          all((ROOT / "icons/open" / f"{art}.png").is_file() for _, _, art in bp.PREFERENCES_PAGES))

    # the Installer, the uninstaller and GFOpenUpMenu name none of the old stubs to run
    stubs = "|".join(OLD13 + NEW9 + ["GFOpenPrefs"])
    for f in ("pack/Install-GFOpenUp", "pack/Uninstall-GFOpenUp"):
        t = (ROOT / f).read_text(encoding="latin-1")
        guarded = re.findall(rf'\(run "SYS:Prefs/({stubs})\b', t)
        # an old stub is run only as the fallback where SYS:Preferences/GFOpenPrefs isn't there (Uninstall's GFOpenTypes),
        # and in the Installer only OpenUp's own OpenTypes, before the GF names, as its own uninstaller ran it (0.8.13)
        check(f"{f}: runs GFOpenPrefs from SYS:Preferences, an old name only as a fallback",
              guarded == [] or guarded == ["OpenTypes"], str(guarded))
    t = (ROOT / "pack/Install-GFOpenUp").read_text(encoding="latin-1")
    check("pack/Install-GFOpenUp: an OpenUp from before the GF names is taken out with its own OpenUpTool, "
          "its settings copied first (OLDNAMES), its dock after (OLDNAMES DOCK), never both blocks in S:User-Startup",
          t.index('" OLDNAMES >>') < t.index('" DETECT >NIL:"') < t.index('"SYS:C/OpenUpTool UNINSTALL')
          < t.index('" INSTALL Parts/GFOpenUp >>') < t.index('" OLDNAMES DOCK >>')
          and '";BEGIN OpenUp\\"' in t)
    menu = (ROOT / "src/openupmenu.c").read_text(encoding="latin-1")
    check("GFOpenUpMenu: no entry runs an old stub", not re.search(rf'"SYS:Prefs/({stubs})"', menu))
    entries = re.findall(r'HOW_PAGE, "([^"]+)", +"SYS:Preferences/([^"]+)", +"([A-Z]+)"', menu)
    pagewords = {p: w for p, w, _ in bp.PREFERENCES_PAGES}
    check("GFOpenUpMenu's page entries: each a page icon by its plain name, with its page's SECTION word",
          entries and all(label == page and pagewords.get(page) == word for label, page, word in entries), str(entries))

    amisans_checks()
    gfos_fonts_checks()
    openprinter_checks()
    with tempfile.TemporaryDirectory() as ktmp:
        kickstartset_checks(Path(ktmp))
    with tempfile.TemporaryDirectory() as vtmp:
        versions_stoves_checks(Path(vtmp))
    with tempfile.TemporaryDirectory() as ctmp:
        compress_files_checks(Path(ctmp))
    rc_version_checks()
    icon_step_checks()

    print(f"{'all passed' if not fails else str(fails) + ' FAILED'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
