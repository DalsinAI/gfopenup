#!/usr/bin/env python3
"""The dock and the drives in the AOS drawer style (Dale, 12 November 2026, 13:52 "go with those", 15:04 "those work").

    python3 tools/aos_icons_art.py icons/open

Our own drawing in the genre of today's Amiga icon sets: the drives and drawers are a chunky navy box seen from the
front and a little above, its handle recess lit warm, with what it holds standing large in the opening; the dock's
programs are rendered objects with a sheen over the top and a faint floor reflection. Each is drawn at 512 x 512 and
cut down; chosen, something moves (ICONS.md section 1): what a drawer holds rises out of it, and an object lifts off
its reflection. They work as GlowIcons: every pixel solid or see-through and each frame at most 255 colours, which
the pack's open_icons() (build_pack.py) and openicons_part.py make of them.

OPEN names the pictures for icons/open (drawn at 75 x 75, 64 at the pack's ICON_SCALE); tools/open_icons_art.py
leaves those names alone. openicons_part.py takes frames() for the Shell, the drives and the kinds.

MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops

# ---- drawing helpers ------------------------------------------------------------------------------

B = 512

ACC = (233, 84, 32)        # the warm accent

RED = (222, 52, 56)

def L(): return Image.new("L", (B, B), 0)

def rgba(c, a=255): return Image.new("RGBA", (B, B), tuple(c) + (a,))

def grad(top, bot, y0=0, y1=B, horiz=False):
    t = np.clip((np.arange(B) - y0) / max(1, y1 - y0), 0, 1)
    a = np.array(top, np.float32); b = np.array(bot, np.float32)
    row = a[None] + (b - a)[None] * t[:, None]
    arr = np.repeat(row[:, None, :], B, 1) if not horiz else np.repeat(row[None, :, :], B, 0)
    return Image.fromarray(arr.astype(np.uint8)).convert("RGBA")

def paint(img, fill, m):
    f = fill if isinstance(fill, Image.Image) else rgba(fill)
    f = f.copy(); f.putalpha(ImageChops.multiply(f.getchannel("A"), m))
    return Image.alpha_composite(img, f)

def rr(box, r):
    m = L(); ImageDraw.Draw(m).rounded_rectangle(box, r, fill=255); return m

def poly(pts):
    m = L(); ImageDraw.Draw(m).polygon(pts, fill=255); return m

def ell(box):
    m = L(); ImageDraw.Draw(m).ellipse(box, fill=255); return m

def line(pts, w):
    m = L(); d = ImageDraw.Draw(m); d.line(pts, fill=255, width=w, joint="curve")
    for x, y in (pts[0], pts[-1]): d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=255)
    return m

def sub(a, b): return ImageChops.subtract(a, b)

def add(*ms):
    o = ms[0]
    for m in ms[1:]: o = ImageChops.lighter(o, m)
    return o

def shadow(img, m, dy=14, blur=14, a=0.32):
    s = rgba((20, 24, 40)); s.putalpha(ImageChops.offset(m, 0, dy).filter(ImageFilter.GaussianBlur(blur)).point(lambda v: int(v * a)))
    return Image.alpha_composite(img, s)

def emboss(img, m, w=6, a=0.55):
    """the soft highlight along a shape's top edge, and a darker lip along its bottom"""
    top = sub(m, ImageChops.offset(m, 0, w)).filter(ImageFilter.GaussianBlur(2))
    img = paint(img, (255, 255, 255, ), top.point(lambda v: int(v * a)))
    bot = sub(m, ImageChops.offset(m, 0, -w)).filter(ImageFilter.GaussianBlur(2))
    return paint(img, (0, 0, 0), bot.point(lambda v: int(v * 0.18)))

def engrave(img, m, base):
    """an emblem set into a surface: darker fill, light lower edge"""
    img = paint(img, tuple(int(c * 0.72) for c in base), m)
    lo = sub(ImageChops.offset(m, 0, 4), m)
    return paint(img, (255, 255, 255), lo.point(lambda v: int(v * 0.45)))

def light(c, k): return tuple(min(255, int(v + (255 - v) * k)) for v in c)

def dark(c, k): return tuple(int(v * (1 - k)) for v in c)

def drive(body=(206, 210, 218), emblem=None, emblem_col=None, led=(90, 220, 120), label=None):
    img = Image.new("RGBA", (B, B))
    top = rr((56, 150, 456, 340), 34); front = rr((56, 300, 456, 372), 34)
    whole = add(top, front)
    img = shadow(img, whole, 18, 18, 0.35)
    img = paint(img, dark(body, 0.38), front)
    img = paint(img, grad(light(body, 0.25), dark(body, 0.06), 150, 340), top)
    img = emboss(img, top, 6)
    # front: the slot line and the LED
    img = paint(img, dark(body, 0.55), rr((90, 344, 300, 352), 4))
    img = paint(img, led, ell((392, 338, 410, 356)))
    img = paint(img, (255, 255, 255), ell((395, 340, 402, 347)).point(lambda v: v // 2))
    if label:
        img = paint(img, label, rr((92, 186, 300, 300), 14))
        img = paint(img, (255, 255, 255), sub(rr((92, 186, 300, 300), 14), ImageChops.offset(rr((92, 186, 300, 300), 14), 0, 4)).point(lambda v: v // 3))
        for i, y in enumerate((214, 242, 270)):
            img = paint(img, dark(label, 0.35), rr((114, y, 260 - i * 34, y + 10), 5))
    if emblem is not None:
        img = paint(img, emblem_col, emblem)
    return img

def boing(cx, cy, R):
    """a small ball, red and white squares wrapped on it, lit from the top left: the boot drive's emblem"""
    y, x = np.mgrid[0:B, 0:B].astype(np.float32)
    dx, dy = (x - cx) / R, (y - cy) / R
    r2 = dx * dx + dy * dy
    z = np.sqrt(np.clip(1 - r2, 0, 1))
    rot = 0.35
    X = dx * math.cos(rot) - dy * math.sin(rot); Y = dx * math.sin(rot) + dy * math.cos(rot)
    lon = np.arctan2(X, z + 1e-6); lat = np.arcsin(np.clip(Y, -1, 1))
    chk = ((np.floor(lon / (math.pi / 6)) + np.floor(lat / (math.pi / 6))) % 2)
    col = np.where(chk[..., None] > 0, np.array([250, 250, 252], np.float32), np.array([222, 40, 48], np.float32))
    shade = 0.62 + 0.38 * np.clip(-dx * 0.5 - dy * 0.6 + z * 0.65, 0, 1)
    col = col * shade[..., None]
    a = np.clip((1 - np.sqrt(r2)) * R * 0.6, 0, 1)
    im = Image.fromarray(np.clip(col, 0, 255).astype(np.uint8)).convert("RGBA")
    im.putalpha(Image.fromarray((a * 255).astype(np.uint8)))
    return im

def gamepad(cx, cy, s, col):
    m = add(rr((cx - 60 * s, cy - 26 * s, cx + 60 * s, cy + 22 * s), 24 * s),
            ell((cx - 66 * s, cy - 18 * s, cx - 22 * s, cy + 40 * s)), ell((cx + 22 * s, cy - 18 * s, cx + 66 * s, cy + 40 * s)))
    return m

def ram():
    img = Image.new("RGBA", (B, B))
    pcb = add(rr((40, 196, 472, 330), 10))
    img = shadow(img, pcb, 16, 16, 0.35)
    img = paint(img, grad((40, 150, 96), (22, 104, 66), 196, 330), pcb)
    img = emboss(img, pcb, 5)
    for i in range(6):
        x = 66 + i * 66
        chip = rr((x, 214, x + 52, 282), 5)
        img = paint(img, grad((58, 62, 72), (30, 32, 40), 214, 282), chip)
        img = paint(img, (255, 255, 255), sub(chip, ImageChops.offset(chip, 0, 3)).point(lambda v: v // 4))
    for i in range(26):
        x = 56 + i * 16
        img = paint(img, grad((248, 214, 120), (200, 150, 60), 300, 326), rr((x, 300, x + 10, 326), 2))
    img = paint(img, Image.new("RGBA", (B, B), (0, 0, 0, 0)), L())
    notch = rr((240, 306, 262, 340), 4); img.putalpha(sub(img.getchannel("A"), notch))
    return img

def usb():
    img = Image.new("RGBA", (B, B))
    body = rr((70, 196, 340, 316), 40); plug = rr((330, 214, 452, 298), 8)
    img = shadow(img, add(body, plug), 16, 16, 0.35)
    img = paint(img, grad((236, 238, 242), (190, 194, 204), 214, 298, ), plug)
    img = emboss(img, plug, 4)
    img = paint(img, (60, 64, 76), add(rr((366, 232, 390, 248), 3), rr((404, 232, 428, 248), 3)))
    img = paint(img, grad((70, 76, 92), (36, 40, 52), 196, 316), body)
    img = emboss(img, body, 6, 0.35)
    img = paint(img, ACC, rr((100, 248, 160, 264), 8))
    return img

def rsq(col_top, col_bot, r=96):
    img = Image.new("RGBA", (B, B)); m = rr((40, 40, 472, 472), r)
    img = shadow(img, m, 14, 16, 0.32)
    img = paint(img, grad(col_top, col_bot, 40, 472), m)
    return emboss(img, m, 7, 0.4), m

def terminal():
    img, m = rsq((70, 74, 86), (34, 36, 44))
    img = paint(img, (255, 255, 255), line([(120, 190), (200, 256), (120, 322)], 30))
    img = paint(img, ACC, rr((228, 306, 340, 336), 12))
    return img

def viewer():
    img = Image.new("RGBA", (B, B))
    back = poly([(110, 120), (430, 150), (410, 400), (90, 370)])
    img = shadow(img, back, 12, 14, 0.25); img = paint(img, (232, 236, 242), back)
    frame = rr((60, 130, 452, 412), 20)
    img = shadow(img, frame, 14, 16, 0.3); img = paint(img, (252, 252, 253), frame)
    pic = rr((84, 154, 428, 388), 8)
    img = paint(img, grad((120, 190, 250), (200, 230, 255), 154, 330), pic)
    img = paint(img, (255, 214, 90), ell((330, 180, 386, 236)))
    img = paint(img, (92, 168, 110), ImageChops.multiply(poly([(84, 388), (84, 320), (200, 230), (290, 330), (340, 290), (428, 360), (428, 388)]), pic))
    img = paint(img, (60, 130, 84), ImageChops.multiply(poly([(200, 388), (290, 300), (380, 388)]), pic))
    return img

def paper(lines=True):
    img = Image.new("RGBA", (B, B))
    p = poly([(110, 52), (334, 52), (410, 128), (410, 460), (110, 460)])
    p = ImageChops.multiply(p, rr((110, 52, 410, 460), 18))
    img = shadow(img, p, 12, 14, 0.3)
    img = paint(img, grad((255, 255, 255), (236, 239, 244), 52, 460), p)
    fold = poly([(334, 52), (334, 128), (410, 128)])
    img = shadow(img, fold, 4, 6, 0.25); img = paint(img, (222, 226, 234), fold)
    if lines:
        for i, y in enumerate((170, 210, 250, 290, 330, 370)):
            img = paint(img, (178, 186, 200), rr((150, y, 370 - (60 if i == 5 else 0), y + 12), 6))
    return img, p

def text():
    img, p = paper()
    return paint(img, ACC, rr((150, 410, 260, 422), 6))

def edit():
    img, p = paper()
    pen = line([(210, 430), (420, 160)], 44)
    img = shadow(img, pen, 10, 10, 0.3)
    img = paint(img, grad((255, 200, 70), (236, 150, 40), 160, 430), pen)
    tip = poly([(196, 450), (208, 404), (236, 426)])
    img = paint(img, (238, 214, 180), tip); img = paint(img, (50, 50, 60), poly([(196, 450), (200, 436), (210, 444)]))
    img = paint(img, RED, line([(398, 188), (420, 160)], 44))
    return img

def paint_icon():
    img = Image.new("RGBA", (B, B))
    pal = sub(ell((50, 110, 440, 420)), ell((140, 300, 210, 370)))
    img = shadow(img, pal, 14, 16, 0.32)
    img = paint(img, grad((236, 214, 182), (206, 178, 140), 110, 420), pal)
    img = emboss(img, pal, 6)
    for (x, y), c in zip([(120, 190), (200, 150), (290, 160), (350, 220), (330, 300)],
                         [RED, (255, 196, 60), (70, 170, 90), (60, 130, 230), (150, 80, 200)]):
        blob = ell((x, y, x + 58, y + 50)); img = paint(img, c, blob)
        img = paint(img, (255, 255, 255), ell((x + 12, y + 8, x + 28, y + 20)).point(lambda v: v // 2))
    brush = line([(250, 440), (450, 250)], 26)
    img = shadow(img, brush, 8, 8, 0.3); img = paint(img, (120, 80, 50), brush)
    img = paint(img, (200, 204, 212), line([(236, 452), (268, 420)], 34)); img = paint(img, (40, 40, 48), line([(200, 488), (236, 452)], 30))
    return img

def media():
    img, m = rsq((255, 104, 72), (214, 46, 60))
    film = L()
    for i in range(7):
        film = add(film, rr((76 + i * 56, 78, 104 + i * 56, 104), 6), rr((76 + i * 56, 408, 104 + i * 56, 434), 6))
    img = paint(img, (255, 255, 255), film.point(lambda v: v // 3))
    tri = poly([(208, 170), (352, 256), (208, 342)])
    img = shadow(img, tri, 6, 8, 0.3)
    return paint(img, (255, 255, 255), tri)

def monitor():
    img = Image.new("RGBA", (B, B))
    scr = rr((40, 70, 472, 380), 28)
    stand = add(poly([(214, 380), (298, 380), (318, 440), (194, 440)]), rr((150, 430, 362, 456), 12))
    img = shadow(img, add(scr, stand), 14, 16, 0.32)
    img = paint(img, grad((200, 204, 212), (150, 156, 168), 380, 456), stand)
    img = paint(img, grad((78, 82, 94), (40, 42, 52), 70, 380), scr); img = emboss(img, scr, 6, 0.4)
    inner = rr((64, 94, 448, 356), 12)
    img = paint(img, (22, 26, 34), inner)
    for y in (160, 220, 280): img = paint(img, (44, 52, 66), rr((80, y, 432, y + 3), 1))
    g = line([(84, 320), (150, 280), (200, 300), (260, 190), (320, 230), (380, 150), (430, 170)], 12)
    img = paint(img, (90, 210, 130), ImageChops.multiply(g, inner))
    area = poly([(84, 330), (84, 320), (150, 280), (200, 300), (260, 190), (320, 230), (380, 150), (430, 170), (430, 356), (84, 356)])
    img = paint(img, (90, 210, 130), ImageChops.multiply(area, inner).point(lambda v: v // 6))
    return img

def systool():
    img, m = rsq((120, 128, 144), (70, 76, 90))
    cog = L(); d = ImageDraw.Draw(cog); c = 256
    d.ellipse((c - 120, c - 120, c + 120, c + 120), fill=255)
    for i in range(10):
        a = i * math.pi / 5; ca, sa = math.cos(a), math.sin(a)
        pts = [(c + ca * r - sa * w, c + sa * r + ca * w) for r, w in ((100, -26), (152, -20), (152, 20), (100, 26))]
        d.polygon(pts, fill=255)
    cog = sub(cog, ell((c - 52, c - 52, c + 52, c + 52)))
    img = shadow(img, cog, 8, 10, 0.4)
    img = paint(img, grad((246, 247, 250), (196, 202, 214), 100, 410), cog)
    return emboss(img, cog, 5, 0.6)

def install():
    img = Image.new("RGBA", (B, B))
    left = poly([(70, 210), (256, 290), (256, 470), (70, 390)]); right = poly([(256, 290), (442, 210), (442, 390), (256, 470)])
    top = poly([(70, 210), (256, 130), (442, 210), (256, 290)])
    img = shadow(img, add(left, right, top), 14, 16, 0.32)
    img = paint(img, (200, 150, 96), left); img = paint(img, (176, 126, 76), right); img = paint(img, (222, 178, 124), top)
    img = paint(img, (236, 214, 176), poly([(160, 172), (190, 160), (376, 238), (346, 250)]))
    arr = add(rr((226, 30, 286, 180), 14), poly([(176, 160), (336, 160), (256, 250)]))
    img = shadow(img, arr, 8, 10, 0.35)
    return paint(img, grad(light(ACC, 0.1), dark(ACC, 0.1), 30, 250), arr)


# ---- sheen, rim, floor reflection -------------------------------------------------------------------

def gloss(img, m, k=0.55, top=0.0, bottom=0.5):
    """a rendered sheen: white fading down over the top part of shape m, with a curved lower edge"""
    bb = m.getbbox()
    if not bb: return img
    x0, y0, x1, y1 = bb; h = y1 - y0
    band = L(); ImageDraw.Draw(band).ellipse((x0 - (x1 - x0) * .4, y0 - h * 1.1, x1 + (x1 - x0) * .4, y0 + h * bottom), fill=255)
    g = grad((255, 255, 255), (255, 255, 255)); 
    fade = np.clip(1 - (np.arange(B) - y0) / max(1, h * bottom), 0, 1)
    fm = Image.fromarray((np.repeat(fade[:, None], B, 1) * 255 * k).astype(np.uint8))
    a = ImageChops.multiply(ImageChops.multiply(band, m), fm)
    return paint(img, (255, 255, 255), a)

def rim(img, m, col=(255, 255, 255), k=0.7, w=3):
    edge = sub(m, ImageChops.offset(m, 0, w))
    return paint(img, col, edge.point(lambda v: int(v * k)))

def reflect(img, m, k=0.25):
    """a faint mirror of the object under it, as the OS4 sets stand on a glossy floor"""
    bb = m.getbbox()
    if not bb: return img
    y1 = bb[3]
    flip = img.transpose(Image.FLIP_TOP_BOTTOM)
    off = ImageChops.offset(flip, 0, 2 * y1 - B)
    a = off.getchannel("A")
    fade = np.clip(1 - (np.arange(B) - y1) / 60.0, 0, 1) * (np.arange(B) >= y1)
    fm = Image.fromarray((np.repeat(fade[:, None], B, 1) * 255 * k).astype(np.uint8))
    off.putalpha(ImageChops.multiply(a, fm))
    out = Image.new("RGBA", (B, B)); out = Image.alpha_composite(out, off)
    return Image.alpha_composite(out, img)

def glow(img, m, col, r=26, k=0.9):
    g = rgba(col); g.putalpha(m.filter(ImageFilter.GaussianBlur(r)).point(lambda v: min(255, int(v * k * 1.6))))
    return Image.alpha_composite(g, img) if False else Image.alpha_composite(img, g)

def pad_icon():
    img = Image.new("RGBA", (B, B)); pad = gamepad(256, 270, 3.2, None)
    img = shadow(img, pad, 10, 12, 0.4); img = paint(img, grad((70, 74, 90), (30, 32, 42), 170, 420), pad)
    img = gloss(img, pad, 0.4, bottom=0.5)
    img = paint(img, (230, 232, 240), add(rr((96, 244, 196, 274), 8), rr((131, 209, 161, 309), 8)))
    for (x, y), c in zip([(330, 200), (380, 250), (330, 300), (280, 250)], [RED, ACC, (80, 150, 240), (90, 200, 120)]):
        img = paint(img, c, ell((x, y, x + 44, y + 44)))
    return img


# ---- the drawer: a chunky navy box seen from the front and a little above ------------------------

NAVY = ((70, 110, 200), (18, 34, 92))

def tray(body=NAVY):
    """the drawer alone: a chunky rounded box, its rim round the opening, a front with the handle recess"""
    top, bot = body
    img = Image.new("RGBA", (B, B))
    rimm = add(poly([(132, 262), (380, 262), (430, 318), (82, 318)]), rr((82, 300, 430, 336), 16))
    front = ImageChops.multiply(poly([(86, 318), (426, 318), (410, 486), (102, 486)]), rr((86, 300, 426, 486), 34))
    whole = add(rimm, front)
    img = shadow(img, whole, 14, 18, 0.65)
    img = paint(img, grad(top, dark(bot, 0.25), 318, 486), front)
    img = gloss(img, front, 0.3, bottom=0.3)
    slot = rr((156, 380, 356, 434), 26)
    img = paint(img, (8, 10, 24), slot)
    warm = ell((186, 402, 326, 460)).filter(ImageFilter.GaussianBlur(14))
    img = paint(img, (214, 126, 74), ImageChops.multiply(warm, slot).point(lambda v: int(v * .8)))
    img = paint(img, (255, 255, 255), sub(ImageChops.offset(slot, 0, 4), slot).point(lambda v: v // 3))
    img = paint(img, (0, 0, 0), sub(slot, ImageChops.offset(slot, 0, 6)).point(lambda v: v // 2))
    img = paint(img, (255, 255, 255), sub(front, ImageChops.offset(front, 4, 0)).point(lambda v: v // 4))
    img = paint(img, grad(light(top, 0.4), top, 262, 336), rimm)
    opening = poly([(150, 274), (362, 274), (402, 314), (110, 314)])
    img = paint(img, grad((6, 10, 26), (26, 44, 100), 274, 314), opening)
    img = rim(img, rimm, light(top, 0.85), 0.95, 4)
    return img, opening

def drawer(emblem=None, scale=0.66, lift=0, body=NAVY):
    img, opening = tray(body)
    if emblem is None:
        return img
    em = emblem().resize((int(B * scale),) * 2, Image.LANCZOS)
    bb = em.getchannel("A").getbbox() or (0, 0, em.width, em.height)
    layer = Image.new("RGBA", (B, B))
    # the thing stands sunk into the opening: its foot a little below the rim's front edge
    layer.alpha_composite(em, ((B - em.width) // 2, 338 - bb[3] - lift))
    front_edge = add(rr((82, 300, 430, 336), 16).point(lambda v: v), ImageChops.multiply(poly([(86, 318), (426, 318), (410, 486), (102, 486)]), rr((86, 300, 426, 486), 34)))
    front_edge = sub(front_edge, poly([(0, 0), (B, 0), (B, 318), (0, 318)]))
    layer.putalpha(ImageChops.multiply(layer.getchannel("A"), sub(rgba((0, 0, 0)).getchannel("A"), front_edge)))
    return Image.alpha_composite(img, layer)

def boing_obj():
    img = Image.new("RGBA", (B, B))
    img = shadow(img, ell((56, 56, 456, 456)), 16, 20, 0.5)
    img = Image.alpha_composite(img, boing(256, 256, 200))
    return paint(img, (255, 255, 255), ell((130, 100, 260, 190)).filter(ImageFilter.GaussianBlur(20)).point(lambda v: int(v * .55)))

def cog_obj():
    img = systool()
    # the cog alone, without its plate
    cog = L(); d = ImageDraw.Draw(cog); c = 256
    d.ellipse((c - 150, c - 150, c + 150, c + 150), fill=255)
    for i in range(10):
        a = i * math.pi / 5; ca, sa = math.cos(a), math.sin(a)
        d.polygon([(c + ca * r - sa * w, c + sa * r + ca * w) for r, w in ((120, -34), (200, -26), (200, 26), (120, 34))], fill=255)
    cog = sub(cog, ell((c - 66, c - 66, c + 66, c + 66)))
    out = Image.new("RGBA", (B, B)); out = shadow(out, cog, 12, 14, 0.5)
    out = paint(out, grad((236, 238, 244), (140, 146, 160), 56, 456), cog)
    out = gloss(out, cog, 0.5, bottom=0.45)
    return rim(out, cog, (255, 255, 255), 0.8, 5)

def globe_obj():
    y, x = np.mgrid[0:B, 0:B].astype(np.float32); c = 256; R = 200
    dx, dy = (x - c) / R, (y - c) / R; r2 = dx * dx + dy * dy; z = np.sqrt(np.clip(1 - r2, 0, 1))
    base = np.array([40, 110, 220], np.float32) * (0.45 + 0.6 * np.clip(-dx * .4 - dy * .6 + z * .7, 0, 1))[..., None]
    lon = np.arctan2(dx, z + 1e-6); lat = np.arcsin(np.clip(dy, -1, 1))
    grid = (np.abs(np.sin(lon * 4)) < 0.06) | (np.abs(np.sin(lat * 4)) < 0.06)
    col = np.where(grid[..., None], base * 0.5 + 120, base)
    a = np.clip((1 - np.sqrt(r2)) * R * .6, 0, 1)
    im = Image.fromarray(np.clip(col, 0, 255).astype(np.uint8)).convert("RGBA"); im.putalpha(Image.fromarray((a * 255).astype(np.uint8)))
    out = shadow(Image.new("RGBA", (B, B)), ell((56, 56, 456, 456)), 14, 18, 0.5)
    out = Image.alpha_composite(out, im)
    return paint(out, (255, 255, 255), ell((140, 100, 260, 180)).filter(ImageFilter.GaussianBlur(18)).point(lambda v: int(v * .5)))

def doc_obj(): return paper()[0]

def ram_obj(): return ram()

def usb_obj(): return usb()


def cd_obj():
    """a compact disc, silver with a rainbow sheen round its hub, lying a little tilted towards us"""
    y, x = np.mgrid[0:B, 0:B].astype(np.float32)
    cx, cy, R, ry = 256, 270, 210, 150                      # an ellipse: the disc seen from a little above
    dx, dy = (x - cx) / R, (y - cy) / ry
    r = np.sqrt(dx * dx + dy * dy)
    ang = np.arctan2(dy, dx)
    hue = (ang / (2 * math.pi) + 0.5 + r * 0.35) % 1.0
    k = np.clip(1 - np.abs(r - 0.62) * 3.2, 0, 1) * 0.55   # the rainbow band, strongest mid-way out
    rain = np.stack([0.5 + 0.5 * np.cos(2 * math.pi * (hue - t)) for t in (0, 1 / 3, 2 / 3)], -1) * 255
    silver = (200 + 40 * np.clip(-dy * 0.6 + 0.3, -1, 1))[..., None] * np.ones(3, np.float32)
    col = silver * (1 - k[..., None]) + rain * k[..., None]
    a = np.clip((1 - r) * R * 0.5, 0, 1) * np.clip((r - 0.17) * R * 0.5, 0, 1)
    im = Image.fromarray(np.clip(col, 0, 255).astype(np.uint8)).convert("RGBA")
    im.putalpha(Image.fromarray((a * 255).astype(np.uint8)))
    disc = im.getchannel("A")
    out = shadow(Image.new("RGBA", (B, B)), disc, 14, 16, 0.45)
    out = Image.alpha_composite(out, im)
    hub = ell((cx - 70, cy - 50, cx + 70, cy + 50))
    out = paint(out, (226, 230, 238), ImageChops.multiply(hub, disc))
    out = rim(out, disc, (255, 255, 255), 0.6, 3)
    return paint(out, (255, 255, 255), ell((120, 150, 300, 230)).filter(ImageFilter.GaussianBlur(22)).point(lambda v: int(v * .5)))


def floppy_obj():
    """a 3.5-inch disk from the front: dark body, metal shutter at the top, white label below"""
    img = Image.new("RGBA", (B, B))
    body = sub(rr((86, 66, 426, 446), 18), poly([(386, 66), (426, 66), (426, 106)]))   # the cut corner
    img = shadow(img, body, 16, 18, 0.42)
    img = paint(img, grad((70, 78, 96), (34, 38, 50), 66, 446), body)
    img = emboss(img, body, 6, 0.4)
    shutter = rr((166, 66, 346, 196), 6)
    img = paint(img, grad((236, 238, 244), (160, 166, 178), 66, 196), shutter)
    img = paint(img, (40, 44, 56), rr((276, 92, 316, 176), 6))                    # the window in it
    img = paint(img, (255, 255, 255), sub(shutter, ImageChops.offset(shutter, 0, 4)).point(lambda v: v // 3))
    label = rr((126, 252, 386, 430), 12)
    img = paint(img, grad((250, 250, 252), (222, 226, 234), 252, 430), label)
    img = paint(img, RED, rr((126, 252, 386, 276), 6))
    for i, y in enumerate((304, 340, 376)):
        img = paint(img, (150, 156, 170), rr((150, y, 362 - i * 50, y + 10), 5))
    return img


def net_obj():
    """a network share: a small server box with two links running out to other machines"""
    img = Image.new("RGBA", (B, B))
    links = add(line([(256, 250), (110, 400)], 16), line([(256, 250), (402, 400)], 16))
    img = shadow(img, links, 6, 8, 0.35)
    img = paint(img, (60, 130, 230), links)
    for cx in (110, 402):
        node = rr((cx - 62, 360, cx + 62, 446), 14)
        img = shadow(img, node, 8, 10, 0.4)
        img = paint(img, grad((236, 238, 244), (170, 176, 190), 360, 446), node)
        img = paint(img, grad((70, 120, 210), (40, 80, 170), 372, 420), rr((cx - 46, 372, cx + 46, 420), 6))
    box = rr((150, 60, 362, 290), 22)
    img = shadow(img, box, 14, 16, 0.45)
    img = paint(img, grad((86, 92, 110), (40, 44, 56), 60, 290), box)
    img = emboss(img, box, 6, 0.4)
    for y in (100, 162, 224):
        bay = rr((176, y, 336, y + 44), 8)
        img = paint(img, (28, 30, 40), bay)
        img = paint(img, (90, 220, 120), ell((304, y + 16, 318, y + 30)))
    return img


# ---- the dock's objects ---------------------------------------------------------------------------

def clock():
    img = Image.new("RGBA", (B, B)); face = ell((56, 56, 456, 456))
    img = shadow(img, face, 14, 18, 0.45)
    img = paint(img, grad((210, 214, 224), (120, 126, 140), 56, 456), face)
    inner = ell((86, 86, 426, 426)); img = paint(img, grad((255, 255, 255), (228, 232, 240), 86, 426), inner)
    for i in range(12):
        a = i * math.pi / 6; r0, r1 = (130, 160) if i % 3 else (118, 160)
        img = paint(img, (60, 64, 76), line([(256 + math.sin(a) * r0, 256 - math.cos(a) * r0), (256 + math.sin(a) * r1, 256 - math.cos(a) * r1)], 10 if i % 3 else 16))
    img = paint(img, (40, 44, 56), line([(256, 256), (256 + 70, 256 - 80)], 18))
    img = paint(img, (40, 44, 56), line([(256, 256), (256 - 10, 256 - 140)], 12))
    img = paint(img, RED, line([(256, 286), (256 + 110, 256 + 80)], 6)); img = paint(img, RED, ell((240, 240, 272, 272)))
    return img

def calculator():
    img = Image.new("RGBA", (B, B)); body = rr((96, 40, 416, 472), 36)
    img = shadow(img, body, 14, 18, 0.45); img = paint(img, grad((86, 90, 104), (40, 42, 52), 40, 472), body)
    img = emboss(img, body, 6, 0.35)
    scr = rr((128, 72, 384, 164), 14); img = paint(img, grad((170, 220, 180), (120, 180, 140), 72, 164), scr)
    img = paint(img, (30, 60, 40), rr((250, 110, 360, 130), 4))
    for r in range(4):
        for c in range(4):
            x, y = 132 + c * 64, 196 + r * 66
            col = ACC if c == 3 else (226, 228, 236)
            k = rr((x, y, x + 52, y + 52), 12); img = paint(img, col, k); img = emboss(img, k, 3, 0.5)
    return img

def archive():
    img = Image.new("RGBA", (B, B))
    left = poly([(110, 230), (256, 292), (256, 450), (110, 388)]); right = poly([(256, 292), (402, 230), (402, 388), (256, 450)])
    top = poly([(110, 230), (256, 168), (402, 230), (256, 292)])
    img = shadow(img, add(left, right, top), 14, 16, 0.4)
    img = paint(img, (200, 150, 96), left); img = paint(img, (176, 126, 76), right); img = paint(img, (222, 178, 124), top)
    band = add(poly([(170, 204), (196, 194), (342, 256), (316, 266)]), poly([(316, 266), (342, 256), (342, 420), (316, 430)]))
    img = paint(img, (90, 96, 112), band)
    for sx in (1, -1):
        cx = 256 - sx * 196
        a = add(line([(cx, 300), (cx + sx * 70, 300)], 30), poly([(cx + sx * 64, 262), (cx + sx * 104, 300), (cx + sx * 64, 338)]))
        img = shadow(img, a, 5, 6, 0.4); img = paint(img, (90, 200, 120), a)
    return img

def ftp():
    img = globe_obj()
    for pts, col in (([(300, 360), (430, 360)], (90, 210, 120)), ([(212, 410), (82, 410)], ACC)):
        a = add(line(pts, 34), poly([(pts[1][0], pts[1][1] - 40), (pts[1][0] + (40 if pts[1][0] > pts[0][0] else -40), pts[1][1]), (pts[1][0], pts[1][1] + 40)]))
        img = shadow(img, a, 6, 6, 0.4); img = paint(img, col, a)
    return img

def putty():
    img = terminal()
    plug = add(rr((300, 300, 420, 400), 18), rr((330, 260, 346, 304), 4), rr((374, 260, 390, 304), 4))
    img = shadow(img, plug, 6, 8, 0.4); img = paint(img, grad((240, 242, 246), (180, 186, 198), 260, 400), plug)
    return img

def look():
    img = Image.new("RGBA", (B, B)); win = rr((50, 80, 462, 430), 26)
    img = shadow(img, win, 14, 18, 0.45); img = paint(img, (246, 247, 250), win)
    img = paint(img, grad((70, 120, 210), (40, 80, 170), 80, 140), ImageChops.multiply(rr((50, 80, 462, 140), 26), add(rr((50, 80, 462, 140), 26))))
    for i, c in enumerate([RED, ACC, (255, 200, 60), (80, 190, 110), (60, 130, 230), (150, 80, 200)]):
        x = 82 + (i % 3) * 120; y = 170 + (i // 3) * 120
        s = rr((x, y, x + 98, y + 98), 16); img = paint(img, c, s); img = emboss(img, s, 4, 0.4)
    return img

def trashcan():
    img = Image.new("RGBA", (B, B))
    can = poly([(116, 130), (396, 130), (370, 470), (142, 470)]); lid = rr((90, 92, 422, 136), 16); knob = rr((216, 60, 296, 100), 14)
    img = shadow(img, add(can, lid), 14, 18, 0.45)
    img = paint(img, grad((206, 210, 220), (110, 116, 130), 130, 470, horiz=False), can)
    for x in (180, 256, 332):
        img = paint(img, (90, 96, 110), line([(x, 170), (x + (x - 256) * 0.1, 430)], 14))
        img = paint(img, (255, 255, 255), line([(x + 10, 170), (x + 10 + (x - 256) * 0.1, 430)], 4).point(lambda v: v // 2))
    img = paint(img, grad((232, 234, 240), (160, 166, 178), 92, 136), lid); img = paint(img, (120, 126, 140), knob)
    return img

def drawer_files():
    def docs():
        im = Image.new("RGBA", (B, B))
        for dx, ang in ((-40, -8), (40, 8), (0, 0)):
            p, _ = paper(); p = p.rotate(ang, resample=Image.BICUBIC); im.alpha_composite(ImageChops.offset(p, dx, 0))
        return im
    return drawer(docs, .7)

# ---- the pictures, at rest and chosen ----------------------------------------------------------------
LIFT = 34                   # how far a chosen object rises, of 512


def obj(fn, k=0.35, chosen=False):
    """a standalone rendered object: its drawing, a sheen over the top, a faint floor reflection; chosen, it rises
    off its reflection"""
    img = fn()
    m = img.getchannel("A").point(lambda v: 255 if v > 200 else 0)
    img = gloss(img, m, k, bottom=0.42)
    if not chosen:
        return reflect(img, m, 0.18)
    up = Image.new("RGBA", (B, B))
    up.alpha_composite(img, (0, 0), (0, LIFT))          # moved up, not wrapped round
    floor = reflect(img, m, 0.18)
    floor.putalpha(ImageChops.multiply(floor.getchannel("A"), sub(rgba((0, 0, 0)).getchannel("A"), m)))
    return Image.alpha_composite(floor, up)


def ide_drive():
    return drive(body=(58, 64, 78), label=(232, 84, 52), led=(80, 160, 255))


def acfs_drive():
    """a hard disk held in a folder on the host (ACFS): a light drive with a blue label, told apart from the
    A1200's own dark IDE drive"""
    return drive(label=(70, 110, 200))


def held(emblem, scale):
    """a drawer holding EMBLEM; chosen, it rises out of it"""
    return lambda chosen: drawer(emblem, scale, lift=LIFT if chosen else 0)


def thing(fn, k=0.35):
    return lambda chosen: obj(fn, k, chosen)


def files_drawer(chosen):
    def docs():
        im = Image.new("RGBA", (B, B))
        for dx, ang in ((-40, -8), (40, 8), (0, 0)):
            p, _ = paper()
            im.alpha_composite(ImageChops.offset(p.rotate(ang, resample=Image.BICUBIC), dx, 0))
        return im
    return drawer(docs, .7, lift=LIFT if chosen else 0)


def empty_drawer(chosen):
    """the drawer alone; chosen, a sheet of paper stands in it"""
    return drawer(doc_obj, .6) if chosen else drawer(None)


PICTURES = {
    # the dock's programs
    "SHELL": thing(terminal, .3), "FILES": files_drawer, "VIEW": thing(viewer), "CLOCK": thing(clock),
    "CALC": thing(calculator), "LOOK": thing(look), "WRITE": thing(text), "PAINT": thing(paint_icon),
    "EDIT": thing(edit), "PLAY": thing(media, .3), "COMPRESS": thing(archive), "FTP": thing(ftp),
    "PUTTY": thing(putty, .3), "DRAWER": empty_drawer, "MONITOR": thing(monitor), "PREFS": thing(cog_obj),
    "INSTALL": thing(install), "TRASHCAN": thing(trashcan),
    # the drives, each its own picture
    "SYSTEM": held(boing_obj, .62), "WORK": held(doc_obj, .64), "GAMES": held(pad_icon, .74),
    "RAM": held(ram_obj, .78), "USB": held(usb_obj, .78), "IDE": held(ide_drive, .74),
    "HARDDISK": held(acfs_drive, .74),
    "CD": held(cd_obj, .80), "FLOPPY": held(floppy_obj, .72), "NETWORK": held(net_obj, .76),
}

# icons/open NAME: picture (the pack's own programs; open_icons() in build_pack.py gives each its icon)
OPEN = {
    "GFOpenFiles": "FILES", "GFOpenView": "VIEW", "GFOpenWrite": "WRITE", "GFOpenEdit": "EDIT", "GFOpenPlay": "PLAY",
    "Install-GFOpenUp": "INSTALL", "GFOpenMonitor": "MONITOR", "GFOpenPrefs": "PREFS", "GFOpenDrawer": "DRAWER",
    "GFOpenCompress": "COMPRESS", "Look": "LOOK", "GFOpenCalc": "CALC", "GFOpenFTP": "FTP", "GFOpenPuTTY": "PUTTY",
}
SIZE = 75                   # 64 at the pack's ICON_SCALE


def frames(name: str, size: int) -> list[Image.Image]:
    """NAME's picture at rest and chosen, size x size, in true colour with soft edges (the caller makes the
    GlowIcon: glowicon() here, or the pack's own)."""
    fn = PICTURES[name]
    return [fn(c).resize((size, size), Image.LANCZOS) for c in (False, True)]


def glowicon(im: Image.Image) -> Image.Image:
    """IM as a GlowIcon holds it: every pixel solid (alpha 110 and up) or see-through, at most 255 colours"""
    a = np.array(im.getchannel("A")) >= 110
    q = im.convert("RGB").quantize(colors=255, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).convert("RGB")
    out = q.convert("RGBA")
    out.putalpha(Image.fromarray((a * 255).astype(np.uint8)))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    a = ap.parse_args(argv)
    a.out.mkdir(parents=True, exist_ok=True)
    for name, pic in OPEN.items():
        rest, chosen = frames(pic, SIZE)
        rest.save(a.out / f"{name}.png", optimize=True)
        chosen.save(a.out / f"{name}-selected.png", optimize=True)
    print(f"{len(OPEN)} icons in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
