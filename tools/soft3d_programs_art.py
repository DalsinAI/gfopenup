#!/usr/bin/env python3
"""Our programs' icons in the Soft 3D style (amicradle docs/ICON_STANDARD.md, docs/howto/12-draw-icons.md), drawn
as the kinds and the drives are (tools/soft3d_kinds_art.py, tools/soft3d_drives_art.py), at the same sizes:

    python3 tools/soft3d_programs_art.py OUT_DIR           NAME-64|32-normal|selected.png
    python3 tools/soft3d_programs_art.py --sheet OUT.png   the sheet: both sizes, both frames, light and dark

A program is one object, what it is. Chosen, it lifts with the glow and one thing in it moves.

GFOpenUpdate: a part's parcel, its lid open, and the blue arrow bringing the new version down into it; chosen,
the arrow goes deeper in. GFOpenNotifier: a bell; chosen, it swings and rings. AmiPortAtlas: an atlas with
a world map on its cover; chosen, the cover opens from the spine. TaitoX: an arcade cabinet, its game on the
screen; chosen, the joystick is pushed over and a shot flies up the screen.

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


GOLD = (232, 176, 52)


def draw_gfopennotifier(f, small, chosen=False):
    """GFOpenNotifier: a bell, the thing that tells you; chosen, it swings and its clapper rings out to the side."""
    sw = 40 if chosen else 0                                                               # the swing
    f.ell([226 + sw, 40, 286 + sw, 100], d3.dark(GOLD, 0.2))                               # the ring on top
    f.poly([(256 + sw - 70, 90), (256 + sw + 70, 90), (256 + sw + 150, 380), (256 + sw - 150, 380)], GOLD)  # the bell
    f.ell([256 + sw - 110, 70, 256 + sw + 110, 260], GOLD, d3.light(GOLD, 0.45))           # its round shoulder
    f.rr([256 + sw - 190, 360, 256 + sw + 190, 420], 30, d3.dark(GOLD, 0.15), d3.light(GOLD, 0.3))   # its lip
    cx = 256 + sw + (90 if chosen else 0)                                                  # the clapper
    f.ell([cx - 45, 400, cx + 45, 490], d3.dark(GOLD, 0.45))
    if not small:
        f.rr([256 + sw - 60, 140, 256 + sw - 30, 300], 15, d3.light(GOLD, 0.6))           # the shine
    return 40


NAVY = (44, 72, 138)                            # the atlas's cloth cover
PAGES = (240, 234, 220)
SEA = (120, 182, 226)
LAND = (98, 168, 84)


def draw_amiportatlas(f, small, chosen=False):
    """AmiPortAtlas: an atlas, the book of every game; its cover a world map. Chosen, the cover opens from the spine."""
    A, B, C, D = (40, 300), (170, 70), (472, 150), (342, 380)                 # spine front, spine back, back, front
    T = 70                                                                     # the book's depth
    def down(p, h=T):
        return (p[0], p[1] + h)
    f.poly([A, D, down(D), down(A)], d3.dark(NAVY, 0.1))                       # the back cover's edge
    f.poly([D, C, down(C), down(D)], d3.dark(NAVY, 0.3))
    f.poly([(A[0] + 8, A[1] + 10), D, down(D, T - 16), down(A, T - 16)], PAGES)   # the pages, front
    f.poly([D, (C[0], C[1] + 8), down(C, T - 16), down(D, T - 16)], d3.dark(PAGES, 0.16))   # and right
    if not small:
        for k in (24, 40):                                                     # the page edges
            f.line([(A[0] + 12, A[1] + k), (D[0] - 6, D[1] + k)], 3, d3.dark(PAGES, 0.14))
    if chosen:
        f.poly([A, B, C, D], d3.light(PAGES, 0.3))                             # the first page, under the cover
        f.line([(A[0] + 30, A[1] - 20), (B[0] + 26, B[1] + 30)], 6, d3.dark(PAGES, 0.1))
    lx, ly = (-70, -110) if chosen else (0, 0)                                 # the cover's free edge, lifted
    c, d = (C[0] + lx, C[1] + ly), (D[0] + lx, D[1] + ly)
    def at(u, v):                                                              # u: spine to edge, v: front to back
        return (A[0] + u * (d[0] - A[0]) + v * (B[0] - A[0]) + u * v * (c[0] - d[0] - B[0] + A[0]),
                A[1] + u * (d[1] - A[1]) + v * (B[1] - A[1]) + u * v * (c[1] - d[1] - B[1] + A[1]))
    if chosen:
        f.poly([d, c, (c[0], c[1] + 18), (d[0], d[1] + 18)], d3.dark(NAVY, 0.35))   # the cover's board edge
    f.poly([A, d, c, B], NAVY)                                                 # the cover
    f.poly([A, at(0.1, 0), at(0.1, 1), B], d3.dark(NAVY, 0.25))                # the spine's cloth
    f.poly([at(0.2, 0.12), at(0.92, 0.12), at(0.92, 0.88), at(0.2, 0.88)], d3.light(NAVY, 0.55))  # the map's frame
    f.poly([at(0.23, 0.16), at(0.89, 0.16), at(0.89, 0.84), at(0.23, 0.84)], SEA)          # the map
    lands = [[(0.30, 0.50), (0.38, 0.78), (0.52, 0.80), (0.56, 0.62), (0.46, 0.44)],      # three lands
             [(0.56, 0.24), (0.62, 0.46), (0.78, 0.50), (0.86, 0.34), (0.74, 0.20)],
             [(0.64, 0.62), (0.70, 0.78), (0.84, 0.76), (0.82, 0.60)]]
    for land in (lands if not small else lands[:2]):
        f.poly([at(u, v) for u, v in land], LAND)
    if not small:
        f.line([at(0.23, 0.55), at(0.89, 0.55)], 3, d3.light(SEA, 0.45))      # the equator
        f.poly([B, c, (c[0], c[1] + 8), (B[0], B[1] + 8)], d3.light(NAVY, 0.35))   # the cover's lit back edge
    return 40

CAB = (58, 66, 104)                             # the cabinet's paint
MARQUEE = (214, 70, 60)
SCREEN = (22, 26, 40)
PANEL = (90, 98, 136)


def draw_taitox(f, small, chosen=False):
    """TaitoX (the Taito X board's emulator): an arcade cabinet, seen from the front left, its game on the
    screen. Chosen, the joystick is pushed over and a shot flies up the screen."""
    dx, dy = 80, -50                                                            # the cabinet's depth, to the back right
    f.poly([(352, 40), (352 + dx, 40 + dy), (352 + dx, 470 + dy), (352, 470)], d3.dark(CAB, 0.35))   # its right side
    f.rr([150, 40, 352, 470], 10, CAB, d3.light(CAB, 0.2))                     # its front
    f.rr([164, 52, 338, 112], 8, MARQUEE, d3.light(MARQUEE, 0.35))             # the marquee, lit
    f.rr([170, 122, 332, 272], 10, d3.dark(CAB, 0.5))                          # the screen's bezel
    f.rr([184, 134, 318, 260], 8, SCREEN, d3.light(SCREEN, 0.12))              # the screen
    f.poly([(251, 220), (229, 252), (273, 252)], (110, 200, 255))              # the player's ship
    if not small:
        f.ell([198, 148, 230, 172], (240, 200, 70))                            # an enemy, and another
        f.ell([272, 160, 304, 184], (240, 200, 70))
    if chosen:
        f.rr([245, 150, 257, 204], 6, (255, 250, 220))                         # the shot, going up
    f.poly([(120, 320), (382, 320), (382 + dx // 2, 320 + dy // 2), (150 + dx // 2, 320 + dy // 2)], d3.light(PANEL, 0.3))   # the panel's top
    f.poly([(120, 320), (382, 320), (372, 372), (130, 372)], PANEL)           # the control panel, out in front
    sx = 44 if chosen else 0                                                     # the stick, pushed over
    f.line([(206, 334), (206 + sx, 300)], 14 if not small else 22, d3.dark(PANEL, 0.6))
    f.ell([184 + sx, 278, 228 + sx, 322], MARQUEE, d3.light(MARQUEE, 0.4))     # its ball
    f.ell([268, 326, 300, 350], (250, 210, 60))                                # two buttons
    if not small:
        f.ell([314, 326, 346, 350], (110, 200, 255))
    f.rr([222, 396, 280, 452], 6, d3.dark(CAB, 0.45))                          # the coin door
    if not small:
        f.rr([244, 408, 258, 438], 4, (250, 210, 60))                          # its slot, lit
    return 40


DRAW = {"GFOpenUpdate": draw_gfopenupdate, "GFOpenNotifier": draw_gfopennotifier,
        "AmiPortAtlas": draw_amiportatlas, "TaitoX": draw_taitox}


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
