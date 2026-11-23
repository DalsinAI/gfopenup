#!/usr/bin/env python3
"""Our programs' Soft 3D icons (tools/soft3d_programs_art.py): GFOpenUpdate's, GFOpenNotifier's and AmiPortAtlas's two frames at both sizes, and with
    AMICRADLE_GUEST=<amicradle-guest checkout> python3 tests/test_soft3d_programs.py      its icon file, tool types kept
MIT, Copyright (c) 2026 AmiCradle Team."""
import os, sys
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import numpy as np
import soft3d_programs_art as p3
import openicons_part as part

fails = 0
def check(what, ok, more=""):
    global fails
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else f": {more}"))
    fails += 0 if ok else 1

for name, n in [(x, n) for x in ("GFOpenUpdate", "GFOpenNotifier", "AmiPortAtlas") for n in (64, 32)]:
    rest, chosen = (np.array(p3.frames(name, n, c)) for c in (False, True))
    check(f"{name} at {n} px: two frames of {n} x {n}", rest.shape == chosen.shape == (n, n, 4), str(rest.shape))
    a = np.concatenate([rest[..., 3].ravel(), chosen[..., 3].ravel()])
    check(f"{name} {n} px: each pixel solid or see-through", set(np.unique(a)) <= {0, 255}, str(np.unique(a)[:8]))
    check(f"{name} {n} px: something there, and chosen is another picture",
          (rest[..., 3] > 0).sum() > n * n // 4 and (rest != chosen).any())
    blue = ((chosen[..., 2] > 200) & (chosen[..., 0] < 120) & (chosen[..., 3] > 0)).sum()
    check(f"{name} {n} px chosen: the blue glow", blue > 0)
check("a name with no drawing is refused", "Nothing" not in p3.DRAW)
try:
    part.picture_frames("S3P:Nothing", 64)
    check("S3P: a program with no drawing stops the build", False)
except KeyError:
    check("S3P: a program with no drawing stops the build", True)
check("S3P:GFOpenUpdate is the programs' drawing", all(f.size == (64, 64) for f in part.picture_frames("S3P:GFOpenUpdate", 64)))

guest = os.environ.get("AMICRADLE_GUEST")
if guest:
    import tempfile, pathlib
    part.generator(pathlib.Path(guest))
    from openicons import info as infofmt
    old = part.aos_icon("S3D:tool", "tool")
    d = pathlib.Path(tempfile.mkdtemp())
    (d / "GFOpenUpdate.info").write_bytes(old)
    (d / "Other.info").write_bytes(old)
    check("own_programs: only the programs it has a drawing for", part.own_programs(pathlib.Path(guest), [d / "GFOpenUpdate.info", d / "Other.info", d / "Gone.info"]) == 1)
    icon = infofmt.read_info((d / "GFOpenUpdate.info").read_bytes())
    check("GFOpenUpdate's icon file: a tool, 64 x 64", icon["type"] == "tool" and (icon["width"], icon["height"]) == (64, 64),
          str({k: icon.get(k) for k in ("type", "width", "height")}))
    check("and another picture than the plain tool's", (d / "GFOpenUpdate.info").read_bytes() != old)
else:
    print("skip the icon file (set AMICRADLE_GUEST to an amicradle-guest checkout)")
print("all passed" if not fails else f"{fails} FAILED")
sys.exit(1 if fails else 0)
