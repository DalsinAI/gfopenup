#!/usr/bin/env python3
"""Icons for the Open apps, drawn to sit with Boxie's isometric set (the
modern pack's drawers) with an OS 4.1 finish: soft top light, a gloss on the
upper faces, a dark outline. Drawn four times over at 64 x 64 and reduced.

    python3 tools/open_icons_art.py icons/open    # writes NAME.png and NAME-selected.png (needs Pillow)

Every icon has two frames, as docs/ICONS.md asks: NAME.png at rest and
NAME-selected.png, the same picture with something in it moved (a drawer
pulled open, the play button turned to pause), which Workbench shows when the
icon is clicked. Both frames are cropped and scaled alike, so nothing jumps
but the part that moves. The pack build (build_pack.py, open_icons) turns
them into GlowIcons.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

SIZE = 64
S = 4
PAD = 20                 # room round the 64-unit frame; done() crops to what was drawn
W = (SIZE + 2 * PAD) * S


class TDraw:
    """ImageDraw in 64-unit-frame pixels, moved into the padded canvas."""

    def __init__(self, img):
        self.d = ImageDraw.Draw(img)

    @staticmethod
    def _m(xy):
        o = PAD * S
        if xy and isinstance(xy[0], (tuple, list)):
            return [(x + o, y + o) for x, y in xy]
        return [v + o for v in xy]

    def polygon(self, xy, **kw):
        self.d.polygon(self._m(xy), **kw)

    def line(self, xy, **kw):
        self.d.line(self._m(xy), **kw)

    def ellipse(self, xy, **kw):
        self.d.ellipse(self._m(xy), **kw)

    def arc(self, xy, a, b, **kw):
        self.d.arc(self._m(xy), a, b, **kw)


def shade(c, k):
    return tuple(max(0, min(255, round(v * k))) for v in c[:3]) + ((c[3],) if len(c) > 3 else (255,))


def mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3)) + (255,)


class Pic:
    """One icon: isometric drawing in 64-pixel units, origin at the bottom centre."""

    def __init__(self, ox=32, oy=56):
        self.img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
        self.d = TDraw(self.img)
        self.ox, self.oy = ox, oy

    # 2:1 isometric: x to the right-down, y to the left-down, z up
    def p(self, x, y, z):
        return ((self.ox + (x - y)) * S, (self.oy + (x + y) / 2 - z) * S)

    def poly(self, pts3, fill, outline=None, width=1):
        pts = [self.p(*q) for q in pts3]
        self.d.polygon(pts, fill=fill)
        if outline:
            self.d.line(pts + [pts[0]], fill=outline, width=int(width * S), joint="curve")

    def flat(self, pts2, fill, outline=None, width=1):
        pts = [(x * S, y * S) for x, y in pts2]
        self.d.polygon(pts, fill=fill)
        if outline:
            self.d.line(pts + [pts[0]], fill=outline, width=int(width * S), joint="curve")

    def line(self, a, b, fill, width=1):
        self.d.line([self.p(*a), self.p(*b)], fill=fill, width=int(width * S))

    def box(self, x0, y0, z0, w, d, h, colour, outline=True, gloss=True):
        """A block from (x0, y0, z0); returns its three face colours."""
        top, left, right = shade(colour, 1.22), colour, shade(colour, 0.72)
        ink = shade(colour, 0.32)
        x1, y1, z1 = x0 + w, y0 + d, z0 + h
        self.poly([(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)], left)             # front-left face (y = y1)
        self.poly([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], right)            # front-right face (x = x1)
        self.poly([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], top)              # top
        if gloss:                                                                              # OS 4.1's sheen on the top
            self.poly([(x0, y0, z1), (x0 + w * 0.55, y0, z1), (x0, y0 + d * 0.55, z1)], shade(top, 1.12))
            self.poly([(x0, y1, z1), (x0 + w * 0.45, y1, z1), (x0 + w * 0.45, y1, z1 - h * 0.16), (x0, y1, z1 - h * 0.16)],
                      shade(left, 1.12))
        if outline:
            for a, b in [((x0, y1, z0), (x1, y1, z0)), ((x1, y1, z0), (x1, y0, z0)), ((x1, y0, z0), (x1, y0, z1)),
                         ((x1, y0, z1), (x0, y0, z1)), ((x0, y0, z1), (x0, y1, z1)), ((x0, y1, z1), (x0, y1, z0))]:
                self.line(a, b, ink, 1)
            self.line((x0, y1, z1), (x1, y1, z1), shade(top, 0.8), 0.6)
            self.line((x1, y1, z1), (x1, y0, z1), shade(top, 0.8), 0.6)
            self.line((x1, y1, z1), (x1, y1, z0), shade(left, 0.6), 0.6)
        return top, left, right

    def on_left(self, x, z, y):
        """A point on a front-left face at depth y."""
        return (x, y, z)

    def shadow(self, x0, y0, w, d):
        sh = Image.new("RGBA", (W, W), (0, 0, 0, 0))
        dd = ImageDraw.Draw(sh)
        pts = [self.p(x0, y0, 0), self.p(x0 + w, y0, 0), self.p(x0 + w, y0 + d, 0), self.p(x0, y0 + d, 0)]
        pts = [(x + (3 + PAD) * S, y + (2 + PAD) * S) for x, y in pts]
        dd.polygon(pts, fill=(20, 20, 40, 120))
        sh = sh.filter(ImageFilter.GaussianBlur(2.5 * S))
        self.img = Image.alpha_composite(sh, self.img)
        self.d = TDraw(self.img)

    def bbox(self):
        return self.img.getchannel("A").point(lambda a: 255 if a > 24 else 0).getbbox()

    def done(self, margin: int = 2, box=None) -> Image.Image:
        """Cropped to what was drawn (or to box), fitted in 64 x 64, centred and sitting on the bottom."""
        pic = self.img.crop(box or self.bbox())
        room = (SIZE - 2 * margin) * S
        k = min(1.0, room / pic.width, room / pic.height)
        pic = pic.resize((max(1, round(pic.width * k)), max(1, round(pic.height * k))), Image.LANCZOS)
        canvas = Image.new("RGBA", (SIZE * S, SIZE * S), (0, 0, 0, 0))
        canvas.alpha_composite(pic, ((SIZE * S - pic.width) // 2, SIZE * S - margin * S - pic.height))
        return canvas.resize((SIZE, SIZE), Image.LANCZOS)


INK = (34, 30, 54, 255)
BLUE = (58, 112, 196, 255)
TEAL = (48, 150, 160, 255)
ORANGE = (232, 150, 52, 255)
GREEN = (78, 170, 82, 255)
RED = (206, 70, 64, 255)
PAPER = (240, 238, 228, 255)
GREY = (150, 156, 170, 255)
DARK = (64, 70, 92, 255)
GOLD = (236, 196, 72, 255)
PURPLE = (126, 92, 190, 255)


def window_slab(p: Pic, colour=BLUE, x0=-14, y0=6, w=30, d=4, h=34):
    """An upright window (a thin slab) facing front-left; returns face mappers."""
    p.box(x0, y0, 0, w, d, h, (210, 214, 224, 255))
    y = y0 + d
    p.poly([(x0, y, h - 6), (x0 + w, y, h - 6), (x0 + w, y, h), (x0, y, h)], colour)               # title bar
    p.poly([(x0 + w - 5, y, h - 5), (x0 + w - 1, y, h - 5), (x0 + w - 1, y, h - 1), (x0 + w - 5, y, h - 1)], shade(colour, 1.4))
    return y


def icon_openfiles(f=0):
    p = Pic()
    p.shadow(-14, 6, 30, 5)
    y = window_slab(p)
    p.line((1, y, 2), (1, y, 27), INK, 0.8)                                                         # between the panes
    for x0 in (-12, 3):
        p.poly([(x0, y, 20), (x0 + 7, y, 20), (x0 + 7, y, 25), (x0, y, 25)], ORANGE)              # a drawer
        p.poly([(x0, y, 25), (x0 + 3, y, 25), (x0 + 3, y, 26), (x0, y, 26)], ORANGE)
        for i, z in enumerate((16, 12, 8, 4)):
            p.line((x0, y, z), (x0 + 11 - 3 * (i % 2), y, z), DARK, 0.9)                          # files
    x0, z0 = ((2.5, 10.6), (-12.5, 14.6))[f]                                                        # the chosen one moves
    p.poly([(x0, y, z0), (x0 + 13, y, z0), (x0 + 13, y, z0 + 2.8), (x0, y, z0 + 2.8)], (90, 150, 240, 255))
    return p


def icon_openview(f=0):
    p = Pic()
    p.shadow(-12, 4, 28, 6)
    p.box(-12, 4, 0, 26, 4, 30, (196, 150, 84, 255))                                                # the frame
    y = 8
    p.poly([(-9, y, 3), (11, y, 3), (11, y, 27), (-9, y, 27)], (120, 180, 230, 255))               # sky
    p.poly([(-9, y, 3), (-1, y, 15), (5, y, 8), (11, y, 16), (11, y, 3)], (70, 150, 80, 255))      # hills
    p.poly([(4, y, 20), (7, y, 20), (7, y, 23), (4, y, 23)], GOLD)                                  # the sun
    # a magnifier in front
    cx, cy, r = ((44, 40, 9), (36, 30, 10))[f]                                                      # over the picture when chosen
    p.d.line([(cx * S + 5 * S, cy * S + 5 * S), (cx * S + 13 * S, cy * S + 13 * S)], fill=INK, width=4 * S)
    p.d.line([(cx * S + 5 * S, cy * S + 5 * S), (cx * S + 12 * S, cy * S + 12 * S)], fill=(150, 90, 40, 255), width=int(2.6 * S))
    p.d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], fill=(200, 230, 250, 200), outline=INK, width=int(1.6 * S))
    p.d.ellipse([(cx - r + 3) * S, (cy - r + 2) * S, (cx - 1) * S, (cy - 3) * S], fill=(255, 255, 255, 170))
    return p


def icon_openedit(f=0):
    p = Pic()
    p.shadow(-12, 2, 24, 26)
    p.box(-12, 2, 0, 24, 26, 3, PAPER, gloss=False)                                                 # a pad of paper
    for i in range(6):                                                                              # its lines
        y = 7 + i * 3.4
        p.line((-9, y, 3.05), (8 - (5 if i == 5 else 0), y, 3.05), (130, 150, 190, 255), 0.7)
    p.poly([(-12, 2, 3.05), (12, 2, 3.05), (12, 5, 3.05), (-12, 5, 3.05)], RED)                     # the binding
    # a pencil lying across it
    if f:                                                                                           # a line being written
        p.line((-9, 24, 3.05), (-2, 24, 3.05), (40, 60, 140, 255), 0.9)
    a, b = ((-2, 26, 4), (16, 6, 12)) if not f else ((-2, 22, 7), (16, 2, 15))
    ax, ay = p.p(*a)
    bx, by = p.p(*b)
    dx, dy = bx - ax, by - ay
    n = math.hypot(dx, dy)
    nx, ny = -dy / n * 2.4 * S, dx / n * 2.4 * S
    body = [(ax + nx, ay + ny), (bx + nx, by + ny), (bx - nx, by - ny), (ax - nx, ay - ny)]
    p.d.polygon(body, fill=GOLD, outline=INK)
    p.d.line([(ax, ay), (bx, by)], fill=shade(GOLD, 0.8), width=S)
    tip = (ax - dx / n * 7 * S, ay - dy / n * 7 * S)
    p.d.polygon([(ax + nx, ay + ny), (ax - nx, ay - ny), tip], fill=(240, 210, 160, 255), outline=INK)
    p.d.polygon([(ax + nx * .35 - dx / n * 4.6 * S, ay + ny * .35 - dy / n * 4.6 * S),
                 (ax - nx * .35 - dx / n * 4.6 * S, ay - ny * .35 - dy / n * 4.6 * S), tip], fill=INK)
    p.d.polygon([(bx + nx, by + ny), (bx - nx, by - ny), (bx - nx + dx / n * 4 * S, by - ny + dy / n * 4 * S),
                 (bx + nx + dx / n * 4 * S, by + ny + dy / n * 4 * S)], fill=(230, 120, 140, 255), outline=INK)
    return p


def icon_openwrite(f=0):
    """A page standing up, set like a document: a heading, text, a picture."""
    p = Pic()
    p.shadow(-12, 6, 26, 5)
    p.box(-12, 6, 0, 26, 3, 36, PAPER, gloss=False)                                                # the page
    y = 9.05
    p.poly([(-9, y, 30), (6, y, 30), (6, y, 33), (-9, y, 33)], (40, 70, 160, 255))                 # the heading
    for i, z in enumerate((26, 23, 20)):                                                           # a paragraph
        p.line((-9, y, z), (11 - (6 if i == 2 else 0), y, z), (110, 120, 140, 255), 0.8)
    p.poly([(1, y, 7), (11, y, 7), (11, y, 16), (1, y, 16)], (120, 180, 230, 255))                 # a picture in the text
    p.poly([(1, y, 7), (5, y, 12), (8, y, 9), (11, y, 13), (11, y, 7)], (70, 150, 80, 255))
    for z in (15, 12, 9):                                                                          # text beside it
        p.line((-9, y, z), (-1, y, z), (110, 120, 140, 255), 0.8)
    cz = (5.5, 5.5)[f]
    if f:                                                                                          # chosen: a line being typed
        p.line((-9, y, 5.5), (-3, y, 5.5), (110, 120, 140, 255), 0.8)
    cx = (-9, -2.6)[f]
    p.line((cx, y, cz - 1.6), (cx, y, cz + 1.6), RED, 0.9)                                         # the cursor
    return p


def icon_dock(f=0):
    p = Pic(oy=54)
    p.shadow(-18, 6, 34, 10)
    p.box(-18, 6, 0, 34, 10, 4, DARK)                                                               # the dock
    for i, c in enumerate((BLUE, ORANGE, GREEN, RED)):
        x = -16 + i * 8.2
        p.box(x, 9, 4, 6, 5, 9 + ((3, 6)[f] if i == 1 + f else 0), c)                               # its buttons; one pops up
    return p


def menu_panel(p, x0=-14, y=10, w=24, rows=5, top=36, hi=2):
    p.box(x0, y - 3, top - rows * 6 - 9, w, 3, rows * 6 + 4, (226, 228, 236, 255))
    for i in range(rows):
        z = top - 9 - i * 6
        if i == hi:
            p.poly([(x0 + 1, y, z - 2.5), (x0 + w - 1, y, z - 2.5), (x0 + w - 1, y, z + 2.5), (x0 + 1, y, z + 2.5)], BLUE)
        p.line((x0 + 3, y, z), (x0 + w - 6 - (i % 3) * 3, y, z), (255, 255, 255, 255) if i == hi else DARK, 0.9)


def icon_menus(f=0):
    p = Pic()
    p.shadow(-16, 4, 30, 8)
    p.box(-16, 2, 32, 32, 6, 5, BLUE)                                                               # the menu bar
    p.line((-12, 8, 34.5), (-6, 8, 34.5), (255, 255, 255, 255), 0.9)
    p.line((-2, 8, 34.5), (4, 8, 34.5), (255, 255, 255, 255), 0.9)
    menu_panel(p, -12, 9, 22, 4, 32, 1 + 2 * f)
    # the pointer, following the highlight down
    pts = [(x, y + 8 * f) for x, y in [(40, 34), (40, 48), (43.5, 45), (46, 50.5), (48, 49.5), (45.5, 44.2), (50, 44)]]
    p.flat(pts, (255, 255, 255, 255), INK, 1)
    return p


def icon_sound(f=0):
    p = Pic()
    p.shadow(-12, 4, 22, 14)
    p.box(-12, 4, 0, 22, 14, 30, (70, 72, 90, 255))                                                 # the cabinet
    y = 18
    for zc, r in ((20, 7.5), (7, 4.2)):                                                             # the cones
        pts = [(-1 + r * math.cos(t), y, zc + r * math.sin(t)) for t in [i * math.pi / 12 for i in range(24)]]
        p.poly(pts, (40, 40, 52, 255), (150, 156, 176, 255), 0.8)
        pts = [(-1 + r * .4 * math.cos(t), y, zc + r * .4 * math.sin(t)) for t in [i * math.pi / 12 for i in range(24)]]
        p.poly(pts, (110, 116, 140, 255))
    for k, r in enumerate((6, 10, 14, 18)[:3 + f]):                                                 # the sound, louder when chosen
        p.d.arc([(40 - r) * S, (26 - r) * S, (40 + r) * S, (26 + r) * S], -50, 50,
                fill=(60, 150, 230, 255 - k * (50 - 12 * f)), width=int(2 * S))
    return p


def icon_title(f=0):
    p = Pic(oy=54)
    p.shadow(-20, 10, 40, 6)
    p.box(-20, 10, 18, 40, 6, 7, (222, 226, 236, 255))                                              # the title bar
    y = 16
    p.poly([(-18, y, 19.5), (-12, y, 19.5), (-12, y, 23.5), (-18, y, 23.5)], (200, 60, 60, 255))    # the logo
    p.poly([(-15, y, 19.5), (-12, y, 19.5), (-12, y, 21.5), (-15, y, 21.5)], (60, 100, 200, 255))
    p.line((-9, y, 21.5), (2, y, 21.5), DARK, 1)                                                     # free memory
    p.box(-20, 10, 0, 40, 6, 18, (150, 176, 214, 255), gloss=False)                                 # the screen under it
    cx, cy = 46, 22                                                                                 # a clock
    p.d.ellipse([(cx - 10) * S, (cy - 10) * S, (cx + 10) * S, (cy + 10) * S], fill=PAPER, outline=INK, width=int(1.5 * S))
    (mx, my), (hx, hy) = (((0, -7), (5, 2)), ((7, 0), (2, 5)))[f]                                  # its hands move on
    p.d.line([(cx * S, cy * S), ((cx + mx) * S, (cy + my) * S)], fill=INK, width=int(1.4 * S))
    p.d.line([(cx * S, cy * S), ((cx + hx) * S, (cy + hy) * S)], fill=INK, width=int(1.4 * S))
    return p


def icon_look(f=0):
    p = Pic()
    y = window_slab(p, PURPLE, -16, 4, 30, 4, 32)
    for i, c in enumerate((RED, ORANGE, GOLD, GREEN, BLUE, PURPLE)):                               # swatches
        x = -13 + (i % 3) * 8
        z = 16 - (i // 3) * 9
        p.poly([(x, y, z), (x + 6, y, z), (x + 6, y, z + 6), (x, y, z + 6)], c, (255, 255, 255, 255) if f and i == 0 else INK,
               1.2 if f and i == 0 else 0.5)
    # a brush, dipping into the red when chosen
    dx, dy = (0, 0) if not f else (-5, 5)
    p.d.line([((40 + dx) * S, (54 + dy) * S), ((55 + dx) * S, (26 + dy) * S)], fill=INK, width=int(4 * S))
    p.d.line([((40 + dx) * S, (54 + dy) * S), ((55 + dx) * S, (26 + dy) * S)], fill=(160, 100, 50, 255), width=int(2.4 * S))
    p.d.polygon([((52 + dx) * S, (30 + dy) * S), ((58 + dx) * S, (33 + dy) * S), ((60 + dx) * S, (22 + dy) * S), ((56 + dx) * S, (18 + dy) * S)],
                fill=RED, outline=INK)
    return p


def icon_windows(f=0):
    p = Pic()
    p.shadow(-16, 2, 32, 16)
    window_slab(p, shade(BLUE, 0.7), -18, 0, 26, 3, 28)
    window_slab(p, TEAL, -12, 7, 26, 3, 30)
    x = -6 + 4 * f                                                                                  # the front one slides out
    y = window_slab(p, (BLUE, ORANGE)[f], x, 14, 26, 3, 32)
    for i in range(3):
        p.line((x + 3, y, 20 - i * 5), (x + 21 - i * 3, y, 20 - i * 5), DARK, 0.9)
    return p


def icon_types(f=0):
    p = Pic()
    p.shadow(-12, 6, 22, 4)
    for i, c in enumerate((GREEN, ORANGE, BLUE)):                                                   # three documents
        x = -14 + i * 8
        p.box(x, 4 + i * 3, 0, 16, 2, 26 - i * 2, PAPER, gloss=False)
        y = 6 + i * 3
        p.poly([(x + 1, y, 22 - i * 2 - 6), (x + 15, y, 22 - i * 2 - 6), (x + 15, y, 22 - i * 2 - 1), (x + 1, y, 22 - i * 2 - 1)], c)
    # an arrow to a program
    p.flat([(x + 5 * f, y) for x, y in [(40, 40), (50, 40), (50, 35), (60, 44), (50, 53), (50, 48), (40, 48)]], GREEN, INK, 1)
    return p


def icon_setup(f=0):
    p = Pic()
    p.shadow(-12, 6, 24, 20)
    p.box(-12, 6, 0, 24, 20, 18, (214, 168, 104, 255))                                              # a box
    p.box(-13, 5, 18, 26, 22, 3, (232, 192, 132, 255))                                              # its lid
    # a wand with a star
    p.d.line([(28 * S, 40 * S), (48 * S, 16 * S)], fill=INK, width=int(3.6 * S))
    p.d.line([(28 * S, 40 * S), (48 * S, 16 * S)], fill=(250, 250, 250, 255), width=int(1.8 * S))
    cx, cy, R, r = (50, 13, 9, 3.8) if not f else (50, 13, 11.5, 4.6)                              # the star flares and sparkles
    turn = math.pi / 10 * f
    star = [(cx + (R if k % 2 == 0 else r) * math.cos(-math.pi / 2 + turn + k * math.pi / 5),
             cy + (R if k % 2 == 0 else r) * math.sin(-math.pi / 2 + turn + k * math.pi / 5)) for k in range(10)]
    p.flat(star, GOLD, INK, 1)
    if f:
        for sx, sy, k in ((34, 8, 2.6), (60, 26, 2.2), (38, 24, 1.8)):
            p.flat([(sx, sy - k), (sx + k * .35, sy - k * .35), (sx + k, sy), (sx + k * .35, sy + k * .35),
                    (sx, sy + k), (sx - k * .35, sy + k * .35), (sx - k, sy), (sx - k * .35, sy - k * .35)], (255, 244, 170, 255), INK, 0.5)
    return p


def icon_play(f=0):
    p = Pic()
    p.shadow(-14, 6, 30, 5)
    p.box(-14, 6, 0, 30, 4, 30, DARK)                                                               # a screen
    y = 10
    for x in range(-12, 15, 5):                                                                     # film holes
        p.poly([(x, y, 26), (x + 2.6, y, 26), (x + 2.6, y, 28.5), (x, y, 28.5)], (230, 236, 246, 255))
        p.poly([(x, y, 1.5), (x + 2.6, y, 1.5), (x + 2.6, y, 4), (x, y, 4)], (230, 236, 246, 255))
    p.poly([(-12, y, 6), (14, y, 6), (14, y, 24), (-12, y, 24)], (70, 120, 210, 255))
    pts = [(-4 + 7 * math.cos(t), y, 15 + 7 * math.sin(t)) for t in [i * math.pi / 12 for i in range(24)]]
    p.poly(pts, GREEN, INK, 0.8)
    if not f:                                                                                       # play, and pause when chosen
        p.poly([(-6.5, y, 19), (-6.5, y, 11), (0, y, 15)], (255, 255, 255, 255))
    else:
        for x in (-7.5, -2.5):
            p.poly([(x, y, 11), (x + 3, y, 11), (x + 3, y, 19), (x, y, 19)], (255, 255, 255, 255))
    return p


def icon_browser(f=0):
    p = Pic()
    cx, cy, r = 32, 28, 17
    sh = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse([(PAD + cx - 15) * S, (PAD + cy + 17) * S, (PAD + cx + 17) * S, (PAD + cy + 23) * S], fill=(20, 20, 40, 120))
    p.img = Image.alpha_composite(sh.filter(ImageFilter.GaussianBlur(2 * S)), p.img)
    p.d = TDraw(p.img)
    img = p.img
    ring = (226, 140, 50, 255)
    p.d.ellipse([(cx - 26) * S, (cy - 6) * S, (cx + 26) * S, (cy + 9) * S], outline=INK, width=int(4.6 * S))   # back of the ring
    p.d.ellipse([(cx - 26) * S, (cy - 6) * S, (cx + 26) * S, (cy + 9) * S], outline=ring, width=int(2.6 * S))
    g = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    gd = TDraw(g)
    for k in range(r * S, 0, -S):                                                                   # a shaded sphere
        t = k / (r * S)
        c = mix((40, 70, 170), (120, 190, 250), 1 - t)
        off = (1 - t) * 6 * S
        gd.ellipse([cx * S - k - off, cy * S - k - off, cx * S + k - off, cy * S + k - off], fill=c)
    gd.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], outline=INK, width=int(1.4 * S))
    for lat in (-0.5, 0, 0.5):                                                                      # lines on it
        yy = cy + lat * r
        hw = r * math.sqrt(1 - lat * lat)
        gd.arc([(cx - hw) * S, (yy - 3) * S, (cx + hw) * S, (yy + 3) * S], 0, 180, fill=(220, 240, 255, 200), width=S)
    for m in ((6,), (11, 1.5))[f]:                                                                  # the meridians turn
        gd.ellipse([(cx - m) * S, (cy - r) * S, (cx + m) * S, (cy + r) * S], outline=(220, 240, 255, 200), width=S)
    gd.ellipse([(cx - 11) * S, (cy - 13) * S, (cx - 3) * S, (cy - 6) * S], fill=(255, 255, 255, 150))  # the gloss
    img = Image.alpha_composite(img, g)
    d = TDraw(img)
    d.arc([(cx - 26) * S, (cy - 6) * S, (cx + 26) * S, (cy + 9) * S], 0, 180, fill=INK, width=int(4.6 * S))  # front of the ring
    d.arc([(cx - 26) * S, (cy - 6) * S, (cx + 26) * S, (cy + 9) * S], 0, 180, fill=ring, width=int(2.6 * S))
    p.img = img
    return p


def icon_compress(f=0):
    p = Pic()
    p.shadow(-14, 4, 22, 20)
    p.box(-14, 4, 0, 22, 20, 18, (214, 168, 104, 255))
    p.box(-15, 3, 18 + 3 * f, 24, 22, 4, (232, 192, 132, 255))                                      # the lid lifts
    p.flat([(x + 4 * f, y) for x, y in [(36, 30), (46, 30), (46, 24), (60, 36), (46, 48), (46, 42), (36, 42)]], GREEN, INK, 1)
    return p


def icon_printer(f=0):
    p = Pic()
    p.shadow(-16, 4, 30, 20)
    p.box(-10, 8, 18, 18, 4, 14, PAPER, gloss=False)                                                # paper in
    p.box(-16, 4, 0, 30, 20, 18, (210, 212, 222, 255))                                              # the printer
    p.poly([(-12, 24, 6), (10, 24, 6), (10, 24, 10), (-12, 24, 10)], DARK)                          # the slot
    out = (30, 36)[f]                                                                               # the page comes further out
    p.poly([(-11, 24, 0), (9, 24, 0), (11, out, -2), (-9, out, -2)], PAPER, INK, 0.6)
    if f:
        for k in (1, 2):
            p.line((-7, 24 + 4 * k, -0.7 * k), (5, 24 + 4 * k, -0.7 * k), (60, 60, 80, 255), 0.6)
    p.poly([(9, 24, 14), (12, 24, 14), (12, 24, 16), (9, 24, 16)], (GREEN, (120, 240, 120, 255))[f])   # its light
    return p


def icon_install(f=0):
    p = Pic()
    p.shadow(-12, 6, 24, 20)
    p.box(-12, 6, 0, 24, 20, 18, BLUE)
    p.box(-13, 5, 18, 26, 22, 3, shade(BLUE, 1.15))
    p.flat([(x, y + 6 * f) for x, y in [(32, 34), (44, 22), (37, 22), (37, 8), (27, 8), (27, 22), (20, 22)]], GREEN, INK, 1)   # down into it
    return p


def icon_zune(f=0):
    p = Pic()
    y = window_slab(p, (90, 90, 110, 255), -14, 6, 28, 4, 30)
    for i in range(3):
        p.poly([(-11, y, 18 - i * 6), (11, y, 18 - i * 6), (11, y, 22 - i * 6), (-11, y, 22 - i * 6)],
               (90, 150, 240, 255) if f and i == 1 else (196, 200, 214, 255), INK, 0.5)                # a gadget pressed
    return p


def drawer_pic(f=0):
    """Boxie's drawer, a blue filing cabinet; chosen, its top drawer is pulled open."""
    p = Pic()
    p.shadow(-10, 4, 20, 14)
    c = (54, 110, 160, 255)
    p.box(-10, 4, 0, 20, 14, 36, c)
    y = 18
    for z0 in (3, 19):
        p.poly([(-8, y, z0), (8, y, z0), (8, y, z0 + 14), (-8, y, z0 + 14)], shade(c, 0.85), shade(c, 0.4), 0.7)
        p.poly([(-3, y, z0 + 9), (3, y, z0 + 9), (3, y, z0 + 11), (-3, y, z0 + 11)], shade(c, 0.45))
    if f:
        p.box(-8, y - 2, 19, 16, 9, 14, shade(c, 0.9))                                              # out it comes
        p.poly([(-7, y - 1, 33), (7, y - 1, 33), (7, y + 6, 33), (-7, y + 6, 33)], shade(c, 0.35))   # its inside
        for k in range(3):                                                                          # files in it
            p.box(-6 + k * 4.5, y + k * 0.5, 30, 3.4, 5, 5, (PAPER, GOLD, PAPER)[k], gloss=False)
        p.poly([(-3, y + 7, 28), (3, y + 7, 28), (3, y + 7, 30), (-3, y + 7, 30)], shade(c, 0.45))
    return p


def frames(fn) -> tuple[Image.Image, Image.Image]:
    """An icon's two frames, cropped to the box both fill so only the moving part moves."""
    a, b = fn(0), fn(1)
    ba, bb = a.bbox(), b.bbox()
    box = (min(ba[0], bb[0]), min(ba[1], bb[1]), max(ba[2], bb[2]), max(ba[3], bb[3]))
    return a.done(box=box), b.done(box=box)


def drawer(badge=None):
    """The drawer with a badge at its foot like Boxie's; the badge's own frame plays too."""
    out = list(frames(drawer_pic))
    if badge:
        for i, pic in enumerate(frames(badge)):
            out[i].alpha_composite(pic.resize((30, 30), Image.LANCZOS), (0, 34))
    return tuple(out)


ICONS = {
    "OpenFiles": icon_openfiles, "OpenView": icon_openview, "OpenEdit": icon_openedit, "OpenWrite": icon_openwrite, "Dock": icon_dock,
    "Menus": icon_menus, "Sound": icon_sound, "TitleBar": icon_title, "Look": icon_look, "Windows": icon_windows,
    "OpenTypes": icon_types, "OpenUp-Setup": icon_setup, "OpenPlay": icon_play, "OpenBrowser": icon_browser,
    "OpenCompress": icon_compress, "Printers and Queue": icon_printer, "Install-OpenUp": icon_install, "ZuneTest": icon_zune,
    "drawer-OpenUp": lambda: drawer(icon_install), "drawer-OpenPrint": lambda: drawer(icon_printer),
    "drawer-OpenBrowser": lambda: drawer(icon_browser), "drawer-OpenWrite": lambda: drawer(icon_openwrite),
}


def harden(im: Image.Image) -> Image.Image:
    """For a GlowIcon: every pixel solid or see-through."""
    px = im.load()
    out = im.copy()
    o = out.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            o[x, y] = (r, g, b, 255) if a >= 110 else (0, 0, 0, 0)
    return out


if __name__ == "__main__":
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    for name, fn in ICONS.items():
        rest, chosen = fn() if name.startswith("drawer-") else frames(fn)
        rest.save(out / f"{name}.png")
        chosen.save(out / f"{name}-selected.png")
