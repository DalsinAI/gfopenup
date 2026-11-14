#!/usr/bin/env python3
"""drive_icons_sheet: the drive icons' specimen sheet for Dale, before they ship.

    python3 tools/drive_icons_sheet.py OUT.png [BACKDROP.png]

Every drive picture (tools/aos_icons_art.py) as the pack ships it (a GlowIcon: 255 colours, hard alpha) on the
Workbench backdrop: at 1x (64 x 64) and at 2x, then at the size each display profile would give it (amicradle-teams
designs/DISPLAY-PROFILES-20261113.md; the chipset hires profile drawn for its tall pixels and shown doubled down).
MIT, AmiCradle Team."""
import os, sys
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import aos_icons_art as art

DRIVES = [("SYSTEM", "System"), ("RAM", "RAM Disk"), ("WORK", "Work"), ("GAMES", "Games"),
          ("HARDDISK", "Hard disk (ACFS)"), ("IDE", "IDE partition"), ("USB", "USB stick"), ("CD", "CD"),
          ("FLOPPY", "Floppy"), ("NETWORK", "Network share")]
# the icon size each profile gives a drive: (profile, width, height) -- a proposal for Dale
PROFILES = [("1 chipset lores", 32, 32), ("2 chipset hires 1:2", 64, 32), ("3 chipset laced", 48, 48),
            ("4 RTG up to 1024", 48, 48), ("5 RTG 1080p", 64, 64), ("6 RTG large", 64, 64)]
ICON = 64


def font(px):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",):
        if os.path.exists(p):
            return ImageFont.truetype(p, px)
    return ImageFont.load_default()


def shipped(name, w, h):
    """NAME at rest as the pack ships it, w x h"""
    return art.glowicon(art.frames(name, 512)[0].resize((w, h), Image.LANCZOS))


def label(d, xy, text, f, centre=True):
    x, y = xy
    if centre:
        x -= d.textlength(text, font=f) / 2
    d.text((x + 1, y + 1), text, font=f, fill=(0, 0, 0, 200))
    d.text((x, y), text, font=f, fill=(255, 255, 255, 255))


def sheet(out, backdrop=None):
    col = 150
    W = 40 + col * len(DRIVES)
    H = 1060
    if backdrop and os.path.exists(backdrop):
        bd = Image.open(backdrop).convert("RGBA")
        k = max(W / bd.width, H / bd.height)                # cover the sheet, cropped, never stretched
        im = bd.resize((int(bd.width * k + 1), int(bd.height * k + 1)), Image.LANCZOS).crop((0, 0, W, H))
    else:
        im = Image.new("RGBA", (W, H), (64, 84, 120, 255))
    d = ImageDraw.Draw(im)
    f13, f16, f20 = font(13), font(16), font(20)
    label(d, (20, 14), "GFOS drive icons, AOS style: as shipped (GlowIcon, 255 colours), at rest", f20, False)
    y = 60
    label(d, (20, y), "1x, 64 x 64", f16, False)
    for i, (n, t) in enumerate(DRIVES):
        x = 20 + i * col + (col - ICON) // 2
        im.alpha_composite(shipped(n, ICON, ICON), (x, y + 30))
        label(d, (x + ICON // 2, y + 30 + ICON + 6), t, f13)
    y = 200
    label(d, (20, y), "2x (each pixel doubled, to judge the drawing)", f16, False)
    for i, (n, t) in enumerate(DRIVES):
        x = 20 + i * col + (col - 2 * ICON) // 2
        im.alpha_composite(shipped(n, ICON, ICON).resize((2 * ICON, 2 * ICON), Image.NEAREST), (x, y + 30))
    y = 400
    label(d, (20, y), "Chosen (the object rises out of the drawer), 1x", f16, False)
    for i, (n, t) in enumerate(DRIVES):
        x = 20 + i * col + (col - ICON) // 2
        im.alpha_composite(art.glowicon(art.frames(n, 512)[1].resize((ICON, ICON), Image.LANCZOS)), (x, y + 30))
    y = 520
    label(d, (20, y), "Per display profile (proposal): the size each profile gives a drive icon", f16, False)
    y += 34
    for p, w, h in PROFILES:
        label(d, (20, y + 20), p + f"  {w} x {h}" + ("  shown on tall pixels" if h < w else ""), f13, False)
        for i, (n, t) in enumerate(DRIVES[:4] + DRIVES[6:8]):
            ic = shipped(n, w, h)
            if h < w:                                       # tall pixels: each row shown twice
                ic = ic.resize((w, 2 * h), Image.NEAREST)
            im.alpha_composite(ic, (420 + i * 90, y + (64 - ic.height) // 2))
        y += 80
    im.convert("RGB").save(out)


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    sheet(*sys.argv[1:])
