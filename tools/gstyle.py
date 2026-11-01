#!/usr/bin/env python3
"""A small drawing kit for icons in the Gorilla style: bold black outlines,
soft top-left light, a gentle gloss and a soft shadow down and to the right.
Shapes are drawn in a 64-unit frame, four times over, and reduced.
MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops

S = 4            # pixels per unit while drawing
PAD = 16         # units of room round the 64-unit frame (a drawer is 82 wide)
N = 64
W = (N + 2 * PAD) * S

INK = (16, 18, 22)
SHADOW = (24, 32, 44)

def P(v):
    return (v + PAD) * S

def rgb(h):
    h = h.lstrip('#')
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

def mix(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))

def lighter(c, t=0.35):
    return mix(c, (255, 255, 255), t)

def darker(c, t=0.3):
    return mix(c, (0, 0, 0), t)

# ---- masks ---------------------------------------------------------------------------------

def blank():
    return Image.new('L', (W, W), 0)

def poly(pts):
    m = blank(); ImageDraw.Draw(m).polygon([(P(x), P(y)) for x, y in pts], fill=255); return m

def rrect(x0, y0, x1, y1, r):
    m = blank(); ImageDraw.Draw(m).rounded_rectangle([P(x0), P(y0), P(x1), P(y1)], radius=r * S, fill=255); return m

def rect(x0, y0, x1, y1):
    m = blank(); ImageDraw.Draw(m).rectangle([P(x0), P(y0), P(x1), P(y1)], fill=255); return m

def ellipse(x0, y0, x1, y1):
    m = blank(); ImageDraw.Draw(m).ellipse([P(x0), P(y0), P(x1), P(y1)], fill=255); return m

def ring(x0, y0, x1, y1, w):
    m = blank(); ImageDraw.Draw(m).ellipse([P(x0), P(y0), P(x1), P(y1)], outline=255, width=max(1, round(w * S))); return m

def stroke(pts, w):
    m = blank(); d = ImageDraw.Draw(m)
    q = [(P(x), P(y)) for x, y in pts]
    d.line(q, fill=255, width=max(1, round(w * S)), joint='curve')
    r = w * S / 2
    for x, y in (q[0], q[-1]):
        d.ellipse([x - r, y - r, x + r, y + r], fill=255)
    return m

def arc(x0, y0, x1, y1, a0, a1, w):
    m = blank(); ImageDraw.Draw(m).arc([P(x0), P(y0), P(x1), P(y1)], a0, a1, fill=255, width=max(1, round(w * S))); return m

def union(*ms):
    out = blank()
    for m in ms: out = ImageChops.lighter(out, m)
    return out

def cut(a, b):
    return ImageChops.subtract(a, b)

def inter(a, b):
    return ImageChops.multiply(a, b)

def dilate(m, w):
    k = 2 * max(1, round(w * S)) + 1
    return m.filter(ImageFilter.MaxFilter(k))

# ---- fills ---------------------------------------------------------------------------------

_yy, _xx = np.mgrid[0:W, 0:W].astype(np.float32)

def linear(p0, p1, c0, c1, mid=None):
    """A gradient from c0 at p0 to c1 at p1 (units); mid=(t, colour) adds a stop."""
    x0, y0 = P(p0[0]), P(p0[1]); x1, y1 = P(p1[0]), P(p1[1])
    dx, dy = x1 - x0, y1 - y0; L = dx * dx + dy * dy or 1
    t = np.clip(((_xx - x0) * dx + (_yy - y0) * dy) / L, 0, 1)
    a = np.array(c0, np.float32); b = np.array(c1, np.float32)
    if mid:
        tm, cm = mid; cm = np.array(cm, np.float32)
        lo = t < tm
        t1 = np.where(lo, t / tm, (t - tm) / (1 - tm))[..., None]
        img = np.where(lo[..., None], a + (cm - a) * t1, cm + (b - cm) * t1)
    else:
        img = a + (b - a) * t[..., None]
    return Image.fromarray(img.astype(np.uint8), 'RGB')

def radial(c, r, c0, c1):
    cx, cy = P(c[0]), P(c[1]); R = r * S
    t = np.clip(np.hypot(_xx - cx, _yy - cy) / R, 0, 1)[..., None]
    a = np.array(c0, np.float32); b = np.array(c1, np.float32)
    return Image.fromarray((a + (b - a) * t).astype(np.uint8), 'RGB')

def solid(c):
    return Image.new('RGB', (W, W), c)

# ---- the icon ------------------------------------------------------------------------------

class Icon:
    def __init__(self):
        self.parts = []      # (mask, fill, outline width, gloss, outline colour)

    def add(self, mask, fill, outline=2.3, gloss=0.0, ink=INK):
        if isinstance(fill, tuple): fill = solid(fill)
        self.parts.append((mask, fill, outline, gloss, ink))
        return mask

    def detail(self, mask, fill):
        """A flat detail with no outline (labels, lines, highlights)."""
        return self.add(mask, fill, outline=0)

    def render(self, shadow=True):
        out = Image.new('RGBA', (W, W), (0, 0, 0, 0))
        if shadow:
            sh = blank()
            for m, f, ow, g, ink in self.parts:
                sh = ImageChops.lighter(sh, dilate(m, ow) if ow else m)
            off = (round(1.8 * S), round(2.6 * S))
            sh = ImageChops.offset(sh, *off).filter(ImageFilter.GaussianBlur(2.4 * S))
            sh = sh.point(lambda v: int(v * 0.42))
            layer = Image.new('RGBA', (W, W), SHADOW + (0,)); layer.putalpha(sh)
            out.alpha_composite(layer)
        for m, f, ow, g, ink in self.parts:
            if ow:
                o = Image.new('RGBA', (W, W), ink + (0,)); o.putalpha(dilate(m, ow)); out.alpha_composite(o)
            fl = f.convert('RGBA'); fl.putalpha(m); out.alpha_composite(fl)
            if g:
                bb = m.getbbox()
                if bb:
                    y0, y1 = bb[1], bb[3]
                    t = np.clip(1 - (_yy - y0) / max(1, (y1 - y0) * 0.55), 0, 1) * g
                    a = (np.asarray(m, np.float32) / 255 * t * 255).astype(np.uint8)
                    gl = Image.new('RGBA', (W, W), (255, 255, 255, 0)); gl.putalpha(Image.fromarray(a, 'L'))
                    out.alpha_composite(gl)
        return out

def finish(frames, size=64, margin=1):
    """Crop every frame to the box they all fill, and reduce them alike."""
    box = None
    for f in frames:
        b = f.getchannel('A').point(lambda v: 255 if v > 8 else 0).getbbox()
        box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3]))
    x0, y0, x1, y1 = box
    side = max(x1 - x0, y1 - y0) + 2 * margin * S
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    crop = (round(cx - side / 2), round(cy - side / 2), round(cx + side / 2), round(cy + side / 2))
    return [f.crop(crop).resize((size, size), Image.LANCZOS) for f in frames]

# ---- shared pieces in the style --------------------------------------------------------------

GREY_HI, GREY_LO = rgb('#FAFAFA'), rgb('#BDBFC3')
BLUE, BLUE_LO = rgb('#5C8FD6'), rgb('#2E5C9E')
GREEN, GREEN_LO = rgb('#5DBB4C'), rgb('#2E7D2A')
ORANGE, ORANGE_LO = rgb('#F4A24A'), rgb('#C2611E')
RED, RED_LO = rgb('#E2564B'), rgb('#9C2A22')
GOLD, GOLD_LO = rgb('#F6D04D'), rgb('#C08A12')
DARK, DARK_LO = rgb('#5A616B'), rgb('#262A30')
PAPER = rgb('#F4F5F6')
CONTENT = rgb('#9A9DA3')

def page(ic, x0, y0, x1, y1, holes=True):
    """Gorilla's document: a rounded white sheet, binder holes at the left."""
    m = ic.add(rrect(x0, y0, x1, y1, 3.2), linear((x0, y0), (x1, y1), GREY_HI, rgb('#D4D6DA')), gloss=0.15)
    if holes:
        for hy in (y0 + (y1 - y0) * 0.28, y0 + (y1 - y0) * 0.72):
            ic.detail(ellipse(x0 + 2.0, hy - 0.8, x0 + 3.6, hy + 0.8), rgb('#7C8088'))
    return m

def prefs_badge(ic, x, y, s=1.0):
    """Our prefs mark: a small blue monitor with a question mark, bottom left."""
    w, h = 15 * s, 11 * s
    ic.add(rrect(x, y + h - 1 * s, x + w, y + h + 3.6 * s, 1.2 * s), linear((x, y + h), (x, y + h + 4 * s), rgb('#DDE4EE'), rgb('#8EA1BC')))
    ic.add(rrect(x + 1 * s, y, x + w - 1 * s, y + h, 1.6 * s), linear((x, y), (x + w, y + h), rgb('#7FB0F0'), BLUE_LO), gloss=0.35)
    q = union(arc(x + 5.2 * s, y + 2.0 * s, x + 9.8 * s, y + 6.4 * s, 190, 90, 1.5 * s),
              stroke([(x + 7.5 * s, y + 6.4 * s), (x + 7.5 * s, y + 7.4 * s)], 1.5 * s),
              ellipse(x + 6.7 * s, y + 8.2 * s, x + 8.3 * s, y + 9.8 * s))
    ic.detail(q, (255, 255, 255))

def magnifier(ic, cx, cy, r, angle=45, handle=10):
    a = math.radians(angle)
    hx0, hy0 = cx + math.cos(a) * r * 0.95, cy + math.sin(a) * r * 0.95
    hx1, hy1 = cx + math.cos(a) * (r + handle), cy + math.sin(a) * (r + handle)
    ic.add(stroke([(hx0, hy0), (hx1, hy1)], 3.6), linear((hx0, hy0), (hx1, hy1), rgb('#6C8C5A'), rgb('#2F4A28')))
    ic.add(ellipse(cx - r, cy - r, cx + r, cy + r), linear((cx - r, cy - r), (cx + r, cy + r), rgb('#E9EEF5'), rgb('#7D8796')))
    ic.add(ellipse(cx - r + 2.2, cy - r + 2.2, cx + r - 2.2, cy + r - 2.2), radial((cx - r * 0.35, cy - r * 0.35), r * 1.6, rgb('#FFFFFF'), rgb('#9EB6D8')), outline=0.8)
    ic.detail(ellipse(cx - r * 0.55, cy - r * 0.6, cx - r * 0.05, cy - r * 0.2), (255, 255, 255))
