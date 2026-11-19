# Release notes for the next GFOpenUp pack: the chipset's monitors

For whoever builds the pack after 0.8.30. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **The GFOpenRTG part brings the chipset's monitors**, `DEVS:Monitors/AGA` and `DEVS:Monitors/ECS` (gfopenrtg `monitors/`; Dale, 19 November 2026). The chipset is treated as a graphics card with monitors of its own, so AGA's and ECS's modes sit in one ScreenMode list with GFOpenRTG's, each native mode named by its chipset (`AGA PAL:High Res`).
  - AGA adds the 31 kHz timings (Multiscan, DblNTSC, DblPAL) on an AGA machine; ECS adds Multiscan on an ECS machine, and on an AGA machine gives ECS's filtered view. ModeIDs don't change.
  - Each does nothing on a chipset that can't show its modes, and ECS does nothing while AGA runs.
  - On GFOpenGfx's graphics.library they add the MonitorSpecs it already knows, so no OS 3.2 driver files are needed (those hang on GFOpenGfx: don't put them in `DEVS:Monitors` there).

## Tested so far

- Lab, 8 November 2026 (gfopenrtg monitors/README.md): GFOpenGfx 47.102 with AA+ and the AGA monitor boots, 138 modes become 194, and screens opened in DblPAL, Multiscan and PAL. OS 3.2.3 47.10 on an A1200 boots with 431 modes. ECS on GFOpenGfx gives 162 modes.
- Built with the open8 stove: AGA 4,132 bytes, ECS 3,972 bytes. `tests/test_build_pack.py` passes.
- GFOpenUp 0.8.30-rc5, lab t5-mameui-20261118 (19 November): `GFOpenUpTool INSTALL Parts/GFOpenRTG` put both in `DEVS:Monitors/`. Run by hand, AGA took the list from 138 modes to 194, the native modes named `AGA PAL:High Res` and so on, with Multiscan, DblNTSC and DblPAL.
- **Known: on the open-ROM machine they don't load at boot.** Its Startup-Sequence runs nothing in `DEVS:Monitors` (GFOpenRTG's modes come from the ROM's `gfopenrtg.monitor`), so the files sit there unused until the startup runs them (a LoadMonDrvs step, amicradle-guest's Startup-Sequence). On Kickstart 3.2.3 the Startup-Sequence's LoadMonDrvs runs them.
- Not yet: AC090's display doesn't model the 31 kHz beam registers, so the picture's timing is unproven on real hardware.
