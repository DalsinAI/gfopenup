# OpenRexx as standard on GFOS

On a GFOS machine (the GFOpenKick ROM) OpenRexx, our ARexx, now goes in as standard, and RexxMast starts at boot
(Dale, 18 November 2026: "it should go in as standard", "if its a GFOS machine"). GFOS has no ARexx of its own, and
GFOpenNotifier and GFOpenUpdate talk through ARexx. The Pi 5 card had neither rexxsyslib.library nor RexxMast.

- **The Installer** ticks "OpenRexx 48: ARexx" by default on the GFOpenKick ROM, a Novice install included. On
  Kickstart 3.2.3 it stays off until ticked, as before.
- **The GFOpenLibs-rexx part** has the startup line `when GFOpenKick startup Run >NIL: SYS:System/RexxMast`, so
  RexxMast starts with the Amiga on GFOS. `Run` keeps the startup going where RexxMast is running already.
- **GFOpenUpTool UPDATE AUTO** installs the pack's GFOpenLibs-rexx part on the GFOpenKick ROM when GFOpenUp's list
  hasn't got it and the disk has no `LIBS:rexxsyslib.library`. An ARexx already on the disk stays as it is. The log
  line says "(OpenRexx installed)" or "(OpenRexx failed)".
- AmiCradle's card build (amicradle `appliance_fresh.OPEN_ROM_PARTS`) installs `GFOpenLibs/rexx` on every GFOS card
  and image, before GFOpenNotifier.

## Tested so far

- `tests/test_openup_unattended.c`: when AUTO adds OpenRexx, and the log line's note.
- `tests/test_build_pack.py`: the rexx part's files and its GFOpenKick-only RexxMast line, and the Installer's default.
