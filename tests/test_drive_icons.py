#!/usr/bin/env python3
# Copyright (c) 2026 AmiCradle Team. MIT licence.
"""The drive icons in the AOS style (tools/aos_icons_art.py) and their specimen sheet (tools/drive_icons_sheet.py):
every drive drawn, each its own picture, each a valid GlowIcon at rest and chosen.
    python3 tests/test_drive_icons.py"""
import os, sys, tempfile
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import numpy as np
import aos_icons_art as art
import drive_icons_sheet as sheet

fails = 0
def check(what, ok, more=""):
    global fails
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else f": {more}"))
    fails += not ok

DRIVES = [n for n, _ in sheet.DRIVES]
check("the sheet's drives: System, RAM, Work, Games, hard disk, IDE, USB, CD, floppy, network share",
      DRIVES == ["SYSTEM", "RAM", "WORK", "GAMES", "HARDDISK", "IDE", "USB", "CD", "FLOPPY", "NETWORK"], str(DRIVES))
check("every drive has a picture", all(n in art.PICTURES for n in DRIVES), str([n for n in DRIVES if n not in art.PICTURES]))
pics = {}
for n in DRIVES:
    rest, chosen = art.frames(n, 64)
    pics[n] = np.array(rest)
    check(f"{n}: 64 x 64, at rest and chosen, the two differ",
          rest.size == chosen.size == (64, 64) and not np.array_equal(np.array(rest), np.array(chosen)))
    g = np.array(art.glowicon(rest))
    a = set(np.unique(g[..., 3]).tolist())
    colours = len({tuple(p) for p in g[g[..., 3] > 0][:, :3].tolist()})
    check(f"{n}: a GlowIcon: hard alpha, at most 255 colours, something drawn",
          a <= {0, 255} and 0 < colours <= 255 and (g[..., 3] > 0).sum() > 64 * 64 // 4, f"alpha {a}, {colours} colours")
names = list(pics)
same = [(x, y) for i, x in enumerate(names) for y in names[i + 1:] if np.array_equal(pics[x], pics[y])]
check("each drive its own picture (the ACFS hard disk no longer the IDE drive's)", not same, str(same))
import openicons_part as part
# GFOpenIcon's kinds (amicradle-guest libraries/openicon ic_drive_default): the files it asks for by kind
GUEST_KINDS = {"ram", "harddisk", "floppy", "cd", "rad", "card", "network", "acfs", "system", "ide", "usb"}
kinds = part.AOS_DRIVE_KINDS
check("every drive kind GFOpenIcon asks for that the set draws is installed",
      set(kinds) == GUEST_KINDS - {"rad", "card"}, str(sorted(set(kinds) ^ (GUEST_KINDS - {"rad", "card"}))))
check("each kind's picture is drawn", all(v in art.PICTURES for v in kinds.values()), str(kinds))
check("the USB stick, IDE partition, CD, floppy and network share each get their own picture",
      [kinds[k] for k in ("usb", "ide", "cd", "floppy", "network")] == ["USB", "IDE", "CD", "FLOPPY", "NETWORK"])
check("each def_drive_<kind>.info fits an FFS name (30)", all(len(f"def_drive_{k}.info") <= 30 for k in kinds))
with tempfile.TemporaryDirectory() as d:
    out = os.path.join(d, "s.png")
    sheet.sheet(out)
    check("the specimen sheet is written", os.path.getsize(out) > 10000)
print("all passed" if not fails else f"{fails} FAILED")
sys.exit(1 if fails else 0)
