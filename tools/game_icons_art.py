#!/usr/bin/env python3
"""Icons for the OpenUp game ports: big, bright and fun, in the Gorilla style
of tools/gstyle.py (bold outlines, light from the top left, gloss, a soft
shadow), drawn larger than an app's icon: 96 x 96 pixels shown at full size,
so the games stand out in their drawer (the user, 10 October 2026: "sexy, fun
and larger than usual"). Our own drawing; nothing from the games' own art.
MIT, Copyright (c) 2026 AmigaChrome Team.

    python3 tools/game_icons_art.py icons/games    # needs Pillow and numpy

Two frames each, as ICONS.md asks, with something that moves when chosen:

| Icon | At rest | Chosen |
|---|---|---|
| Neverball | A glossy marble on a tilted floor, a gold coin ahead | The marble rolls onto the coin, which sparkles |
| Neverputt | A golf ball on the green, the flag in the hole | The ball at the hole's lip, the flag flying |
| AstroMenace | A fighter climbing, engines lit | Both cannons fire, the engines flare |
| drawer-Neverball, drawer-AstroMenace | Our drawer with the game's badge | The badge's own chosen frame |
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gstyle import *            # noqa: E402,F401,F403
from PIL import Image           # noqa: E402

SIZE = 96                       # larger than an app's 64 (shown at 85 per cent): a game stands out
DRAWER_W, DRAWER_H = 82, 60


def star(cx, cy, r, points=5, inner=0.45, turn=-90):
    pts = []
    for i in range(points * 2):
        rr = r if i % 2 == 0 else r * inner
        a = math.radians(turn + i * 180 / points)
        pts.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
    return poly(pts)


def marble(ic, cx, cy, r):
    """A glossy marble: deep blue and red swirl, a bright window of light."""
    ic.add(ellipse(cx - r, cy - r, cx + r, cy + r), radial((cx - r * 0.35, cy - r * 0.4), r * 1.9, rgb('#8FD3FF'), rgb('#123A8C')), gloss=0.15)
    ic.detail(arc(cx - r * 0.8, cy - r * 0.8, cx + r * 0.8, cy + r * 0.8, 200, 330, 2.4), rgb('#FF4D6D'))
    ic.detail(arc(cx - r * 0.5, cy - r * 0.5, cx + r * 0.9, cy + r * 0.9, 20, 120, 2.0), rgb('#FFD447'))
    ic.detail(ellipse(cx - r * 0.62, cy - r * 0.7, cx - r * 0.1, cy - r * 0.28), (255, 255, 255))
    ic.detail(ellipse(cx + r * 0.25, cy + r * 0.35, cx + r * 0.45, cy + r * 0.52), rgb('#CDEBFF'))


def coin(ic, cx, cy, r, sparkle=False):
    ic.add(ellipse(cx - r * 0.72, cy - r, cx + r * 0.72, cy + r), linear((cx - r, cy - r), (cx + r, cy + r), rgb('#FFE97A'), GOLD_LO), gloss=0.3)
    ic.detail(ellipse(cx - r * 0.38, cy - r * 0.6, cx + r * 0.38, cy + r * 0.6), rgb('#E9B82A'))
    ic.detail(star(cx, cy, r * 0.42), rgb('#FFF4B8'))
    if sparkle:
        for (dx, dy, s) in ((-1.5, -1.4, 0.55), (1.45, -1.0, 0.4), (1.2, 1.3, 0.32)):
            ic.add(star(cx + dx * r, cy + dy * r, r * s, 4, 0.3, -45), (255, 255, 255), outline=0.9)


def floor(ic):
    """A tilted floor of tiles, Neverball's playfield seen from above."""
    corners = [(4, 40), (40, 24), (62, 44), (24, 62)]
    ic.add(poly(corners), linear((4, 24), (62, 62), rgb('#7EE0B0'), rgb('#1F8A6A')), gloss=0.12)
    # a checker: lines across the floor in both directions
    for t in (0.25, 0.5, 0.75):
        a0 = (corners[0][0] + (corners[1][0] - corners[0][0]) * t, corners[0][1] + (corners[1][1] - corners[0][1]) * t)
        a1 = (corners[3][0] + (corners[2][0] - corners[3][0]) * t, corners[3][1] + (corners[2][1] - corners[3][1]) * t)
        b0 = (corners[0][0] + (corners[3][0] - corners[0][0]) * t, corners[0][1] + (corners[3][1] - corners[0][1]) * t)
        b1 = (corners[1][0] + (corners[2][0] - corners[1][0]) * t, corners[1][1] + (corners[2][1] - corners[1][1]) * t)
        ic.detail(stroke([a0, a1], 0.7), rgb('#2D9C77'))
        ic.detail(stroke([b0, b1], 0.7), rgb('#2D9C77'))
    # the floor's thickness at the front
    ic.add(poly([(4, 40), (24, 62), (24, 65.5), (4, 43.5)]), rgb('#16664E'), outline=1.6)
    ic.add(poly([(24, 62), (62, 44), (62, 47.5), (24, 65.5)]), rgb('#0F4F3C'), outline=1.6)


def neverball(sel):
    ic = Icon()
    floor(ic)
    if sel:
        marble(ic, 34, 30, 13)
        coin(ic, 49, 37, 6.5, sparkle=True)
    else:
        coin(ic, 46, 34, 6.5)
        marble(ic, 24, 26, 13)
        for i, (x, y) in enumerate(((10, 41), (14, 38.5))):    # rolling: a little trail behind it
            ic.detail(ellipse(x - 1.2 + i, y - 0.8, x + 1.2 + i, y + 0.8), rgb('#BDF2DA'))
    return ic


def green_and_hole(ic):
    ic.add(ellipse(2, 30, 62, 62), radial((24, 40), 40, rgb('#9CEB6E'), rgb('#2F8C2A')), gloss=0.15)
    ic.detail(ellipse(36, 44, 48, 49.5), rgb('#123312'))                # the hole


def neverputt(sel):
    ic = Icon()
    green_and_hole(ic)
    ic.add(rect(41.4, 6, 43, 47), linear((41, 6), (43, 47), rgb('#F4F5F6'), rgb('#9EA3AA')), outline=1.4)
    if sel:   # the flag flies out
        ic.add(poly([(43, 6.5), (60, 9.5), (55, 13.5), (61, 17.5), (43, 18)]), linear((43, 6), (60, 18), rgb('#FF6B5E'), RED_LO), gloss=0.25)
        ball_x, ball_y = 33.5, 44
    else:
        ic.add(poly([(43, 6.5), (56, 11), (43, 17)]), linear((43, 6), (56, 17), rgb('#FF6B5E'), RED_LO), gloss=0.25)
        ball_x, ball_y = 14, 45
    ic.add(ellipse(ball_x - 5.5, ball_y - 5.5, ball_x + 5.5, ball_y + 5.5), radial((ball_x - 2, ball_y - 2.5), 10, (255, 255, 255), rgb('#C8CDD4')), gloss=0.2)
    for dx, dy in ((-1.6, 0.8), (1.4, -0.4), (0.6, 2.2), (-0.4, -1.8), (2.4, 1.6)):
        ic.detail(ellipse(ball_x + dx - 0.55, ball_y + dy - 0.55, ball_x + dx + 0.55, ball_y + dy + 0.55), rgb('#AEB4BC'))
    return ic


def fighter(ic, sel):
    """A sleek fighter climbing to the upper right: swept wings with orange
    stripes, a silver body, a cyan canopy, two blue engines."""
    # engine flames first, behind the hull
    fl = 1.6 if sel else 1.0
    for ex, ey in ((17, 47), (11, 41)):
        ic.add(poly([(ex, ey), (ex - 6 * fl, ey + 9 * fl), (ex - 1.5, ey + 3), (ex - 9 * fl, ey + 6 * fl), (ex + 2, ey + 1)]),
               linear((ex, ey), (ex - 9 * fl, ey + 9 * fl), rgb('#E8FBFF'), rgb('#2F7BFF')), outline=1.2, ink=rgb('#14306E'))
    # wings
    ic.add(poly([(24, 35), (2, 55), (15, 57), (37, 44)]), linear((6, 34), (34, 52), rgb('#59606B'), rgb('#22262C')), gloss=0.2)
    ic.add(poly([(31, 29), (12, 6), (25, 4), (41, 23)]), linear((14, 10), (38, 28), rgb('#59606B'), rgb('#22262C')), gloss=0.2)
    ic.detail(stroke([(9, 51), (29, 39)], 2.0), ORANGE)
    ic.detail(stroke([(18, 8.5), (35, 25)], 2.0), ORANGE)
    # the body: a long dart from the engines to the nose
    body = [(9, 44), (17, 51), (52, 21), (61, 5), (45, 12)]
    ic.add(poly(body), linear((12, 8), (58, 48), rgb('#F2F5F9'), rgb('#7D8796')), gloss=0.35)
    ic.detail(stroke([(17, 44), (52, 15)], 0.9), rgb('#5C8FD6'))
    # canopy
    ic.add(ellipse(40, 17, 49, 24), radial((42, 18.5), 8, rgb('#E6FFFF'), rgb('#1BA6C9')), outline=1.4)
    if sel:   # both cannons fire: two bright bolts ahead of the nose
        for ox, oy in ((0, 0), (5, 5)):
            ic.add(stroke([(54 + ox, 13 + oy), (63 + ox, 2 + oy)], 2.2), linear((54, 13), (63, 2), rgb('#FFF6B0'), rgb('#FF5A3C')), outline=1.0, ink=rgb('#7A1E10'))


def astromenace(sel):
    ic = Icon()
    # a small ringed planet far behind, so the ship has somewhere to be
    ic.add(ellipse(44, 40, 60, 56), radial((48, 44), 18, rgb('#FFC07A'), rgb('#9C3D8C')), outline=1.4, gloss=0.2)
    ic.detail(arc(40, 45, 64, 51, 160, 380, 1.3), rgb('#FFE7C2'))
    for x, y, r in ((6, 24, 1.0), (56, 28, 0.8), (34, 52, 0.7), (4, 6, 1.0)):
        ic.detail(star(x, y, r * 2.4, 4, 0.28, -45), (255, 255, 255))
    fighter(ic, sel)
    return ic


ICONS = {"Neverball": neverball, "Neverputt": neverputt, "AstroMenace": astromenace}


def own_drawer(ic, sel):
    """Our drawer (tools/open_icons_art.py's own_drawer, the same shape)."""
    ic.add(poly([(5, 24), (75, 24), (69, 13), (11, 13)]), linear((0, 13), (0, 24), rgb('#8C8A6C'), rgb('#5F5D45')))
    ic.detail(poly([(13, 15.5), (67, 15.5), (71, 22.5), (9, 22.5)]), rgb('#4A4935'))
    ic.add(rrect(2, 21, 77, 54, 7), linear((2, 21), (77, 54), rgb('#F0EFE2'), rgb('#A9A78A')), gloss=0.3)
    ic.detail(arc(28, 27, 50, 41, 25, 155, 1.8), rgb('#4A4935'))


def drawer_frames(fn):
    """The game's own icon, reduced, as a badge on the front of our drawer."""
    out = []
    for sel in (False, True):
        ic = Icon()
        own_drawer(ic, sel)
        d = ic.render()
        badge = finish([fn(sel).render()], size=40)[0]
        d.alpha_composite(badge.resize((40 * S, 40 * S), Image.LANCZOS), (P(40), P(14)))
        region = (P(0), P(0), P(DRAWER_W + 4), P(DRAWER_H + 2))
        out.append(d.crop(region).resize((DRAWER_W + 4, DRAWER_H + 2), Image.LANCZOS))
    return out


def main(out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for name, fn in ICONS.items():
        rest, chosen = finish([fn(False).render(), fn(True).render()], size=SIZE)
        rest.save(out / f"{name}.png")
        chosen.save(out / f"{name}-selected.png")
    for name, fn in (("Neverball", neverball), ("AstroMenace", astromenace)):
        rest, chosen = drawer_frames(fn)
        rest.save(out / f"drawer-{name}.png")
        chosen.save(out / f"drawer-{name}-selected.png")
    print(f"{len(ICONS)} game icons and 2 drawers in {out}")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
