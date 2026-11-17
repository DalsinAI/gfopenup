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
      set(kinds) == GUEST_KINDS - {"rad", "card"} | {"nvme"}, str(sorted(set(kinds) ^ (GUEST_KINDS - {"rad", "card"}))))
check("the NVMe picture ships as def_drive_nvme ahead of GFOpenIcon's kind (Dale 15:20)", kinds.get("nvme") == "S3D:nvme")
# Soft 3D since Dale's 14 Nov 15:18 "soft 3d sheet is cool, go with that": S3D:<kind> names
import soft3d_drives_art as d3
import soft3d_kinds_art as k3
check("each kind's picture is drawn", all(v.startswith("S3D:") and v[4:] in d3.KINDS for v in kinds.values()), str(kinds))
check("the USB stick, IDE partition, CD, floppy and network share each get their own picture",
      [kinds[k] for k in ("usb", "ide", "cd", "floppy", "network")] == ["S3D:usb", "S3D:ide", "S3D:cd", "S3D:floppy", "S3D:network"])
check("the boot drive and RAM disk by device are Soft 3D", part.AOS_DEVICES == {"DH0": "S3D:system", "RAM": "S3D:ram"})
check("the five Kinds are Soft 3D, each with its Workbench type",
      set(part.AOS_KINDS) == set(part.KINDS) == set(part.KIND_TYPES)
      and all(v.startswith("S3D:") and v[4:] in d3.KINDS + k3.KINDS for v in part.AOS_KINDS.values()))
check("the dock's faces keep their AOS pictures",
      part.AOS_FACES["SYS:System/Shell"] in art.PICTURES and all(v in art.PICTURES for v in part.AOS_BY_NAME.values()))
check("every volume with a Disk.info of its own is a Soft 3D drive, Work: the plain hard disk",
      all(v.startswith("S3D:") and len(part.picture_frames(v, 64)) == 2 for v in part.AOS_VOLUMES.values())
      and part.AOS_VOLUMES["Work:Disk"] == "S3D:harddisk", str(part.AOS_VOLUMES))
fr = part.picture_frames("S3D:usb", 64)
check("picture_frames gives S3D:<kind> at rest and chosen, 64 x 64", [f.size for f in fr] == [(64, 64)] * 2
      and not np.array_equal(np.array(fr[0]), np.array(fr[1])))
try:
    part.picture_frames("S3D:nothing", 64)
    check("an unknown S3D kind is refused", False)
except KeyError:
    check("an unknown S3D kind is refused", True)
# the Games: volume: a Soft 3D hard disk with a game pad on it, its own picture, not a kind of drive
import soft3d_drives_art as s3d
check("Games: takes the Soft 3D picture", part.AOS_VOLUMES.get("Games:Disk") == "S3D:games", str(part.AOS_VOLUMES))
check("games is a volume's picture, not a kind of drive", "games" in s3d.DRAW and "games" not in s3d.KINDS
      and "games" not in kinds.values())
grest, gchosen = part.picture_frames("S3D:games", 64)
hrest = part.picture_frames("S3D:harddisk", 64)[0]
srest = part.picture_frames("S3D:system", 64)[0]
ga, ha = np.array(grest), np.array(hrest)
check("S3D:games: 64 x 64, at rest and chosen differ, not the hard disk's nor the system's picture",
      grest.size == gchosen.size == (64, 64) and not np.array_equal(ga, np.array(gchosen))
      and not np.array_equal(ga, ha) and not np.array_equal(ga, np.array(srest)))
check("S3D:games: the hard disk's outline (the same body), so it stands as the other drives do",
      np.array_equal(ga[..., 3] > 0, ha[..., 3] > 0))
# Demos:, Media: and Emulation: (Dale, 17 November 2026: "update the icons so they are like games , representative")
vols = {"demos": "Demos:Disk", "media": "Media:Disk", "emulation": "Emulation:Disk"}
vpics = {"games": ga}
for v, dest in vols.items():
    check(f"{dest[:-5]} takes the Soft 3D picture S3D:{v}", part.AOS_VOLUMES.get(dest) == "S3D:" + v, str(part.AOS_VOLUMES))
    check(f"{v} is a volume's picture, not a kind of drive", v in s3d.DRAW and v not in s3d.KINDS and "S3D:" + v not in kinds.values())
    for n in (64, 32):
        vr, vc = part.picture_frames("S3D:" + v, n)
        hr = part.picture_frames("S3D:harddisk", n)[0]
        check(f"S3D:{v} at {n}: at rest and chosen differ, the hard disk's outline, not its picture",
              vr.size == vc.size == (n, n) and not np.array_equal(np.array(vr), np.array(vc))
              and np.array_equal(np.array(vr)[..., 3] > 0, np.array(hr)[..., 3] > 0)
              and not np.array_equal(np.array(vr), np.array(hr)))
    vpics[v] = np.array(part.picture_frames("S3D:" + v, 64)[0])
vn = list(vpics)
vsame = [(x, y) for i, x in enumerate(vn) for y in vn[i + 1:] if np.array_equal(vpics[x], vpics[y])]
check("Games, Demos, Media and Emulation each have their own picture", not vsame, str(vsame))
check("each volume's Disk.info name fits an FFS name (30)", all(len(d.replace(":", "-") + ".info") <= 30 for d in part.AOS_VOLUMES))
from PIL import Image
check("the boot drive's logo is AmiCradle's own (the ball and the A), kept with the tool", s3d.LOGO.is_file())
check("the logo keeps its shape and sits centred", s3d.logo_box(988, 669, 256, 250, 310) == (101, 145, 310, 210),
      str(s3d.logo_box(988, 669, 256, 250, 310)))
flat = s3d.draw_system(s3d.Flat(512), False) and None
f512 = s3d.Flat(512); s3d.draw_system(f512, False)
px = np.array(f512.img)[145:355, 101:411, :3].astype(int)
red = ((px[..., 0] > 170) & (px[..., 1] < 90) & (px[..., 2] < 90)).sum()
blue = ((px[..., 2] > 180) & (px[..., 0] < 60)).sum()
check("the boot drive carries the ball's red squares and no Deep cradle arc", red > 2000 and blue == 0, f"red {red}, blue {blue}")
check("each def_drive_<kind>.info fits an FFS name (30)", all(len(f"def_drive_{k}.info") <= 30 for k in kinds))
with tempfile.TemporaryDirectory() as d:
    out = os.path.join(d, "s.png")
    sheet.sheet(out)
    check("the specimen sheet is written", os.path.getsize(out) > 10000)
print("all passed" if not fails else f"{fails} FAILED")
sys.exit(1 if fails else 0)
