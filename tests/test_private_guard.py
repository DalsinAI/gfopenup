#!/usr/bin/env python3
"""Host checks of build_pack.py's guard against material that is never released (check_private): a part marked
as a private pack's, and a file whose SHA-256 is in the deny list, stop the build. Made-up bytes only.

    python3 tests/test_private_guard.py

MIT, Copyright (c) 2026 AmiCradle Team."""
from __future__ import annotations

import hashlib
import os
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_pack as bp  # noqa: E402

checks = 0


def check(ok: bool, what: str) -> None:
    global checks
    checks += 1
    if not ok:
        raise SystemExit(f"FAIL: {what}")


def main() -> int:
    with tempfile.TemporaryDirectory() as t:
        top = Path(t) / "GFOpenUp"
        part = top / "Parts" / "GFOpenEdit"
        part.mkdir(parents=True)
        (part / "Part").write_text('part GFOpenEdit 1.0\ncopy "GFOpenEdit" "SYS:Tools/GFOpenEdit"\n')
        (part / "GFOpenEdit").write_bytes(b"our own program")
        check(bp.check_private(top, set()) == [], "a pack of our own parts has no problem")

        held = b"made-up bytes standing for a file that is never released"
        deny = {hashlib.sha256(held).hexdigest()}
        check(bp.check_private(top, deny) == [], "a deny list that matches nothing is no problem")
        (part / "data.bin").write_bytes(held)
        found = bp.check_private(top, deny)
        check(len(found) == 1 and found[0].startswith("Parts/GFOpenEdit/data.bin: "), "a denied file is found by its bytes")
        check("made-up" not in found[0], "the problem line names the path only")
        (part / "data.bin").unlink()

        private = top / "Parts" / "TPExample"
        private.mkdir()
        (private / "Part").write_text("part TPExample 1.0\n")
        (private / "testingpack.PRIVATE").write_text("not for release\n")      # AmigaOS names ignore case
        found = bp.check_private(top, set())
        check(len(found) == 1 and found[0].startswith("Parts/TPExample/testingpack.PRIVATE: "), "a marked part is found")
        (private / "testingpack.PRIVATE").unlink()
        (top / "PRIVATE-NOT-FOR-RELEASE").write_text("x")
        check(len(bp.check_private(top, set())) == 1, "the private pack's top marker is found")

        listed = Path(t) / "DENY.sha256"
        listed.write_text(f"# comment\n{next(iter(deny))}\nnot-a-digest\n\n")
        check(bp.private_deny_list(listed) == deny, "the deny list reads digests and skips the rest")
        check(bp.private_deny_list(Path(t) / "none") == set(), "no list on this machine: empty")
        os.environ["GF_TESTING_DENY"] = str(listed)
        check(bp.private_deny_list() == deny, "GF_TESTING_DENY names the list")
        del os.environ["GF_TESTING_DENY"]
    print(f"private guard: {checks} checks pass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
