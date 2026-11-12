#!/usr/bin/env python3
"""The dock's programs in the tile look (Dale, 12 November 2026, 11:46: #704's tile look on every OpenDock icon).

    python3 tools/tile_icons_art.py icons/open --guest ../amicradle-guest

The pictures are amicradle-guest's tile_glyphs.py (libraries/openicon/romicons), the one drawing of the look the
ROM's own icons have: the graphic on the dark tile, at rest and chosen (something moves, ICONS.md section 1).
They are drawn at 75 x 75, so the pack's ICON_SCALE (85 per cent) shows them at 64, the size of the ROM's own.
Each NAME here is left out of tools/open_icons_art.py, so a redraw of the others keeps these.

MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

SIZE = 75           # 64 at ICON_SCALE

# icons/open NAME: tile_glyphs graphic
TILE = {
    "GFOpenFiles": "FILES", "GFOpenView": "VIEW", "GFOpenWrite": "TEXT", "GFOpenEdit": "EDIT",
    "GFOpenPlay": "MEDIA", "Install-GFOpenUp": "INSTALL", "GFOpenMonitor": "MONITOR", "GFOpenPrefs": "SYSTOOL",
    "GFOpenDrawer": "DRAWER",
}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    ap.add_argument("--guest", type=Path, required=True, help="an amicradle-guest checkout with tile_glyphs.frames")
    a = ap.parse_args(argv)
    sys.path.insert(0, str(a.guest / "libraries" / "openicon" / "romicons"))
    import tile_glyphs
    a.out.mkdir(parents=True, exist_ok=True)
    for name, graphic in TILE.items():
        rest, chosen = tile_glyphs.frames(graphic, SIZE, "dark")
        rest.save(a.out / f"{name}.png", optimize=True)
        chosen.save(a.out / f"{name}-selected.png", optimize=True)
    print(f"{len(TILE)} tile icons in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
