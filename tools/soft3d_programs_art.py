#!/usr/bin/env python3
"""Our programs' icons in the Soft 3D style (amicradle docs/ICON_STANDARD.md, docs/howto/12-draw-icons.md), drawn
as the kinds and the drives are (tools/soft3d_kinds_art.py, tools/soft3d_drives_art.py), at the same sizes:

    python3 tools/soft3d_programs_art.py OUT_DIR           NAME-64|32-normal|selected.png
    python3 tools/soft3d_programs_art.py --sheet OUT.png   the sheet: both sizes, both frames, light and dark

A program is one object, what it is. Chosen, it lifts with the glow and one thing in it moves.

GFOpenUpdate: a part's parcel, its lid open, and the blue arrow bringing the new version down into it; chosen,
the arrow goes deeper in.

MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import soft3d_drives_art as d3                                                  # noqa: E402
import soft3d_kinds_art as k3                                                   # noqa: E402

CARD = (205, 160, 104)                          # the parcel's card
BLUE = k3.BLUE


def draw_gfopenupdate(f, small, chosen=False):
    f.poly([(60, 230), (256, 170), (452, 230), (256, 290)], d3.dark(CARD, 0.35))          # the open top, inside
    f.poly([(60, 230), (256, 290), (256, 490), (60, 420)], CARD)                           # its left side
    f.poly([(256, 290), (452, 230), (452, 420), (256, 490)], d3.dark(CARD, 0.18))         # its right side
    f.poly([(60, 230), (20, 170), (216, 112), (256, 170)], d3.light(CARD, 0.25))           # the flaps, open
    f.poly([(452, 230), (492, 170), (296, 112), (256, 170)], d3.light(CARD, 0.12))
    if not small:
        f.poly([(130, 300), (180, 316), (180, 336), (130, 320)], d3.light(CARD, 0.45))   # the label
    drop = 80 if chosen else 0                                                             # the arrow, going in
    w = 110 if not small else 140
    f.rr([256 - w // 2, 0 + drop, 256 + w // 2, 150 + drop], 16, BLUE, d3.light(BLUE, 0.4))
    f.poly([(256 - w - 60, 140 + drop), (256 + w + 60, 140 + drop), (256, 280 + drop)], d3.light(BLUE, 0.1))
    return 40


DRAW = {"GFOpenUpdate": draw_gfopenupdate}


def render(name, S, chosen, small):
    from PIL import Image
    f = d3.Flat(S)
    thick = DRAW[name](f, small, chosen)
    im = d3.solid(f.img, thick, S, chosen)
    if chosen:
        up = Image.new("RGBA", (S, S))
        up.alpha_composite(im, (0, 0), (0, int(16 * S / 512)))
        im = up
    return im


_FITTED = {}


def frames(name, size, chosen):
    """NAME at rest or chosen, size x size, hard alpha, as soft3d_kinds_art.frames"""
    big = size >= 48
    S = 512 if big else 256
    if (name, S) not in _FITTED:
        pair = [render(name, S, c, not big) for c in (False, True)]
        from PIL import ImageChops
        m = ImageChops.lighter(pair[0].getchannel("A"), pair[1].getchannel("A")).point(lambda v: 255 if v >= 60 else 0)
        x0, y0, x1, y1 = m.getbbox()
        side = int(max(x1 - x0, y1 - y0) * 1.06)
        cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
        box = (cx - side // 2, cy - side // 2, cx - side // 2 + side, cy - side // 2 + side)
        _FITTED[name, S] = [p.crop(box) for p in pair]
    im = d3.hard(_FITTED[name, S][1 if chosen else 0], size)
    return im if big else d3.edged(im)


def sheet(out):
    from PIL import Image, ImageDraw
    rows = [("64 px", 64, False, 1), ("64 px chosen", 64, True, 1), ("32 px", 32, False, 1),
            ("32 px chosen", 32, True, 1), ("64 px at 2x", 64, False, 2), ("64 px chosen at 2x", 64, True, 2)]
    names = list(DRAW) + ["tool"]               # beside the plain tool, for scale
    cell, left, top = 180, 190, 40
    hs = [max(n * z, 40) + 18 for _, n, _, z in rows]
    h1 = top + sum(hs)
    im = Image.new("RGBA", (left + cell * len(names) + 20, 2 * h1 + 20), (255, 255, 255, 255))
    dr = ImageDraw.Draw(im)
    f, fb = d3.font(13), d3.font(15)
    for half, (bg, ink) in enumerate((((232, 234, 238), (30, 34, 44)), ((43, 47, 54), (230, 232, 236)))):
        y0 = half * (h1 + 20)
        dr.rectangle([0, y0, im.width, y0 + h1], fill=bg)
        for i, n in enumerate(names):
            label = n if n in DRAW else "Tool (default)"
            x = left + i * cell + cell // 2
            dr.text((x - dr.textlength(label, font=fb) / 2, y0 + 12), label, font=fb, fill=ink)
        y = y0 + top
        for (label, n, chosen, z), h in zip(rows, hs):
            dr.text((16, y + h // 2 - 8), label, font=f, fill=ink)
            for i, name in enumerate(names):
                p = frames(name, n, chosen) if name in DRAW else k3.frames(name, n, chosen)
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
    for name in DRAW:
        for chosen, state in ((False, "normal"), (True, "selected")):
            for n in (64, 32):
                frames(name, n, chosen).save(a.out / f"{name}-{n}-{state}.png")
    print(f"{len(DRAW) * 4} pictures in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
