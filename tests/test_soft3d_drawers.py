#!/usr/bin/env python3
# Copyright (c) 2026 AmiCradle Team. MIT licence.
"""Every drawer is the Soft 3D folder, a named one with its mark on the front (Dale, 17 November 2026: "we should
redo all the folder icons"; tools/soft3d_kinds_art.py, tools/soft3d_motifs.py, tools/openicons_part.py).
    python3 tests/test_soft3d_drawers.py            the drawings and the pack's lists
    AMICRADLE_GUEST=<amicradle-guest checkout> python3 tests/test_soft3d_drawers.py      and the manifest and icon files"""
import os, sys
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import numpy as np
import soft3d_kinds_art as k3
import soft3d_motifs as marks
import openicons_part as part

fails = 0
def check(what, ok, more=""):
    global fails
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else f": {more}"))
    fails += not ok

plain = {n: [np.array(k3.frames("drawer", n, c)) for c in (False, True)] for n in (64, 32)}
check("the plain drawer: 64 and 32 px, at rest and chosen differ",
      all(plain[n][0].shape == (n, n, 4) and not np.array_equal(*plain[n]) for n in (64, 32)))
bad, same, pics = [], [], {}
for m in marks.MOTIFS:
    for n in (64, 32):
        rest, chosen = (np.array(k3.frames("drawer:" + m, n, c)) for c in (False, True))
        ok = (rest.shape == (n, n, 4) and not np.array_equal(rest, chosen)
              and set(np.unique(rest[..., 3]).tolist()) <= {0, 255}
              and np.array_equal(rest[..., 3] > 0, plain[n][0][..., 3] > 0)          # the plain drawer's outline
              and np.array_equal(chosen[..., 3] > 0, plain[n][1][..., 3] > 0)
              and not np.array_equal(rest, plain[n][0]))                              # and a mark on it
        if not ok:
            bad.append((m, n))
    pics[m] = np.array(k3.frames("drawer:" + m, 64, False))
    white = (pics[m][..., :3].astype(int).min(-1) > 225) & (plain[64][0][..., :3].astype(int).min(-1) <= 225)
    if white.sum() < 30:
        bad.append((m, "no white mark to see at 64 px"))
names = list(pics)
same = [(a, b) for i, a in enumerate(names) for b in names[i + 1:] if np.array_equal(pics[a], pics[b])]
check(f"each of the {len(marks.MOTIFS)} marks: 64 and 32 px, two frames, hard alpha, the plain drawer's outline, a white mark",
      not bad, str(bad))
check("no two marks alike", not same, str(same))
for name, table in (("GFOpenUp's own drawers", part.OWN_DRAWERS), ("AmiCradle's drawers on Work: and SYS:", part.NAMED_DRAWERS),
                    ("the system drawers on a GFOpenKick disk", part.OPENKICK_DRAWERS),
                    ("the drawers other parts put on the Media drive", part.MEDIA_DRAWERS)):
    miss = sorted({m for m in table.values() if m not in marks.MOTIFS})
    check(f"{name}: every mark is drawn", not miss, str(miss))
check("Media:midi gets the folder with the sound mark, where it has no icon and only where Media is mounted",
      part.media_drawer_lines() == ['when MediaDrive default "Drawers/Media-midi.info" "Media:midi.info"'],
      str(part.media_drawer_lines()))
check("a named drawer's file name fits an FFS name (30)",
      all(len(d.replace(":", "-") + ".info") <= 30 for d in part.NAMED_DRAWERS))
check("the Work window's drawers on the Pi 5 are named: Benchmarks, GFTestingPack, GFOpenBrowser",
      part.NAMED_DRAWERS.get("Work:Benchmarks") == "gauge" and part.NAMED_DRAWERS.get("Work:GFTestingPack") == "flask"
      and part.NAMED_DRAWERS.get("Work:GFOpenBrowser") == "globe")
check("a drawer's picture name: the folder, or the folder with its mark",
      part.drawer_picture({"name": "x", "motif": ""}) == "S3D:drawer"
      and part.drawer_picture({"name": "Prefs", "motif": "sliders"}) == "S3D:drawer:sliders")
for what, call in (("a motif with no mark is refused, not drawn plain", lambda: part.drawer_picture({"name": "x", "motif": "nothing"})),
                   ("an unknown mark's picture is refused", lambda: part.picture_frames("S3D:drawer:nothing", 64)),
                   ("only a drawer takes a mark", lambda: part.picture_frames("S3D:tool:gear", 64))):
    try:
        call()
        check(what, False)
    except KeyError:
        check(what, True)
fr = part.picture_frames("S3D:drawer:gear", 64)
check("picture_frames gives S3D:drawer:<mark> at rest and chosen, 64 x 64",
      [f.size for f in fr] == [(64, 64)] * 2 and not np.array_equal(np.array(fr[0]), np.array(fr[1])))

guest = os.environ.get("AMICRADLE_GUEST")
if guest:
    load_manifest = part.generator(__import__("pathlib").Path(guest))[0]
    from openicons import info as infofmt
    rows = [e for e in load_manifest() if e["wbtype"] == "drawer"]
    miss = sorted({e["motif"] for e in rows if e["motif"] and e["motif"] not in marks.MOTIFS})
    check(f"the manifest's {len(rows)} drawers: every motif has a Soft 3D mark", len(rows) >= 30 and not miss, str(miss))
    new = part.aos_icon("S3D:drawer:gear", "drawer")
    icon = infofmt.read_info(new)
    check("a marked drawer's icon file: a drawer, 64 x 64, with its window", icon["type"] == "drawer"
          and (icon["width"], icon["height"]) == (64, 64) and icon.get("drawer"), str({k: icon.get(k) for k in ("type", "width", "height")}))
    old = bytearray(part.aos_icon("S3D:drawer", "drawer"))
    old[58:66] = bytes([0, 0, 0, 40, 0, 0, 0, 60])                  # its place
    old[126:134] = bytes([0, 0, 0, 7, 0, 0, 0, 9])                  # where its window was scrolled to
    again = part.aos_icon("S3D:drawer:gear", "drawer", bytes(old))
    check("a face keeps the drawer's place and its window's scroll", again[58:66] == old[58:66] and again[126:134] == old[126:134])
else:
    print("skip the manifest and icon files (set AMICRADLE_GUEST to an amicradle-guest checkout)")
print("all passed" if not fails else f"{fails} FAILED")
sys.exit(1 if fails else 0)
