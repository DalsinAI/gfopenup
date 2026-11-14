#!/usr/bin/env python3
"""The drive icons by kind in the Soft 3D style (Dale, 14 November 2026, 12:33: "b soft 3d";
amicradle-teams teams/DECISION-ICONS-SOFT3D-LOGO-CRADLE-20261114-1233.md).

    python3 tools/soft3d_drives_art.py OUT_DIR             KIND-64|32-normal|selected.png, KIND-32-chipset-...png
    python3 tools/soft3d_drives_art.py --sheet OUT.png     Dale's sheet: all nine on a light and a dark desktop

Our own drawing. Each drive is an object drawn flat, seen from above, then tilted towards the viewer (a
perspective that narrows it at the back), given its thickness, lit from the top left with soft gradients, edged
with a thin dark line (so silver holds on a light desktop) and set on a soft shadow. Chosen, it lifts 2 px with a
blue glow (#4aa3ff) and a longer shadow.

Sizes (Team 5, Ivanova, 14 Nov 13:01: 64 px for drives and drawers until Dale says otherwise):
- 64 x 64, drawn at 512 and cut down.
- 32 x 32, its own drawing: the same objects with the fine detail left out and what tells them apart made
  bigger (fewer, larger features), drawn at 256, cut down and given hard edges and the dark edge pixel by pixel.
- the chipset set: the 32 px drawing in one 16-colour palette (PAL15 and transparent), shared by every drive,
  so a chipset screen with few pens shows all nine with the same 16.

NVMe and RAM are told apart by shape first: NVMe is a short black M.2 card with a silver label and its gold
contacts at one short end; RAM is a long green stick with a row of black chips and gold contacts along its whole
long edge.

MIT, Copyright (c) 2026 AmiCradle Team.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter

KINDS = ["system", "harddisk", "nvme", "ide", "usb", "cd", "floppy", "ram", "network"]
LABELS = {"system": "Boot", "harddisk": "Hard disk", "nvme": "NVMe", "ide": "IDE drive", "usb": "USB stick",
          "cd": "CD/DVD", "floppy": "Floppy", "ram": "RAM disk", "network": "Network"}
GLOW = (74, 163, 255)                   # #4aa3ff, the chosen glow
EDGE = (22, 26, 36)                     # the thin dark edge
LOGO_ARC, LOGO_DOT = (13, 148, 136), (245, 158, 11)    # the Cradle mark's colours (logo concept 3)

# the chipset set: one 16-colour palette for every drive (transparent, then these 15): a grey ramp of five for the
# bodies, the edge and the light, and the accents, two shades where a drive's light runs across one
PAL15 = [(22, 26, 36), (70, 76, 90), (130, 138, 152), (196, 202, 212), (246, 248, 251),
         (40, 64, 120), (60, 120, 220), (74, 163, 255), (214, 92, 36), (246, 150, 84), (200, 56, 52),
         (226, 182, 72), (30, 120, 70), (13, 118, 110), (52, 170, 156)]


# ---- drawing on a flat square ------------------------------------------------------------------------

class Flat:
    """an object's top, drawn flat on an S x S canvas (S = 512 for 64 px, 256 for 32 px)"""

    def __init__(self, S):
        self.S = S
        self.img = Image.new("RGBA", (S, S))

    def k(self, v):
        return v * self.S / 512

    def box(self, b):
        return [self.k(v) for v in b]

    def mask(self, kind, b, r=0, width=0):
        m = Image.new("L", (self.S, self.S), 0)
        d = ImageDraw.Draw(m)
        b = self.box(b)
        if kind == "rr":
            d.rounded_rectangle(b, self.k(r), fill=255)
        elif kind == "ell":
            d.ellipse(b, fill=255)
        elif kind == "ring":
            d.ellipse(b, outline=255, width=max(1, int(self.k(width))))
        elif kind == "poly":
            d.polygon(b, fill=255)
        elif kind == "line":
            d.line(b, fill=255, width=max(1, int(self.k(width))))
        return m

    def fill(self, col, m, top=None):
        """col over the mask; with top, a vertical gradient from top (the light) to col"""
        if top is None:
            f = Image.new("RGBA", (self.S, self.S), tuple(col) + (255,))
        else:
            t = np.linspace(0, 1, self.S)[:, None, None]
            a, b = np.array(top, np.float32), np.array(col, np.float32)
            arr = np.repeat((a + (b - a) * t), self.S, axis=1)
            f = Image.fromarray(arr.astype(np.uint8), "RGB").convert("RGBA")
        f.putalpha(m)
        self.img = Image.alpha_composite(self.img, f)

    def rr(self, b, r, col, top=None):
        self.fill(col, self.mask("rr", b, r), top)

    def ell(self, b, col, top=None):
        self.fill(col, self.mask("ell", b), top)

    def ring(self, b, w, col):
        self.fill(col, self.mask("ring", b, width=w))

    def poly(self, pts, col):
        self.fill(col, self.mask("poly", [p for xy in pts for p in xy]))

    def line(self, pts, w, col):
        self.fill(col, self.mask("line", [p for xy in pts for p in xy], width=w))


def light(c, k):
    return tuple(min(255, int(v + (255 - v) * k)) for v in c)


def dark(c, k):
    return tuple(int(v * (1 - k)) for v in c)


def cradle_mark(f, cx, cy, r, small):
    """the Cradle mark (logo concept 3): an open arc holding a dot"""
    w = 34 if small else 22
    m = Image.new("L", (f.S, f.S), 0)
    d = ImageDraw.Draw(m)
    b = f.box([cx - r, cy - r, cx + r, cy + r])
    d.arc(b, 20, 160, fill=255, width=max(2, int(f.k(w))))
    f.fill(LOGO_ARC, m)
    dr = r * 0.36
    f.ell([cx - dr, cy - dr + r * 0.28, cx + dr, cy + dr + r * 0.28], LOGO_DOT, light(LOGO_DOT, 0.35))


# ---- the nine drives, flat. small: the 32 px drawing (fewer, bigger features). Returns (Flat, thickness) --

def hd_body(f, col=(196, 202, 212), small=False):
    f.rr([70, 40, 442, 472], 34, col, light(col, 0.45))
    if not small:
        for x, y in ((100, 70), (412, 70), (100, 442), (412, 442)):        # the screws
            f.ell([x - 12, y - 12, x + 12, y + 12], dark(col, 0.25), light(col, 0.2))
    return f


def draw_system(f, small):
    hd_body(f, small=small)
    cradle_mark(f, 256, 200, 150 if small else 120, small)
    return 70


def draw_harddisk(f, small):
    col = (196, 202, 212)
    hd_body(f, col, small)
    f.rr([110, 90, 402, 260] if not small else [100, 80, 412, 240], 18, (238, 241, 246), (255, 255, 255))   # label
    if not small:
        for i, y in enumerate((125, 165, 205)):
            f.rr([140, y, 360 - 50 * i, y + 16], 8, (150, 160, 176))
        f.ell([196, 300, 316, 420], dark(col, 0.12), light(col, 0.25))  # the spindle's dome
        f.ell([236, 340, 276, 380], dark(col, 0.35))
    else:
        f.ell([176, 290, 336, 450], dark(col, 0.2), light(col, 0.15))
    return 70


def draw_nvme(f, small):
    pcb = (34, 38, 46)
    x0, x1 = (150, 362)
    f.rr([x0, 30, x1, 482], 14, pcb, (64, 70, 82))
    # the screw notch's half-moon at the far end, the gold contacts at the near end, their key gap
    m = f.mask("ell", [234, 12, 278, 56])
    f.img.putalpha(ImageChops.subtract(f.img.getchannel("A"), m))
    gold = (226, 182, 72)
    if small:
        f.rr([x0 + 14, 420, x1 - 14, 476], 4, gold, light(gold, 0.4))
    else:
        for i in range(12):
            x = x0 + 16 + i * 16
            if 6 <= i <= 7:
                continue
            f.rr([x, 430, x + 10, 476], 3, gold, light(gold, 0.4))
    # the silver label over most of the card, a blue stripe on it
    f.rr([x0 + 14, 70, x1 - 14, 400], 10, (214, 220, 230), (250, 252, 255))
    f.rr([x0 + 14, 120, x1 - 14, 170 if not small else 190], 0, (60, 120, 220))
    if not small:
        for y in (220, 255, 290):
            f.rr([x0 + 40, y, x1 - 60, y + 14], 7, (150, 158, 172))
    return 22


def draw_ide(f, small):
    body = (70, 76, 90)
    f.rr([100, 80, 412, 472], 26, body, light(body, 0.3))
    f.rr([130, 110, 382, 330] if not small else [124, 104, 388, 320], 14, (232, 112, 42), light((232, 112, 42), 0.3))
    if not small:
        for i, y in enumerate((150, 190, 230)):
            f.rr([160, y, 330 - 40 * i, y + 16], 8, light((232, 112, 42), 0.5))
    # the 44-pin connector along the near edge
    f.rr([126, 400, 386, 452], 6, (30, 32, 38))
    pins = 10 if small else 20
    for i in range(pins):
        x = 138 + i * (236 / pins)
        f.rr([x, 414, x + (236 / pins) * 0.5, 440], 2, (226, 182, 72))
    return 46


def draw_usb(f, small):
    body = (200, 56, 52)
    f.rr([170, 40, 342, 340], 46, body, light(body, 0.35))
    m = f.mask("ell", [236, 72, 276, 112])
    f.img.putalpha(ImageChops.subtract(f.img.getchannel("A"), m))
    metal = (196, 202, 212)
    f.rr([196, 320, 316, 482], 8, metal, light(metal, 0.5))
    f.rr([218, 372, 248, 400], 2, (40, 44, 52))
    f.rr([264, 372, 294, 400], 2, (40, 44, 52))
    if not small:
        f.rr([200, 180, 312, 200], 8, light(body, 0.4))
    return 48


def draw_cd(f, small):
    f.ell([40, 40, 472, 472], (200, 206, 218), (244, 246, 250))
    # the rainbow sheen: a few soft coloured wedges
    sheen = Image.new("RGBA", (f.S, f.S))
    d = ImageDraw.Draw(sheen)
    b = f.box([40, 40, 472, 472])
    for a0, col in ((200, (255, 120, 160)), (230, (255, 220, 110)), (260, (120, 230, 200)), (290, (120, 160, 255))):
        d.pieslice(b, a0, a0 + 30, fill=col + (110,))
        d.pieslice(b, a0 + 180, a0 + 210, fill=col + (80,))
    sheen = sheen.filter(ImageFilter.GaussianBlur(f.k(18)))
    sheen.putalpha(ImageChops.multiply(sheen.getchannel("A"), f.mask("ell", [40, 40, 472, 472])))
    f.img = Image.alpha_composite(f.img, sheen)
    f.ell([196, 196, 316, 316], (170, 176, 190))           # the clamp ring
    hole = f.mask("ell", [234, 234, 278, 278] if not small else [226, 226, 286, 286])
    f.img.putalpha(ImageChops.subtract(f.img.getchannel("A"), hole))
    if not small:
        f.ring([150, 150, 362, 362], 3, (220, 224, 232))
    return 10


def draw_floppy(f, small):
    body = (40, 64, 120)
    f.rr([60, 50, 452, 462], 16, body, light(body, 0.3))
    m = f.mask("poly", [452, 50, 452, 90, 412, 50])         # the cut corner
    f.img.putalpha(ImageChops.subtract(f.img.getchannel("A"), m))
    metal = (196, 202, 212)
    f.rr([150, 50, 362, 190], 6, metal, light(metal, 0.5))  # the shutter
    f.rr([280, 74, 330, 166], 4, (40, 44, 52))
    f.rr([110, 250, 402, 462], 10, (240, 242, 246), (255, 255, 255))   # the label
    if not small:
        for i, y in enumerate((300, 340, 380)):
            f.rr([140, y, 360 - 40 * i, y + 12], 6, (160, 168, 184))
    return 26


def draw_ram(f, small):
    pcb = (30, 120, 70)
    f.rr([30, 150, 482, 362], 10, pcb, light(pcb, 0.3))
    # the key notch in the middle of the contact edge
    m = f.mask("rr", [236, 330, 262, 370], 6)
    f.img.putalpha(ImageChops.subtract(f.img.getchannel("A"), m))
    n = 4 if small else 8
    w = 420 / n
    for i in range(n):
        x = 46 + i * w
        f.rr([x + 4, 176, x + w - 8, 290], 6, (34, 36, 42), (70, 74, 84))
    gold = (226, 182, 72)
    if small:
        f.rr([46, 312, 230, 352], 3, gold, light(gold, 0.4))
        f.rr([270, 312, 466, 352], 3, gold, light(gold, 0.4))
    else:
        for i in range(40):
            x = 46 + i * 10.5
            if 228 < x < 268:
                continue
            f.rr([x, 314, x + 6, 352], 2, gold, light(gold, 0.4))
    return 18


def draw_network(f, small):
    body = (13, 120, 112)
    f.rr([60, 70, 452, 452], 36, body, light(body, 0.35))
    # three linked nodes on the top
    pts = [(256, 150), (150, 340), (362, 340)]
    for a, b in ((0, 1), (0, 2), (1, 2)):
        f.line([pts[a], pts[b]], 30 if small else 18, light(body, 0.75))
    r = 52 if small else 40
    for x, y in pts:
        f.ell([x - r, y - r, x + r, y + r], (245, 246, 250), (255, 255, 255))
    return 64


DRAW = {"system": draw_system, "harddisk": draw_harddisk, "nvme": draw_nvme, "ide": draw_ide, "usb": draw_usb,
        "cd": draw_cd, "floppy": draw_floppy, "ram": draw_ram, "network": draw_network}


# ---- tilt, thickness, light, edge, shadow ------------------------------------------------------------

def perspective_coeffs(src, dst):
    """the 8 coefficients that map dst quad back to src (PIL's PERSPECTIVE takes the inverse)"""
    rows = []
    for (x, y), (u, v) in zip(dst, src):
        rows.append([x, y, 1, 0, 0, 0, -u * x, -u * y])
        rows.append([0, 0, 0, x, y, 1, -v * x, -v * y])
    A = np.array(rows, np.float64)
    bvec = np.array([c for p in src for c in p], np.float64)
    return np.linalg.solve(A, bvec).tolist()


def tilt(img, S):
    """the flat top seen from the front and above: squashed to about 60 % of its depth, narrower at the back"""
    src = [(0, 0), (S, 0), (S, S), (0, S)]
    k = S / 512
    dst = [(52 * k, 96 * k), (460 * k, 96 * k), (500 * k, 410 * k), (12 * k, 410 * k)]
    return img.transform((S, S), Image.PERSPECTIVE, perspective_coeffs(src, dst), Image.BICUBIC)


def solid(flat, thick, S, chosen):
    top = tilt(flat, S)
    th = max(1, int(thick * S / 512 * 0.8))
    m = top.getchannel("A")
    out = Image.new("RGBA", (S, S))
    # the shadow: soft, below and to the right; chosen, longer
    off = int((th + (26 if chosen else 14)) * S / 512) + 2
    sh = Image.new("RGBA", (S, S), (16, 20, 32, 0))
    sh.putalpha(ImageChops.offset(m, int(8 * S / 512), off).filter(ImageFilter.GaussianBlur(14 * S / 512))
                .point(lambda v: int(v * (0.30 if chosen else 0.38))))
    out = Image.alpha_composite(out, sh)
    # the thickness: the top's own colours, darker, stacked below it
    side = Image.eval(top.convert("RGB"), lambda v: int(v * 0.55)).convert("RGBA")
    side.putalpha(m)
    for i in range(th, 0, -1):
        out = Image.alpha_composite(out, ImageChops.offset(side, 0, i))
    whole = add(m, *[ImageChops.offset(m, 0, i) for i in range(1, th + 1)])
    # the light: from the top left, on the top face
    yy, xx = np.mgrid[0:S, 0:S] / S
    shade = np.clip(0.18 - 0.36 * (xx * 0.45 + yy * 0.55), -0.14, 0.18)
    arr = np.array(top, np.float32)
    rgb = arr[..., :3]
    rgb = np.where(shade[..., None] > 0, rgb + (255 - rgb) * shade[..., None], rgb * (1 + shade[..., None]))
    arr[..., :3] = np.clip(rgb, 0, 255)
    out = Image.alpha_composite(out, Image.fromarray(arr.astype(np.uint8), "RGBA"))
    # the near edge's highlight, where the top meets the thickness
    lip = ImageChops.subtract(m, ImageChops.offset(m, 0, -max(1, int(3 * S / 512))))
    out = paint(out, (255, 255, 255), lip.point(lambda v: v // 3))
    # the thin dark edge round the whole object
    w = max(1, int(round(5 * S / 512)))
    ring = ImageChops.subtract(whole.filter(ImageFilter.MaxFilter(2 * w + 1)), whole)
    out = paint(out, EDGE, ring.point(lambda v: int(v * 0.85)))
    if chosen:
        glow = Image.new("RGBA", (S, S), GLOW + (0,))
        g = whole.filter(ImageFilter.MaxFilter(2 * w + 1)).filter(ImageFilter.GaussianBlur(14 * S / 512))
        glow.putalpha(ImageChops.subtract(g, whole).point(lambda v: min(255, int(v * 1.6))))
        out = Image.alpha_composite(glow, out)
    return out


def add(*ms):
    o = ms[0]
    for m in ms[1:]:
        o = ImageChops.lighter(o, m)
    return o


def paint(img, col, m):
    f = Image.new("RGBA", img.size, tuple(col) + (255,))
    f.putalpha(m)
    return Image.alpha_composite(img, f)


def render(kind, S, chosen, small):
    f = Flat(S)
    thick = DRAW[kind](f, small)
    im = solid(f.img, thick, S, chosen)
    if chosen:                                          # lifted 2 px at 64 (16 of 512)
        up = Image.new("RGBA", (S, S))
        up.alpha_composite(im, (0, 0), (0, int(16 * S / 512)))
        im = up
    return im


def hard(im, n):
    """n x n with every pixel solid or see-through (as a GlowIcon holds it); the dark edge kept a whole pixel"""
    small = im.resize((n, n), Image.LANCZOS)
    a = np.array(small)
    solid_px = a[..., 3] >= 110
    a[..., 3] = np.where(solid_px, 255, 0)
    return Image.fromarray(a, "RGBA")


_FRAMES = {}


def fitted(kind, S, small):
    """KIND at rest and chosen, drawn at S, both cut to one square round what they cover (with a 3 % margin), so
    every drive fills its icon and the chosen one moves against the same frame"""
    key = (kind, S, small)
    if key not in _FRAMES:
        pair = [render(kind, S, c, small) for c in (False, True)]
        m = ImageChops.lighter(pair[0].getchannel("A"), pair[1].getchannel("A")).point(lambda v: 255 if v >= 60 else 0)
        x0, y0, x1, y1 = m.getbbox()
        side = int(max(x1 - x0, y1 - y0) * 1.06)
        cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
        box = (cx - side // 2, cy - side // 2, cx - side // 2 + side, cy - side // 2 + side)
        _FRAMES[key] = [p.crop(box) for p in pair]
    return _FRAMES[key]


def frames(kind, size, chosen):
    """KIND at rest or chosen, size x size, hard alpha. 48 and up: drawn at 512. Smaller: its own drawing."""
    big = size >= 48
    im = hard(fitted(kind, 512 if big else 256, not big)[1 if chosen else 0], size)
    return im if big else edged(im)


def edged(im):
    """the small drawing's dark edge, pixel by pixel: every object pixel beside a see-through or glow pixel takes
    the edge colour (mostly), so silver keeps its outline on a light desktop at 1x"""
    a = np.array(im).astype(np.int32)
    on = a[..., 3] > 0
    glow = on & (a[..., 2] > 200) & (a[..., 0] < 140) & (a[..., 1] > 120)
    body = on & ~glow
    out = ~body
    nb = np.zeros_like(on)
    nb[1:] |= out[:-1]; nb[:-1] |= out[1:]; nb[:, 1:] |= out[:, :-1]; nb[:, :-1] |= out[:, 1:]
    rim = body & nb
    edge = np.array(EDGE, np.int32)
    a[rim, :3] = (a[rim, :3] * 3 + edge * 7) // 10
    return Image.fromarray(a.astype(np.uint8), "RGBA")


def yuv(rgb):
    """brightness and the two colour differences, so the nearest colour keeps a pixel's hue (weighted 3 to 1)"""
    r, g, b = (rgb[..., i].astype(np.float64) for i in range(3))
    y = 0.299 * r + 0.587 * g + 0.114 * b
    return np.stack([y, 0.564 * (b - y), 0.713 * (r - y)], -1)


def chipset(im):
    """the 32 px picture in the shared 16-colour palette (PAL15 and transparent), nearest colour, no dither"""
    a = np.array(im).astype(np.int32)
    pal = np.array(PAL15, np.int32)
    d = ((yuv(a[..., :3])[..., None, :] - yuv(pal)[None, None]) ** 2 * np.array([1.0, 3.0, 3.0])).sum(-1)
    out = pal[d.argmin(-1)]
    alpha = (a[..., 3] > 0) * 255
    return Image.fromarray(np.dstack([out, alpha]).astype(np.uint8), "RGBA")


def colours(im):
    a = np.array(im)
    return {tuple(p[:3]) for p in a.reshape(-1, 4) if p[3]}


def font(px):
    from PIL import ImageFont
    p = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    return ImageFont.truetype(p, px) if Path(p).exists() else ImageFont.load_default()


def blow(im, k):
    a = np.array(im)
    return Image.fromarray(np.repeat(np.repeat(a, k, 0), k, 1), "RGBA")


SHEET_ROWS = [("64 px", 64, "normal", False, 1), ("64 px chosen", 64, "selected", False, 1),
              ("32 px", 32, "normal", False, 1), ("32 px chosen", 32, "selected", False, 1),
              ("32 px, 16 colours", 32, "normal", True, 1), ("64 px at 2x", 64, "normal", False, 2),
              ("32 px at 3x", 32, "normal", False, 3), ("16 colours at 3x", 32, "normal", True, 3)]


def sheet(out):
    """all nine drives on GFOpenBench's light desktop and on a dark one, each row a size or state"""
    cell, left, top = 150, 170, 40
    hs = [max(n * k, 40) + 18 for _, n, _, _, k in SHEET_ROWS]
    h1 = top + sum(hs)
    im = Image.new("RGBA", (left + cell * len(KINDS) + 20, 2 * h1 + 20), (255, 255, 255, 255))
    d = ImageDraw.Draw(im)
    f, fb = font(13), font(15)
    for half, (bg, ink) in enumerate((((232, 234, 238), (30, 34, 44)), ((43, 47, 54), (230, 232, 236)))):
        y0 = half * (h1 + 20)
        d.rectangle([0, y0, im.width, y0 + h1], fill=bg)
        for i, k in enumerate(KINDS):
            x = left + i * cell + cell // 2
            d.text((x - d.textlength(LABELS[k], font=fb) / 2, y0 + 12), LABELS[k], font=fb, fill=ink)
        y = y0 + top
        for (name, n, state, chip, k), h in zip(SHEET_ROWS, hs):
            d.text((16, y + h // 2 - 8), name, font=f, fill=ink)
            for i, kind in enumerate(KINDS):
                p = frames(kind, n, state == "selected")
                if chip:
                    p = chipset(p)
                p = blow(p, k)
                im.alpha_composite(p, (left + i * cell + (cell - p.width) // 2, y + (h - p.height) // 2))
            y += h
    im.convert("RGB").save(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out", type=Path)
    ap.add_argument("--sheet", action="store_true", help="write Dale's sheet to OUT instead")
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
                    chipset(im).save(a.out / f"{kind}-32-chipset-{state}.png")
    print(f"{len(KINDS) * 6} pictures in {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
