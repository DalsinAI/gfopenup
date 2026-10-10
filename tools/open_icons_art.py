#!/usr/bin/env python3
"""Icons for the Open apps, drawn in the Gorilla style (ICONS.md section 3):
bold black outlines, soft light from the top left, a gentle gloss and a soft
shadow down and to the right. Our own drawing, with tools/gstyle.py; nothing
is traced from Gorilla. MIT, Copyright (c) 2026 Dalsin Limited.

    python3 tools/open_icons_art.py icons/open    # needs Pillow and numpy

Every icon has two frames, as ICONS.md asks: NAME.png at rest and
NAME-selected.png, with something in the picture moved (the pencil writes on,
the play button turns to pause), which Workbench shows while the icon is
chosen. Both frames are cropped and scaled alike (gstyle.finish), so nothing
jumps but the part that moves. Prefs editors carry the prefs mark, bottom left.

Drawers of our own: badge-NAME.png (the badge alone, where it sits on a
drawer) and drawer-NAME.png (the badge on a drawer of our own drawing). The
pack build puts the badge on Gorilla's drawer when it ships Gorilla, so our
drawers match the system's; drawer-NAME.png is for a pack made without it.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gstyle import *            # noqa: E402,F401,F403
from PIL import Image           # noqa: E402

# ---- pieces ------------------------------------------------------------------------------------


def perp(a):
    return (-math.sin(a), math.cos(a))


def pencil(ic, tx, ty, angle=-128, length=34, w=3.6, body=ORANGE, body_lo=ORANGE_LO):
    """A pencil, its point at (tx, ty), lying along angle (degrees) from there."""
    a = math.radians(angle)
    d = (math.cos(a), math.sin(a))
    n = perp(a)
    at = lambda t, s=0: (tx + d[0] * t + n[0] * s, ty + d[1] * t + n[1] * s)
    cone, ferrule, rubber = 8, length - 7, length - 3
    ic.add(poly([at(cone, -w), at(ferrule, -w), at(ferrule, w), at(cone, w)]),
           linear(at(cone, -w), at(cone, w), lighter(body, 0.3), body_lo), outline=1.5)
    ic.add(poly([at(ferrule, -w), at(rubber, -w), at(rubber, w), at(ferrule, w)]),
           linear(at(ferrule, -w), at(ferrule, w), rgb('#E7EAEE'), rgb('#8D949E')), outline=1.5)
    ic.add(poly([at(rubber, -w), at(length, -w * 0.8), at(length, w * 0.8), at(rubber, w)]),
           linear(at(rubber, -w), at(rubber, w), rgb('#F7A8B4'), rgb('#C25A6A')), outline=1.5)
    ic.add(poly([at(0), at(cone, -w), at(cone, w)]), linear(at(cone, -w), at(cone, w), rgb('#F4D9AE'), rgb('#C79A5A')), outline=1.5)
    ic.detail(poly([at(0), at(2.8, -w * 0.35), at(2.8, w * 0.35)]), INK)


def arrow(ic, x, y, dx, dy, size=1.0, colour=GREEN, colour_lo=GREEN_LO):
    """A fat arrow whose point is at (x, y), pointing along (dx, dy)."""
    a = math.atan2(dy, dx)
    d = (math.cos(a), math.sin(a))
    n = perp(a)
    s = size
    at = lambda t, u: (x - d[0] * t * s + n[0] * u * s, y - d[1] * t * s + n[1] * u * s)
    pts = [at(0, 0), at(9, -8), at(9, -3.6), at(20, -3.6), at(20, 3.6), at(9, 3.6), at(9, 8)]
    ic.add(poly(pts), linear(at(20, -4), at(0, 4), lighter(colour, 0.25), colour_lo), outline=1.6, gloss=0.3)


def window(ic, x0, y0, x1, y1, active=True, title_h=5.0, body=PAPER):
    """A window: a title bar (the accent when active) over a light body."""
    ic.add(rrect(x0, y0, x1, y1, 2.2), solid(body))
    bar = linear((0, y0), (0, y0 + title_h), lighter(BLUE, 0.2), BLUE_LO) if active else linear((0, y0), (0, y0 + title_h), rgb('#E2E5E9'), rgb('#B6BBC2'))
    ic.detail(inter(rrect(x0, y0, x1, y1, 2.2), rect(x0, y0, x1, y0 + title_h)), bar)
    ic.detail(rect(x0, y0 + title_h, x1, y0 + title_h + 0.6), INK)
    sq = 2.2
    ic.detail(rect(x1 - 2.6 - sq, y0 + (title_h - sq) / 2, x1 - 2.6, y0 + (title_h + sq) / 2), (255, 255, 255) if active else rgb('#7C8088'))


def clock_face(ic, cx, cy, r, hour_a, minute_a):
    ic.add(ellipse(cx - r, cy - r, cx + r, cy + r), linear((cx - r, cy - r), (cx + r, cy + r), rgb('#F2F4F7'), rgb('#9CA4B0')), gloss=0.2)
    ri = r - 2.2
    ic.add(ellipse(cx - ri, cy - ri, cx + ri, cy + ri), radial((cx - ri * 0.4, cy - ri * 0.4), ri * 1.8, rgb('#5E86BE'), rgb('#1D3A66')), outline=0.8)
    for k in range(12):
        a = math.radians(k * 30)
        x, y = cx + math.cos(a) * ri * 0.78, cy + math.sin(a) * ri * 0.78
        ic.detail(ellipse(x - 0.55, y - 0.55, x + 0.55, y + 0.55), (230, 236, 245))
    for a, L, wd in ((hour_a, ri * 0.5, 1.6), (minute_a, ri * 0.78, 1.1)):
        t = math.radians(a - 90)
        ic.detail(stroke([(cx, cy), (cx + math.cos(t) * L, cy + math.sin(t) * L)], wd), (250, 250, 250))
    ic.detail(ellipse(cx - 1, cy - 1, cx + 1, cy + 1), (250, 250, 250))


def box(ic, x0, y0, w, h, lid_up=0.0):
    """An open cardboard box, seen front-on from a little above; lid_up lifts
    the front flap (0 lying open, 1 standing up)."""
    depth = h * 0.32
    ic.add(poly([(x0, y0), (x0 + w, y0), (x0 + w - 3, y0 - depth), (x0 + 3, y0 - depth)]), solid(rgb('#7A4A18')), outline=1.6)
    ic.add(rrect(x0, y0, x0 + w, y0 + h, 1.4), linear((x0, y0), (x0 + w, y0 + h), rgb('#F2B865'), rgb('#B9701F')), gloss=0.15)
    ic.detail(rect(x0 + w * 0.38, y0 + 1, x0 + w * 0.62, y0 + h * 0.34), rgb('#F8DDB0'))      # tape
    fl = 6 + 6 * (1 - lid_up)
    if lid_up < 0.5:
        ic.add(poly([(x0 - 1, y0), (x0 + w + 1, y0), (x0 + w + 4, y0 + fl * 0.0 - 0.1), (x0 - 4, y0 - 0.1)]), solid(rgb('#D99A45')), outline=1.2)
    ic.add(poly([(x0 + 3, y0 - depth), (x0 + w - 3, y0 - depth), (x0 + w - 1, y0 - depth - (8 if lid_up else 3)), (x0 + 1, y0 - depth - (8 if lid_up else 3))]),
           linear((0, y0 - depth - 8), (0, y0 - depth), rgb('#F5C67A'), rgb('#C98A35')), outline=1.4)


# ---- the Open apps ---------------------------------------------------------------------------


def openfiles(sel):
    ic = Icon()
    ic.add(rrect(4, 8, 60, 54, 3.5), linear((4, 8), (60, 54), GREY_HI, rgb('#C9CCD1')), gloss=0.12)
    ic.detail(rrect(5.2, 9.2, 58.8, 15.5, 2.4), linear((0, 9), (0, 16), rgb('#7FB0F0'), BLUE))
    for x0 in (8, 33):
        ic.add(rect(x0, 19, x0 + 22, 50), (255, 255, 255), outline=0.9)
        for i in range(5):
            y = 22 + i * 5.6
            ic.detail(rect(x0 + 2.5, y, x0 + 6, y + 3), rgb('#E2B24A'))
            ic.detail(rect(x0 + 7.5, y + 0.6, x0 + 19, y + 2.4), CONTENT)
    x0 = 8 if sel else 33
    y = 22 + 2 * 5.6
    ic.detail(rect(x0 + 0.8, y - 1.0, x0 + 21.2, y + 4.0), BLUE)
    ic.detail(rect(x0 + 2.5, y, x0 + 6, y + 3), GOLD)
    ic.detail(rect(x0 + 7.5, y + 0.6, x0 + 19, y + 2.4), (255, 255, 255))
    if sel:
        ic.add(stroke([(31, 35), (36, 35)], 2.2), GREEN, outline=1.0)
        ic.add(poly([(28.5, 35), (32.5, 31.5), (32.5, 38.5)]), GREEN, outline=1.0)
    return ic


def openview(sel):
    ic = Icon()
    ic.add(rrect(4, 10, 48, 50, 3), linear((4, 10), (48, 50), GREY_HI, rgb('#C9CCD1')), gloss=0.12)
    ic.add(rect(8, 14, 44, 44), linear((0, 14), (0, 44), rgb('#9FD0FF'), rgb('#E9F4FF')), outline=0.9)
    ic.detail(poly([(8, 44), (8, 36), (17, 27), (26, 37), (31, 32), (44, 42), (44, 44)]), linear((0, 27), (0, 44), GREEN, GREEN_LO))
    ic.detail(ellipse(33, 17, 40, 24), GOLD)
    if sel:
        magnifier(ic, 30, 30, 10.5, 45, 13)
    else:
        magnifier(ic, 47, 42, 9.5, 45, 11)
    return ic


def openedit(sel):
    """A notepad and a pencil; chosen, the pencil has written a line more."""
    ic = Icon()
    page(ic, 8, 9, 46, 59, holes=False)
    for i in range(5):
        x = 12.5 + i * 7
        ic.add(rrect(x, 4.5, x + 3, 13, 1.5), linear((x, 4), (x + 3, 13), rgb('#E9ECF0'), rgb('#7D848E')), outline=1.1)
    widths = [26, 22, 27, 18, 24]
    for i, wd in enumerate(widths):
        ic.detail(rect(13, 20 + i * 6, 13 + wd, 21.7 + i * 6), CONTENT)
    if sel:
        ic.detail(rect(13, 50, 33, 51.7), BLUE_LO)
        pencil(ic, 34, 51, angle=-55)
    else:
        ic.detail(rect(13, 50, 22, 51.7), BLUE_LO)
        pencil(ic, 23, 51, angle=-55)
    return ic


def openwrite(sel):
    """A laid-out page (a heading, two columns, a picture) and a pen nib."""
    ic = Icon()
    page(ic, 6, 5, 46, 59, holes=False)
    ic.detail(rect(11, 11, 41, 15), BLUE_LO)
    ic.add(rect(11, 19, 24, 31), linear((0, 19), (0, 31), rgb('#9FD0FF'), rgb('#E9F4FF')), outline=0.8)
    ic.detail(poly([(11, 31), (16, 24), (20, 28), (24, 25), (24, 31)]), GREEN)
    for i in range(3):
        ic.detail(rect(27, 20 + i * 4, 41, 21.4 + i * 4), CONTENT)
    for i in range(5):
        wd = 30 if i != 4 else 18
        ic.detail(rect(11, 35 + i * 4, 11 + wd, 36.4 + i * 4), CONTENT)
    # the nib: a fountain pen's point, drawn upright and leaning
    cx, cy = (52, 44) if not sel else (47, 50)
    a = math.radians(-30)
    rot = lambda x, y: (cx + x * math.cos(a) - y * math.sin(a), cy + x * math.sin(a) + y * math.cos(a))
    ic.add(poly([rot(0, 12), rot(-6, -2), rot(-5, -10), rot(5, -10), rot(6, -2)]),
           linear(rot(-6, -10), rot(6, 6), rgb('#FFE7A0'), rgb('#B98410')), gloss=0.3)
    ic.detail(stroke([rot(0, 11), rot(0, -1)], 0.9), INK)
    ic.detail(ellipse(*rot(-1.3, -2.6), *rot(1.3, 0.0)), INK)
    ic.add(rrect(*rot(-6.5, -22), *rot(6.5, -10), 2), linear(rot(-6, -20), rot(6, -10), rgb('#5A616B'), rgb('#1F2329')), gloss=0.3)
    if sel:
        ic.detail(stroke([(14, 56), (20, 54), (26, 56), (33, 54)], 1.2), BLUE_LO)
    return ic


def openplay(sel):
    ic = Icon()
    ic.add(rrect(4, 10, 60, 52, 3), linear((4, 10), (60, 52), DARK, DARK_LO), gloss=0.18)
    for y in (12.5, 47):
        for i in range(8):
            x = 8 + i * 6.6
            ic.detail(rrect(x, y, x + 3.4, y + 2.6, 0.8), rgb('#E8EAED'))
    ic.detail(rect(7, 17, 57, 45), linear((0, 17), (0, 45), rgb('#3A6BB0'), rgb('#1B3D6E')))
    ic.add(ellipse(20, 19, 44, 43), linear((20, 19), (44, 43), rgb('#FFFFFF'), rgb('#C9D3E0')), gloss=0.3)
    if sel:
        ic.detail(rect(27.5, 25.5, 30.5, 36.5), BLUE_LO)
        ic.detail(rect(33.5, 25.5, 36.5, 36.5), BLUE_LO)
    else:
        ic.detail(poly([(28.5, 24.5), (28.5, 37.5), (39, 31)]), BLUE_LO)
    return ic


def openbrowser(sel):
    """A globe in its ring; chosen, the world has turned."""
    ic = Icon()
    cx, cy, r = 31, 32, 21
    sphere = ellipse(cx - r, cy - r, cx + r, cy + r)
    ic.add(sphere, radial((cx - 8, cy - 9), r * 2.0, rgb('#7DB6F2'), rgb('#123A78')))
    shift = 9 if sel else 0
    land = []
    for dx in (-2 * r - 4, 0, 2 * r + 4):
        o = dx + shift
        land.append(poly([(cx - 14 + o, cy - 12), (cx - 4 + o, cy - 16), (cx + 2 + o, cy - 10), (cx - 3 + o, cy - 4),
                          (cx - 1 + o, cy + 4), (cx - 7 + o, cy + 10), (cx - 12 + o, cy + 2), (cx - 16 + o, cy - 4)]))
        land.append(poly([(cx + 6 + o, cy - 2), (cx + 15 + o, cy - 6), (cx + 19 + o, cy + 3), (cx + 13 + o, cy + 14), (cx + 7 + o, cy + 9)]))
    ic.detail(inter(union(*land), sphere), linear((cx - r, cy - r), (cx + r, cy + r), rgb('#9BE07C'), rgb('#2E7D2A')))
    ic.detail(inter(ellipse(cx - r * 0.75, cy - r * 0.85, cx - r * 0.05, cy - r * 0.35), sphere), (255, 255, 255, ))
    # the ring, tilted: its back half behind the globe is hidden by drawing only the front arc over it
    ring_m = cut(ellipse(cx - r - 9, cy - 7, cx + r + 9, cy + 9), ellipse(cx - r - 6, cy - 4, cx + r + 6, cy + 6))
    front = cut(ring_m, rect(cx - r - 10, cy - 8, cx + r + 10, cy + 1))
    back = cut(cut(ring_m, sphere), front)
    ic.add(back, linear((cx - r, 0), (cx + r, 0), GOLD, GOLD_LO), outline=1.2)
    ic.add(front, linear((cx - r, 0), (cx + r, 0), rgb('#FFE58A'), GOLD_LO), outline=1.2)
    return ic


def opencompress(sel):
    """A box with an arrow coming out of it; chosen, the lid is up and the arrow further out."""
    ic = Icon()
    box(ic, 6, 28, 34, 28, lid_up=1.0 if sel else 0.0)
    if sel:
        arrow(ic, 60, 26, 1, -0.55, size=1.15)
    else:
        arrow(ic, 50, 32, 1, -0.55, size=1.0)
    return ic


def printer(sel):
    """A printer, a page in the tray; chosen, the page is further out and the light on."""
    ic = Icon()
    ic.add(rect(16, 4, 48, 22), linear((16, 4), (48, 22), GREY_HI, rgb('#D4D6DA')))
    for i in range(3):
        ic.detail(rect(20, 8 + i * 4, 44, 9.4 + i * 4), CONTENT)
    ic.add(rrect(5, 18, 59, 44, 4), linear((5, 18), (59, 44), rgb('#E9ECF0'), rgb('#8E96A2')), gloss=0.25)
    ic.add(rrect(9, 36, 55, 50, 2.5), linear((0, 36), (0, 50), rgb('#C9CED6'), rgb('#6F7782')))
    ic.detail(rect(12, 39, 52, 41), INK)
    out = 14 if sel else 6
    ic.add(rect(15, 40, 49, 40 + out + 4), linear((0, 40), (0, 40 + out + 4), (255, 255, 255), rgb('#D4D6DA')), outline=1.4)
    for i in range(2 if not sel else 3):
        ic.detail(rect(19, 47 + i * 4, 44, 48.3 + i * 4), CONTENT)
    ic.add(ellipse(47, 24, 52, 29), rgb('#7CFF6A') if sel else rgb('#2E5A2A'), outline=1.0)
    return ic


def install(sel):
    """The OpenUp box with an arrow over it; chosen, the arrow sinks into the box."""
    ic = Icon()
    box(ic, 12, 34, 40, 24)
    y = 30 if sel else 22
    arrow(ic, 32, y, 0, 1, size=1.25)
    return ic


def uninstall(sel):
    """The OpenUp box with an arrow lifting out of it; chosen, it is out."""
    ic = Icon()
    box(ic, 12, 34, 40, 24)
    y = 4 if sel else 12
    arrow(ic, 32, y, 0, -1, size=1.25, colour=RED, colour_lo=RED_LO)
    return ic


def setup(sel):
    """The OpenUp box and a wand; chosen, the star flares and sparkles."""
    ic = Icon()
    box(ic, 18, 36, 38, 22)
    ic.add(stroke([(14, 44), (36, 18)], 3.4), linear((14, 44), (36, 18), rgb('#5A616B'), rgb('#1F2329')))
    ic.detail(stroke([(31, 24), (36, 18)], 3.4), (250, 250, 250))
    r = 10 if sel else 7.5
    cx, cy = 39, 14
    pts = []
    for k in range(10):
        a = math.radians(-90 + k * 36)
        rr = r if k % 2 == 0 else r * 0.45
        pts.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
    ic.add(poly(pts), radial((cx - 2, cy - 2), r * 1.6, rgb('#FFF6B8'), GOLD_LO), outline=1.5, gloss=0.2)
    if sel:
        for (x, y, s) in ((54, 8, 3.2), (52, 24, 2.4), (27, 6, 2.6)):
            ic.add(poly([(x, y - s), (x + s * 0.3, y - s * 0.3), (x + s, y), (x + s * 0.3, y + s * 0.3), (x, y + s),
                         (x - s * 0.3, y + s * 0.3), (x - s, y), (x - s * 0.3, y - s * 0.3)]), GOLD, outline=1.0)
    prefs_badge(ic, 2, 40, 1.35)
    return ic


def zunetest(sel):
    """A window of gadgets; chosen, a button is pressed and the box ticked."""
    ic = Icon()
    window(ic, 4, 6, 60, 56, active=True, title_h=6)
    # a button
    bx0, by0, bx1, by1 = 10, 17, 34, 25
    if sel:
        ic.detail(rect(bx0, by0, bx1, by1), rgb('#8E96A2'))
        ic.detail(rect(bx0 + 1, by0 + 1, bx1, by1), rgb('#C9CED6'))
    else:
        ic.detail(rect(bx0, by0, bx1, by1), (255, 255, 255))
        ic.detail(rect(bx0 + 1, by0 + 1, bx1, by1), rgb('#7C8088'))
        ic.detail(rect(bx0 + 1, by0 + 1, bx1 - 1, by1 - 1), rgb('#E2E5E9'))
    ic.detail(rect(bx0 + 6, by0 + 3.4, bx1 - 6, by0 + 4.8), DARK)
    # a check box
    ic.detail(rect(40, 17, 48, 25), rgb('#7C8088'))
    ic.detail(rect(41, 18, 47, 24), (255, 255, 255))
    if sel:
        ic.detail(stroke([(42.2, 21), (43.8, 22.8), (46.4, 19)], 1.3), BLUE_LO)
    # a slider
    ic.detail(rect(10, 32, 54, 35), rgb('#B6BBC2'))
    kx = 30 if sel else 22
    ic.add(rrect(kx, 29.5, kx + 6, 37.5, 1.2), linear((kx, 29), (kx + 6, 38), lighter(BLUE, 0.3), BLUE_LO), outline=1.0)
    # a list
    ic.detail(rect(10, 41, 54, 52), (255, 255, 255))
    for i in range(3):
        ic.detail(rect(12, 42.5 + i * 3.2, 46, 43.8 + i * 3.2), BLUE if i == 1 else CONTENT)
    return ic


# ---- prefs editors ------------------------------------------------------------------------------


def look(sel):
    ic = Icon()
    cols = [(RED, RED_LO), (ORANGE, ORANGE_LO), (GOLD, GOLD_LO), (GREEN, GREEN_LO), (BLUE, BLUE_LO)]
    for i, (c, lo) in enumerate(cols):
        a = math.radians(-70 + i * 17 + (6 if sel and i == 4 else 0))
        cx, cy = 30, 52
        L, w = 38, 5.2
        tip = (cx + math.cos(a) * L, cy + math.sin(a) * L)
        n = (-math.sin(a) * w, math.cos(a) * w)
        pts = [(cx - n[0], cy - n[1]), (cx + n[0], cy + n[1]), (tip[0] + n[0], tip[1] + n[1]), (tip[0] - n[0], tip[1] - n[1])]
        ic.add(poly(pts), linear((cx, cy), tip, lighter(c, 0.25), lo), outline=1.4)
    ic.add(ellipse(27, 49, 33, 55), rgb('#B8BCC4'), outline=1.2)
    bx, by = (44, 14) if not sel else (40, 20)
    ic.add(stroke([(bx, by), (bx + 12, by - 12)], 3.6), linear((bx, by), (bx + 12, by - 12), rgb('#C48A4A'), rgb('#7A4E22')))
    ic.add(poly([(bx - 4.5, by + 4.5), (bx - 1.5, by - 2.5), (bx + 2.5, by + 1.5)]), rgb('#C0C4CC'))
    ic.add(poly([(bx - 9, by + 9.5), (bx - 4.5, by + 4.5), (bx - 1, by + 8), (bx - 5.5, by + 12)]), RED if sel else rgb('#3B3F46'), outline=1.2)
    prefs_badge(ic, 3, 40, 1.35)
    return ic


def dock(sel):
    ic = Icon()
    ic.add(rrect(3, 34, 61, 50, 6), linear((0, 34), (0, 50), rgb('#E9EEF5'), rgb('#AFBACB')), gloss=0.35)
    cols = [BLUE, GREEN, ORANGE, RED]
    for i, c in enumerate(cols):
        x = 9 + i * 12.5
        up = 7 if (sel and i == 2) else 0
        ic.add(rrect(x, 26 - up, x + 9.5, 35.5 - up, 2.4), linear((x, 26 - up), (x + 9.5, 35.5 - up), lighter(c, 0.3), darker(c, 0.25)), outline=1.3, gloss=0.4)
        if i in (0, 2) and not (sel and i == 2):
            ic.detail(ellipse(x + 3.9, 43.5, x + 5.6, 45.2), rgb('#2A2E35'))
    prefs_badge(ic, 3, 3, 1.35)
    return ic


def menus(sel):
    """A menu, a row lit; chosen, a row lower down is lit and the pointer follows."""
    ic = Icon()
    ic.add(rect(4, 4, 60, 11), linear((0, 4), (0, 11), rgb('#F7F8F9'), rgb('#C9CED6')))
    ic.detail(rect(12, 4.6, 26, 10.4), BLUE)
    ic.detail(rect(14, 6.8, 24, 8.2), (255, 255, 255))
    ic.detail(rect(30, 6.8, 40, 8.2), DARK)
    ic.add(rect(12, 11, 46, 52), linear((12, 11), (46, 52), (255, 255, 255), rgb('#E2E5E9')))
    lit = 3 if sel else 1
    for i in range(6):
        y = 14 + i * 6.2
        if i == lit:
            ic.detail(rect(13, y - 1.2, 45, y + 3.8), BLUE)
        ic.detail(rect(16, y + 0.6, 16 + (22 if i % 2 == 0 else 17), y + 2.0), (255, 255, 255) if i == lit else DARK)
    py = 14 + lit * 6.2 + 1.5
    px = 40
    ic.add(poly([(px, py), (px, py + 13), (px + 3.4, py + 9.6), (px + 6, py + 15), (px + 8.4, py + 14), (px + 5.8, py + 8.6),
                 (px + 10, py + 8.6)]), (255, 255, 255), outline=1.3)
    prefs_badge(ic, 2, 40, 1.35)
    return ic


def sound(sel):
    """A speaker and its waves; chosen, louder: a wave more."""
    ic = Icon()
    ic.add(rect(10, 22, 20, 40), linear((10, 22), (20, 40), rgb('#7E8794'), rgb('#2A2F37')), gloss=0.3)
    ic.add(poly([(20, 22), (32, 10), (32, 52), (20, 40)]), linear((20, 10), (32, 52), rgb('#7E8794'), rgb('#2A2F37')), gloss=0.3)
    n = 4 if sel else 3
    for k in range(n):
        r = 8 + k * 6
        ic.add(arc(31 - r, 31 - r, 31 + r, 31 + r, -48, 48, 2.6), linear((31, 0), (31 + r, 0), lighter(BLUE, 0.3), BLUE_LO), outline=1.0)
    prefs_badge(ic, 2, 40, 1.35)
    return ic


def titlebar(sel):
    """The screen's title bar, and its clock; chosen, the hands move on."""
    ic = Icon()
    ic.add(rrect(2, 10, 62, 22, 2), linear((0, 10), (0, 22), rgb('#F7F8F9'), rgb('#C9CED6')), gloss=0.25)
    ic.add(rrect(5, 12.4, 12, 19.6, 1.5), linear((5, 12), (12, 20), lighter(BLUE, 0.2), BLUE_LO), outline=1.0)
    for x0, x1 in ((15, 25), (28, 34)):
        ic.detail(rect(x0, 15.4, x1, 16.8), DARK)
    if sel:
        clock_face(ic, 42, 38, 17, 75, 220)
    else:
        clock_face(ic, 42, 38, 17, 30, 150)
    prefs_badge(ic, 2, 40, 1.35)
    return ic


def windows(sel):
    """Three windows stacked; chosen, the front one slides out and is active."""
    ic = Icon()
    window(ic, 22, 4, 58, 30, active=False)
    window(ic, 14, 13, 50, 39, active=False)
    if sel:
        window(ic, 24, 26, 62, 54, active=True)
    else:
        window(ic, 6, 22, 42, 48, active=False)
    prefs_badge(ic, 2, 40, 1.35)
    return ic


def types(sel):
    """Documents, an arrow to a program; chosen, the arrow moves on."""
    ic = Icon()
    page(ic, 14, 2, 36, 30, holes=False)
    page(ic, 6, 8, 28, 36, holes=False)
    for i in range(4):
        ic.detail(rect(9.5, 14 + i * 4.5, 24, 15.4 + i * 4.5), CONTENT)
    ic.add(rrect(38, 30, 62, 52, 3), linear((38, 30), (62, 52), rgb('#E9ECF0'), rgb('#8E96A2')), gloss=0.2)
    ic.detail(rect(41, 33, 59, 46), linear((0, 33), (0, 46), rgb('#3E79C8'), rgb('#1B3D6E')))
    ic.detail(poly([(44, 43), (49, 36), (53, 41), (56, 38), (56, 43)]), GREEN)
    x = 40 if sel else 34
    arrow(ic, x, 28, 1, 0.75, size=0.85)
    prefs_badge(ic, 2, 40, 1.35)
    return ic


# ---- drawers ---------------------------------------------------------------------------------------
# A badge is drawn in the frame a drawer has (82 x 60 units, as Gorilla's), at
# its foot on the right, and has its own two frames.

DRAWER_W, DRAWER_H = 82, 60


def badge_openup(ic, sel):
    box(ic, 52, 46, 20, 12)
    arrow(ic, 62, 44 if sel else 38, 0, 1, size=0.75)


def badge_print(ic, sel):
    ic.add(rect(56, 30, 72, 39), (255, 255, 255))
    ic.add(rrect(51, 36, 77, 50, 2.5), linear((51, 36), (77, 50), rgb('#E9ECF0'), rgb('#8E96A2')), gloss=0.25)
    out = 8 if sel else 4
    ic.add(rect(56, 46, 72, 46 + out + 2), (255, 255, 255), outline=1.3)
    ic.add(ellipse(71, 39, 74, 42), rgb('#7CFF6A') if sel else rgb('#2E5A2A'), outline=0.8)


def badge_browser(ic, sel):
    cx, cy, r = 64, 45, 11
    sphere = ellipse(cx - r, cy - r, cx + r, cy + r)
    ic.add(sphere, radial((cx - 4, cy - 5), r * 2.0, rgb('#7DB6F2'), rgb('#123A78')))
    o = 4 if sel else 0
    land = union(poly([(cx - 8 + o, cy - 6), (cx - 2 + o, cy - 8), (cx + 1 + o, cy - 4), (cx - 3 + o, cy + 4), (cx - 7 + o, cy + 1)]),
                 poly([(cx + 4 + o - 22, cy), (cx + 9 + o - 22, cy - 2), (cx + 10 + o - 22, cy + 5), (cx + 5 + o - 22, cy + 7)]),
                 poly([(cx + 4 + o, cy), (cx + 9 + o, cy - 2), (cx + 10 + o, cy + 5), (cx + 5 + o, cy + 7)]))
    ic.detail(inter(land, sphere), rgb('#6CC24A'))


def badge_write(ic, sel):
    page(ic, 54, 30, 74, 56, holes=False)
    ic.detail(rect(57, 34, 71, 36), BLUE_LO)
    for i in range(4 if sel else 3):
        ic.detail(rect(57, 39 + i * 3.6, 70, 40.2 + i * 3.6), CONTENT)


def own_drawer(ic, sel):
    """A drawer of our own drawing in the style (for a pack without Gorilla):
    an open tray seen from a little above, a handle on its front."""
    ic.add(poly([(5, 24), (75, 24), (69, 13), (11, 13)]), linear((0, 13), (0, 24), rgb('#8C8A6C'), rgb('#5F5D45')))
    ic.detail(poly([(13, 15.5), (67, 15.5), (71, 22.5), (9, 22.5)]), rgb('#4A4935'))
    if sel:
        for i, c in enumerate((rgb('#F4F5F6'), rgb('#DDE7F4'), rgb('#F6E7A6'))):
            x = 17 + i * 14
            ic.add(rect(x, 8 - i, x + 16, 22), c, outline=1.2)
            ic.detail(rect(x + 3, 11 - i, x + 13, 12 - i), CONTENT)
    ic.add(rrect(2, 21, 77, 54, 7), linear((2, 21), (77, 54), rgb('#F0EFE2'), rgb('#A9A78A')), gloss=0.3)
    ic.detail(arc(28, 27, 50, 41, 25, 155, 1.8), rgb('#4A4935'))


def drawer_icon(badge, own: bool):
    def fn(sel):
        ic = Icon()
        if own:
            own_drawer(ic, sel)
        badge(ic, sel)
        return ic
    return fn


# ---- the list ------------------------------------------------------------------------------------

def gamepads(sel):
    """A game pad: a d-pad and four face buttons; chosen, A is pressed and the d-pad's up arm lit."""
    ic = Icon()
    body = union(rrect(6, 22, 58, 42, 9), ellipse(4, 30, 22, 54), ellipse(42, 30, 60, 54))
    ic.add(body, linear((0, 22), (0, 54), rgb('#7E8794'), rgb('#2A2F37')), gloss=0.35)
    # the d-pad, a cross on the left grip
    cx, cy = 17, 33
    ic.add(union(rect(cx - 2.6, cy - 7, cx + 2.6, cy + 7), rect(cx - 7, cy - 2.6, cx + 7, cy + 2.6)), rgb('#1B1E23'), outline=1.0)
    if sel:
        ic.detail(rect(cx - 1.6, cy - 6, cx + 1.6, cy - 2.6), lighter(BLUE, 0.35))
    # the face buttons in a diamond on the right: Y top, X left, B right, A bottom
    bx, by, d, r = 47, 33, 5.6, 3.1
    for (dx, dy), (c, lo), name in (((0, -1), (GOLD, GOLD_LO), "Y"), ((-1, 0), (BLUE, BLUE_LO), "X"),
                                    ((1, 0), (RED, RED_LO), "B"), ((0, 1), (GREEN, GREEN_LO), "A")):
        x, y = bx + dx * d, by + dy * d
        pressed = sel and name == "A"
        top, bottom = (darker(c, 0.15), lo) if pressed else (lighter(c, 0.3), lo)
        ic.add(ellipse(x - r, y - r + (0.8 if pressed else 0), x + r, y + r + (0.8 if pressed else 0)),
               linear((x - r, y - r), (x + r, y + r), top, bottom), outline=1.1, gloss=0.0 if pressed else 0.45)
    # Back and Start between the grips
    for x in (27, 33.5):
        ic.detail(rrect(x, 26.5, x + 4, 28.5, 1), rgb('#C9CED6'))
    prefs_badge(ic, 3, 3, 1.35)
    return ic


ICONS = {
    "OpenFiles": openfiles, "OpenView": openview, "OpenEdit": openedit, "OpenWrite": openwrite,
    "OpenPlay": openplay, "OpenBrowser": openbrowser, "OpenCompress": opencompress,
    "Printers and Queue": printer, "Install-OpenUp": install, "Uninstall-OpenUp": uninstall,
    "OpenUp-Setup": setup, "ZuneTest": zunetest,
    "Look": look, "Dock": dock, "Menus": menus, "Sound": sound, "TitleBar": titlebar, "Windows": windows, "OpenTypes": types,
    "Gamepads": gamepads,
}
BADGES = {"OpenUp": badge_openup, "OpenPrint": badge_print, "OpenBrowser": badge_browser, "OpenWrite": badge_write}


def drawer_frames(badge) -> tuple[list[Image.Image], list[Image.Image]]:
    """The badge on our own drawer, and the badge alone, both in the drawer's
    frame (82 x 60 pixels, Gorilla's drawer's size), so the pack build can lay
    the badge alone on Gorilla's drawer."""
    region = (P(0), P(0), P(DRAWER_W), P(DRAWER_H))
    shrink = lambda im: im.crop(region).resize((DRAWER_W, DRAWER_H), Image.LANCZOS)
    own = [shrink(drawer_icon(badge, True)(s).render()) for s in (False, True)]
    alone = [shrink(drawer_icon(badge, False)(s).render()) for s in (False, True)]
    return own, alone


def main(out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for name, fn in ICONS.items():
        rest, chosen = finish([fn(False).render(), fn(True).render()])
        rest.save(out / f"{name}.png")
        chosen.save(out / f"{name}-selected.png")
    for name, badge in BADGES.items():
        own, alone = drawer_frames(badge)
        own[0].save(out / f"drawer-{name}.png")
        own[1].save(out / f"drawer-{name}-selected.png")
        alone[0].save(out / f"badge-{name}.png")
        alone[1].save(out / f"badge-{name}-selected.png")
    print(f"{len(ICONS)} icons and {len(BADGES)} drawers in {out}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
