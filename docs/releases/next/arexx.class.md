# Release notes for the next GFOpenUp pack: arexx.class on GFOS

For whoever builds the next pack. Paste the sections below into that pack's RELEASE-NOTES.md,
with its own archive, sizes and lab runs.

## What's new

- **SYS:Classes/arexx.class on GFOS** (Dale, 10 November 2026). GFOS had no arexx.class, so no program's ARexx port opened there (GFOpenShares' `GFOPENSHARES.1`, wxMaxima's, every app on apphost.h). The GFOpenUp part now brings ours on the GFOpenKick ROM, where none is:
  - `SYS:Classes/arexx.class` 47.100, GFOpenARexx (amicradle-guest `libraries/openreaction/arexx`, GFOpenReaction, MIT). It is clean-room, API and ABI compatible with OS 3.2's.
  - `SYS:Utilities/GFOpenUp/Licences/GFOpenReaction.txt` (MIT, the AmiCradle Team).

  Under OS 3.2.3's own ROM nothing changes: its disk has ReAction's arexx.class.

## Parts changed

| Part | Change |
|---|---|
| GFOpenUp | on the GFOpenKick ROM: `SYS:Classes/arexx.class` where none is, `Licences/GFOpenReaction.txt` |

## Tested so far

- Host: tests/test_build_pack.py. On the open ROM the class goes where none is, with its licence. Under OS 3.2.3's ROM there is no line for it.
- Lab: see the pull request (a GFOS lab, an app's ARexx port).
