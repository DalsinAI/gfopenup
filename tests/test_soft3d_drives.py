#!/usr/bin/env python3
# Copyright (c) 2026 AmiCradle Team. MIT licence.
"""The drive icons by kind in the Soft 3D style (tools/soft3d_drives_art.py): nine kinds at 64 and 32 px, at rest
and chosen, the 16-colour chipset set, and Dale's sheet.
    python3 tests/test_soft3d_drives.py"""
import os, sys, tempfile
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import numpy as np
from PIL import Image
import soft3d_drives_art as art

fails = 0
def check(what, ok, more=""):
    global fails
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else f": {more}"))
    fails += not ok

check("the nine kinds, in the decision's order",
      art.KINDS == ["system", "harddisk", "nvme", "ide", "usb", "cd", "floppy", "ram", "network"], str(art.KINDS))
check("each kind has a drawing and a label", all(k in art.DRAW and k in art.LABELS for k in art.KINDS))
check("the chipset palette: 15 colours and transparent, no colour twice",
      len(art.PAL15) == 15 and len(set(art.PAL15)) == 15)

def bbox(a):
    ys, xs = np.nonzero(a[..., 3])
    return xs.min(), ys.min(), xs.max(), ys.max()

pics = {}
for kind in art.KINDS:
    for n in (64, 32):
        rest, chosen = (np.array(art.frames(kind, n, c)) for c in (False, True))
        pics[(kind, n)] = rest
        alpha = set(np.unique(rest[..., 3]).tolist()) | set(np.unique(chosen[..., 3]).tolist())
        check(f"{kind} {n}: {n} x {n}, hard alpha, the two frames differ",
              rest.shape == chosen.shape == (n, n, 4) and alpha <= {0, 255} and not np.array_equal(rest, chosen),
              f"{rest.shape} alpha {alpha}")
        x0, y0, x1, y1 = bbox(rest)
        check(f"{kind} {n}: fills its icon (one side at least 80 %)",
              max(x1 - x0 + 1, y1 - y0 + 1) >= 0.8 * n, f"{x1 - x0 + 1} x {y1 - y0 + 1}")
        # the thin dark edge: most of the outline's pixels are dark
        a = rest[..., 3] > 0
        inner = a.copy()
        inner[1:] &= a[:-1]; inner[:-1] &= a[1:]; inner[:, 1:] &= a[:, :-1]; inner[:, :-1] &= a[:, 1:]
        rim = rest[a & ~inner][:, :3].astype(int)
        dark = (rim.sum(1) < 3 * 110).mean()
        check(f"{kind} {n}: a dark edge round it ({dark:.0%} of the outline dark)", dark >= 0.6, f"{dark:.0%}")
        # chosen: lifted and glowing blue
        cb = bbox(chosen)
        blue = chosen[(chosen[..., 3] > 0) & (chosen[..., 2] > 200) & (chosen[..., 0] < 140)]
        check(f"{kind} {n}: chosen, it rises and glows blue", cb[1] <= y0 and len(blue) > 0, f"top {cb[1]} vs {y0}")
        if n == 32:
            c = np.array(art.chipset(art.frames(kind, 32, False)))
            cols = {tuple(p) for p in c[c[..., 3] > 0][:, :3].tolist()}
            check(f"{kind} 32 chipset: only the palette's colours, same shape",
                  cols <= set(art.PAL15) and np.array_equal(c[..., 3] > 0, rest[..., 3] > 0), str(cols - set(art.PAL15)))

names = list(pics)
same = [(x, y) for i, x in enumerate(names) for y in names[i + 1:] if np.array_equal(pics[x], pics[y])]
check("every picture its own", not same, str(same))
# NVMe and RAM told apart by shape: NVMe stands tall, RAM lies long
for n in (64, 32):
    nx0, ny0, nx1, ny1 = bbox(pics[("nvme", n)])
    rx0, ry0, rx1, ry1 = bbox(pics[("ram", n)])
    check(f"NVMe and RAM differ in shape at {n} (NVMe taller than wide, RAM at least twice as wide as tall)",
          (ny1 - ny0) > (nx1 - nx0) and (rx1 - rx0) >= 2 * (ry1 - ry0))
# 32 px is its own drawing, not 64 cut down
for kind in ("harddisk", "nvme", "ram"):
    down = np.array(Image.fromarray(pics[(kind, 64)]).resize((32, 32), Image.LANCZOS))
    check(f"{kind}: the 32 px picture is its own drawing", not np.array_equal(down, pics[(kind, 32)]))
with tempfile.TemporaryDirectory() as t:
    art.main([os.path.join(t, "out")])
    files = sorted(os.listdir(os.path.join(t, "out")))
    check("the pictures: 6 per kind (64/32 at rest and chosen, chipset at rest and chosen)",
          len(files) == 6 * len(art.KINDS) and "nvme-32-chipset-selected.png" in files, str(len(files)))
    art.main(["--sheet", os.path.join(t, "s.png")])
    w, h = Image.open(os.path.join(t, "s.png")).size
    check("the sheet: all nine across, a light and a dark desktop", w > 9 * 140 and h > 1000, f"{w} x {h}")
print("FAILED" if fails else "all good", f"({fails} failed)")
sys.exit(1 if fails else 0)
