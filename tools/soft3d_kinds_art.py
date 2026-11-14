#!/usr/bin/env python3
"""The kinds' default icons (a drawer, a project, a tool and the trashcan) in the Soft 3D style (Dale, 14 November
2026, 12:33: "b soft 3d"), after the drives (tools/soft3d_drives_art.py), in the same way and at the same sizes:

    python3 tools/soft3d_kinds_art.py OUT_DIR            KIND-64|32-normal|selected.png, KIND-32-chipset-...png
    python3 tools/soft3d_kinds_art.py --sheet OUT.png    the sheet: the kinds beside the drives, light and dark

Our own drawing: a drawer is an open folder holding papers, a project a page with its corner turned, a tool a
program's window with its title bar, the trashcan a bin with its lid. Chosen, the drawer's papers rise out of it,
as in the AOS set.

MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import soft3d_drives_art as d3                                                  # noqa: E402

KINDS = ["drawer", "project", "tool", "trashcan"]
LABELS = {"drawer": "Drawer", "project": "Project", "tool": "Tool", "trashcan": "Trashcan"}
BLUE = (59, 120, 216)
PAPER = (246, 248, 251)


def draw_drawer(f, small, chosen=False):
    body = BLUE
    f.rr([40, 60, 210, 140], 18, d3.dark(body, 0.15), d3.light(body, 0.2))          # the tab
    f.rr([40, 100, 472, 472], 24, d3.dark(body, 0.2), d3.light(body, 0.1))        # the back
    lift = 50 if chosen else 0
    f.rr([90, 70 - lift, 422, 360 - lift], 10, (226, 230, 236), (255, 255, 255))   # papers
    f.rr([110, 110 - lift, 440, 380 - lift], 10, PAPER, (255, 255, 255))
    if not small:
        for i, y in enumerate((150, 190, 230)):
            f.rr([150, y - lift, 380 - 50 * i, y + 14 - lift], 7, (170, 178, 192))
    f.rr([40, 220, 472, 472], 24, body, d3.light(body, 0.4))                        # the front flap
    f.rr([196, 250, 316, 274], 12, d3.light(body, 0.55))                             # its handle
    return 40


def draw_project(f, small, chosen=False):
    m = f.mask("poly", [110, 30, 340, 30, 402, 92, 402, 482, 110, 482])
    f.fill((214, 220, 230), m, (255, 255, 255))
    f.poly([(340, 30), (340, 92), (402, 92)], (190, 198, 212))                      # the turned corner
    rows = (140, 190, 240, 290, 340, 390) if not small else (150, 230, 310, 390)
    for i, y in enumerate(rows):
        f.rr([150, y, 362 - (60 if i % 3 == 2 else 0), y + (18 if not small else 34)], 9, (120, 140, 176))
    f.rr([150, 92, 290, 112 if not small else 124], 8, BLUE)                       # the title line
    return 18


def draw_tool(f, small, chosen=False):
    f.rr([40, 70, 472, 442], 22, (226, 230, 238), (250, 251, 253))                 # the window
    f.rr([40, 70, 472, 160], 22, BLUE, d3.light(BLUE, 0.4))                          # its title bar
    f.rr([40, 120, 472, 160], 0, BLUE)
    f.ell([70, 95, 110, 135] if not small else [64, 88, 120, 144], (255, 255, 255))   # the close button
    if not small:
        f.rr([140, 104, 300, 124], 10, d3.light(BLUE, 0.6))                          # the title
    # a gear in the window: the program at work
    cx, cy, r = 256, 300, 100 if not small else 110
    for k in range(8):
        import math
        a = math.radians(k * 45)
        x, y = cx + r * math.cos(a), cy + r * math.sin(a)
        t = 26 if not small else 32
        f.ell([x - t, y - t, x + t, y + t], (232, 112, 42))
    f.ell([cx - r, cy - r, cx + r, cy + r], (232, 112, 42), d3.light((232, 112, 42), 0.35))
    f.ell([cx - r * 0.38, cy - r * 0.38, cx + r * 0.38, cy + r * 0.38], (226, 230, 238))
    return 34


def draw_trashcan(f, small, chosen=False):
    body = (128, 136, 150)
    f.ell([50, 110, 462, 480], body, d3.light(body, 0.45))                          # the bin, seen from above
    f.ell([96, 150, 416, 440], d3.dark(body, 0.45), d3.dark(body, 0.25))            # its inside
    if chosen:                                                                       # chosen: the lid lifts and tips
        f.ell([60, 40, 452, 230], (170, 178, 190), d3.light(body, 0.6))
        f.rr([216, 110, 296, 150], 20, d3.dark(body, 0.3))
    else:
        f.ell([40, 100, 472, 490], (170, 178, 190), d3.light(body, 0.6))
        f.rr([216, 270, 296, 310], 20, d3.dark(body, 0.3))
    return 120


DRAW = {"drawer": draw_drawer, "project": draw_project, "tool": draw_tool, "trashcan": draw_trashcan}


def render(kind, S, chosen, small):
    from PIL import Image
    f = d3.Flat(S)
    thick = DRAW[kind](f, small, chosen)
    im = d3.solid(f.img, thick, S, chosen)
    if chosen:
        up = Image.new("RGBA", (S, S))
        up.alpha_composite(im, (0, 0), (0, int(16 * S / 512)))
        im = up
    return im


def frames(kind, size, chosen):
    """KIND at rest or chosen, size x size, hard alpha, as the drives (soft3d_drives_art.frames)"""
    big = size >= 48
    S = 512 if big else 256
    pair = [render(kind, S, c, not big) for c in (False, True)]
    from PIL import ImageChops
    m = ImageChops.lighter(pair[0].getchannel("A"), pair[1].getchannel("A")).point(lambda v: 255 if v >= 60 else 0)
    x0, y0, x1, y1 = m.getbbox()
    side = int(max(x1 - x0, y1 - y0) * 1.06)
    cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
    box = (cx - side // 2, cy - side // 2, cx - side // 2 + side, cy - side // 2 + side)
    im = d3.hard(pair[1 if chosen else 0].crop(box), size)
    return im if big else d3.edged(im)


def sheet(out):
    from PIL import Image, ImageDraw
    rows = [("64 px", 64, False, False, 1), ("64 px chosen", 64, True, False, 1), ("32 px", 32, False, False, 1),
            ("32 px, 16 colours", 32, False, True, 1), ("64 px at 2x", 64, False, False, 2),
            ("32 px at 3x", 32, False, False, 3)]
    items = [("k", k) for k in KINDS] + [("d", k) for k in ("system", "harddisk", "floppy")]
    cell, left, top = 150, 170, 40
    hs = [max(n * z, 40) + 18 for _, n, _, _, z in rows]
    h1 = top + sum(hs)
    im = Image.new("RGBA", (left + cell * len(items) + 20, 2 * h1 + 20), (255, 255, 255, 255))
    dr = ImageDraw.Draw(im)
    f, fb = d3.font(13), d3.font(15)
    for half, (bg, ink) in enumerate((((232, 234, 238), (30, 34, 44)), ((43, 47, 54), (230, 232, 236)))):
        y0 = half * (h1 + 20)
        dr.rectangle([0, y0, im.width, y0 + h1], fill=bg)
        for i, (src, k) in enumerate(items):
            name = LABELS[k] if src == "k" else d3.LABELS[k]
            x = left + i * cell + cell // 2
            dr.text((x - dr.textlength(name, font=fb) / 2, y0 + 12), name, font=fb, fill=ink)
        y = y0 + top
        for (name, n, chosen, chip, z), h in zip(rows, hs):
            dr.text((16, y + h // 2 - 8), name, font=f, fill=ink)
            for i, (src, k) in enumerate(items):
                p = frames(k, n, chosen) if src == "k" else d3.frames(k, n, chosen)
                if chip:
                    p = d3.chipset(p)
                p = d3.blow(p, z)
                im.alpha_composite(p, (left + i * cell + (cell - p.width) // 2, y + (h - p.height) // 2))
            y += h
    im.convert("RGB").save(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    ap.add_argument("--sheet", action="store_true")
    a = ap.parse_args(argv)
    if a.sheet:
        sheet(a.out)
        return 0
    a.out.mkdir(parents=True, exist_ok=True)
    for kind in KINDS:
        for chosen, state in ((False, "normal"), (True, "selected")):
            for n in (64, 32):
                im = frames(kind, n, chosen)
                im.save(a.out / f"{kind}-{n}-{state}.png")
                if n == 32:
                    d3.chipset(im).save(a.out / f"{kind}-32-chipset-{state}.png")
    print(f"{len(KINDS) * 6} pictures in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
