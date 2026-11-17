#!/usr/bin/env python3
"""The marks on the front of a named drawer in the Soft 3D style (Dale, 17 November 2026: "we should redo all the
folder icons"): one white mark for each kind of drawer, drawn flat on the folder's front flap, so every drawer is
the same folder (tools/soft3d_kinds_art.py, draw_drawer) and its mark says what it holds.

A mark is drawn in a square of 100 units (x across, y down) with a Pen; stamp() puts it on the flap with a dark
edge under it so white holds on the blue. `small` is the 32 px drawing: leave fine parts out.

MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

from PIL import Image, ImageChops

sys.path.insert(0, str(Path(__file__).resolve().parent))
import soft3d_drives_art as d3                                                  # noqa: E402

WHITE = (246, 248, 251)
UNDER = (22, 42, 96)                    # the dark edge under a mark, on the blue flap


class Pen:
    """draws a mark in a 100 x 100 square that sits at zone (512 units) of an S x S canvas"""

    def __init__(self, S, zone):
        self.f = d3.Flat(S)
        self.x0, self.y0, self.x1, self.y1 = zone
        self.k = ((self.x1 - self.x0) + (self.y1 - self.y0)) / 200

    def _b(self, b):
        return [(self.x0 if i % 2 == 0 else self.y0) + v / 100 * ((self.x1 - self.x0) if i % 2 == 0 else (self.y1 - self.y0))
                for i, v in enumerate(b)]

    def _m(self, kind, b, r=0, w=0):
        return self.f.mask(kind, self._b(b), r * self.k, w * self.k)

    def _on(self, m):
        self.f.fill(WHITE, m)

    def _off(self, m):
        self.f.img.putalpha(ImageChops.subtract(self.f.img.getchannel("A"), m))

    def rr(self, x0, y0, x1, y1, r=0):
        self._on(self._m("rr", [x0, y0, x1, y1], r))

    def ell(self, x0, y0, x1, y1):
        self._on(self._m("ell", [x0, y0, x1, y1]))

    def poly(self, pts):
        self._on(self._m("poly", [v for p in pts for v in p]))

    def line(self, pts, w):
        self._on(self._m("line", [v for p in pts for v in p], w=w))

    def ring(self, x0, y0, x1, y1, w):
        self._on(self._m("ring", [x0, y0, x1, y1], w=w))

    def cut_rr(self, x0, y0, x1, y1, r=0):
        self._off(self._m("rr", [x0, y0, x1, y1], r))

    def cut_ell(self, x0, y0, x1, y1):
        self._off(self._m("ell", [x0, y0, x1, y1]))

    def cut_poly(self, pts):
        self._off(self._m("poly", [v for p in pts for v in p]))

    def cut_line(self, pts, w):
        self._off(self._m("line", [v for p in pts for v in p], w=w))

    def cut_ring(self, x0, y0, x1, y1, w):
        self._off(self._m("ring", [x0, y0, x1, y1], w=w))


# ---- the marks ----------------------------------------------------------------------------------------

def sliders(p, small):
    for x, y in ((20, 58), (50, 26), (80, 70)):
        p.line([(x, 6), (x, 94)], 7)
        p.rr(x - 14, y - 11, x + 14, y + 11, 5)


def wrench(p, small):
    p.line([(20, 86), (62, 40)], 17)
    p.ell(48, 4, 96, 52)
    p.cut_poly([(70, 30), (92, -4), (112, 14), (84, 40)])


def hammer(p, small):
    p.line([(26, 94), (60, 40)], 13)
    p.line([(34, 18), (88, 46)], 28)


def commodity(p, small):
    p.rr(6, 6, 94, 94, 20)
    p.cut_rr(20, 20, 80, 80, 10)
    p.ell(34, 34, 66, 66)


def gear(p, small):
    for k in range(8):
        a = math.radians(k * 45)
        x, y = 50 + 39 * math.cos(a), 50 + 39 * math.sin(a)
        p.ell(x - 11, y - 11, x + 11, y + 11)
    p.ell(16, 16, 84, 84)
    p.cut_ell(36, 36, 64, 64)


def chip(p, small):
    for o in ((32, 68) if small else (30, 50, 70)):
        p.line([(o, 2), (o, 98)], 8)
        p.line([(2, o), (98, o)], 8)
    p.rr(16, 16, 84, 84, 8)
    p.cut_rr(36, 36, 64, 64, 4)


def archive(p, small):
    p.rr(10, 36, 90, 94, 6)
    p.rr(3, 10, 97, 32, 6)
    p.cut_rr(36, 50, 64, 62, 5)


def puzzle(p, small):
    p.rr(8, 34, 70, 96, 8)
    p.ell(24, 8, 54, 40)
    p.ell(60, 50, 94, 82)


def rocketlaunch(p, small):
    p.poly([(26, 46), (6, 78), (30, 70)])
    p.poly([(74, 46), (94, 78), (70, 70)])
    p.ell(30, 0, 70, 76)
    p.cut_ell(41, 20, 59, 38)
    p.poly([(40, 80), (60, 80), (50, 100)])


def monitor(p, small):
    p.rr(4, 6, 96, 72, 8)
    p.cut_rr(14, 16, 86, 62, 3)
    p.rr(42, 72, 58, 86)
    p.rr(22, 84, 78, 96, 5)


def printer(p, small):
    p.rr(26, 4, 74, 40, 3)
    p.rr(6, 34, 94, 76, 8)
    p.cut_rr(20, 60, 80, 68, 2)
    p.rr(24, 64, 76, 98, 3)


def mount(p, small):
    p.line([(50, 2), (50, 34)], 14)
    p.poly([(28, 28), (72, 28), (50, 54)])
    p.rr(6, 60, 94, 96, 8)
    p.cut_ell(72, 70, 88, 86)


def keyboard(p, small):
    p.rr(2, 22, 98, 82, 8)
    cols = 3 if small else 5
    w = 80 / cols
    for r in range(2):
        for c in range(cols):
            p.cut_rr(11 + c * w, 31 + r * 15, 9 + (c + 1) * w, 41 + r * 15, 2)
    p.cut_rr(24, 62, 76, 72, 2)


def tag(p, small):
    p.poly([(4, 50), (38, 12), (96, 12), (96, 88), (38, 88)])
    p.cut_ell(28, 40, 48, 60)


def star(p, small):
    pts = []
    for k in range(10):
        a = math.radians(-90 + k * 36)
        r = 50 if k % 2 == 0 else 21
        pts.append((50 + r * math.cos(a), 52 + r * math.sin(a)))
    p.poly(pts)


def code(p, small):
    p.line([(34, 20), (8, 50), (34, 80)], 11)
    p.line([(66, 20), (92, 50), (66, 80)], 11)
    if not small:
        p.line([(58, 14), (42, 86)], 9)


def scroll(p, small):
    p.rr(18, 12, 82, 88, 3)
    p.rr(6, 2, 94, 20, 9)
    p.rr(6, 80, 94, 98, 9)
    for y in ((38, 60) if small else (34, 48, 62)):
        p.cut_line([(30, y), (70, y)], 6)


def letter(p, small):
    p.poly([(50, 4), (94, 96), (72, 96), (64, 76), (36, 76), (28, 96), (6, 96)])
    p.cut_poly([(50, 32), (59, 58), (41, 58)])


def locale(p, small):
    p.rr(4, 8, 96, 70, 16)
    p.poly([(22, 64), (18, 96), (50, 66)])
    p.cut_line([(22, 30), (78, 30)], 7)
    p.cut_line([(22, 48), (60, 48)], 7)


def openbook(p, small):
    p.poly([(50, 22), (4, 10), (4, 82), (50, 94)])
    p.poly([(50, 22), (96, 10), (96, 82), (50, 94)])
    p.cut_line([(50, 20), (50, 96)], 6)


def layers(p, small):
    def diamond(dy):
        return [(50, 40 + dy), (96, 60 + dy), (50, 80 + dy), (4, 60 + dy)]
    p.poly(diamond(18))
    for dy in (0, -18) if not small else (-10,):
        p.cut_poly(diamond(dy + 7))
        p.poly(diamond(dy))


def terminal(p, small):
    p.rr(2, 10, 98, 90, 10)
    p.cut_line([(18, 32), (38, 50), (18, 68)], 9)
    p.cut_line([(48, 68), (74, 68)], 9)


def page(p, small):
    p.poly([(16, 2), (62, 2), (86, 26), (86, 98), (16, 98)])
    for y in ((48, 72) if small else (42, 58, 74)):
        p.cut_line([(30, y), (72, y)], 7)


def pages(p, small):
    p.rr(32, 2, 94, 78, 4)
    p.cut_rr(0, 16, 76, 100, 4)
    p.rr(8, 22, 68, 98, 4)
    for y in ((46, 70) if small else (42, 58, 74)):
        p.cut_line([(20, y), (56, y)], 7)


def link(p, small):
    p.ring(2, 28, 58, 72, 12)
    p.ring(42, 28, 98, 72, 12)


def hourglass(p, small):
    p.poly([(16, 8), (84, 8), (55, 50), (84, 92), (16, 92), (45, 50)])
    p.rr(8, 2, 92, 14, 4)
    p.rr(8, 86, 92, 98, 4)


def icon(p, small):
    p.rr(6, 6, 94, 94, 12)
    p.cut_rr(18, 18, 82, 82, 5)
    p.ell(32, 32, 68, 68)


def pattern(p, small):
    for r in range(3):
        for c in range(3):
            if (r + c) % 2 == 0:
                p.rr(4 + c * 32, 4 + r * 32, 32 + c * 32, 32 + r * 32, 3)


def question(p, small):
    p.ring(22, 2, 78, 58, 15)
    p.cut_rr(14, 30, 50, 66)
    p.line([(50, 52), (50, 70)], 15)
    p.ell(40, 80, 60, 100)


def flag(p, small):
    p.line([(14, 4), (14, 98)], 9)
    p.poly([(18, 8), (94, 8), (76, 32), (94, 56), (18, 56)])


def pointer(p, small):
    p.poly([(14, 2), (14, 84), (34, 66), (48, 98), (64, 90), (50, 60), (78, 60)])


def waveform(p, small):
    hs = (30, 62, 96, 50, 80, 36, 18) if not small else (40, 90, 60, 26)
    w = 100 / len(hs)
    for i, h in enumerate(hs):
        p.rr(i * w + w * 0.18, 50 - h / 2, (i + 1) * w - w * 0.18, 50 + h / 2, 3)


def cube(p, small):
    p.poly([(50, 2), (94, 24), (50, 46), (6, 24)])
    p.poly([(6, 34), (45, 54), (45, 98), (6, 76)])
    p.poly([(55, 54), (94, 34), (94, 76), (55, 98)])


def dock(p, small):
    p.rr(2, 64, 98, 88, 9)
    for x in (10, 39, 68):
        p.rr(x, 26, x + 22, 56, 5)


def joystick(p, small):
    p.rr(8, 70, 92, 96, 9)
    p.line([(50, 72), (50, 30)], 12)
    p.ell(32, 2, 68, 38)


def film(p, small):
    p.rr(4, 10, 96, 90, 6)
    n = 3 if small else 5
    w = 84 / n
    for i in range(n):
        p.cut_rr(10 + i * w, 16, 4 + (i + 1) * w, 26, 2)
        p.cut_rr(10 + i * w, 74, 4 + (i + 1) * w, 84, 2)
    p.cut_poly([(38, 34), (68, 50), (38, 66)])


def install(p, small):
    p.rr(6, 52, 94, 98, 8)
    p.cut_rr(20, 40, 80, 84, 4)
    p.line([(50, 2), (50, 44)], 15)
    p.poly([(26, 38), (74, 38), (50, 68)])


def globe(p, small):
    p.ell(4, 4, 96, 96)
    p.cut_line([(4, 50), (96, 50)], 6)
    p.cut_ring(28, 4, 72, 96, 6)
    if not small:
        p.cut_line([(14, 27), (86, 27)], 5)
        p.cut_line([(14, 73), (86, 73)], 5)


def swap(p, small):
    p.line([(8, 30), (70, 30)], 12)
    p.poly([(64, 8), (96, 30), (64, 52)])
    p.line([(30, 70), (92, 70)], 12)
    p.poly([(36, 48), (4, 70), (36, 92)])


def gauge(p, small):
    p.ring(2, 16, 98, 112, 14)
    p.cut_rr(-4, 66, 104, 120)
    p.line([(50, 62), (74, 32)], 9)
    p.ell(40, 52, 60, 72)
    p.rr(2, 62, 98, 72, 4)


def flask(p, small):
    p.poly([(38, 6), (62, 6), (62, 38), (92, 92), (8, 92), (38, 38)])
    p.rr(30, 2, 70, 12, 4)
    p.cut_poly([(47, 14), (53, 14), (53, 42), (66, 64), (34, 64), (47, 42)])
    p.rr(8, 88, 92, 98, 5)


MOTIFS = {f.__name__: f for f in (
    sliders, wrench, hammer, commodity, gear, chip, archive, puzzle, rocketlaunch, monitor, printer, mount, keyboard,
    tag, star, code, scroll, letter, locale, openbook, layers, terminal, page, pages, link, hourglass, icon, pattern,
    question, flag, pointer, waveform, cube, dock, joystick, film, install, globe, swap, gauge, flask)}


def stamp(f, motif, zone, small):
    """MOTIF's mark on the flat canvas f, in zone (512 units), white over a dark edge a little below it"""
    p = Pen(f.S, zone)
    MOTIFS[motif](p, small)
    a = p.f.img.getchannel("A")
    if not small:                       # at 32 px the edge would be a part of a pixel: it only muddies the mark
        off = max(1, round(8 * f.S / 512))
        under = Image.new("RGBA", f.img.size, UNDER + (255,))
        under.putalpha(ImageChops.offset(a, 0, off).point(lambda v: int(v * 0.75)))
        f.img = Image.alpha_composite(f.img, under)
    f.img = Image.alpha_composite(f.img, p.f.img)
