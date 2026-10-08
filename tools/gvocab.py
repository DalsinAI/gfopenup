#!/usr/bin/env python3
"""The things icons are made of, in the Gorilla style: pages, drawers, disks,
windows, globes, plugs, notes and a hundred more, each drawn in its own
64-unit frame so it can be a whole icon or a badge on another one.

    from gvocab import Pen, OBJECTS
    ic = Icon(); p = Pen(ic)
    slots = OBJECTS["page"](p, sel=False)        # draws a page; returns where things go on it
    OBJECTS["globe"](p.slot(slots, "face"), sel=False)

Every object is obj(pen, sel=False, **options) and returns its slots:
"face" (on the thing itself: a page's text, a monitor's screen) and "badge"
(at its foot, on the right, overlapping), each (x, y, scale) in the
object's frame. sel is the chosen frame: something moves (ICONS.md).
MIT, Copyright (c) 2026 AmigaChrome Team. Our own drawing; nothing traced.
"""
from __future__ import annotations

import math
import os
from functools import lru_cache

from PIL import ImageDraw, ImageFont

import gstyle as g
from gstyle import rgb, lighter, darker, mix

# ---- colours -----------------------------------------------------------------------------------

WHITE = (255, 255, 255)
INK = g.INK
PAPER_HI, PAPER_LO = g.GREY_HI, rgb('#CFD2D7')
STEEL_HI, STEEL_LO = rgb('#E9ECF0'), rgb('#8A929D')
METAL_HI, METAL_LO = rgb('#F4F6F8'), rgb('#9EA6B0')
BEIGE_HI, BEIGE_LO = rgb('#F0EFE2'), rgb('#A9A78A')
PCB_HI, PCB_LO = rgb('#3FA45A'), rgb('#1C5A2C')
CHIP_HI, CHIP_LO = rgb('#5A616B'), rgb('#1F2329')
GOLDPIN = rgb('#D9B24A')
SKY_HI, SKY_LO = rgb('#9FD0FF'), rgb('#E9F4FF')
SCREEN_HI, SCREEN_LO = rgb('#3E79C8'), rgb('#1B3D6E')
PURPLE, PURPLE_LO = rgb('#9B6BD6'), rgb('#5A3290')
TEAL, TEAL_LO = rgb('#3FB8B0'), rgb('#1E6E69')
PINK, PINK_LO = rgb('#F28CB0'), rgb('#B04A72')
BROWN, BROWN_LO = rgb('#B9824A'), rgb('#6E4520')
AMBER = rgb('#FFB020')
LED_ON, LED_OFF = rgb('#7CFF6A'), rgb('#2E5A2A')
PCGREY_HI, PCGREY_LO = rgb('#E2E0D8'), rgb('#9C998C')

COLOURS = {
    "blue": (g.BLUE, g.BLUE_LO), "green": (g.GREEN, g.GREEN_LO), "orange": (g.ORANGE, g.ORANGE_LO), "red": (g.RED, g.RED_LO),
    "gold": (g.GOLD, g.GOLD_LO), "purple": (PURPLE, PURPLE_LO), "teal": (TEAL, TEAL_LO), "pink": (PINK, PINK_LO),
    "grey": (STEEL_HI, STEEL_LO), "dark": (g.DARK, g.DARK_LO), "brown": (BROWN, BROWN_LO), "white": (PAPER_HI, PAPER_LO),
    "beige": (BEIGE_HI, BEIGE_LO), "pcgrey": (PCGREY_HI, PCGREY_LO),
}


def colour(name, default="blue"):
    return COLOURS.get(name or default, COLOURS[default])


FONT_DIRS = ("/usr/share/fonts/truetype/dejavu", "/usr/share/fonts/dejavu", "/usr/local/share/fonts")


@lru_cache(maxsize=64)
def font(px: int, bold=True, mono=False, serif=False):
    name = "DejaVuSans" + ("Mono" if mono else "") if not serif else "DejaVuSerif"
    name += "-Bold.ttf" if bold else ".ttf"
    for d in FONT_DIRS:
        p = os.path.join(d, name)
        if os.path.exists(p):
            return ImageFont.truetype(p, px)
    return ImageFont.load_default(size=px)


# ---- the pen: an Icon seen through a move and a scale ------------------------------------------


class Pen:
    def __init__(self, ic, ox=0.0, oy=0.0, s=1.0, k=1.0):
        self.ic, self.ox, self.oy, self.s, self.k = ic, ox, oy, s, k

    def T(self, x, y):
        return (self.ox + x * self.s, self.oy + y * self.s)

    def sub(self, x, y, s, k=None):
        return Pen(self.ic, *self.T(x, y), self.s * s, self.k if k is None else k)

    def slot(self, slots, which="badge", k=None):
        x, y, s = slots[which]
        return self.sub(x, y, s, k=(0.75 if which == "face" else 1.0) if k is None else k)

    # masks, in the object's 64-unit frame
    def poly(self, pts):
        return g.poly([self.T(*p) for p in pts])

    def rrect(self, x0, y0, x1, y1, r):
        a, b = self.T(x0, y0), self.T(x1, y1)
        return g.rrect(a[0], a[1], b[0], b[1], max(0.01, r * self.s))

    def rect(self, x0, y0, x1, y1):
        a, b = self.T(x0, y0), self.T(x1, y1)
        return g.rect(a[0], a[1], b[0], b[1])

    def ellipse(self, x0, y0, x1, y1):
        a, b = self.T(x0, y0), self.T(x1, y1)
        return g.ellipse(a[0], a[1], b[0], b[1])

    def stroke(self, pts, w):
        return g.stroke([self.T(*p) for p in pts], w * self.s)

    def arc(self, x0, y0, x1, y1, a0, a1, w):
        a, b = self.T(x0, y0), self.T(x1, y1)
        return g.arc(a[0], a[1], b[0], b[1], a0, a1, w * self.s)

    def ring(self, x0, y0, x1, y1, w):
        a, b = self.T(x0, y0), self.T(x1, y1)
        return g.ring(a[0], a[1], b[0], b[1], w * self.s)

    def text(self, x, y, h, s, bold=True, mono=False, serif=False):
        """A mask of text, centred on (x, y), h units tall."""
        m = g.blank()
        px = max(4, round(h * self.s * g.S * 1.25))
        cx, cy = self.T(x, y)
        ImageDraw.Draw(m).text((g.P(cx), g.P(cy)), s, fill=255, font=font(px, bold, mono, serif), anchor="mm")
        return m

    # fills
    def lin(self, p0, p1, c0, c1, mid=None):
        return g.linear(self.T(*p0), self.T(*p1), c0, c1, mid)

    def rad(self, c, r, c0, c1):
        return g.radial(self.T(*c), r * self.s, c0, c1)

    def add(self, m, fill, outline=2.3, gloss=0.0):
        ow = 0 if not outline else max(0.75, outline * min(1.0, self.s ** 0.45) * self.k)
        return self.ic.add(m, fill, outline=ow, gloss=gloss)

    def detail(self, m, fill):
        return self.ic.add(m, fill, outline=0)


def slots(face=(16, 16, 0.5), badge=(30, 30, 0.52)):
    """Where things go on an object: on its face, as a badge at its foot on the
    right, and (for the prefs mark) at its foot on the left."""
    return {"face": face, "badge": badge, "bl": (-2, 32, 0.46), "tr": (32, -2, 0.46)}


def star_pts(cx, cy, r, inner=0.45, n=5, rot=-90):
    pts = []
    for k in range(n * 2):
        a = math.radians(rot + k * 180 / n)
        rr = r if k % 2 == 0 else r * inner
        pts.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
    return pts


def lines(p, x0, y0, x1, n, gap=5.0, h=1.6, c=g.CONTENT, short=None):
    for i in range(n):
        x = x1 if not (short and i == n - 1) else x0 + (x1 - x0) * short
        p.detail(p.rect(x0, y0 + i * gap, x, y0 + i * gap + h), c)


OBJECTS: dict = {}


def obj(name):
    def reg(fn):
        OBJECTS[name] = fn
        return fn
    return reg


# ---- documents ------------------------------------------------------------------------------------


@obj("page")
def page(p, sel=False, tint=None, lines_on=False, holes=False, corner=True, **kw):
    """A sheet with a folded corner; chosen, the corner lifts further."""
    f = 9 if not sel else 12
    hi = lighter(colour(tint)[0], 0.85) if tint else PAPER_HI
    pts = [(12, 4), (52 - f, 4), (52, 4 + f), (52, 60), (12, 60)] if corner else [(12, 4), (52, 4), (52, 60), (12, 60)]
    p.add(p.poly(pts), p.lin((12, 4), (52, 60), hi, PAPER_LO), gloss=0.15)
    if corner:
        p.add(p.poly([(52 - f, 4), (52, 4 + f), (52 - f, 4 + f)]), p.lin((52 - f, 4), (52, 4 + f), WHITE, rgb('#B9BDC4')), outline=1.4)
    if tint:
        p.detail(p.rect(12.6, 9, 52 - f - 1 if corner else 51.4, 15), colour(tint)[0])
    if holes:
        for hy in (18, 46):
            p.detail(p.ellipse(14.5, hy - 1, 16.5, hy + 1), rgb('#7C8088'))
    if lines_on:
        lines(p, 18, 20, 46, 7, gap=5.2, short=0.6)
    return slots(face=(16, 18, 0.5), badge=(30, 30, 0.52))


@obj("lines")
def text_lines(p, sel=False, **kw):
    lines(p, 6, 10, 58, 8, gap=6, h=3, short=0.5 if not sel else 0.8)
    return slots()


@obj("scroll")
def scroll(p, sel=False, **kw):
    """A rolled script; chosen, it unrolls a little more."""
    bot = 52 if not sel else 58
    p.add(p.rect(14, 10, 50, bot), p.lin((14, 10), (50, bot), rgb('#FBF3DA'), rgb('#D9C79A')), gloss=0.1)
    p.add(p.rrect(8, 4, 56, 13, 4.5), p.lin((0, 4), (0, 13), rgb('#FBF3DA'), rgb('#C9B47E')), gloss=0.3)
    p.add(p.rrect(8, bot - 4, 56, bot + 5, 4.5), p.lin((0, bot - 4), (0, bot + 5), rgb('#FBF3DA'), rgb('#C9B47E')), gloss=0.3)
    return slots(face=(17, 15, 0.47), badge=(30, 30, 0.52))


@obj("book")
def book(p, sel=False, tint="blue", **kw):
    """A closed book; chosen, its ribbon slips out."""
    c, lo = colour(tint)
    p.add(p.rect(14, 8, 52, 58), p.lin((14, 8), (52, 58), WHITE, PAPER_LO), outline=2.0)
    p.add(p.rrect(10, 4, 48, 56, 2.5), p.lin((10, 4), (48, 56), lighter(c, 0.2), lo), gloss=0.25)
    p.detail(p.rect(12, 4, 15, 56), darker(c, 0.25))
    rb = 62 if sel else 59
    p.add(p.poly([(38, 54), (42, 54), (42, rb), (40, rb - 2), (38, rb)]), g.RED, outline=1.2)
    return slots(face=(15, 14, 0.5), badge=(30, 30, 0.52))


@obj("books")
def books(p, sel=False, **kw):
    for i, t in enumerate(("red", "blue", "green")):
        c, lo = colour(t)
        x = 8 + i * 15
        y = 8 if not (sel and i == 2) else 3
        p.add(p.rrect(x, y + i * 2, x + 13, 58, 2), p.lin((x, y), (x + 13, 58), lighter(c, 0.2), lo), gloss=0.25)
        p.detail(p.rect(x + 2, y + 12 + i * 2, x + 11, y + 15 + i * 2), lighter(c, 0.6))
    return slots(face=(16, 16, 0.5), badge=(30, 30, 0.52))


@obj("clipboard")
def clipboard(p, sel=False, **kw):
    p.add(p.rrect(10, 8, 54, 60, 3), p.lin((10, 8), (54, 60), rgb('#C99A5E'), rgb('#7A5224')), gloss=0.2)
    p.add(p.rect(15, 14, 49, 56), p.lin((15, 14), (49, 56), WHITE, PAPER_LO), outline=1.4)
    p.add(p.rrect(22, 4, 42, 13, 2), p.lin((22, 4), (42, 13), METAL_HI, METAL_LO), gloss=0.4)
    lines(p, 19, 22, 45, 5 if not sel else 6, gap=5)
    return slots(face=(17, 20, 0.47), badge=(30, 30, 0.52))


@obj("notepad")
def notepad(p, sel=False, **kw):
    page(p, sel=False, corner=False)
    for i in range(5):
        x = 16 + i * 7.5
        p.add(p.rrect(x, 0, x + 3, 9, 1.5), p.lin((x, 0), (x + 3, 9), STEEL_HI, STEEL_LO), outline=1.1)
    lines(p, 17, 18, 47, 7 if sel else 6, gap=5.5, short=0.55)
    return slots(face=(16, 18, 0.5))


@obj("calendar")
def calendar(p, sel=False, **kw):
    p.add(p.rrect(8, 8, 56, 58, 3), p.lin((8, 8), (56, 58), WHITE, PAPER_LO), gloss=0.15)
    p.detail(p.rect(8.6, 8.6, 55.4, 20), g.RED)
    for x in (18, 46):
        p.add(p.rrect(x - 2, 3, x + 2, 13, 1.5), p.lin((x, 3), (x, 13), STEEL_HI, STEEL_LO), outline=1.0)
    for r in range(4):
        for c in range(6):
            on = (r, c) == ((2, 3) if sel else (1, 2))
            p.detail(p.rect(12 + c * 7, 24 + r * 7.5, 17 + c * 7, 29 + r * 7.5), g.RED if on else rgb('#B6BBC2'))
    return slots(face=(16, 22, 0.47))


@obj("grid")
def grid(p, sel=False, tint="green", commas=False, **kw):
    """A spreadsheet: a page of cells, a header row; chosen, another cell is picked."""
    c = colour(tint)[0]
    p.add(p.rrect(6, 6, 58, 58, 2.5), p.lin((6, 6), (58, 58), WHITE, PAPER_LO), gloss=0.12)
    p.detail(p.rect(6.6, 6.6, 57.4, 14), lighter(c, 0.35))
    for i in range(1, 4):
        p.detail(p.rect(6.6 + i * 13, 6.6, 7.4 + i * 13, 57.4), rgb('#B6BBC2'))
    for j in range(1, 7):
        p.detail(p.rect(6.6, 6.6 + j * 7.4, 57.4, 7.2 + j * 7.4), rgb('#B6BBC2'))
    r, cc = (4, 2) if sel else (2, 1)
    p.detail(p.rect(7 + cc * 13, 7 + r * 7.4, 6.8 + (cc + 1) * 13, 6.8 + (r + 1) * 7.4), c)
    if commas:
        for j in range(2, 7):
            for i in range(4):
                p.detail(p.text(13 + i * 13, 11 + j * 7.4, 5, ","), rgb('#5A616B'))
    return slots(face=(16, 16, 0.5))


@obj("cardindex")
def cardindex(p, sel=False, **kw):
    for i in range(3):
        y = 8 + i * 7 - (4 if sel and i == 1 else 0)
        p.add(p.rrect(8 + i * 3, y, 52 + i * 3, y + 30, 2), p.lin((0, y), (0, y + 30), WHITE, PAPER_LO), outline=1.6)
        p.detail(p.rect(12 + i * 3, y + 3, 30 + i * 3, y + 5), g.BLUE if i == 2 else g.CONTENT)
    p.add(p.rrect(4, 30, 60, 58, 3), p.lin((4, 30), (60, 58), lighter(g.BLUE, 0.2), g.BLUE_LO), gloss=0.25)
    return slots(face=(18, 30, 0.45))


@obj("newspaper")
def newspaper(p, sel=False, **kw):
    p.add(p.rect(8, 8 if not sel else 5, 56, 58), p.lin((8, 8), (56, 58), WHITE, PAPER_LO), gloss=0.12)
    p.detail(p.rect(12, 12, 52, 18), INK)
    p.add(p.rect(12, 22, 30, 36), p.lin((0, 22), (0, 36), SKY_HI, SKY_LO), outline=0.8)
    lines(p, 33, 22, 52, 3, gap=4.5)
    lines(p, 12, 40, 30, 4, gap=4.5)
    lines(p, 33, 40, 52, 4, gap=4.5)
    return slots(face=(16, 20, 0.47))


@obj("easel")
def easel(p, sel=False, **kw):
    p.add(p.stroke([(32, 6), (14, 60)], 3), BROWN)
    p.add(p.stroke([(32, 6), (50, 60)], 3), BROWN)
    p.add(p.rect(8, 10, 56, 42), p.lin((8, 10), (56, 42), WHITE, PAPER_LO), gloss=0.15)
    bars = (10, 16, 22) if not sel else (14, 22, 18)
    for i, (h, c) in enumerate(zip(bars, (g.BLUE, g.GREEN, g.ORANGE))):
        p.add(p.rect(16 + i * 11, 38 - h, 24 + i * 11, 38), c, outline=1.0)
    return slots(face=(16, 12, 0.45))


# ---- storage ----------------------------------------------------------------------------------------


@obj("drawer")
def drawer(p, sel=False, tint=None, **kw):
    """An open tray seen from a little above, its handle on the front; chosen,
    papers show in it."""
    hi, lo = (BEIGE_HI, BEIGE_LO) if not tint else (lighter(colour(tint)[0], 0.55), colour(tint)[1])
    p.add(p.poly([(3, 26), (61, 26), (56, 13), (8, 13)]), p.lin((0, 13), (0, 26), darker(lo, 0.1), darker(lo, 0.4)))
    p.detail(p.poly([(10, 15.5), (54, 15.5), (58, 24.5), (6, 24.5)]), darker(lo, 0.55))
    if sel:
        for i, c in enumerate((PAPER_HI, rgb('#DDE7F4'), rgb('#F6E7A6'))):
            x = 12 + i * 12
            p.add(p.rect(x, 8 - i, x + 14, 24), c, outline=1.2)
            p.detail(p.rect(x + 3, 11 - i, x + 11, 12 - i), g.CONTENT)
    p.add(p.rrect(1, 22, 63, 54, 6), p.lin((1, 22), (63, 54), hi, lo), gloss=0.3)
    p.detail(p.arc(20, 27, 40, 40, 25, 155, 1.8), darker(lo, 0.5))
    return slots(face=(18, 28, 0.38), badge=(32, 26, 0.52))


@obj("cabinet")
def cabinet(p, sel=False, tint="blue", **kw):
    """A filing cabinet; chosen, the top drawer is pulled out."""
    c, lo = colour(tint)
    p.add(p.rrect(10, 4, 54, 60, 3), p.lin((10, 4), (54, 60), lighter(c, 0.2), lo), gloss=0.2)
    for i in range(3):
        y = 9 + i * 17
        dx = 4 if (sel and i == 0) else 0
        p.add(p.rrect(14 - dx, y, 50 + dx, y + 14, 1.5), p.lin((0, y), (0, y + 14), lighter(c, 0.45), c), outline=1.3)
        p.add(p.rrect(26, y + 5, 38, y + 8.5, 1.2), p.lin((26, y + 5), (38, y + 9), METAL_HI, METAL_LO), outline=1.0)
    return slots(face=(18, 18, 0.45))


@obj("harddisk")
def harddisk(p, sel=False, tint=None, light=None, **kw):
    """A drive seen from a little above; chosen, its light comes on."""
    hi, lo = (METAL_HI, METAL_LO) if not tint else colour(tint)
    p.add(p.poly([(4, 30), (60, 30), (54, 18), (10, 18)]), p.lin((0, 18), (0, 30), lighter(hi, 0.3), lo), gloss=0.35)
    p.add(p.rrect(3, 28, 61, 48, 3), p.lin((3, 28), (61, 48), hi, darker(lo, 0.15)), gloss=0.2)
    p.detail(p.rect(9, 37, 40, 39), darker(lo, 0.25))
    p.detail(p.rect(9, 41, 34, 42.4), darker(lo, 0.25))
    on = light or (AMBER if sel else None)
    p.add(p.ellipse(48, 35, 54, 41), on if on else LED_OFF, outline=1.0)
    return slots(face=(12, 8, 0.4), badge=(30, 26, 0.52))


@obj("floppy")
def floppy(p, sel=False, tint="dark", label=True, hd=False, **kw):
    """A 3.5 inch disk; chosen, the shutter slides open."""
    c, lo = colour(tint)
    p.add(p.poly([(6, 4), (54, 4), (60, 10), (60, 60), (6, 60)]), p.lin((6, 4), (60, 60), lighter(c, 0.25), lo), gloss=0.2)
    sx = 22 if not sel else 30
    p.add(p.rect(18, 4, 46, 22), p.lin((18, 4), (46, 22), METAL_HI, METAL_LO), outline=1.4)
    p.detail(p.rect(sx, 7, sx + 7, 19), darker(lo, 0.4))
    if label:
        p.add(p.rrect(13, 30, 53, 59, 1.5), p.lin((0, 30), (0, 59), WHITE, PAPER_LO), outline=1.2)
    if hd:
        p.detail(p.rect(52, 52, 56, 56), WHITE)
        p.detail(p.text(48, 12, 6, "HD"), WHITE)
    return slots(face=(17, 31, 0.42), badge=(30, 30, 0.52))


@obj("disc")
def disc(p, sel=False, tint=None, **kw):
    """A CD; chosen, the light on it moves round."""
    p.add(p.ellipse(4, 4, 60, 60), p.rad((22, 20), 50, WHITE, rgb('#A8B0BC')), gloss=0.15)
    rc = (rgb('#F7B2D6'), rgb('#9FD8F7'), rgb('#FBE38E'))
    a0 = 200 if not sel else 250
    for i, c in enumerate(rc):
        p.detail(p.arc(8 + i * 3, 8 + i * 3, 56 - i * 3, 56 - i * 3, a0 + i * 6, a0 + 50 + i * 6, 2.2), c)
        p.detail(p.arc(8 + i * 3, 8 + i * 3, 56 - i * 3, 56 - i * 3, a0 + 180 + i * 6, a0 + 230 + i * 6, 2.2), c)
    if tint:
        p.detail(g.cut(p.ellipse(10, 10, 54, 54), p.ellipse(22, 22, 42, 42)), lighter(colour(tint)[0], 0.3))
    p.add(p.ellipse(25, 25, 39, 39), p.lin((25, 25), (39, 39), rgb('#E2E6EB'), rgb('#8D96A2')), outline=1.2)
    p.add(p.ellipse(29.5, 29.5, 34.5, 34.5), rgb('#EEF1F5'), outline=1.0)
    return slots(face=(30, 8, 0.4), badge=(30, 30, 0.52))


@obj("cartridge")
def cartridge(p, sel=False, tint="blue", window=True, **kw):
    """A removable cartridge (Zip, Jaz, SyQuest, MO, tape); chosen, its door slides."""
    c, lo = colour(tint)
    p.add(p.rrect(4, 8, 60, 56, 4), p.lin((4, 8), (60, 56), lighter(c, 0.2), lo), gloss=0.25)
    p.add(p.rrect(12, 32, 52, 52, 1.5), p.lin((0, 32), (0, 52), WHITE, PAPER_LO), outline=1.2)
    if window:
        x = 16 if not sel else 24
        p.add(p.rect(16, 12, 48, 24), p.lin((16, 12), (48, 24), METAL_HI, METAL_LO), outline=1.2)
        p.detail(p.rect(x, 14, x + 16, 22), darker(lo, 0.4))
    return slots(face=(16, 33, 0.4), badge=(30, 30, 0.52))


@obj("tape")
def tape(p, sel=False, **kw):
    p.add(p.rrect(3, 12, 61, 52, 3), p.lin((3, 12), (61, 52), CHIP_HI, CHIP_LO), gloss=0.25)
    p.add(p.rrect(10, 17, 54, 36, 2), p.lin((0, 17), (0, 36), WHITE, PAPER_LO), outline=1.2)
    a = 0 if not sel else 45
    for cx in (20, 44):
        p.add(p.ellipse(cx - 6, 21, cx + 6, 33), rgb('#3A2A1E'), outline=1.0)
        p.detail(p.ellipse(cx - 2.4, 24.6, cx + 2.4, 29.4), WHITE)
        t = math.radians(a)
        p.detail(p.stroke([(cx + math.cos(t) * 2, 27 + math.sin(t) * 2), (cx - math.cos(t) * 2, 27 - math.sin(t) * 2)], 1.0), INK)
    p.add(p.poly([(16, 52), (48, 52), (44, 44), (20, 44)]), p.lin((0, 44), (0, 52), STEEL_HI, STEEL_LO), outline=1.2)
    return slots(face=(18, 18, 0.35))


@obj("ramstick")
def ramstick(p, sel=False, **kw):
    """A memory module; chosen, its chips light up."""
    p.add(p.rrect(2, 18, 62, 44, 2), p.lin((2, 18), (62, 44), PCB_HI, PCB_LO), gloss=0.2)
    for i in range(4):
        x = 6 + i * 14
        p.add(p.rect(x, 22, x + 10, 36), p.lin((x, 22), (x + 10, 36), CHIP_HI, CHIP_LO), outline=1.0)
        if sel:
            p.detail(p.rect(x + 2, 24, x + 8, 25.4), LED_ON)
    for i in range(18):
        p.detail(p.rect(5 + i * 3.1, 40, 6.8 + i * 3.1, 44), GOLDPIN)
    p.detail(p.rect(30, 39, 33, 44.5), rgb('#14381E'))
    return slots(face=(16, 10, 0.4), badge=(30, 30, 0.52))


@obj("flashcard")
def flashcard(p, sel=False, kind="cf", **kw):
    """A CompactFlash, SD or PCMCIA card; chosen, its light comes on."""
    if kind == "sd":
        p.add(p.poly([(14, 4), (42, 4), (50, 12), (50, 60), (14, 60)]), p.lin((14, 4), (50, 60), rgb('#4A5562'), rgb('#20252C')), gloss=0.2)
        for i in range(5):
            p.detail(p.rect(17 + i * 6, 6, 21 + i * 6, 13), GOLDPIN)
        p.add(p.rect(18, 22, 46, 54), p.lin((0, 22), (0, 54), g.BLUE, g.BLUE_LO), outline=1.0)
        p.detail(p.text(32, 38, 9, "SD"), WHITE)
        return slots(face=(18, 22, 0.42))
    if kind == "pcmcia":
        p.add(p.rrect(6, 4, 58, 60, 2), p.lin((6, 4), (58, 60), METAL_HI, METAL_LO), gloss=0.25)
        p.detail(p.rect(8, 54, 56, 59), CHIP_LO)
        p.add(p.rect(12, 12, 52, 46), p.lin((0, 12), (0, 46), lighter(g.BLUE, 0.2), g.BLUE_LO), outline=1.0)
        return slots(face=(14, 12, 0.55))
    p.add(p.rrect(6, 10, 58, 56, 2), p.lin((6, 10), (58, 56), rgb('#4A5562'), rgb('#20252C')), gloss=0.2)
    p.add(p.rect(11, 16, 53, 44), p.lin((0, 16), (0, 44), METAL_HI, METAL_LO), outline=1.0)
    p.detail(p.text(32, 30, 11, "CF"), INK)
    for i in range(14):
        p.detail(p.rect(9 + i * 3.4, 50, 10.4 + i * 3.4, 55), GOLDPIN)
    return slots(face=(16, 16, 0.45))


@obj("usbstick")
def usbstick(p, sel=False, **kw):
    p.add(p.rect(22, 2, 42, 18), p.lin((22, 2), (42, 18), METAL_HI, METAL_LO), gloss=0.3)
    p.detail(p.rect(26, 6, 30, 10), INK)
    p.detail(p.rect(34, 6, 38, 10), INK)
    p.add(p.rrect(16, 16, 48, 60, 5), p.lin((16, 16), (48, 60), lighter(g.BLUE, 0.2), g.BLUE_LO), gloss=0.3)
    p.add(p.ellipse(29, 48, 35, 54), LED_ON if sel else LED_OFF, outline=1.0)
    return slots(face=(20, 22, 0.38))


@obj("bin")
def trashcan(p, sel=False, full=False, **kw):
    """The trashcan; chosen, its lid lifts."""
    if full:
        for (cx, cy, r, c) in ((22, 21, 6.5, WHITE), (40, 19, 7.0, rgb('#F6E7A6')), (31, 16, 6.0, rgb('#D6E6F8'))):
            p.add(p.ellipse(cx - r, cy - r * 0.85, cx + r, cy + r * 0.85), p.rad((cx - r * 0.4, cy - r * 0.4), r * 1.8, WHITE, darker(c, 0.18)), outline=1.6)
    p.add(p.poly([(14, 24), (50, 24), (46, 60), (18, 60)]), p.lin((14, 24), (50, 60), rgb('#EEF0F2'), rgb('#9BA1A9')), gloss=0.15)
    for x in (24, 32, 40):
        p.detail(p.stroke([(x, 29), (x - 0.4 * (x - 32) / 8, 55)], 1.4), rgb('#8C929B'))
    if sel:
        p.add(p.poly([(34, 6), (56, 11), (55, 16.5), (33, 11.5)]), p.lin((34, 6), (54, 16), rgb('#F7F8F9'), rgb('#AEB4BC')), gloss=0.3)
        p.add(p.poly([(41, 5.2), (47, 6.6), (46.5, 9), (40.5, 7.6)]), rgb('#C6CBD2'), outline=1.4)
        p.add(p.rrect(12, 22, 52, 27, 2), p.lin((12, 22), (52, 27), rgb('#F7F8F9'), rgb('#B3B9C1')), outline=1.6)
    else:
        p.add(p.rrect(11, 18, 53, 25, 2.5), p.lin((11, 18), (53, 25), rgb('#F7F8F9'), rgb('#B3B9C1')), gloss=0.3)
        if not full:
            p.add(p.rrect(27, 13, 37, 19, 2), rgb('#C6CBD2'), outline=1.4)
    return slots(face=(20, 30, 0.38))


@obj("box")
def cardboard_box(p, sel=False, tint="brown", **kw):
    """A cardboard box; chosen, the flaps open."""
    hi, lo = (rgb('#F2B865'), rgb('#B9701F')) if tint == "brown" else (lighter(colour(tint)[0], 0.3), colour(tint)[1])
    if sel:
        p.add(p.poly([(6, 26), (2, 12), (18, 14), (22, 26)]), p.lin((0, 12), (0, 26), lighter(hi, 0.2), lo), outline=1.6)
        p.add(p.poly([(58, 26), (62, 12), (46, 14), (42, 26)]), p.lin((0, 12), (0, 26), lighter(hi, 0.2), lo), outline=1.6)
        p.detail(p.rect(7, 24, 57, 28), darker(lo, 0.45))
    p.add(p.rrect(6, 26, 58, 60, 1.6), p.lin((6, 26), (58, 60), hi, lo), gloss=0.15)
    if not sel:
        p.add(p.poly([(4, 26), (60, 26), (56, 18), (8, 18)]), p.lin((0, 18), (0, 26), lighter(hi, 0.25), hi), outline=1.6, gloss=0.3)
        p.detail(p.rect(27, 18.6, 37, 25.4), rgb('#F8DDB0'))
    p.detail(p.rect(27, 26.6, 37, 36), rgb('#F8DDB0'))
    return slots(face=(18, 32, 0.42), badge=(30, 30, 0.52))


@obj("safe")
def safe(p, sel=False, **kw):
    p.add(p.rrect(6, 6, 58, 58, 4), p.lin((6, 6), (58, 58), rgb('#8A929D'), rgb('#3A4049')), gloss=0.25)
    p.add(p.rrect(11, 11, 53, 53, 2.5), p.lin((11, 11), (53, 53), rgb('#9AA2AD'), rgb('#4A515B')), outline=1.2)
    p.add(p.ellipse(22, 22, 42, 42), p.lin((22, 22), (42, 42), METAL_HI, METAL_LO), outline=1.4)
    a = math.radians(-60 if not sel else 30)
    p.detail(p.stroke([(32, 32), (32 + math.cos(a) * 8, 32 + math.sin(a) * 8)], 2.0), INK)
    return slots(face=(20, 20, 0.38))


# ---- machines ---------------------------------------------------------------------------------------


@obj("window")
def window(p, sel=False, active=True, tint="blue", body=None, **kw):
    """An application window; chosen, it becomes the active one."""
    c, lo = colour(tint)
    on = active or sel
    p.add(p.rrect(3, 6, 61, 58, 2.5), g.solid(body or g.PAPER))
    bar = p.lin((0, 6), (0, 15), lighter(c, 0.25), lo) if on else p.lin((0, 6), (0, 15), rgb('#E2E5E9'), rgb('#B6BBC2'))
    p.detail(g.inter(p.rrect(3, 6, 61, 58, 2.5), p.rect(3, 6, 61, 15)), bar)
    p.detail(p.rect(3, 15, 61, 16), INK)
    p.detail(p.rect(53, 8.5, 58, 13.5), WHITE if on else rgb('#7C8088'))
    p.detail(p.rect(6, 8.5, 10, 12.5), WHITE if on else rgb('#7C8088'))
    return slots(face=(18, 22, 0.44), badge=(30, 30, 0.52))


@obj("terminal")
def terminal(p, sel=False, **kw):
    """A Shell window, its prompt; chosen, the cursor has moved on."""
    window(p, sel=True, body=rgb('#1C232C'))
    p.detail(p.text(14, 26, 9, ">", mono=True), rgb('#7CFF6A'))
    lines(p, 20, 24.5, 40, 1, h=2.4, c=rgb('#A9D8A0'))
    cx = 42 if not sel else 50
    p.detail(p.rect(cx, 23.5, cx + 4, 28.5), rgb('#7CFF6A'))
    lines(p, 9, 34, 52, 3, gap=5.5, h=2, c=rgb('#5E7A6A'), short=0.5)
    return slots(face=(18, 26, 0.4))


@obj("monitor")
def monitor(p, sel=False, dark=False, **kw):
    """A monitor; chosen, the screen lights."""
    p.add(p.poly([(26, 46), (38, 46), (40, 54), (24, 54)]), p.lin((0, 46), (0, 54), STEEL_HI, STEEL_LO), outline=1.6)
    p.add(p.rrect(16, 52, 48, 58, 2), p.lin((16, 52), (48, 58), STEEL_HI, STEEL_LO), gloss=0.3)
    p.add(p.rrect(3, 6, 61, 48, 3.5), p.lin((3, 6), (61, 48), METAL_HI, METAL_LO), gloss=0.2)
    hi, lo = (SCREEN_HI, SCREEN_LO) if not dark else (rgb('#2A3038'), rgb('#0E1216'))
    if sel and not dark:
        hi, lo = lighter(SCREEN_HI, 0.25), SCREEN_HI
    p.detail(p.rect(8, 11, 56, 42), p.lin((0, 11), (0, 42), hi, lo))
    p.detail(p.poly([(8, 11), (30, 11), (8, 30)]), lighter(hi, 0.12))
    return slots(face=(15, 9, 0.53), badge=(30, 28, 0.52))


@obj("computer")
def amiga(p, sel=False, kind="wedge", **kw):
    """A home computer: the Amiga's wedge (or a breadbox, or a rubber-key one); chosen, its power light is on."""
    if kind == "breadbox":
        p.add(p.poly([(4, 54), (60, 54), (54, 26), (10, 26)]), p.lin((0, 26), (0, 54), rgb('#B9AE94'), rgb('#7A6F55')), gloss=0.3)
        for r in range(3):
            for c in range(9):
                x = 13 + c * 4.4 + r * 1.2
                p.detail(p.rect(x, 31 + r * 6, x + 3, 34 + r * 6), rgb('#3A3226'))
        p.add(p.ellipse(52, 48, 56, 52), g.RED if sel else rgb('#5A2020'), outline=0.8)
        return slots(badge=(30, 22, 0.52))
    if kind == "rubber":
        p.add(p.poly([(6, 50), (58, 50), (54, 26), (10, 26)]), p.lin((0, 26), (0, 50), rgb('#3A3F47'), rgb('#14171B')), gloss=0.3)
        for r in range(4):
            for c in range(9):
                x = 13 + c * 4.3 + r * 0.9
                p.detail(p.rect(x, 30 + r * 4.8, x + 3, 33 + r * 4.8), rgb('#8E949C'))
        for i, cc in enumerate((g.RED, g.GOLD, g.GREEN, g.BLUE)):
            p.detail(p.poly([(40 + i * 3, 50), (44 + i * 3, 50), (52 + i * 3, 26), (48 + i * 3, 26)]), cc) if False else None
        return slots(badge=(30, 22, 0.52))
    p.add(p.poly([(2, 56), (62, 56), (62, 46), (56, 30), (8, 30), (2, 46)]), p.lin((0, 30), (0, 56), rgb('#F2EFE4'), rgb('#B1AC98')), gloss=0.3)
    p.detail(p.rect(2.6, 46, 61.4, 47.2), rgb('#8F8A76'))
    for r in range(3):
        for c in range(10):
            x = 12 + c * 4.0 + r * 0.8
            p.detail(p.rect(x, 33 + r * 4.2, x + 3, 35.6 + r * 4.2), rgb('#CFCAB8'))
    p.add(p.ellipse(52, 49.5, 56, 53.5), LED_ON if sel else LED_OFF, outline=0.8)
    return slots(badge=(30, 22, 0.52))


@obj("pctower")
def pc_tower(p, sel=False, **kw):
    p.add(p.rrect(16, 2, 48, 62, 3), p.lin((16, 2), (48, 62), PCGREY_HI, PCGREY_LO), gloss=0.2)
    p.add(p.rect(20, 8, 44, 13), p.lin((0, 8), (0, 13), STEEL_HI, STEEL_LO), outline=1.0)
    p.add(p.rect(20, 16, 44, 21), p.lin((0, 16), (0, 21), STEEL_HI, STEEL_LO), outline=1.0)
    p.add(p.ellipse(29, 44, 35, 50), LED_ON if sel else rgb('#5A616B'), outline=1.0)
    return slots(face=(18, 24, 0.44), badge=(30, 30, 0.52))


@obj("arcade")
def arcade(p, sel=False, **kw):
    p.add(p.poly([(12, 62), (52, 62), (52, 4), (12, 4)]), p.lin((12, 4), (52, 62), rgb('#4B5BD0'), rgb('#22296A')), gloss=0.2)
    p.add(p.rect(16, 12, 48, 32), p.lin((0, 12), (0, 32), SCREEN_HI, SCREEN_LO), outline=1.2)
    p.add(p.poly([(12, 36), (52, 36), (56, 44), (8, 44)]), p.lin((0, 36), (0, 44), CHIP_HI, CHIP_LO), outline=1.6)
    p.add(p.stroke([(20, 40), (20 if not sel else 23, 34)], 1.6), INK, outline=0)
    p.add(p.ellipse(18, 32, 22 + (3 if sel else 0), 36), g.RED, outline=1.0)
    for i, c in enumerate((g.GOLD, g.GREEN)):
        p.add(p.ellipse(32 + i * 7, 38, 36 + i * 7, 41.5), c, outline=0.9)
    return slots(face=(18, 12, 0.45))


@obj("keyboard")
def keyboard(p, sel=False, **kw):
    """A keyboard; chosen, a key is pressed."""
    p.add(p.poly([(2, 50), (62, 50), (58, 24), (6, 24)]), p.lin((0, 24), (0, 50), rgb('#F2EFE4'), rgb('#B1AC98')), gloss=0.3)
    for r in range(4):
        for c in range(10):
            x = 9 + c * 4.8 + r * 0.6
            y = 27 + r * 5.4
            down = sel and (r, c) == (2, 5)
            p.detail(p.rect(x, y + (1 if down else 0), x + 3.8, y + 4 + (0.6 if down else 0)), rgb('#9C977F') if down else WHITE)
    return slots(face=(16, 8, 0.4), badge=(30, 24, 0.52))


@obj("key")
def keycap(p, sel=False, letter="", **kw):
    """One key; chosen, it is pressed."""
    d = 4 if sel else 0
    p.add(p.rrect(8, 14 + d, 56, 58, 6), p.lin((8, 14), (56, 58), rgb('#D9D4C2'), rgb('#8F8A76')))
    p.add(p.rrect(14, 10 + d, 50, 46 + d, 5), p.lin((14, 10), (50, 46), WHITE, rgb('#CFCAB8')), outline=1.4, gloss=0.35)
    if letter:
        p.detail(p.text(32, 28 + d, 16 if len(letter) < 3 else 11, letter), INK)
    return slots(face=(16, 12 + d, 0.5))


@obj("mouse")
def mouse(p, sel=False, **kw):
    """A mouse; chosen, its left button is pressed."""
    p.add(p.stroke([(32, 8), (32, 2), (44, 0)], 1.6), INK, outline=0)
    p.add(p.rrect(14, 8, 50, 60, 16), p.lin((14, 8), (50, 60), rgb('#F2EFE4'), rgb('#B1AC98')), gloss=0.3)
    p.detail(p.rect(31.4, 9, 32.6, 28), rgb('#8F8A76'))
    p.detail(p.rect(14, 27.4, 50, 28.6), rgb('#8F8A76'))
    if sel:
        p.detail(g.inter(p.rrect(14, 8, 50, 60, 16), p.rect(14, 8, 31.4, 27.4)), rgb('#C9C3AE'))
    return slots(face=(18, 30, 0.4), badge=(30, 30, 0.52))


@obj("joystick")
def joystick(p, sel=False, **kw):
    """A joystick; chosen, the stick tilts and the button lights."""
    p.add(p.rrect(6, 40, 58, 58, 5), p.lin((6, 40), (58, 58), CHIP_HI, CHIP_LO), gloss=0.25)
    tip = (32, 10) if not sel else (42, 12)
    p.add(p.stroke([(32, 44), tip], 4.4), p.lin((28, 44), (36, 10), rgb('#5A616B'), rgb('#1F2329')))
    p.add(p.ellipse(tip[0] - 7, tip[1] - 7, tip[0] + 7, tip[1] + 7), p.rad((tip[0] - 3, tip[1] - 3), 12, rgb('#FF8A7A'), g.RED_LO), gloss=0.3)
    p.add(p.ellipse(10, 42, 20, 49), g.RED if sel else g.RED_LO, outline=1.2)
    return slots(face=(16, 16, 0.4), badge=(30, 22, 0.52))


@obj("gamepad")
def gamepad(p, sel=False, **kw):
    """A gamepad; chosen, a button is pressed."""
    p.add(g.union(p.rrect(4, 18, 60, 48, 14)), p.lin((4, 18), (60, 48), rgb('#5A616B'), rgb('#1F2329')), gloss=0.25)
    p.detail(p.rect(12, 30, 26, 35), rgb('#C9CED6'))
    p.detail(p.rect(16.5, 25.5, 21.5, 39.5), rgb('#C9CED6'))
    for (x, y, c) in ((44, 28, g.RED), (51, 34, g.GOLD), (44, 38, g.GREEN), (37, 34, g.BLUE)):
        pressed = sel and c == g.RED
        p.add(p.ellipse(x - 3, y - 3, x + 3, y + 3), darker(c, 0.3) if pressed else c, outline=1.0)
    return slots(face=(18, 18, 0.4), badge=(30, 26, 0.52))


@obj("printer")
def printer(p, sel=False, **kw):
    """A printer, a page in the tray; chosen, the page is further out and the light on."""
    p.add(p.rect(16, 4, 48, 22), p.lin((16, 4), (48, 22), PAPER_HI, PAPER_LO))
    lines(p, 20, 8, 44, 3, gap=4, h=1.4)
    p.add(p.rrect(4, 18, 60, 44, 4), p.lin((4, 18), (60, 44), STEEL_HI, STEEL_LO), gloss=0.25)
    p.add(p.rrect(9, 36, 55, 50, 2.5), p.lin((0, 36), (0, 50), rgb('#C9CED6'), rgb('#6F7782')))
    p.detail(p.rect(12, 39, 52, 41), INK)
    out = 14 if sel else 6
    p.add(p.rect(15, 40, 49, 40 + out + 4), p.lin((0, 40), (0, 40 + out + 4), WHITE, PAPER_LO), outline=1.4)
    p.add(p.ellipse(47, 24, 52, 29), LED_ON if sel else LED_OFF, outline=1.0)
    return slots(face=(18, 2, 0.4), badge=(30, 30, 0.52))


@obj("scanner")
def scanner(p, sel=False, **kw):
    p.add(p.poly([(2, 46), (62, 46), (58, 26), (6, 26)]), p.lin((0, 26), (0, 46), STEEL_HI, STEEL_LO), gloss=0.3)
    p.add(p.rrect(2, 44, 62, 54, 2), p.lin((0, 44), (0, 54), rgb('#B9C0CA'), rgb('#5E6672')))
    p.detail(p.poly([(10, 42), (54, 42), (52, 30), (12, 30)]), rgb('#2A3A4A'))
    y = 38 if not sel else 33
    p.detail(p.poly([(11, y + 1.6), (53, y + 1.6), (53, y), (11, y)]), rgb('#B8FFB0'))
    return slots(face=(18, 26, 0.36))


@obj("camera")
def camera(p, sel=False, **kw):
    """A camera; chosen, the flash fires."""
    p.add(p.rrect(16, 12, 30, 22, 2), p.lin((16, 12), (30, 22), CHIP_HI, CHIP_LO))
    p.add(p.rrect(4, 18, 60, 54, 5), p.lin((4, 18), (60, 54), rgb('#5A616B'), rgb('#1F2329')), gloss=0.25)
    p.add(p.ellipse(18, 22, 46, 50), p.lin((18, 22), (46, 50), METAL_HI, METAL_LO))
    p.add(p.ellipse(23, 27, 41, 45), p.rad((28, 31), 20, rgb('#7DB6F2'), rgb('#0E2A55')), outline=1.2)
    p.detail(p.ellipse(27, 30, 32, 35), WHITE)
    p.add(p.rect(46, 22, 56, 28), WHITE if sel else rgb('#C9CED6'), outline=1.0)
    if sel:
        for a in (-60, -30, 0):
            t = math.radians(a)
            p.add(p.stroke([(51 + math.cos(t) * 8, 25 + math.sin(t) * 8), (51 + math.cos(t) * 13, 25 + math.sin(t) * 13)], 1.6), g.GOLD, outline=0.8)
    return slots(face=(20, 24, 0.38), badge=(30, 30, 0.52))


@obj("tablet")
def tablet(p, sel=False, **kw):
    p.add(p.rrect(4, 14, 60, 56, 4), p.lin((4, 14), (60, 56), rgb('#5A616B'), rgb('#1F2329')), gloss=0.25)
    p.detail(p.rect(10, 20, 54, 50), rgb('#3A4049'))
    pts = [(14, 44), (24, 34), (32, 40), (44, 26)] if not sel else [(14, 44), (24, 34), (32, 40), (44, 26), (50, 30)]
    p.detail(p.stroke(pts, 1.6), rgb('#9FD0FF'))
    tip = pts[-1]
    p.add(p.stroke([tip, (tip[0] + 12, tip[1] - 20)], 3.4), p.lin(tip, (tip[0] + 12, tip[1] - 20), STEEL_HI, STEEL_LO))
    return slots(face=(14, 20, 0.45))


@obj("modem")
def modem(p, sel=False, **kw):
    p.add(p.poly([(2, 50), (62, 50), (58, 30), (6, 30)]), p.lin((0, 30), (0, 50), rgb('#F2EFE4'), rgb('#B1AC98')), gloss=0.3)
    for i in range(6):
        on = (i % 2 == 0) != sel
        p.add(p.ellipse(12 + i * 7, 41, 16 + i * 7, 45), LED_ON if on else LED_OFF, outline=0.8)
    for k, a in enumerate((8, 14, 20)):
        p.add(p.arc(32 - a, 26 - a, 32 + a, 26 + a, 220, 320, 2.0), g.BLUE, outline=0.8) if (sel or k < 2) else None
    return slots(face=(18, 26, 0.36), badge=(30, 28, 0.52))


@obj("calculator")
def calculator(p, sel=False, **kw):
    p.add(p.rrect(12, 4, 52, 60, 4), p.lin((12, 4), (52, 60), rgb('#5A616B'), rgb('#1F2329')), gloss=0.25)
    p.detail(p.rect(17, 9, 47, 19), rgb('#B8D8B0'))
    p.detail(p.text(40, 14, 7, "42" if not sel else "7", mono=True), INK)
    for r in range(4):
        for c in range(4):
            on = sel and (r, c) == (3, 3)
            p.add(p.rrect(17 + c * 7.6, 24 + r * 8.6, 22.5 + c * 7.6, 30 + r * 8.6, 1), g.ORANGE if c == 3 else (rgb('#8E96A2') if on else rgb('#C9CED6')), outline=0.8)
    return slots(face=(18, 22, 0.45))


@obj("speaker")
def speaker(p, sel=False, waves=None, muted=False, **kw):
    """A speaker and its waves; chosen, louder."""
    p.add(p.rect(8, 22, 18, 42), p.lin((8, 22), (18, 42), rgb('#7E8794'), rgb('#2A2F37')), gloss=0.3)
    p.add(p.poly([(18, 22), (32, 8), (32, 56), (18, 42)]), p.lin((18, 8), (32, 56), rgb('#7E8794'), rgb('#2A2F37')), gloss=0.3)
    if muted:
        p.add(p.stroke([(38, 22), (56, 42)], 3.4), g.RED, outline=1.0)
        p.add(p.stroke([(56, 22), (38, 42)], 3.4), g.RED, outline=1.0)
        return slots(badge=(30, 30, 0.52))
    n = (waves or 3) + (1 if sel else 0)
    for k in range(n):
        r = 8 + k * 6
        p.add(p.arc(31 - r, 32 - r, 31 + r, 32 + r, -48, 48, 2.6), p.lin((31, 0), (31 + r, 0), lighter(g.BLUE, 0.3), g.BLUE_LO), outline=1.0)
    return slots(face=(36, 20, 0.4), badge=(30, 30, 0.52))


@obj("microphone")
def microphone(p, sel=False, **kw):
    p.add(p.arc(16, 18, 48, 48, 0, 180, 2.4), STEEL_LO, outline=1.0)
    p.add(p.stroke([(32, 48), (32, 58)], 2.8), STEEL_LO, outline=1.0)
    p.add(p.rrect(20, 54, 44, 60, 2), p.lin((20, 54), (44, 60), STEEL_HI, STEEL_LO))
    p.add(p.rrect(23, 4, 41, 40, 9), p.lin((23, 4), (41, 40), rgb('#7E8794'), rgb('#2A2F37')), gloss=0.35)
    for i in range(4):
        p.detail(p.rect(26, 10 + i * 5, 38, 11 + i * 5), rgb('#9AA2AD'))
    if sel:
        p.add(p.ellipse(44, 6, 50, 12), g.RED, outline=1.0)
    return slots(badge=(30, 30, 0.52))


@obj("faders")
def faders(p, sel=False, **kw):
    p.add(p.rrect(4, 6, 60, 58, 4), p.lin((4, 6), (60, 58), rgb('#5A616B'), rgb('#1F2329')), gloss=0.2)
    ks = (36, 22, 44, 28) if not sel else (24, 36, 30, 44)
    for i, k in enumerate(ks):
        x = 13 + i * 13
        p.detail(p.rect(x - 1, 12, x + 1, 52), rgb('#14171B'))
        p.add(p.rrect(x - 4.5, k - 3, x + 4.5, k + 3, 1), p.lin((x - 4, k - 3), (x + 4, k + 3), STEEL_HI, STEEL_LO), outline=1.0)
    return slots(face=(16, 16, 0.45))


@obj("clock")
def clock(p, sel=False, battery=False, **kw):
    """A clock face; chosen, the hands move on."""
    cx, cy, r = 32, 32, 28
    p.add(p.ellipse(cx - r, cy - r, cx + r, cy + r), p.lin((cx - r, cy - r), (cx + r, cy + r), METAL_HI, METAL_LO), gloss=0.2)
    ri = r - 4
    p.add(p.ellipse(cx - ri, cy - ri, cx + ri, cy + ri), p.rad((cx - ri * 0.4, cy - ri * 0.4), ri * 1.8, rgb('#5E86BE'), rgb('#1D3A66')), outline=1.0)
    for k in range(12):
        a = math.radians(k * 30)
        x, y = cx + math.cos(a) * ri * 0.8, cy + math.sin(a) * ri * 0.8
        p.detail(p.ellipse(x - 1, y - 1, x + 1, y + 1), (230, 236, 245))
    ha, ma = (30, 150) if not sel else (75, 220)
    for a, L, wd in ((ha, ri * 0.5, 2.8), (ma, ri * 0.78, 1.8)):
        t = math.radians(a - 90)
        p.detail(p.stroke([(cx, cy), (cx + math.cos(t) * L, cy + math.sin(t) * L)], wd), (250, 250, 250))
    p.detail(p.ellipse(cx - 2, cy - 2, cx + 2, cy + 2), (250, 250, 250))
    return slots(badge=(32, 32, 0.5))


@obj("stopwatch")
def stopwatch(p, sel=False, **kw):
    p.add(p.rrect(27, 2, 37, 10, 2), p.lin((27, 2), (37, 10), METAL_HI, METAL_LO))
    p.add(p.ellipse(8, 10, 56, 58), p.lin((8, 10), (56, 58), METAL_HI, METAL_LO), gloss=0.2)
    p.add(p.ellipse(13, 15, 51, 53), WHITE, outline=1.2)
    a = math.radians((40 if not sel else 160) - 90)
    p.detail(p.stroke([(32, 34), (32 + math.cos(a) * 15, 34 + math.sin(a) * 15)], 1.8), g.RED)
    p.detail(p.ellipse(30, 32, 34, 36), INK)
    return slots(badge=(32, 32, 0.5))


@obj("gauge")
def gauge(p, sel=False, tint="green", **kw):
    """A dial; chosen, the needle swings."""
    p.add(p.ellipse(4, 8, 60, 64), p.lin((4, 8), (60, 64), METAL_HI, METAL_LO), gloss=0.2)
    p.add(p.ellipse(9, 13, 55, 59), rgb('#1C232C'), outline=1.0)
    for i, c in enumerate((g.GREEN, g.GOLD, g.RED)):
        p.detail(p.arc(13, 17, 51, 55, 180 + i * 60, 240 + i * 60, 3), c)
    a = math.radians(210 if not sel else 300)
    p.detail(p.stroke([(32, 36), (32 + math.cos(a) * 17, 36 + math.sin(a) * 17)], 2.2), WHITE)
    p.detail(p.ellipse(29, 33, 35, 39), WHITE)
    return slots(badge=(32, 30, 0.5))


@obj("graph")
def graph(p, sel=False, **kw):
    p.add(p.rrect(4, 6, 60, 58, 3), p.lin((4, 6), (60, 58), WHITE, PAPER_LO), gloss=0.12)
    for i in range(1, 4):
        p.detail(p.rect(8, 6 + i * 12.5, 56, 6.6 + i * 12.5), rgb('#D4D8DD'))
    pts = [(8, 46), (18, 36), (26, 40), (36, 22), (46, 28), (56, 14)] if not sel else [(8, 40), (18, 44), (26, 28), (36, 32), (46, 18), (56, 22)]
    p.add(p.stroke(pts, 2.4), g.GREEN, outline=1.0)
    return slots(face=(16, 16, 0.45))


@obj("pie")
def pie(p, sel=False, **kw):
    p.add(p.ellipse(6, 6, 58, 58), p.lin((6, 6), (58, 58), lighter(g.BLUE, 0.2), g.BLUE_LO), gloss=0.2)
    a = 110 if not sel else 160
    m = g.blank()
    a0, b0 = p.T(6, 6), p.T(58, 58)
    ImageDraw.Draw(m).pieslice([g.P(a0[0]), g.P(a0[1]), g.P(b0[0]), g.P(b0[1])], -90, -90 + a, fill=255)
    p.add(m, p.lin((6, 6), (58, 58), lighter(g.ORANGE, 0.2), g.ORANGE_LO), outline=1.4)
    return slots(badge=(32, 32, 0.5))


@obj("board")
def board(p, sel=False, tint=None, chips=3, **kw):
    """A circuit board; chosen, its light comes on."""
    hi, lo = (PCB_HI, PCB_LO) if not tint else colour(tint)
    p.add(p.rrect(2, 12, 62, 52, 2.5), p.lin((2, 12), (62, 52), hi, lo), gloss=0.2)
    for i in range(10):
        p.detail(p.stroke([(6 + i * 5.5, 16), (6 + i * 5.5, 48)], 0.6), lighter(hi, 0.25))
    for i in range(chips):
        x = 8 + i * 17
        p.add(p.rect(x, 20, x + 13, 34), p.lin((x, 20), (x + 13, 34), CHIP_HI, CHIP_LO), outline=1.0)
    p.detail(p.rect(6, 47, 58, 52), GOLDPIN)
    p.add(p.ellipse(52, 38, 57, 43), LED_ON if sel else LED_OFF, outline=0.8)
    return slots(face=(18, 14, 0.4), badge=(30, 26, 0.52))


@obj("card")
def card(p, sel=False, kind="graphics", **kw):
    """An expansion card: graphics (a fan and a port), sound (jacks) or network (a socket); chosen, the fan turns or a light shows."""
    p.add(p.rrect(2, 12, 58, 48, 2), p.lin((2, 12), (58, 48), PCB_HI, PCB_LO), gloss=0.2)
    p.add(p.rect(56, 8, 62, 52), p.lin((56, 8), (62, 52), METAL_HI, METAL_LO), outline=1.4)
    for i in range(12):
        p.detail(p.rect(8 + i * 3.6, 48, 10 + i * 3.6, 53), GOLDPIN)
    if kind == "graphics":
        p.add(p.ellipse(10, 15, 40, 45), p.lin((10, 15), (40, 45), rgb('#3A3F47'), rgb('#14171B')), outline=1.2)
        a0 = 0 if not sel else 30
        for k in range(5):
            t = math.radians(a0 + k * 72)
            p.detail(p.stroke([(25, 30), (25 + math.cos(t) * 12, 30 + math.sin(t) * 12)], 3.2), rgb('#7E8794'))
        p.detail(p.ellipse(22, 27, 28, 33), rgb('#C9CED6'))
        p.add(p.rect(44, 18, 52, 40), p.lin((44, 18), (52, 40), CHIP_HI, CHIP_LO), outline=1.0)
    elif kind == "sound":
        for i, c in enumerate((g.GREEN, g.PINK if hasattr(g, "PINK") else PINK, g.BLUE)):
            p.add(p.ellipse(50, 16 + i * 10, 56, 22 + i * 10), c, outline=0.8)
        p.add(p.rect(10, 20, 30, 38), p.lin((10, 20), (30, 38), CHIP_HI, CHIP_LO), outline=1.0)
        p.add(p.rect(34, 24, 44, 34), p.lin((34, 24), (44, 34), CHIP_HI, CHIP_LO), outline=1.0)
    else:
        p.add(p.rect(44, 22, 56, 34), p.lin((44, 22), (56, 34), STEEL_HI, STEEL_LO), outline=1.0)
        p.detail(p.rect(46, 26, 54, 32), INK)
        p.add(p.rect(12, 20, 32, 38), p.lin((12, 20), (32, 38), CHIP_HI, CHIP_LO), outline=1.0)
        p.add(p.ellipse(36, 38, 40, 42), LED_ON if sel else LED_OFF, outline=0.7)
    return slots(badge=(30, 26, 0.52))


@obj("chip")
def chip(p, sel=False, label="", **kw):
    """A chip; chosen, its pins shine."""
    for i in range(6):
        for x0, x1 in ((4, 12), (52, 60)):
            p.add(p.rect(x0, 12 + i * 7.6, x1, 15.5 + i * 7.6), GOLDPIN if not sel else lighter(GOLDPIN, 0.4), outline=0.8)
    p.add(p.rrect(10, 8, 54, 58, 2.5), p.lin((10, 8), (54, 58), CHIP_HI, CHIP_LO), gloss=0.25)
    p.detail(p.ellipse(14, 12, 19, 17), rgb('#14171B'))
    if label:
        p.detail(p.text(32, 34, 11 if len(label) < 4 else 8, label), rgb('#E2E5E9'))
    return slots(face=(16, 17, 0.5), badge=(30, 30, 0.52))


# ---- connections --------------------------------------------------------------------------------------


@obj("netplug")
def netplug(p, sel=False, **kw):
    """A network plug and its cable; chosen, its lights show."""
    p.add(p.stroke([(32, 46), (32, 62)], 6), p.lin((28, 0), (36, 0), lighter(g.BLUE, 0.3), g.BLUE_LO))
    p.add(p.rrect(18, 14, 46, 48, 3), p.lin((18, 14), (46, 48), rgb('#D6EAF8'), rgb('#8EB4D6')), gloss=0.35)
    p.add(p.rect(22, 6, 42, 18), p.lin((22, 6), (42, 18), METAL_HI, METAL_LO), outline=1.4)
    for i in range(6):
        p.detail(p.rect(24 + i * 3, 7, 25.6 + i * 3, 12), GOLDPIN)
    if sel:
        p.add(p.ellipse(22, 36, 27, 41), LED_ON, outline=0.8)
        p.add(p.ellipse(37, 36, 42, 41), AMBER, outline=0.8)
    return slots(face=(20, 20, 0.38), badge=(30, 30, 0.52))


@obj("dsub")
def dsub(p, sel=False, pins=9, tint="grey", **kw):
    """A serial or parallel plug, seen end-on; chosen, a cable is in it."""
    c, lo = colour(tint)
    p.add(p.rrect(2, 18, 62, 46, 4), p.lin((2, 18), (62, 46), METAL_HI, METAL_LO), gloss=0.3)
    p.add(p.poly([(10, 23), (54, 23), (50, 41), (14, 41)]), p.lin((0, 23), (0, 41), lighter(c, 0.1), lo), outline=1.4)
    n = 5 if pins <= 9 else 13
    for i in range(n):
        x = 14 + i * (36 / (n - 1))
        p.detail(p.ellipse(x - 1.2, 27, x + 1.2, 29.4), GOLDPIN)
    for i in range(n - 1):
        x = 16 + i * (32 / max(1, n - 2))
        p.detail(p.ellipse(x - 1.2, 34, x + 1.2, 36.4), GOLDPIN)
    p.add(p.ellipse(4, 29, 8, 33), METAL_LO, outline=0.8)
    p.add(p.ellipse(56, 29, 60, 33), METAL_LO, outline=0.8)
    if sel:
        p.add(p.stroke([(32, 46), (32, 62)], 6), INK, outline=0)
    return slots(face=(20, 20, 0.38), badge=(30, 30, 0.52))


@obj("usbplug")
def usbplug(p, sel=False, **kw):
    p.add(p.stroke([(32, 40), (32, 62)], 5), rgb('#3A3F47'))
    p.add(p.rrect(20, 20, 44, 44, 3), p.lin((20, 20), (44, 44), rgb('#5A616B'), rgb('#1F2329')), gloss=0.3)
    p.add(p.rect(22, 4, 42, 22), p.lin((22, 4), (42, 22), METAL_HI, METAL_LO), outline=1.4)
    p.detail(p.rect(26, 8, 30, 12), INK)
    p.detail(p.rect(34, 8, 38, 12), INK)
    # the trident
    p.detail(p.stroke([(32, 41), (32, 26)], 1.2), WHITE)
    p.detail(p.stroke([(32, 34), (27, 30), (27, 27)], 1.0), WHITE)
    p.detail(p.stroke([(32, 36), (37, 32), (37, 29)], 1.0), WHITE)
    if sel:
        p.add(p.ellipse(39, 39, 43, 43), LED_ON, outline=0.7)
    return slots(badge=(30, 30, 0.52))


@obj("cable")
def cable(p, sel=False, **kw):
    pts = [(4, 50), (16, 42), (28, 48), (40, 36), (52, 40), (60, 30)] if not sel else [(4, 46), (16, 50), (28, 40), (40, 44), (52, 32), (60, 36)]
    p.add(p.stroke(pts, 4), p.lin((0, 0), (64, 0), lighter(g.BLUE, 0.3), g.BLUE_LO))
    return slots()


@obj("wifi")
def wifi(p, sel=False, bars=None, **kw):
    """A Wi-Fi fan; chosen, a ring more."""
    n = 3 if not sel else 4
    for k in range(n):
        r = 10 + k * 12
        p.add(p.arc(32 - r, 54 - r, 32 + r, 54 + r, 225, 315, 5), p.lin((0, 54 - r), (0, 54), lighter(g.BLUE, 0.3), g.BLUE_LO), outline=1.2)
    p.add(p.ellipse(27, 49, 37, 59), g.BLUE, outline=1.2)
    return slots(badge=(30, 30, 0.52))


@obj("network")
def network(p, sel=False, broken=False, **kw):
    """Three machines joined; broken, the link is cut; chosen, data flows."""
    for (x, y) in ((22, 4), (2, 40), (42, 40)):
        p.add(p.rrect(x, y, x + 20, y + 16, 2), p.lin((x, y), (x + 20, y + 16), METAL_HI, METAL_LO), gloss=0.25)
        p.detail(p.rect(x + 3, y + 3, x + 17, y + 11), SCREEN_LO)
    for a, b in (((32, 20), (12, 40)), ((32, 20), (52, 40)), ((22, 48), (42, 48))):
        p.add(p.stroke([a, b], 2.2), g.RED if broken and a == (32, 20) and b == (52, 40) else g.BLUE, outline=1.0)
    if broken:
        p.add(p.stroke([(38, 26), (46, 34)], 2.6), WHITE, outline=1.0)
    if sel and not broken:
        p.add(p.ellipse(19, 27, 25, 33), g.GOLD, outline=0.8)
    return slots(badge=(30, 30, 0.52))


@obj("chain")
def chain(p, sel=False, **kw):
    d = 0 if not sel else 3
    p.add(p.ring(4 + d, 18, 36 + d, 46, 6), p.lin((0, 18), (0, 46), METAL_HI, METAL_LO), outline=1.4)
    p.add(p.ring(28 - d, 18, 60 - d, 46, 6), p.lin((0, 18), (0, 46), METAL_HI, METAL_LO), outline=1.4)
    return slots(badge=(30, 30, 0.52))


@obj("envelope")
def envelope(p, sel=False, dot=False, **kw):
    """An envelope; chosen, its flap opens and a letter shows."""
    if sel:
        p.add(p.poly([(4, 20), (32, 2), (60, 20)]), p.lin((0, 2), (0, 20), WHITE, PAPER_LO), outline=1.8)
        p.add(p.rect(12, 8, 52, 34), WHITE, outline=1.4)
        lines(p, 17, 13, 46, 3, gap=4.5)
    p.add(p.rrect(4, 18, 60, 54, 2), p.lin((4, 18), (60, 54), PAPER_HI, PAPER_LO), gloss=0.2)
    p.detail(p.stroke([(5, 53), (32, 34), (59, 53)], 1.2), rgb('#9AA0A8'))
    if not sel:
        p.add(p.poly([(4, 18), (60, 18), (32, 38)]), p.lin((4, 18), (60, 38), WHITE, rgb('#C9CDD3')), outline=1.6)
    if dot:
        p.add(p.ellipse(48, 10, 62, 24), g.RED, outline=1.4)
    return slots(face=(20, 26, 0.38), badge=(30, 30, 0.52))


@obj("bubble")
def bubble(p, sel=False, two=False, **kw):
    """A speech bubble; two, a conversation; chosen, the dots move."""
    if two:
        p.add(p.rrect(26, 4, 62, 30, 9), p.lin((26, 4), (62, 30), lighter(g.GREEN, 0.3), g.GREEN_LO), gloss=0.3)
        p.add(p.poly([(50, 28), (58, 38), (44, 29)]), g.GREEN_LO, outline=1.4)
    p.add(p.rrect(2, 16 if two else 8, 46 if two else 60, 46 if two else 46, 10), p.lin((2, 10), (60, 46), WHITE, rgb('#BFD4EA')), gloss=0.3)
    p.add(p.poly([(12, 44), (8, 58), (24, 44)]), rgb('#BFD4EA'), outline=1.4)
    xs = (14, 24, 34) if two else (18, 32, 46)
    for i, x in enumerate(xs):
        up = 2 if (sel and i == 1) else 0
        cy = (31 if two else 27) - up
        p.detail(p.ellipse(x - 2.6, cy - 2.6, x + 2.6, cy + 2.6), g.BLUE_LO)
    return slots(face=(16, 12, 0.48), badge=(30, 30, 0.52))


@obj("globe")
def globe(p, sel=False, ring=False, **kw):
    """The world; chosen, it has turned."""
    cx, cy, r = 32, 32, 27
    sphere = p.ellipse(cx - r, cy - r, cx + r, cy + r)
    p.add(sphere, p.rad((cx - 10, cy - 11), r * 2.0, rgb('#7DB6F2'), rgb('#123A78')))
    shift = 11 if sel else 0
    land = []
    for dx in (-2 * r - 4, 0, 2 * r + 4):
        o = dx + shift
        land.append(p.poly([(cx - 18 + o, cy - 15), (cx - 5 + o, cy - 20), (cx + 3 + o, cy - 13), (cx - 4 + o, cy - 5),
                            (cx - 1 + o, cy + 5), (cx - 9 + o, cy + 13), (cx - 15 + o, cy + 3), (cx - 20 + o, cy - 5)]))
        land.append(p.poly([(cx + 8 + o, cy - 3), (cx + 19 + o, cy - 8), (cx + 24 + o, cy + 4), (cx + 17 + o, cy + 18), (cx + 9 + o, cy + 11)]))
    p.detail(g.inter(g.union(*land), sphere), p.lin((cx - r, cy - r), (cx + r, cy + r), rgb('#9BE07C'), rgb('#2E7D2A')))
    p.detail(g.inter(p.ellipse(cx - r * 0.75, cy - r * 0.85, cx - r * 0.05, cy - r * 0.35), sphere), lighter(rgb('#7DB6F2'), 0.5))
    if ring:
        ring_m = g.cut(p.ellipse(cx - r - 6, cy - 6, cx + r + 6, cy + 8), p.ellipse(cx - r - 3, cy - 3, cx + r + 3, cy + 5))
        front = g.cut(ring_m, p.rect(cx - r - 8, cy - 8, cx + r + 8, cy + 1))
        p.add(g.cut(g.cut(ring_m, sphere), front), p.lin((cx - r, 0), (cx + r, 0), g.GOLD, g.GOLD_LO), outline=1.2)
        p.add(front, p.lin((cx - r, 0), (cx + r, 0), rgb('#FFE58A'), g.GOLD_LO), outline=1.2)
    return slots(face=(20, 20, 0.4), badge=(30, 30, 0.52))


# ---- pictures and media ----------------------------------------------------------------------------


@obj("landscape")
def landscape(p, sel=False, frame=True, checks=False, rainbow=False, dots=False, **kw):
    """A framed picture of hills and the sun; chosen, the sun has risen."""
    if frame:
        p.add(p.rrect(2, 8, 62, 56, 2.5), p.lin((2, 8), (62, 56), rgb('#E9C27A'), rgb('#9C6A22')), gloss=0.25)
    m = p.rect(7, 13, 57, 51)
    if checks:
        p.detail(m, WHITE)
        for r in range(8):
            for c in range(10):
                if (r + c) % 2 == 0:
                    p.detail(p.rect(7 + c * 5, 13 + r * 4.75, 12 + c * 5, 17.75 + r * 4.75), rgb('#C9CED6'))
    else:
        p.detail(m, p.lin((0, 13), (0, 51), SKY_HI, SKY_LO))
    if rainbow:
        for i, c in enumerate((g.RED, g.ORANGE, g.GOLD, g.GREEN, g.BLUE, PURPLE)):
            p.detail(g.inter(p.arc(14 + i * 2.2, 18 + i * 2.2, 50 - i * 2.2, 54 - i * 2.2, 180, 360, 2.4), m), c)
    sy = 22 if not sel else 18
    p.detail(g.inter(p.ellipse(40, sy, 49, sy + 9), m), g.GOLD)
    p.detail(g.inter(p.poly([(7, 51), (7, 40), (19, 28), (30, 40), (37, 33), (57, 47), (57, 51)]), m), p.lin((0, 28), (0, 51), g.GREEN, g.GREEN_LO))
    if dots:
        for i in range(6):
            p.detail(p.ellipse(10 + i * 8, 46, 13 + i * 8, 49), (g.RED, g.GOLD, g.BLUE, PURPLE, g.ORANGE, WHITE)[i])
    if not frame:
        p.add(m, (0, 0, 0), outline=1.6) if False else None
    return slots(face=(16, 14, 0.5), badge=(30, 30, 0.52))


@obj("photo")
def photo(p, sel=False, **kw):
    """A photograph with a white border; chosen, it tilts."""
    t = 0 if not sel else 1
    p.add(p.poly([(4, 10 + t * 2), (58, 6 - t * 2), (62, 54 - t * 2), (8, 58 + t * 2)]), PAPER_HI)
    m = p.poly([(9, 14 + t * 2), (54, 11 - t * 2), (57, 44 - t * 2), (12, 47 + t * 2)])
    p.detail(m, p.lin((0, 11), (0, 47), SKY_HI, SKY_LO))
    p.detail(g.inter(p.ellipse(38, 16, 46, 24), m), g.GOLD)
    p.detail(g.inter(p.poly([(9, 47), (9, 36), (20, 26), (30, 36), (36, 30), (60, 42), (60, 50)]), m), p.lin((0, 26), (0, 50), g.GREEN, g.GREEN_LO))
    return slots(face=(16, 14, 0.45), badge=(30, 30, 0.52))


@obj("photostack")
def photostack(p, sel=False, **kw):
    p.add(p.poly([(10, 4), (60, 10), (56, 50), (6, 44)]), PAPER_HI, outline=1.8)
    p.sub(0, 4 if not sel else 0, 0.92).ic  # placement only
    photo(p.sub(2, 10 if not sel else 6, 0.92), sel=False)
    return slots(badge=(30, 30, 0.52))


@obj("film")
def film(p, sel=False, play=False, frame_only=False, **kw):
    """A strip of film; chosen, it has run on a frame."""
    p.add(p.rrect(4, 8, 60, 56, 2.5), p.lin((4, 8), (60, 56), g.DARK, g.DARK_LO), gloss=0.18)
    off = 0 if not sel else 3.3
    for y in (10.5, 50):
        for i in range(8):
            x = 7 + ((i * 6.6 + off) % 52.8)
            p.detail(p.rrect(x, y, x + 3.4, y + 2.6, 0.8), rgb('#E8EAED'))
    p.detail(p.rect(7, 15, 57, 47), p.lin((0, 15), (0, 47), rgb('#3A6BB0'), rgb('#1B3D6E')))
    if not frame_only:
        p.detail(p.rect(31.4, 15, 32.6, 47), g.DARK_LO)
    if play:
        p.add(p.ellipse(20, 19, 44, 43), p.lin((20, 19), (44, 43), WHITE, rgb('#C9D3E0')), gloss=0.3)
        if sel:
            p.detail(p.rect(27.5, 25.5, 30.5, 36.5), g.BLUE_LO)
            p.detail(p.rect(33.5, 25.5, 36.5, 36.5), g.BLUE_LO)
        else:
            p.detail(p.poly([(28.5, 24.5), (28.5, 37.5), (39, 31)]), g.BLUE_LO)
    return slots(face=(16, 16, 0.44), badge=(30, 30, 0.52))


@obj("flipbook")
def flipbook(p, sel=False, **kw):
    """A flip book; chosen, the figure has moved on."""
    for i in range(3):
        o = (2 - i) * 3
        p.add(p.rect(10 + o, 6 + o, 54 + o, 50 + o), p.lin((0, 6), (0, 54), WHITE, PAPER_LO), outline=1.6)
    x = 22 if not sel else 34
    p.add(p.ellipse(x, 12, x + 10, 22), g.ORANGE, outline=1.4)
    p.add(p.stroke([(x + 5, 22), (x + 5, 34)], 2.2), g.ORANGE, outline=1.0)
    p.add(p.stroke([(x + 5, 34), (x, 44)], 2.2), g.ORANGE, outline=1.0)
    p.add(p.stroke([(x + 5, 34), (x + 10, 44)], 2.2), g.ORANGE, outline=1.0)
    return slots(face=(16, 12, 0.44))


@obj("note")
def note(p, sel=False, two=True, **kw):
    """Music notes; chosen, they bounce."""
    d = -3 if sel else 0
    if two:
        p.add(g.union(p.ellipse(6, 42 + d, 22, 54 + d), p.stroke([(20.5, 48 + d), (20.5, 12 + d)], 3.4), p.ellipse(34, 36 - d, 50, 48 - d),
                      p.stroke([(48.5, 42 - d), (48.5, 6 - d)], 3.4), p.stroke([(20.5, 12 + d), (48.5, 6 - d)], 5.0)),
              p.lin((6, 6), (50, 54), rgb('#4A5562'), rgb('#14171B')), gloss=0.2)
    else:
        p.add(g.union(p.ellipse(14, 42 + d, 34, 56 + d), p.stroke([(32, 50 + d), (32, 8 + d)], 3.6), p.poly([(32, 8 + d), (50, 18 + d), (50, 26 + d), (32, 18 + d)])),
              p.lin((14, 8), (50, 56), rgb('#4A5562'), rgb('#14171B')), gloss=0.2)
    return slots(badge=(30, 30, 0.52))


@obj("waveform")
def waveform(p, sel=False, scissors=False, **kw):
    """A sound wave; chosen, it shifts."""
    p.add(p.rrect(2, 12, 62, 52, 4), p.lin((2, 12), (62, 52), rgb('#1C232C'), rgb('#0B0F14')), gloss=0.15)
    ph = 0 if not sel else 1.2
    pts = [(6 + i * 2, 32 + math.sin(i * 0.55 + ph) * (14 * math.exp(-((i - 13) / 9) ** 2))) for i in range(27)]
    p.detail(p.stroke(pts, 1.8), rgb('#7CFF6A'))
    return slots(badge=(30, 30, 0.52))


@obj("patterngrid")
def patterngrid(p, sel=False, notes=False, **kw):
    """A tracker's pattern; chosen, the play line moves down."""
    p.add(p.rrect(4, 4, 60, 60, 3), p.lin((4, 4), (60, 60), rgb('#1C232C'), rgb('#0B0F14')), gloss=0.12)
    row = 4 if not sel else 6
    for r in range(10):
        y = 8 + r * 5
        if r == row:
            p.detail(p.rect(6, y - 0.8, 58, y + 3.6), rgb('#2E5C9E'))
        for c in range(4):
            on = (r * 3 + c * 5) % 7 < 3
            col = rgb('#7CFF6A') if on else rgb('#33503A')
            if notes and on and c % 2 == 0:
                col = rgb('#FFD86A')
            p.detail(p.rect(8 + c * 13, y, 17 + c * 13, y + 2.6), col)
    return slots(badge=(30, 30, 0.52))


@obj("player")
def player(p, sel=False, wave=False, **kw):
    """A player window with transport buttons; chosen, play turns to pause."""
    p.add(p.rrect(2, 10, 62, 54, 4), p.lin((2, 10), (62, 54), rgb('#5A616B'), rgb('#1F2329')), gloss=0.25)
    p.detail(p.rect(7, 15, 57, 32), rgb('#1C2A20'))
    if wave:
        pts = [(9 + i * 2, 24 + math.sin(i * 0.8 + (1 if sel else 0)) * 5) for i in range(24)]
        p.detail(p.stroke(pts, 1.4), rgb('#7CFF6A'))
    else:
        lines(p, 10, 19, 48, 2, gap=5, h=2, c=rgb('#7CFF6A'), short=0.6)
    for i in range(4):
        x = 9 + i * 12.5
        p.add(p.rrect(x, 37, x + 10, 48, 1.5), p.lin((x, 37), (x + 10, 48), STEEL_HI, STEEL_LO), outline=1.0)
    if sel:
        p.detail(p.rect(37.4, 39.5, 39, 45.5), INK)
        p.detail(p.rect(40.6, 39.5, 42.2, 45.5), INK)
    else:
        p.detail(p.poly([(37.5, 39.5), (37.5, 45.5), (43, 42.5)]), INK)
    return slots(badge=(30, 30, 0.52))


@obj("wireframe")
def wireframe(p, sel=False, kind="sphere", **kw):
    """A wireframe sphere or cube; chosen, it has turned."""
    c = rgb('#3FB8B0')
    if kind == "cube":
        o = 0 if not sel else 4
        f = [(10 + o, 22), (40 + o, 22), (40 - o, 52), (10 - o, 52)]
        b = [(24 - o, 8), (54 - o, 8), (54 + o, 38), (24 + o, 38)]
        f = [(12, 22), (42, 22), (42, 52), (12, 52)]
        b = [(24 + o, 10 - o), (54 + o, 10 - o), (54 - o, 40 + o), (24 - o, 40 + o)]
        for a, bb in zip(f, b):
            p.add(p.stroke([a, bb], 1.8), c, outline=0.8)
        p.add(p.stroke(f + [f[0]], 1.8), c, outline=0.8)
        p.add(p.stroke(b + [b[0]], 1.8), c, outline=0.8)
        return slots(badge=(30, 30, 0.52))
    p.add(p.ring(6, 6, 58, 58, 1.8), c, outline=0.8)
    for k in range(3):
        w = [26, 16, 5][k]
        x0 = 32 - w + (4 if sel else 0)
        p.add(p.ring(x0, 6, 64 - x0 + (8 if sel else 0) - 8 if sel else 64 - x0, 58, 1.6), c, outline=0.8)
    for y in (18, 32, 46):
        hw = math.sqrt(max(0, 26 ** 2 - (y - 32) ** 2))
        p.add(p.stroke([(32 - hw, y), (32 + hw, y)], 1.6), c, outline=0.8)
    return slots(badge=(30, 30, 0.52))


# ---- tools and marks ----------------------------------------------------------------------------------


@obj("cog")
def cog(p, sel=False, tint="grey", teeth=8, **kw):
    """A cog; chosen, it has turned."""
    c, lo = colour(tint)
    a0 = 0 if not sel else 360 / teeth / 2
    pts = []
    for k in range(teeth * 4):
        a = math.radians(a0 + k * 360 / (teeth * 4))
        r = 27 if (k % 4) in (1, 2) else 20
        pts.append((32 + math.cos(a) * r, 32 + math.sin(a) * r))
    p.add(g.cut(p.poly(pts), p.ellipse(25, 25, 39, 39)), p.lin((6, 6), (58, 58), lighter(c, 0.2), lo), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("cogs")
def cogs(p, sel=False, **kw):
    cog(p.sub(-2, 10, 0.72), sel=sel, tint="grey")
    cog(p.sub(26, -2, 0.6), sel=not sel, tint="gold")
    return slots(badge=(32, 32, 0.5))


@obj("wrench")
def wrench(p, sel=False, **kw):
    a = math.radians(-45 if not sel else -30)
    d = (math.cos(a), math.sin(a))
    n = (-d[1], d[0])
    c = (32, 32)
    at = lambda t, u: (c[0] + d[0] * t + n[0] * u, c[1] + d[1] * t + n[1] * u)
    p.add(g.cut(g.union(p.poly([at(-24, -4), at(14, -4), at(14, 4), at(-24, 4)]), p.ellipse(*at(12, -11), *at(12, -11)) if False else p.ellipse(at(20, 0)[0] - 11, at(20, 0)[1] - 11, at(20, 0)[0] + 11, at(20, 0)[1] + 11)),
                p.poly([at(20, -4.5), at(34, -4.5), at(34, 4.5), at(20, 4.5)])), p.lin((10, 10), (54, 54), METAL_HI, METAL_LO), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("hammer")
def hammer(p, sel=False, **kw):
    a = -40 if not sel else -20
    t = math.radians(a)
    p.add(p.stroke([(14, 56), (14 + math.cos(t) * 36, 56 + math.sin(t) * 36)], 5), p.lin((14, 56), (40, 30), rgb('#C48A4A'), rgb('#7A4E22')))
    hx, hy = 14 + math.cos(t) * 36, 56 + math.sin(t) * 36
    n = (-math.sin(t), math.cos(t))
    d = (math.cos(t), math.sin(t))
    q = lambda u, v: (hx + n[0] * u + d[0] * v, hy + n[1] * u + d[1] * v)
    p.add(p.poly([q(-16, -5), q(14, -5), q(14, 5), q(-16, 5)]), p.lin(q(-16, -5), q(14, 5), METAL_HI, rgb('#5A616B')), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("pencil")
def pencil(p, sel=False, angle=-128, **kw):
    """A pencil, its point at the bottom left; chosen, it has moved along."""
    a = math.radians(angle)
    tx, ty = (14, 54) if not sel else (20, 50)
    d = (math.cos(a + math.pi), math.sin(a + math.pi))
    d = (-math.cos(math.radians(-50)), math.sin(math.radians(-50)))
    d = (0.72, -0.69)
    n = (-d[1], d[0])
    w = 5.4
    at = lambda t, s=0: (tx + d[0] * t + n[0] * s, ty + d[1] * t + n[1] * s)
    L = 52
    cone, fer, rub = 11, L - 10, L - 4
    p.add(p.poly([at(cone, -w), at(fer, -w), at(fer, w), at(cone, w)]), p.lin(at(cone, -w), at(cone, w), lighter(g.ORANGE, 0.3), g.ORANGE_LO))
    p.add(p.poly([at(fer, -w), at(rub, -w), at(rub, w), at(fer, w)]), p.lin(at(fer, -w), at(fer, w), STEEL_HI, STEEL_LO))
    p.add(p.poly([at(rub, -w), at(L, -w * 0.8), at(L, w * 0.8), at(rub, w)]), p.lin(at(rub, -w), at(rub, w), rgb('#F7A8B4'), rgb('#C25A6A')))
    p.add(p.poly([at(0), at(cone, -w), at(cone, w)]), p.lin(at(cone, -w), at(cone, w), rgb('#F4D9AE'), rgb('#C79A5A')))
    p.detail(p.poly([at(0), at(4, -w * 0.36), at(4, w * 0.36)]), INK)
    return slots(badge=(32, 32, 0.5))


@obj("pen")
def pen(p, sel=False, tint="gold", **kw):
    """A fountain pen's nib; chosen, it has moved."""
    c, lo = colour(tint)
    cx, cy = (32, 34) if not sel else (28, 38)
    a = math.radians(-35)
    rot = lambda x, y: (cx + x * math.cos(a) - y * math.sin(a), cy + x * math.sin(a) + y * math.cos(a))
    p.add(p.poly([rot(0, 24), rot(-11, -2), rot(-9, -16), rot(9, -16), rot(11, -2)]), p.lin(rot(-11, -16), rot(11, 12), lighter(c, 0.4), lo), gloss=0.3)
    p.detail(p.stroke([rot(0, 22), rot(0, 0)], 1.4), INK)
    p.detail(p.ellipse(*rot(-2.2, -3.4), *rot(2.2, 1.0)), INK)
    p.add(p.rrect(*rot(-10, -38), *rot(10, -16), 3), p.lin(rot(-10, -38), rot(10, -16), rgb('#5A616B'), rgb('#1F2329')), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("quill")
def quill(p, sel=False, **kw):
    d = 0 if not sel else 4
    p.add(p.poly([(12 + d, 58 - d), (20 + d, 40 - d), (50 + d, 4 - d), (56 + d, 8 - d), (28 + d, 44 - d)]), p.lin((20, 40), (56, 4), WHITE, rgb('#B9C4D2')), gloss=0.3)
    p.detail(p.stroke([(14 + d, 56 - d), (52 + d, 7 - d)], 1.0), rgb('#8A96A6'))
    return slots(badge=(32, 32, 0.5))


@obj("brush")
def brush(p, sel=False, paint="red", **kw):
    """A paintbrush; chosen, it has dipped in paint."""
    t = math.radians(-50 if not sel else -40)
    bx, by = 20, 46
    ex, ey = bx + math.cos(t) * 40, by + math.sin(t) * 40
    p.add(p.stroke([(bx, by), (ex, ey)], 5), p.lin((bx, by), (ex, ey), rgb('#C48A4A'), rgb('#7A4E22')))
    n = (-math.sin(t), math.cos(t))
    d = (math.cos(t), math.sin(t))
    q = lambda u, v: (bx + n[0] * u + d[0] * v, by + n[1] * u + d[1] * v)
    p.add(p.poly([q(-4.5, 4), q(4.5, 4), q(4.5, -4), q(-4.5, -4)]), p.lin(q(-4, 0), q(4, 0), METAL_HI, METAL_LO))
    p.add(p.poly([q(-4.5, -4), q(4.5, -4), q(2, -16), q(0, -19), q(-2, -16)]), colour(paint)[0] if sel else rgb('#3B3F46'))
    return slots(badge=(32, 32, 0.5))


@obj("palette")
def palette(p, sel=False, **kw):
    p.add(g.cut(p.ellipse(4, 10, 60, 56), p.ellipse(36, 38, 46, 48)), p.lin((4, 10), (60, 56), rgb('#F0D8A8'), rgb('#B98A4A')), gloss=0.3)
    for i, (x, y) in enumerate(((16, 22), (28, 16), (42, 18), (50, 30), (14, 36))):
        c = (g.RED, g.GOLD, g.GREEN, g.BLUE, PURPLE)[i]
        p.add(p.ellipse(x - 4.5, y - 4.5, x + 4.5, y + 4.5), c, outline=1.0)
    if sel:
        brush(p.sub(20, -6, 0.7), sel=True)
    return slots(badge=(32, 32, 0.5))


@obj("swatches")
def swatches(p, sel=False, **kw):
    cols = [(g.RED, g.RED_LO), (g.ORANGE, g.ORANGE_LO), (g.GOLD, g.GOLD_LO), (g.GREEN, g.GREEN_LO), (g.BLUE, g.BLUE_LO)]
    for i, (c, lo) in enumerate(cols):
        a = math.radians(-70 + i * 17 + (6 if sel and i == 4 else 0))
        cx, cy = 26, 56
        L, w = 46, 6
        tip = (cx + math.cos(a) * L, cy + math.sin(a) * L)
        n = (-math.sin(a) * w, math.cos(a) * w)
        p.add(p.poly([(cx - n[0], cy - n[1]), (cx + n[0], cy + n[1]), (tip[0] + n[0], tip[1] + n[1]), (tip[0] - n[0], tip[1] - n[1])]),
              p.lin((cx, cy), tip, lighter(c, 0.25), lo), outline=1.4)
    p.add(p.ellipse(23, 53, 29, 59), rgb('#B8BCC4'), outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("colourwheel")
def colourwheel(p, sel=False, **kw):
    cols = (g.RED, g.ORANGE, g.GOLD, g.GREEN, TEAL, g.BLUE, PURPLE, PINK)
    a0 = 0 if not sel else 22
    for i, c in enumerate(cols):
        m = g.blank()
        a, b = p.T(4, 4), p.T(60, 60)
        ImageDraw.Draw(m).pieslice([g.P(a[0]), g.P(a[1]), g.P(b[0]), g.P(b[1])], a0 + i * 45, a0 + (i + 1) * 45, fill=255)
        p.detail(m, c)
    p.add(g.cut(p.ellipse(4, 4, 60, 60), p.ellipse(22, 22, 42, 42)), (0, 0, 0), outline=2.0) if False else None
    p.add(p.ring(4, 4, 60, 60, 0.6), INK, outline=1.6)
    p.add(p.ellipse(22, 22, 42, 42), WHITE, outline=1.6)
    return slots(badge=(32, 32, 0.5))


@obj("dropper")
def dropper(p, sel=False, **kw):
    t = math.radians(-45)
    p.add(p.stroke([(18, 46), (42, 22)], 6), p.lin((18, 46), (42, 22), WHITE, rgb('#B9C4D2')), gloss=0.3)
    p.add(p.rrect(38, 8, 56, 26, 6), p.lin((38, 8), (56, 26), rgb('#5A616B'), rgb('#1F2329')), gloss=0.3)
    p.add(p.stroke([(18, 46), (10, 54)], 2.2), g.RED, outline=1.0)
    if sel:
        p.add(p.ellipse(5, 56, 11, 62), g.RED, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("magnifier")
def magnifier(p, sel=False, **kw):
    """A magnifying glass; chosen, it has moved."""
    cx, cy = (26, 26) if not sel else (30, 30)
    r = 18
    a = math.radians(45)
    hx0, hy0 = cx + math.cos(a) * r * 0.95, cy + math.sin(a) * r * 0.95
    hx1, hy1 = cx + math.cos(a) * (r + 18), cy + math.sin(a) * (r + 18)
    p.add(p.stroke([(hx0, hy0), (hx1, hy1)], 6.4), p.lin((hx0, hy0), (hx1, hy1), rgb('#6C8C5A'), rgb('#2F4A28')))
    p.add(p.ellipse(cx - r, cy - r, cx + r, cy + r), p.lin((cx - r, cy - r), (cx + r, cy + r), rgb('#E9EEF5'), rgb('#7D8796')))
    p.add(p.ellipse(cx - r + 4, cy - r + 4, cx + r - 4, cy + r - 4), p.rad((cx - r * 0.35, cy - r * 0.35), r * 1.6, WHITE, rgb('#9EB6D8')), outline=1.2)
    p.detail(p.ellipse(cx - r * 0.55, cy - r * 0.6, cx - r * 0.05, cy - r * 0.2), WHITE)
    return slots(badge=(32, 32, 0.5))


@obj("wand")
def wand(p, sel=False, **kw):
    """A magic wand; chosen, its star flares and sparkles."""
    p.add(p.stroke([(10, 56), (40, 22)], 5), p.lin((10, 56), (40, 22), rgb('#5A616B'), rgb('#1F2329')))
    p.detail(p.stroke([(34, 29), (40, 22)], 5), WHITE)
    r = 14 if sel else 11
    p.add(p.poly(star_pts(46, 16, r)), p.rad((44, 14), r * 1.6, rgb('#FFF6B8'), g.GOLD_LO), outline=1.6, gloss=0.2)
    if sel:
        for (x, y, s) in ((60, 6, 3.6), (58, 30, 2.8), (30, 6, 3)):
            p.add(p.poly(star_pts(x, y, s, inner=0.35, n=4, rot=0)), g.GOLD, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("star")
def star(p, sel=False, tint="gold", **kw):
    """A star; chosen, it shines brighter."""
    c, lo = colour(tint)
    r = 28 if not sel else 30
    p.add(p.poly(star_pts(32, 34, r)), p.rad((28, 28), r * 1.6, lighter(c, 0.55), lo), gloss=0.25)
    if sel:
        for (x, y, s) in ((56, 8, 4), (8, 12, 3.4)):
            p.add(p.poly(star_pts(x, y, s, inner=0.35, n=4, rot=0)), c, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("starburst")
def starburst(p, sel=False, **kw):
    n = 12
    r0, r1 = (28, 16) if not sel else (31, 15)
    p.add(p.poly(star_pts(32, 32, r0, inner=r1 / r0, n=n)), p.rad((28, 28), 36, rgb('#FFF6B8'), g.ORANGE_LO), gloss=0.25)
    p.add(p.poly(star_pts(32, 32, 12)), p.rad((30, 30), 16, WHITE, g.GOLD), outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("sparkle")
def sparkle(p, sel=False, **kw):
    s = 28 if not sel else 31
    p.add(p.poly(star_pts(32, 32, s, inner=0.28, n=4, rot=0)), p.rad((28, 28), 36, WHITE, g.GOLD), gloss=0.2)
    return slots(badge=(32, 32, 0.5))


@obj("lifebuoy")
def lifebuoy(p, sel=False, **kw):
    a0 = 0 if not sel else 45
    p.add(g.cut(p.ellipse(4, 4, 60, 60), p.ellipse(20, 20, 44, 44)), WHITE)
    m = g.blank()
    a, b = p.T(4, 4), p.T(60, 60)
    for k in range(4):
        ImageDraw.Draw(m).pieslice([g.P(a[0]), g.P(a[1]), g.P(b[0]), g.P(b[1])], a0 + k * 90 + 20, a0 + k * 90 + 70, fill=255)
    p.detail(g.cut(g.inter(m, p.ellipse(4, 4, 60, 60)), p.ellipse(20, 20, 44, 44)), g.RED)
    p.add(p.ring(4, 4, 60, 60, 0.4), INK, outline=1.6)
    p.add(p.ring(20, 20, 44, 44, 0.4), INK, outline=1.6)
    return slots(badge=(32, 32, 0.5))


@obj("shield")
def shield(p, sel=False, tint="blue", **kw):
    c, lo = colour(tint)
    p.add(p.poly([(32, 2), (58, 10), (56, 34), (32, 62), (8, 34), (6, 10)]), p.lin((6, 2), (58, 62), lighter(c, 0.3), lo), gloss=0.3)
    p.detail(p.poly([(32, 8), (52, 14), (50, 33), (32, 54)]), lighter(c, 0.15))
    if sel:
        p.add(p.stroke([(20, 32), (29, 41), (45, 22)], 4), WHITE, outline=1.0)
    return slots(face=(18, 16, 0.44), badge=(32, 32, 0.5))


@obj("lock")
def lock(p, sel=False, **kw):
    """A padlock; chosen, it is open."""
    up = 8 if sel else 0
    p.add(p.arc(16, 4 - up, 48, 36 - up, 180, 360, 5), p.lin((16, 4), (48, 36), METAL_HI, METAL_LO), outline=1.6)
    p.add(p.stroke([(18.5, 20 - up), (18.5, 30)], 5), p.lin((16, 0), (21, 0), METAL_HI, METAL_LO), outline=1.6) if up else \
        p.add(p.stroke([(18.5, 20), (18.5, 30)], 5), p.lin((16, 0), (21, 0), METAL_HI, METAL_LO), outline=1.6)
    p.add(p.stroke([(45.5, 20 - up), (45.5, 30 - up)], 5), p.lin((43, 0), (48, 0), METAL_HI, METAL_LO), outline=1.6)
    p.add(p.rrect(10, 28, 54, 60, 4), p.lin((10, 28), (54, 60), rgb('#F6D04D'), rgb('#B98410')), gloss=0.3)
    p.detail(p.ellipse(29, 38, 35, 44), INK)
    p.detail(p.rect(30.8, 42, 33.2, 50), INK)
    return slots(badge=(32, 32, 0.5))


@obj("keylock")
def door_key(p, sel=False, **kw):
    a = 0 if not sel else -15
    t = math.radians(a)
    rot = lambda x, y: (32 + (x - 32) * math.cos(t) - (y - 32) * math.sin(t), 32 + (x - 32) * math.sin(t) + (y - 32) * math.cos(t))
    p.add(g.cut(p.ellipse(*rot(4, 18), *rot(30, 44)) if a == 0 else p.ellipse(4, 18 + a / 3, 30, 44 + a / 3), p.ellipse(11, 26, 19, 36)),
          p.lin((4, 18), (30, 44), rgb('#F6D04D'), rgb('#B98410')), gloss=0.3)
    p.add(g.union(p.poly([rot(28, 28), rot(60, 28), rot(60, 34), rot(28, 34)]), p.poly([rot(48, 34), rot(54, 34), rot(54, 42), rot(48, 42)]),
                  p.poly([rot(40, 34), rot(45, 34), rot(45, 40), rot(40, 40)])), p.lin((28, 28), (60, 42), rgb('#F6D04D'), rgb('#B98410')))
    return slots(badge=(32, 32, 0.5))


@obj("flag")
def flag(p, sel=False, **kw):
    """A flag on its pole; chosen, it waves."""
    p.add(p.stroke([(12, 6), (12, 60)], 3), p.lin((10, 0), (14, 0), METAL_HI, METAL_LO))
    w = 4 if not sel else -4
    p.add(p.poly([(14, 8), (34, 8 + w), (56, 8), (56, 36), (34, 36 + w), (14, 36)]), p.lin((14, 8), (56, 36), lighter(g.RED, 0.2), g.RED_LO), gloss=0.25)
    p.detail(p.poly([(14, 18), (34, 18 + w), (56, 18), (56, 26), (34, 26 + w), (14, 26)]), WHITE)
    return slots(badge=(32, 32, 0.5))


@obj("bookmark")
def bookmark(p, sel=False, tint="red", **kw):
    c, lo = colour(tint)
    bot = 60 if not sel else 56
    p.add(p.poly([(16, 4), (48, 4), (48, bot), (32, bot - 14), (16, bot)]), p.lin((16, 4), (48, 60), lighter(c, 0.25), lo), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("tag")
def tag(p, sel=False, tint="gold", label="", **kw):
    """A label tag on its string; chosen, it swings."""
    c, lo = colour(tint)
    a = 0 if not sel else 10
    t = math.radians(a)
    rot = lambda x, y: (12 + (x - 12) * math.cos(t) - (y - 12) * math.sin(t), 12 + (x - 12) * math.sin(t) + (y - 12) * math.cos(t))
    p.add(g.cut(p.poly([rot(8, 8), rot(36, 8), rot(60, 32), rot(36, 56), rot(8, 30)]), p.ellipse(*rot(13, 13), *rot(19, 19))),
          p.lin((8, 8), (60, 56), lighter(c, 0.4), lo), gloss=0.3)
    if label:
        p.detail(p.text(*rot(32, 30), 11, label), INK)
    return slots(badge=(32, 32, 0.5))


@obj("puzzle")
def puzzle(p, sel=False, tint="green", **kw):
    c, lo = colour(tint)
    d = 0 if not sel else 3
    m = g.union(p.rect(10, 18, 46, 54), p.ellipse(22, 6 - d, 34, 20 - d), p.ellipse(42, 30, 56, 42))
    m = g.cut(m, p.ellipse(4, 30, 16, 42))
    p.add(m, p.lin((10, 6), (56, 54), lighter(c, 0.3), lo), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("seal")
def seal(p, sel=False, **kw):
    p.add(p.poly([(20, 40), (12, 62), (22, 56), (28, 62), (30, 42)]), g.BLUE, outline=1.4)
    p.add(p.poly([(44, 40), (52, 62), (42, 56), (36, 62), (34, 42)]), g.BLUE, outline=1.4)
    p.add(p.poly(star_pts(32, 28, 26, inner=0.85, n=16)), p.rad((26, 22), 34, rgb('#FFE58A'), g.GOLD_LO), gloss=0.3)
    p.add(p.ellipse(22, 18, 42, 38), p.rad((28, 24), 22, rgb('#FFE58A'), g.GOLD), outline=1.2)
    if sel:
        p.detail(p.stroke([(25, 28), (30, 33), (39, 23)], 2.2), WHITE)
    return slots(badge=(32, 32, 0.5))


@obj("crown")
def crown(p, sel=False, **kw):
    """ARexx's crown; chosen, its jewels shine."""
    p.add(p.poly([(6, 50), (4, 16), (20, 30), (32, 10), (44, 30), (60, 16), (58, 50)]), p.lin((4, 10), (60, 50), rgb('#FFE58A'), g.GOLD_LO), gloss=0.3)
    p.add(p.rrect(6, 46, 58, 56, 2), p.lin((0, 46), (0, 56), rgb('#F6D04D'), rgb('#B98410')), outline=1.6)
    for x, c in ((18, g.RED), (32, g.BLUE), (46, g.GREEN)):
        p.add(p.ellipse(x - 3.5, 47.5, x + 3.5, 54.5), lighter(c, 0.4) if sel else c, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("bell")
def bell(p, sel=False, **kw):
    a = 0 if not sel else 12
    t = math.radians(a)
    rot = lambda x, y: (32 + (x - 32) * math.cos(t) - (y - 6) * math.sin(t), 6 + (x - 32) * math.sin(t) + (y - 6) * math.cos(t))
    p.add(p.poly([rot(32, 6), rot(44, 12), rot(48, 40), rot(56, 48), rot(8, 48), rot(16, 40), rot(20, 12)]), p.lin((8, 6), (56, 48), rgb('#FFE58A'), g.GOLD_LO), gloss=0.3)
    p.add(p.ellipse(*rot(27, 46), *rot(37, 56)), g.GOLD_LO, outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("bolt")
def bolt(p, sel=False, **kw):
    """A lightning bolt; chosen, it flashes."""
    c = rgb('#FFE58A') if sel else g.GOLD
    p.add(p.poly([(36, 2), (12, 36), (30, 36), (24, 62), (52, 24), (34, 24), (42, 2)]), p.lin((12, 2), (52, 62), lighter(c, 0.3), g.GOLD_LO), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("power")
def power(p, sel=False, **kw):
    c = g.GREEN if sel else g.BLUE
    p.add(p.arc(8, 10, 56, 58, 300, 240 + 360, 6) if False else p.arc(8, 10, 56, 58, -60, 240, 6), c, outline=1.4)
    p.add(p.stroke([(32, 4), (32, 30)], 6), c, outline=1.4)
    return slots(badge=(32, 32, 0.5))


@obj("rocket")
def rocket(p, sel=False, **kw):
    d = 0 if not sel else -5
    p.add(p.poly([(14, 46 + d), (6, 58 + d), (20, 52 + d)]), g.RED, outline=1.4)
    p.add(p.poly([(50, 46 + d), (58, 58 + d), (44, 52 + d)]), g.RED, outline=1.4)
    p.add(p.poly([(32, 2 + d), (44, 18 + d), (44, 50 + d), (20, 50 + d), (20, 18 + d)]), p.lin((20, 2), (44, 50), WHITE, rgb('#AEB6C2')), gloss=0.3)
    p.add(p.ellipse(26, 18 + d, 38, 30 + d), p.rad((30, 22 + d), 10, rgb('#9FD0FF'), g.BLUE_LO), outline=1.2)
    if sel:
        p.add(p.poly([(24, 50 + d), (40, 50 + d), (32, 64)]), p.lin((0, 50), (0, 64), g.GOLD, g.ORANGE), outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("house")
def house(p, sel=False, **kw):
    p.add(p.rect(12, 28, 52, 58), p.lin((12, 28), (52, 58), rgb('#F2EFE4'), rgb('#B1AC98')), gloss=0.2)
    p.add(p.poly([(4, 32), (32, 6), (60, 32)]), p.lin((4, 6), (60, 32), lighter(g.RED, 0.2), g.RED_LO), gloss=0.3)
    p.add(p.rect(26, 40, 38, 58), BROWN, outline=1.4)
    p.add(p.rect(42, 36, 50, 44), rgb('#FFE58A') if sel else SKY_HI, outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("broom")
def broom(p, sel=False, **kw):
    a = -40 if not sel else -55
    t = math.radians(a)
    p.add(p.stroke([(20, 44), (20 + math.cos(t) * 40, 44 + math.sin(t) * 40)], 3.6), p.lin((20, 44), (50, 14), rgb('#C48A4A'), rgb('#7A4E22')))
    p.add(p.poly([(8, 40), (26, 40), (30, 60), (2, 60)]), p.lin((2, 40), (30, 60), rgb('#F2D58A'), rgb('#B98A2A')), gloss=0.2)
    return slots(badge=(32, 32, 0.5))


@obj("battery")
def battery(p, sel=False, **kw):
    p.add(p.rect(56, 24, 62, 40), METAL_LO, outline=1.4)
    p.add(p.rrect(4, 16, 56, 48, 4), p.lin((4, 16), (56, 48), METAL_HI, METAL_LO), gloss=0.3)
    n = 3 if not sel else 4
    for i in range(n):
        p.detail(p.rect(9 + i * 11.5, 21, 18 + i * 11.5, 43), g.GREEN)
    return slots(badge=(32, 32, 0.5))


@obj("ruler")
def ruler(p, sel=False, **kw):
    t = -30 if not sel else -20
    a = math.radians(t)
    d = (math.cos(a), math.sin(a))
    n = (-d[1], d[0])
    o = (8, 50)
    at = lambda u, v: (o[0] + d[0] * u + n[0] * v, o[1] + d[1] * u + n[1] * v)
    p.add(p.poly([at(0, 0), at(56, 0), at(56, 12), at(0, 12)]), p.lin(at(0, 0), at(0, 12), rgb('#FFE58A'), g.GOLD_LO), gloss=0.3)
    for i in range(1, 12):
        p.detail(p.stroke([at(i * 4.6, 0.4), at(i * 4.6, 4 if i % 2 else 6.5)], 0.8), INK)
    return slots(badge=(32, 32, 0.5))


@obj("scales")
def scales(p, sel=False, **kw):
    t = 0 if not sel else 6
    p.add(p.stroke([(32, 8), (32, 54)], 3), METAL_LO)
    p.add(p.rrect(18, 52, 46, 58, 2), METAL_LO)
    p.add(p.stroke([(8, 16 + t), (56, 16 - t)], 3), METAL_LO)
    for x, y in ((10, 16 + t), (54, 16 - t)):
        p.add(p.arc(x - 9, y + 4, x + 9, y + 20, 0, 180, 3), p.lin((x - 9, 0), (x + 9, 0), rgb('#FFE58A'), g.GOLD_LO), outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("die")
def die(p, sel=False, **kw):
    p.add(p.rrect(6, 6, 58, 58, 9), p.lin((6, 6), (58, 58), WHITE, rgb('#C9CED6')), gloss=0.3)
    pts = ((20, 20), (44, 44), (32, 32)) if not sel else ((20, 20), (44, 20), (20, 44), (44, 44))
    for x, y in pts:
        p.detail(p.ellipse(x - 4, y - 4, x + 4, y + 4), g.RED)
    return slots(badge=(32, 32, 0.5))


@obj("bug")
def bug(p, sel=False, **kw):
    d = 0 if not sel else 3
    for i, y in enumerate((26, 36, 46)):
        p.add(p.stroke([(18, y), (6, y - 4 + (d if i == 1 else 0))], 2.2), INK, outline=0)
        p.add(p.stroke([(46, y), (58, y - 4 + (d if i == 1 else 0))], 2.2), INK, outline=0)
    p.add(p.ellipse(16, 18, 48, 58), p.lin((16, 18), (48, 58), lighter(g.GREEN, 0.2), g.GREEN_LO), gloss=0.35)
    p.add(p.ellipse(22, 6, 42, 24), CHIP_LO, outline=1.4)
    p.detail(p.rect(31.4, 24, 32.6, 58), darker(g.GREEN_LO, 0.3))
    return slots(badge=(32, 32, 0.5))


@obj("moon")
def moon(p, sel=False, **kw):
    p.add(g.cut(p.ellipse(6, 6, 58, 58), p.ellipse(22, 0, 70, 48)), p.lin((6, 6), (58, 58), rgb('#FFF3B0'), g.GOLD_LO), gloss=0.3)
    if sel:
        p.add(p.poly(star_pts(50, 44, 7, inner=0.4)), g.GOLD, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("cloud")
def cloud(p, sel=False, storm=False, **kw):
    if storm:
        bolt(p.sub(18, 26, 0.55), sel=sel)
    p.add(g.union(p.ellipse(4, 22, 30, 46), p.ellipse(16, 8, 46, 38), p.ellipse(34, 18, 60, 44), p.rrect(10, 30, 54, 46, 8)),
          p.lin((4, 8), (60, 46), (WHITE if not storm else rgb('#9AA2AD')), (rgb('#BFD4EA') if not storm else rgb('#4A515B'))), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("branches")
def branches(p, sel=False, **kw):
    p.add(p.stroke([(18, 58), (18, 6)], 3.2), g.BLUE, outline=1.0)
    p.add(p.stroke([(18, 44), (44, 32), (44, 14 if not sel else 10)], 3.2), g.ORANGE, outline=1.0)
    for x, y, c in ((18, 6, g.BLUE), (18, 58, g.BLUE), (18, 30, g.BLUE), (44, 14 if not sel else 10, g.ORANGE)):
        p.add(p.ellipse(x - 5, y - 5, x + 5, y + 5), c, outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("tiles")
def tiles(p, sel=False, **kw):
    for r in range(3):
        for c in range(3):
            on = (r + c + (1 if sel else 0)) % 2 == 0
            p.add(p.rect(4 + c * 19, 4 + r * 19, 21 + c * 19, 21 + r * 19), lighter(g.BLUE, 0.3) if on else rgb('#C9D3E0'), outline=1.4)
    return slots(badge=(32, 32, 0.5))


@obj("eye")
def eye(p, sel=False, **kw):
    p.add(g.inter(p.ellipse(2, 10, 62, 74), p.ellipse(2, -10, 62, 54)), WHITE, outline=2.0)
    x = 32 if not sel else 38
    p.add(p.ellipse(x - 12, 20, x + 12, 44), p.rad((x - 4, 28), 16, rgb('#7DB6F2'), g.BLUE_LO), outline=1.2)
    p.detail(p.ellipse(x - 5, 27, x + 5, 37), INK)
    p.detail(p.ellipse(x - 6, 24, x - 2, 28), WHITE)
    return slots(badge=(32, 32, 0.5))


@obj("person")
def person(p, sel=False, two=False, **kw):
    def one(x, c):
        p.add(p.ellipse(x - 8, 8, x + 8, 24), p.lin((x - 8, 8), (x + 8, 24), rgb('#F6D2B0'), rgb('#C08A5E')), gloss=0.3)
        p.add(p.rrect(x - 14, 28, x + 14, 60, 10), p.lin((x - 14, 28), (x + 14, 60), lighter(c, 0.2), darker(c, 0.3)), gloss=0.3)
    if two:
        one(44, g.GREEN)
        one(22, g.BLUE if not sel else g.ORANGE)
    else:
        one(32, g.BLUE if not sel else g.ORANGE)
    return slots(badge=(32, 32, 0.5))


@obj("mouth")
def mouth(p, sel=False, **kw):
    """A mouth speaking; chosen, it opens wider."""
    o = 6 if not sel else 12
    p.add(g.union(p.ellipse(4, 24 - o, 60, 40 + o)), p.lin((4, 20), (60, 44), lighter(g.RED, 0.2), g.RED_LO), gloss=0.3)
    p.detail(p.ellipse(12, 30 - o * 0.6, 52, 34 + o * 0.6), rgb('#4A1010'))
    p.detail(p.rect(18, 30 - o * 0.6, 46, 32 - o * 0.6 + 2), WHITE)
    return slots(badge=(32, 32, 0.5))


@obj("yak")
def yak(p, sel=False, **kw):
    """A yak's head, horns and all; chosen, it nods."""
    d = 0 if not sel else 3
    p.add(p.arc(2, 4 + d, 26, 30 + d, 160, 300, 4), rgb('#EDE6D2'), outline=1.4)
    p.add(p.arc(38, 4 + d, 62, 30 + d, 240, 380, 4), rgb('#EDE6D2'), outline=1.4)
    p.add(p.poly([(14, 14 + d), (50, 14 + d), (54, 36 + d), (44, 60 + d), (20, 60 + d), (10, 36 + d)]), p.lin((10, 14), (54, 60), rgb('#8A6A4A'), rgb('#3A2614')), gloss=0.25)
    for x in (24, 40):
        p.detail(p.ellipse(x - 3, 28 + d, x + 3, 34 + d), INK)
    p.add(p.rrect(22, 44 + d, 42, 58 + d, 6), rgb('#5A4026'), outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("seed")
def seed(p, sel=False, **kw):
    p.add(p.ellipse(16, 30, 48, 60), p.lin((16, 30), (48, 60), rgb('#C48A4A'), rgb('#6E4520')), gloss=0.3)
    h = 14 if not sel else 22
    p.add(p.stroke([(32, 32), (32, 32 - h)], 2.6), g.GREEN, outline=1.0)
    p.add(p.poly([(32, 32 - h + 4), (18, 32 - h - 4), (24, 32 - h + 6)]), g.GREEN, outline=1.2)
    p.add(p.poly([(32, 32 - h + 2), (48, 32 - h - 8), (42, 32 - h + 4)]), g.GREEN, outline=1.2)
    return slots(badge=(32, 32, 0.5))


@obj("fish")
def fish(p, sel=False, **kw):
    t = 4 if not sel else -4
    p.add(p.poly([(46, 32), (60, 20 + t), (60, 44 - t)]), g.ORANGE_LO, outline=1.4)
    p.add(p.ellipse(4, 18, 50, 46), p.lin((4, 18), (50, 46), lighter(g.ORANGE, 0.2), g.ORANGE_LO), gloss=0.3)
    p.detail(p.ellipse(13, 26, 19, 32), INK)
    return slots(badge=(32, 32, 0.5))


@obj("bird")
def bird(p, sel=False, **kw):
    w = 0 if not sel else 10
    p.add(p.poly([(26, 30), (46, 10 + w), (40, 34)]), g.BLUE_LO, outline=1.4)
    p.add(p.ellipse(10, 24, 50, 50), p.lin((10, 24), (50, 50), lighter(g.BLUE, 0.3), g.BLUE_LO), gloss=0.3)
    p.add(p.ellipse(38, 16, 58, 36), p.lin((38, 16), (58, 36), lighter(g.BLUE, 0.3), g.BLUE_LO), gloss=0.3)
    p.add(p.poly([(56, 24), (64, 28), (56, 30)]), g.GOLD, outline=1.0)
    p.detail(p.ellipse(48, 22, 52, 26), INK)
    return slots(badge=(32, 32, 0.5))


@obj("penguin")
def penguin(p, sel=False, **kw):
    p.add(p.ellipse(12, 6, 52, 62), p.lin((12, 6), (52, 62), rgb('#3A3F47'), rgb('#0B0F14')), gloss=0.25)
    p.detail(p.ellipse(19, 18, 45, 60), WHITE)
    for x in (26, 38):
        p.detail(p.ellipse(x - 3, 14, x + 3, 20), WHITE)
        p.detail(p.ellipse(x - 1.4, 15.6, x + 1.4, 18.4), INK)
    p.add(p.poly([(26, 22), (38, 22), (32, 30)]), g.GOLD, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("knife")
def knife(p, sel=False, **kw):
    a = 0 if not sel else 6
    p.add(p.poly([(4, 40 + a), (12, 26 + a * 0.5), (44, 16), (46, 32)]), p.lin((4, 16), (46, 40), METAL_HI, METAL_LO), gloss=0.35)
    p.add(p.rrect(44, 20, 62, 32, 3), p.lin((44, 20), (62, 32), rgb('#5A616B'), rgb('#1F2329')), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("pointer")
def pointer(p, sel=False, faded=False, **kw):
    """The Workbench pointer; chosen, it clicks."""
    d = 0 if not sel else 2
    pts = [(14 + d, 4 + d), (14 + d, 52 + d), (25 + d, 41 + d), (33 + d, 59 + d), (41 + d, 55 + d), (33 + d, 38 + d), (50 + d, 38 + d)]
    p.add(p.poly(pts), p.lin((14, 4), (50, 59), WHITE if not faded else rgb('#E2E5E9'), rgb('#B9C4D2') if not faded else rgb('#E9EEF5')), outline=2.0 if not faded else 1.0, gloss=0.2)
    if sel:
        for a in (200, 240, 280):
            t = math.radians(a)
            p.add(p.stroke([(14 + math.cos(t) * 5, 4 + math.sin(t) * 5), (14 + math.cos(t) * 11, 4 + math.sin(t) * 11)], 1.6), g.GOLD, outline=0.8)
    return slots(badge=(32, 32, 0.5))


@obj("target")
def target(p, sel=False, **kw):
    for i, c in enumerate((g.RED, WHITE, g.RED, WHITE, g.RED)):
        r = 28 - i * 5.5
        p.add(p.ellipse(32 - r, 32 - r, 32 + r, 32 + r), c, outline=1.2 if i == 0 else 0)
    return slots(badge=(32, 32, 0.5))


@obj("switch")
def switch(p, sel=False, **kw):
    p.add(p.rrect(4, 18, 60, 46, 14), p.lin((4, 18), (60, 46), g.GREEN if sel else rgb('#9AA2AD'), g.GREEN_LO if sel else rgb('#5E6672')), gloss=0.3)
    x = 46 if sel else 18
    p.add(p.ellipse(x - 12, 20, x + 12, 44), p.lin((x - 12, 20), (x + 12, 44), WHITE, rgb('#C9CED6')), outline=1.4, gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("dot")
def dot(p, sel=False, tint="gold", **kw):
    c, lo = colour(tint)
    r = 16 if not sel else 20
    p.add(p.ellipse(32 - r, 32 - r, 32 + r, 32 + r), p.rad((28, 28), r * 1.8, lighter(c, 0.4), lo), gloss=0.3)
    return slots(badge=(32, 32, 0.5))


@obj("light")
def light(p, sel=False, tint="gold", **kw):
    c = AMBER if tint == "gold" else colour(tint)[0]
    p.add(p.ellipse(10, 10, 54, 54), p.rad((24, 24), 50, lighter(c, 0.5) if sel else c, darker(c, 0.4)), gloss=0.4)
    return slots(badge=(32, 32, 0.5))


@obj("progress")
def progress(p, sel=False, **kw):
    p.add(p.rrect(4, 24, 60, 40, 4), WHITE)
    w = 30 if not sel else 44
    p.detail(p.rrect(6, 26, 6 + w, 38, 3), p.lin((0, 26), (0, 38), lighter(g.GREEN, 0.3), g.GREEN_LO))
    return slots(badge=(32, 32, 0.5))


# ---- interface pieces ------------------------------------------------------------------------------


@obj("panes")
def panes(p, sel=False, **kw):
    """Two file panes, a file chosen; chosen, the choice moves across."""
    p.add(p.rrect(2, 6, 62, 56, 3.5), p.lin((2, 6), (62, 56), PAPER_HI, rgb('#C9CCD1')), gloss=0.12)
    p.detail(p.rrect(3.2, 7.2, 60.8, 14, 2.4), p.lin((0, 7), (0, 14), rgb('#7FB0F0'), g.BLUE))
    for x0 in (6, 33):
        p.add(p.rect(x0, 17, x0 + 25, 52), WHITE, outline=0.9)
        for i in range(5):
            y = 20 + i * 6.2
            p.detail(p.rect(x0 + 2.5, y, x0 + 6, y + 3.4), rgb('#E2B24A'))
            p.detail(p.rect(x0 + 7.5, y + 0.8, x0 + 22, y + 2.6), g.CONTENT)
    x0 = 6 if sel else 33
    y = 20 + 2 * 6.2
    p.detail(p.rect(x0 + 0.8, y - 1.2, x0 + 24.2, y + 4.4), g.BLUE)
    p.detail(p.rect(x0 + 7.5, y + 0.8, x0 + 22, y + 2.6), WHITE)
    return slots(face=(18, 18, 0.44), badge=(30, 30, 0.52))


@obj("dock")
def dock(p, sel=False, **kw):
    """A dock of four buttons; chosen, one pops up."""
    p.add(p.rrect(2, 34, 62, 52, 7), p.lin((0, 34), (0, 52), rgb('#E9EEF5'), rgb('#AFBACB')), gloss=0.35)
    for i, c in enumerate((g.BLUE, g.GREEN, g.ORANGE, g.RED)):
        x = 7 + i * 13.5
        up = 8 if (sel and i == 2) else 0
        p.add(p.rrect(x, 22 - up, x + 11, 33 - up, 2.6), p.lin((x, 22 - up), (x + 11, 33 - up), lighter(c, 0.3), darker(c, 0.25)), outline=1.4, gloss=0.4)
        if i in (0, 2) and not (sel and i == 2):
            p.detail(p.ellipse(x + 4.4, 43, x + 6.6, 45.2), rgb('#2A2E35'))
    return slots(badge=(30, 30, 0.52))


@obj("menu")
def menu(p, sel=False, **kw):
    """A pulled-down menu; chosen, a lower row is lit."""
    p.add(p.rect(2, 2, 62, 10), p.lin((0, 2), (0, 10), rgb('#F7F8F9'), rgb('#C9CED6')))
    p.detail(p.rect(10, 2.6, 26, 9.4), g.BLUE)
    p.add(p.rect(10, 10, 50, 58), p.lin((10, 10), (50, 58), WHITE, rgb('#E2E5E9')))
    lit = 3 if sel else 1
    for i in range(6):
        y = 14 + i * 7.2
        if i == lit:
            p.detail(p.rect(11, y - 1.4, 49, y + 4.4), g.BLUE)
        p.detail(p.rect(15, y + 0.6, 15 + (26 if i % 2 == 0 else 20), y + 2.2), WHITE if i == lit else g.DARK)
    return slots(face=(18, 18, 0.4), badge=(30, 30, 0.52))


@obj("requester")
def requester(p, sel=False, **kw):
    window(p, sel=True)
    p.add(p.rect(9, 22, 55, 40), WHITE, outline=1.0)
    lines(p, 13, 25, 50, 3, gap=4.5, h=1.6)
    for i, (x, c) in enumerate(((9, g.GREEN), (39, g.RED))):
        pressed = sel and i == 0
        p.add(p.rrect(x, 45, x + 16, 53, 1.5), darker(c, 0.25) if pressed else c, outline=1.0)
    return slots(face=(20, 20, 0.36), badge=(30, 30, 0.52))


@obj("gadgets")
def gadgets(p, sel=False, sliders=True, **kw):
    """A window of sliders and buttons; chosen, the knob moves."""
    window(p, sel=True)
    for i in range(3):
        y = 24 + i * 9
        p.detail(p.rect(9, y + 1.5, 55, y + 3.5), rgb('#B6BBC2'))
        k = (18, 36, 28)[i] + (10 if sel and i == 1 else 0)
        p.add(p.rrect(k, y - 1.5, k + 7, y + 6.5, 1.2), p.lin((k, y), (k + 7, y + 6), lighter(g.BLUE, 0.3), g.BLUE_LO), outline=1.0)
    p.add(p.rrect(9, 49, 27, 55, 1.2), WHITE, outline=1.0)
    p.add(p.rrect(37, 49, 55, 55, 1.2), WHITE, outline=1.0)
    return slots(badge=(30, 30, 0.52))


@obj("titlebar")
def titlebar(p, sel=False, **kw):
    p.add(p.rrect(2, 22, 62, 36, 2), p.lin((0, 22), (0, 36), rgb('#F7F8F9'), rgb('#C9CED6')), gloss=0.25)
    p.add(p.rrect(5, 25, 13, 33, 1.5), p.lin((5, 25), (13, 33), lighter(g.BLUE, 0.2), g.BLUE_LO), outline=1.0)
    for x0, x1 in ((17, 29), (33, 40)):
        p.detail(p.rect(x0, 28.4, x1, 29.8), g.DARK)
    p.detail(p.text(53, 29.4, 6, "12:00" if not sel else "12:05", mono=True), g.DARK)
    return slots(badge=(30, 30, 0.52))


@obj("windows")
def windows(p, sel=False, n=3, **kw):
    """Windows stacked; chosen, the front one slides out and goes active."""
    window(p.sub(22, -2, 0.6), active=False)
    window(p.sub(12, 12, 0.6), active=False)
    if n >= 3:
        if sel:
            window(p.sub(26, 26, 0.6), active=True)
        else:
            window(p.sub(2, 26, 0.6), active=False)
    return slots(badge=(30, 30, 0.52))


@obj("iconpic")
def iconpic(p, sel=False, **kw):
    """An icon: a little drawer with its name under it; chosen, it is picked."""
    drawer(p.sub(6, 2, 0.8), sel=sel)
    p.detail(p.rect(14, 50, 50, 56), g.BLUE if sel else WHITE)
    p.detail(p.rect(18, 52, 46, 54), WHITE if sel else g.DARK)
    return slots(badge=(30, 30, 0.52))


@obj("startbutton")
def startbutton(p, sel=False, **kw):
    p.add(p.ellipse(4, 4, 60, 60), p.rad((22, 20), 54, lighter(g.BLUE, 0.45) if sel else lighter(g.BLUE, 0.2), g.BLUE_LO), gloss=0.4)
    p.detail(p.poly([(32, 14), (50, 46), (14, 46)]), WHITE)
    p.detail(p.poly([(32, 26), (41, 42), (23, 42)]), g.BLUE)
    return slots(badge=(32, 32, 0.5))


@obj("bootscreen")
def bootscreen(p, sel=False, **kw):
    monitor(p, sel=False)
    p.add(p.rect(20, 18, 44, 22), WHITE, outline=0.8)
    for i in range(3):
        p.add(p.rect(20, 25 + i * 5, 44, 28 + i * 5), g.BLUE if (i == (1 if sel else 0)) else WHITE, outline=0.8)
    return slots(badge=(30, 28, 0.52))


@obj("code")
def code(p, sel=False, **kw):
    """Code lines, indented; chosen, a line more."""
    p.add(p.rrect(4, 6, 60, 58, 3), p.lin((4, 6), (60, 58), rgb('#1C232C'), rgb('#0B0F14')), gloss=0.1)
    rows = [(8, 30, g.BLUE), (14, 40, rgb('#7CFF6A')), (14, 34, rgb('#FFD86A')), (20, 44, rgb('#7CFF6A')), (14, 26, rgb('#C9CED6')), (8, 18, g.BLUE)]
    for i, (x, w, c) in enumerate(rows[:5 if not sel else 6]):
        p.detail(p.rect(x, 12 + i * 7.5, x + w, 14.4 + i * 7.5), c)
    return slots(badge=(30, 30, 0.52))


@obj("hexgrid")
def hexgrid(p, sel=False, **kw):
    p.add(p.rrect(4, 6, 60, 58, 3), p.lin((4, 6), (60, 58), WHITE, PAPER_LO), gloss=0.1)
    for r in range(6):
        for c in range(4):
            on = sel and (r, c) == (2, 1)
            if on:
                p.detail(p.rect(7 + c * 13, 9 + r * 8, 18 + c * 13, 16 + r * 8), g.BLUE)
            p.detail(p.text(12.5 + c * 13, 12.6 + r * 8, 5, "%02X" % ((r * 37 + c * 91) % 256), mono=True), WHITE if on else g.DARK)
    return slots(badge=(30, 30, 0.52))


@obj("letters")
def letters(p, sel=False, **kw):
    p.add(p.rrect(4, 6, 60, 58, 3), p.lin((4, 6), (60, 58), WHITE, PAPER_LO), gloss=0.1)
    s = "ABCDEFGHIJKLMNOP"
    for i, ch in enumerate(s):
        r, c = divmod(i, 4)
        on = sel and i == 6
        if on:
            p.detail(p.rect(7 + c * 13, 9 + r * 12, 18 + c * 13, 20 + r * 12), g.BLUE)
        p.detail(p.text(12.5 + c * 13, 14.6 + r * 12, 8, ch), WHITE if on else g.DARK)
    return slots(badge=(30, 30, 0.52))


@obj("text")
def big_text(p, sel=False, s="Aa", tint="dark", outline_only=False, pixels=False, serif=False, mono=False, **kw):
    """Letters as the picture; chosen, they lift."""
    c, lo = colour(tint)
    d = -2 if sel else 0
    n = len(s)
    h = 40 if n <= 2 else 30 if n == 3 else 22 if n == 4 else 17
    m = p.text(32, 32 + d, h, s, serif=serif, mono=mono or pixels)
    if pixels:
        m = m.resize((m.width // 16, m.height // 16)).resize(m.size)
    if outline_only:
        p.add(g.cut(g.dilate(m, 1.4 * p.s), m), p.lin((0, 8), (0, 56), lighter(c, 0.3), lo), outline=0.8)
    else:
        p.add(m, p.lin((0, 8), (0, 56), lighter(c, 0.35), lo), outline=1.8, gloss=0.25)
    return slots(badge=(32, 32, 0.5))


@obj("brackets")
def brackets(p, sel=False, kind="<>", **kw):
    c = g.BLUE
    if kind == "{}":
        return big_text(p, sel=sel, s="{ }", tint="blue")
    d = 0 if not sel else 2
    p.add(p.stroke([(24 - d, 12), (8 - d, 32), (24 - d, 52)], 5), c, outline=1.4)
    p.add(p.stroke([(40 + d, 12), (56 + d, 32), (40 + d, 52)], 5), c, outline=1.4)
    if kind == "</>":
        p.add(p.stroke([(36, 10), (28, 54)], 4), g.ORANGE, outline=1.2)
    return slots(badge=(32, 32, 0.5))


# ---- signs (mostly badges) ------------------------------------------------------------------------------


def _glyph_disc(p, tint):
    c, lo = colour(tint)
    p.add(p.ellipse(4, 4, 60, 60), p.rad((22, 20), 56, lighter(c, 0.35), lo), gloss=0.35)


@obj("question")
def question(p, sel=False, tint="blue", **kw):
    _glyph_disc(p, tint)
    p.detail(p.text(32, 33 - (2 if sel else 0), 38, "?"), WHITE)
    return slots()


@obj("exclaim")
def exclaim(p, sel=False, tint="red", **kw):
    _glyph_disc(p, tint)
    p.detail(p.text(32, 33 - (2 if sel else 0), 38, "!"), WHITE)
    return slots()


@obj("info")
def info(p, sel=False, tint="blue", **kw):
    _glyph_disc(p, tint)
    p.detail(p.text(32, 33 - (2 if sel else 0), 38, "i", serif=True), WHITE)
    return slots()


@obj("tick")
def tick(p, sel=False, tint="green", **kw):
    _glyph_disc(p, tint)
    p.detail(p.stroke([(18, 33), (28, 43), (47, 21 - (3 if sel else 0))], 6.5), WHITE)
    return slots()


@obj("cross")
def cross(p, sel=False, tint="red", **kw):
    _glyph_disc(p, tint)
    o = 1.5 if sel else 0
    p.detail(p.stroke([(20 - o, 20 - o), (44 + o, 44 + o)], 6.5), WHITE)
    p.detail(p.stroke([(44 + o, 20 - o), (20 - o, 44 + o)], 6.5), WHITE)
    return slots()


@obj("plusminus")
def plusminus(p, sel=False, **kw):
    p.add(p.rrect(4, 4, 30, 30, 4), g.GREEN, outline=1.4)
    p.detail(p.stroke([(17, 9), (17, 25)], 4), WHITE)
    p.detail(p.stroke([(9, 17), (25, 17)], 4), WHITE)
    p.add(p.rrect(34, 34, 60, 60, 4), g.RED, outline=1.4)
    p.detail(p.stroke([(39, 47), (55, 47)], 4), WHITE)
    return slots()


@obj("prompt")
def prompt(p, sel=False, **kw):
    p.add(p.rrect(4, 10, 60, 54, 5), p.lin((4, 10), (60, 54), rgb('#2A3038'), rgb('#0B0F14')), gloss=0.3)
    p.detail(p.stroke([(14, 22), (26, 32), (14, 42)], 4.4), rgb('#7CFF6A'))
    p.detail(p.rect(32 if not sel else 38, 38, 48 if not sel else 52, 43), rgb('#7CFF6A'))
    return slots()


@obj("arrow")
def arrow(p, sel=False, dir="down", tint="green", **kw):
    """A fat arrow; chosen, it moves along its way."""
    c, lo = colour(tint)
    v = {"down": (0, 1), "up": (0, -1), "right": (1, 0), "left": (-1, 0), "upright": (0.7, -0.7), "downright": (0.7, 0.7)}[dir]
    m = 5 if sel else 0
    x, y = 32 + v[0] * (24 + m), 32 + v[1] * (24 + m)
    a = math.atan2(v[1], v[0])
    d = (math.cos(a), math.sin(a))
    n = (-d[1], d[0])
    at = lambda t, u: (x - d[0] * t + n[0] * u, y - d[1] * t + n[1] * u)
    pts = [at(0, 0), at(22, -20), at(22, -9), at(48, -9), at(48, 9), at(22, 9), at(22, 20)]
    p.add(p.poly(pts), p.lin(at(48, -10), at(0, 10), lighter(c, 0.25), lo), gloss=0.3)
    return slots()


@obj("arrows")
def arrows(p, sel=False, kind="both", tint="green", **kw):
    """Two arrows: both ways, or round in a circle; chosen, they move."""
    c, lo = colour(tint)
    if kind == "circle":
        a0 = 30 if not sel else 75
        p.add(p.arc(8, 8, 56, 56, a0, a0 + 250, 7), p.lin((8, 8), (56, 56), lighter(c, 0.25), lo), outline=1.6)
        t = math.radians(a0 + 250)
        x, y = 32 + math.cos(t) * 24, 32 + math.sin(t) * 24
        d = (-math.sin(t), math.cos(t))
        n = (math.cos(t), math.sin(t))
        p.add(p.poly([(x + d[0] * 12, y + d[1] * 12), (x + n[0] * 11, y + n[1] * 11), (x - n[0] * 11, y - n[1] * 11)]), lo, outline=1.6)
        return slots()
    o = 3 if sel else 0
    arrow(p.sub(-6 + o, -12, 0.75), dir="right", tint=tint)
    arrow(p.sub(22 - o, 28, 0.75), dir="left", tint="blue")
    return slots()


@obj("dashed")
def dashed(p, sel=False, **kw):
    for i in range(12):
        t0, t1 = i / 12, (i + 0.55) / 12
        per = 4 * 56
        def pt(t):
            u = (t * per) % per
            if u < 56: return (4 + u, 4)
            if u < 112: return (60, 4 + u - 56)
            if u < 168: return (60 - (u - 112), 60)
            return (4, 60 - (u - 168))
        p.add(p.stroke([pt(t0 + (0.02 if sel else 0)), pt(t1 + (0.02 if sel else 0))], 2.6), g.BLUE, outline=0.8)
    return slots(face=(10, 10, 0.69))


@obj("mark")
def amigachrome_mark(p, sel=False, **kw):
    """AmigaChrome's mark (our own): a ring and a chevron; chosen, it lights."""
    p.add(p.ellipse(4, 4, 60, 60), p.rad((22, 20), 56, lighter(g.BLUE, 0.4 if sel else 0.2), g.BLUE_LO), gloss=0.35)
    p.detail(p.ring(12, 12, 52, 52, 4), WHITE)
    p.detail(p.stroke([(22, 38), (32, 24), (42, 38)], 5), WHITE)
    return slots()


@obj("openup")
def openup_mark(p, sel=False, **kw):
    """The OpenUp box with its arrow; chosen, the arrow sinks in."""
    cardboard_box(p.sub(8, 14, 0.75))
    arrow(p.sub(16, -2 + (6 if sel else 0), 0.5), dir="down")
    return slots()


@obj("prefs")
def prefs_mark(p, sel=False, **kw):
    """The prefs mark: a small blue monitor with a question mark."""
    p.add(p.rrect(2, 40, 62, 56, 4), p.lin((2, 40), (2, 56), rgb('#DDE4EE'), rgb('#8EA1BC')))
    p.add(p.rrect(6, 4, 58, 44, 5), p.lin((6, 4), (58, 44), rgb('#7FB0F0'), g.BLUE_LO), gloss=0.35)
    p.detail(p.text(32, 24 - (2 if sel else 0), 28, "?"), WHITE)
    return slots()


@obj("pipe")
def pipe(p, sel=False, **kw):
    """A pipe section, flanged; chosen, something flows in it."""
    p.add(p.rect(10, 22, 54, 42), p.lin((0, 22), (0, 42), lighter(g.GREEN, 0.35), g.GREEN_LO), gloss=0.35)
    for x in (4, 50):
        p.add(p.rrect(x, 16, x + 10, 48, 2), p.lin((0, 16), (0, 48), lighter(g.GREEN, 0.25), g.GREEN_LO), gloss=0.35)
    if sel:
        for x in (20, 30, 40):
            p.detail(p.ellipse(x - 2.5, 29.5, x + 2.5, 34.5), WHITE)
    return slots(badge=(30, 30, 0.52))


@obj("din")
def din(p, sel=False, **kw):
    """A round DIN plug (MIDI), end-on; chosen, a cable is in it."""
    p.add(p.ellipse(6, 6, 58, 58), p.lin((6, 6), (58, 58), METAL_HI, METAL_LO), gloss=0.3)
    p.add(p.ellipse(13, 13, 51, 51), rgb('#2A3038'), outline=1.2)
    for a in (180, 225, 270, 315, 360):
        t = math.radians(a)
        x, y = 32 + math.cos(t) * 12, 34 + math.sin(t) * 12
        p.detail(p.ellipse(x - 2.4, y - 2.4, x + 2.4, y + 2.4), GOLDPIN)
    p.detail(p.rect(29, 46, 35, 51), METAL_HI)
    if sel:
        p.add(p.ellipse(26, 26, 38, 38), g.GREEN, outline=1.0)
    return slots(badge=(30, 30, 0.52))


@obj("floppydrive")
def floppydrive(p, sel=False, screen=False, **kw):
    """A floppy drive; chosen, a disk goes in (and its light is on)."""
    p.add(p.poly([(4, 30), (60, 30), (54, 18), (10, 18)]), p.lin((0, 18), (0, 30), lighter(BEIGE_HI, 0.3), BEIGE_LO), gloss=0.35)
    p.add(p.rrect(3, 28, 61, 50, 3), p.lin((3, 28), (61, 50), BEIGE_HI, darker(BEIGE_LO, 0.1)), gloss=0.2)
    p.detail(p.rect(10, 34, 44, 37.5), INK)
    if sel:
        p.add(p.rect(14, 24 - 4, 40, 35), p.lin((14, 20), (40, 35), rgb('#5A616B'), rgb('#1F2329')), outline=1.2)
    if screen:
        p.add(p.rect(46, 33, 57, 41), rgb('#1C3A2A'), outline=1.0)
        p.detail(p.text(51.5, 37, 5, "00" if not sel else "07", mono=True), LED_ON)
    else:
        p.add(p.ellipse(48, 40, 53, 45), LED_ON if sel else LED_OFF, outline=0.8)
    return slots(face=(12, 4, 0.4), badge=(30, 26, 0.52))


@obj("magnet")
def magnet(p, sel=False, **kw):
    """A horseshoe magnet; chosen, it pulls."""
    p.add(p.arc(8, 6, 56, 54, 180, 360, 12), p.lin((8, 6), (56, 54), lighter(g.RED, 0.2), g.RED_LO), outline=1.6)
    p.add(p.rect(8, 30, 20, 46), p.lin((8, 30), (20, 46), lighter(g.RED, 0.2), g.RED_LO), outline=1.6)
    p.add(p.rect(44, 30, 56, 46), p.lin((44, 30), (56, 46), lighter(g.RED, 0.2), g.RED_LO), outline=1.6)
    p.add(p.rect(8, 46, 20, 56), p.lin((8, 46), (20, 56), METAL_HI, METAL_LO), outline=1.6)
    p.add(p.rect(44, 46, 56, 56), p.lin((44, 46), (56, 56), METAL_HI, METAL_LO), outline=1.6)
    if sel:
        for x in (14, 50):
            p.add(p.stroke([(x - 4, 60), (x + 4, 63)], 1.4), g.GOLD, outline=0.6)
    return slots(badge=(32, 32, 0.5))


@obj("play")
def play(p, sel=False, **kw):
    """A play button; chosen, it turns to pause."""
    p.add(p.ellipse(4, 4, 60, 60), p.rad((22, 20), 56, lighter(g.GREEN, 0.35), g.GREEN_LO), gloss=0.35)
    if sel:
        p.detail(p.rect(22, 18, 29, 46), WHITE)
        p.detail(p.rect(35, 18, 42, 46), WHITE)
    else:
        p.detail(p.poly([(24, 16), (24, 48), (48, 32)]), WHITE)
    return slots()


@obj("curve")
def curve(p, sel=False, **kw):
    """A curve and its nodes (a vector picture); chosen, a handle moves."""
    p.add(p.rrect(2, 6, 62, 58, 3), p.lin((2, 6), (62, 58), WHITE, PAPER_LO), gloss=0.12)
    hy = 10 if not sel else 18
    pts = []
    for i in range(21):
        t = i / 20
        x = (1 - t) ** 3 * 8 + 3 * (1 - t) ** 2 * t * 18 + 3 * (1 - t) * t * t * 46 + t ** 3 * 56
        y = (1 - t) ** 3 * 48 + 3 * (1 - t) ** 2 * t * hy + 3 * (1 - t) * t * t * 58 + t ** 3 * 20
        pts.append((x, y))
    p.detail(p.stroke([(8, 48), (18, hy)], 0.9), g.BLUE)
    p.detail(p.stroke([(56, 20), (46, 58)], 0.9), g.BLUE)
    p.add(p.stroke(pts, 2.4), g.ORANGE, outline=1.0)
    for x, y in ((8, 48), (56, 20)):
        p.add(p.rect(x - 3, y - 3, x + 3, y + 3), WHITE, outline=1.0)
    for x, y in ((18, hy), (46, 58)):
        p.add(p.ellipse(x - 2.6, y - 2.6, x + 2.6, y + 2.6), g.BLUE, outline=0.8)
    return slots(badge=(30, 30, 0.52))


@obj("plaster")
def plaster(p, sel=False, **kw):
    """A sticking plaster (a patch); chosen, it is on straight."""
    a = math.radians(-35 if not sel else -20)
    d = (math.cos(a), math.sin(a))
    n = (-d[1], d[0])
    at = lambda u, v: (32 + d[0] * u + n[0] * v, 32 + d[1] * u + n[1] * v)
    p.add(g.union(p.poly([at(-22, -10), at(22, -10), at(22, 10), at(-22, 10)]), p.ellipse(at(-22, 0)[0] - 10, at(-22, 0)[1] - 10, at(-22, 0)[0] + 10, at(-22, 0)[1] + 10),
                  p.ellipse(at(22, 0)[0] - 10, at(22, 0)[1] - 10, at(22, 0)[0] + 10, at(22, 0)[1] + 10)), p.lin((4, 4), (60, 60), rgb('#F6CFA6'), rgb('#C98A5A')), gloss=0.3)
    p.detail(p.poly([at(-9, -8), at(9, -8), at(9, 8), at(-9, 8)]), rgb('#F8E8D4'))
    for u in (-5, 0, 5):
        for v in (-4, 0, 4):
            q = at(u, v)
            p.detail(p.ellipse(q[0] - 0.7, q[1] - 0.7, q[0] + 0.7, q[1] + 0.7), rgb('#C98A5A'))
    return slots(badge=(32, 32, 0.5))


@obj("cube")
def cube(p, sel=False, tint="blue", **kw):
    """A solid cube (an object file); chosen, it has turned a little."""
    c, lo = colour(tint)
    o = 0 if not sel else 4
    p.add(p.poly([(32 - o, 4), (58, 16 - o), (32 + o, 28), (6, 16 + o)]), p.lin((6, 4), (58, 28), lighter(c, 0.45), c), gloss=0.3)
    p.add(p.poly([(6, 16 + o), (32 + o, 28), (32 + o, 60), (6, 48 + o)]), p.lin((6, 16), (32, 60), c, lo))
    p.add(p.poly([(32 + o, 28), (58, 16 - o), (58, 48 - o), (32 + o, 60)]), p.lin((32, 16), (58, 60), darker(c, 0.15), darker(lo, 0.3)))
    return slots(badge=(32, 32, 0.5))


@obj("scissors")
def scissors(p, sel=False, **kw):
    """Scissors; chosen, they close."""
    a = 18 if not sel else 6
    for sgn in (1, -1):
        t = math.radians(-90 + sgn * a)
        tip = (32 + math.cos(t) * 30, 34 + math.sin(t) * 30)
        p.add(p.stroke([(32 - math.cos(t) * 4, 34 - math.sin(t) * 4), tip], 4), p.lin((32, 34), tip, METAL_HI, METAL_LO), outline=1.4)
        h = (32 - math.cos(t) * 14 - sgn * 4, 34 - math.sin(t) * 14 + 6)
        p.add(g.cut(p.ellipse(h[0] - 9, h[1] - 9, h[0] + 9, h[1] + 9), p.ellipse(h[0] - 5, h[1] - 5, h[0] + 5, h[1] + 5)), p.lin((h[0] - 9, 0), (h[0] + 9, 0), lighter(g.RED, 0.2), g.RED_LO), outline=1.4)
    p.add(p.ellipse(29.5, 31.5, 34.5, 36.5), METAL_LO, outline=1.0)
    return slots(badge=(32, 32, 0.5))


@obj("blank")
def blank_obj(p, sel=False, **kw):
    return slots()
