#!/usr/bin/env python3
"""stock_default_tools.py DH0 [--out FILE]: the table pack/StockDefaultTools, read from a fresh AmigaOS 3.2.3 DH0.

One line for each project icon of the system that names a default tool:
    <icon, without .info> TAB <its default tool>
The path is SYS:<path in DH0>, or ENVARC:Sys/def_<kind> for the default icons. Only names and tool
strings go in; no icon, no program and no other OS file. FIXTOOLS (GFOpenUpTool) reads the table to put back
the default tool of an icon that GFOpenUp 0.6.14 to 0.7.0 changed to GFOpenView by mistake.

DH0 is a copy of a clean 3.2.3 install (the lab's Instance copy before GFOpenUp). Icons that are not the
system's (GFOpenPrint's, AmiCradle's WBStartup ones) are left out. Icon reading is AmiCradle's
scripts/amiga_icon.py (--amiga-icon FILE)."""
import argparse
import importlib.util
import sys
from pathlib import Path

NOT_THE_SYSTEMS = ("Utilities/GFOpenPrint/", "WBStartup/AC", "Utilities/GFOpenUp/", "Tools/GFOpenEdit.info")
WBPROJECT = 4


def load_reader(path: Path):
    spec = importlib.util.spec_from_file_location("amiga_icon", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def table(dh0: Path, reader) -> list[tuple[str, str]]:
    rows = []
    for f in sorted(dh0.rglob("*.info")):
        rel = f.relative_to(dh0).as_posix()
        if rel.startswith(NOT_THE_SYSTEMS):
            continue
        try:
            icon = reader.read_icon(f.read_bytes())
        except Exception:
            continue
        tool = icon.get("defaultTool")
        if icon.get("type") != WBPROJECT or not tool:
            continue
        name = rel[:-5]
        if name.startswith("Prefs/Env-Archive/Sys/"):
            name = "ENVARC:Sys/" + name[len("Prefs/Env-Archive/Sys/"):]
        else:
            name = "SYS:" + name
        rows.append((name, tool))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("dh0", type=Path)
    ap.add_argument("--out", type=Path, default=Path(__file__).resolve().parent.parent / "pack" / "StockDefaultTools")
    ap.add_argument("--amiga-icon", type=Path, default=Path.home() / "AmiCradle/scripts/amiga_icon.py")
    a = ap.parse_args()
    rows = table(a.dh0, load_reader(a.amiga_icon))
    head = ["; AmigaOS 3.2.3's own project icons and their default tools (names and tool strings only).",
            "; GFOpenUpTool FIXTOOLS puts these back where GFOpenUp 0.6.14 to 0.7.0 changed one to GFOpenView.",
            "; Made by tools/stock_default_tools.py from a clean 3.2.3 install.",
            "; <icon without .info> TAB <default tool>"]
    a.out.write_text("\n".join(head + [f"{n}\t{t}" for n, t in rows]) + "\n", encoding="latin-1")
    print(f"{a.out}: {len(rows)} icons")
    return 0


if __name__ == "__main__":
    sys.exit(main())
