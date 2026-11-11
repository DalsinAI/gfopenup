# Release notes for the next GFOpenUp pack: GFOpenPresentation, and GFOffice after LV-426

For whoever builds the pack after 0.8.28. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **GFOpenPresentation joins the pack** as its own part, `Parts/GFOpenPresentation`, in `SYS:Utilities/GFOpenPresentation`. It is the GFOffice Suite's slides: it opens ODP and PPTX, saves ODP, exports PDF, prints through GFOpenPrint and runs a full-screen show.
  - It has the `GFOPENPRESENTATION` ARexx port (gfoffice #130). The port has the common commands, plus `SLIDES`, `SLIDE`, `NEWSLIDE`, `DELETESLIDE`, `OBJECTS`, `GETTEXT`, `SETTEXT`, `UNDO` and `REDO`.
  - It needs the GFOffice part, for `LIBS:z.library`. It is built with libowf linked in, because owfpresent.library has no program on it yet, so `build_pack.py` builds gfoffice a second time (`OWF=linked`, `build/os3-linked`).
  - There is no Installer choice yet: `GFOpenUpTool INSTALL Parts/GFOpenPresentation`.
- **Formulas naming another workbook keep it through ODS** (gfoffice #129, LV-426 finding 13): "Other.xls!Rate" is no longer saved as this workbook's "Rate". The fix is in owfcalc.library and `C:OWConvert`, in the GFOffice part.

## Tested so far

- `tests/test_build_pack.py`: the part's lines, version, need of GFOffice, and INSTALL alone left out (5).
- gfoffice #130's build, with the guard gfmalloc, in a copy of Instance-32: `tests/arexx/presentation.rexx` ALL PASS (62 checks: open, edit, undo, save, PDF, print, reopen); `calc.rexx` and `write.rexx` ALL PASS; no guard hits.
