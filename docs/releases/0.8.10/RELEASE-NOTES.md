# GFOpenUp 0.8.10 (26 October 2026)

The day's parts, rebuilt with GCC 16's late-combine pass off: GFOpenBench, GFOpenMenus, GFOpenDock and OpenRexx's
rexxsyslib.library. C:ACROMFix brings the open ROMs up to the latest ROM fixes until Team 4's next ROM build. The
GFOffice Suite comes as three parts on its four new libraries, and GFOpenCalc is new. The pack Cradle offers new
instances.

- **C:ACROMFix**, SetPatch for the open ROM (#170; amigachrome-guest #520). The GFOpenUp part, which every install has,
  puts it in SYS:C with the line `C:ACROMFix QUIET` in its block of S:User-Startup. ACROMFix knows each open ROM by
  the CRC-32 of its two halves and patches only what that ROM lacks:
  - system requesters measured in the font they are drawn in (amigachrome-guest #518): full14b, Instance-32's ROM,
    and full15, the standard ROM;
  - a boot's Workbench in its ScreenMode prefs, with one reset at the boot's end (amigachrome-guest #516): full14b.
  On OS 3.2.3's Kickstart, on an open ROM it doesn't know, or on one that already has every fix, it changes nothing.
  `ACROMFix STATUS` says what the ROM has and lacks and what is in place.
- **GCC 16's late-combine pass off** (#167, and each repository's own build). GFOpenAmigaGCC 16.2.0-open3's late-combine
  pass (GCC PR 123853) can store a copy one byte late. In 0.8.9, GFOpenUpTool wrote the manifest's first line ("GFOpenUp",
  a tab, 0.1) a byte late, and GFOpenMenus named its GFOpenWBNotify semaphore "". GFOpenUpTool, GFOpenUpMenu, ACClip, ACDrop
  and GFOpenSnap are now built with `-fno-late-combine-instructions`. So are openamigaprefs (#151),
  openamigabench (#57), OpenRexx (amigachrome-guest #519), GFOpenMUI (openamigamui #16), GFOpenMulticore
  (openamigamulticore #24) and the GFOffice Suite. This holds until GFOpenAmigaGCC open4. The store is gone from
  GFOpenUpTool, GFOpenMenus, GFOpenDock and rexxsyslib.library.
- **GFOpenUpTool 0.8.10**, built with the pass off. Its version is the pack's again.
  - It reads the "needs part" lines (#166).
  - A file where a part's drawer goes is kept in SYS:Storage/GFOpenUp-Superseded with its icon, and the drawer is made
    (#171). REMOVE and UNINSTALL put the file back. Nothing is deleted.
  - A part that doesn't install completely is not listed as installed (#171). An older version's line stays, so the
    next upgrade tries again, and REMOVE and UNINSTALL can still undo the files that went in.
- **GFOpenBench 0.1.42** (openamigabench #49 to #57):
  - "No icons on the desktop", when asked;
  - Snapshot;
  - the prefs on one port;
  - a drive mounted while it runs gets its icon;
  - Icons > Leave Out, written to .backdrop.
- **GFOpenMenus 0.4.6** (openamigaprefs #151; the part, #167).
- **GFOpenDock 0.6.11** (openamigaprefs #150; the part, #168): it wakes once a second at rest, and ten times a second
  only while the pointer is over it.
- **rexxsyslib.library 48.8** (amigachrome-guest #519): the GFOpenLibs/rexx part.
- **GFOpenPrefs 2.6** (openamigaprefs #147 to #151), with "No icons on the desktop" on the Icons page. The GFOpenTypes and
  GFOpenDateTime parts follow it.
- **GFOpenTitle 0.7.6** (openamigaprefs #148, #149): the see-through bar's setting and fade, and the layers check asking
  for 47.106's real table size.
- **The GFOffice Suite** (gfoffice, which was openamigawrite until 25 October; #165, #166, #172):
  - **GFOffice 3.1**, a new part with what the suite's programs share (#172; gfoffice #87 to #90):
    - the suite's four libraries: owfwrite.library 3.1, owfcalc.library 3.2, owfpub.library 3.0 and
      owfpresent.library 3.1, each program's own share of what was owf.library;
    - LIBS:z.library (GFOpenZlib), the dictionary in SYS:Dictionaries, and C:OWConvert on the four libraries.
    It retires LIBS:owf.library, which the four replace. An owf.library an earlier GFOpenUp installed goes with its
    manifest line, as any GFOpenUp file a newer part takes over does; one that came from elsewhere is kept in
    SYS:Storage/GFOpenUp-Superseded. The part's version is owfwrite.library's. GFOpenWrite and GFOpenCalc each need the part ("needs part GFOffice").
  - **GFOpenWrite 2.0.22** (from 2.0.1), on owfwrite.library:
    - text measured with the font that draws it;
    - contents pages' right tab stops with dot leaders;
    - bold as wide as it is laid out;
    - Word's cover pages, with their pictures and text boxes where Word puts them;
    - frames that stay put while you type, and text that wraps round them;
    - text boxes you can type into, with undo, kept when saved, and the ones in the line kept;
    - frames in headers and footers;
    - exact and at-least line spacing;
    - Word's table styles on their cells;
    - ODT frames read back, and bookmarks that keep their places;
    - a lighter undo, and a DOCX round trip;
    - paragraph styles kept;
    - a document's first layout makes its display list once, from an estimate, instead of growing it by doubling;
    - Amiga documents read more completely: Final Writer's every section, with indents, alignment, tab stops and
      the page size; ProWrite's colours and page size; IFF FTXT as its specification says; and Wordworth's text
      boxes (gfoffice #105 to #108).
    It runs on GFOpenLayout 0.7 (openamigalayout #12). Its toolbar shows the Chrome icon set (opengadtools #85 to #88,
    `ogt_iconset_attach()`).
  - **GFOpenCalc 0.1.27**, new, on owfcalc.library: the suite's spreadsheet for XLSX, ODS, XLS and CSV.
    - Formulas, each cell's own font, and formatting set on whole rows and columns.
    - Conditional formatting, comments, data validation with list dropdowns and Excel's alerts, and the AutoFilter,
      all kept in ODS as well as XLSX.
    - Charts drawn on the sheet: columns, bars, lines, pie, doughnut and scatter, with line markers, data labels and
      axis number formats. They are kept in ODS, follow inserted and deleted rows and columns, renamed sheets and
      moved cells, and can be selected and moved.
    It has no Installer choice yet: install it with `GFOpenUpTool INSTALL Parts/GFOpenCalc`.
- **GFOpenCalculator** (#171): the GFOpenUp Desktop Tools calculator, which some instances have by hand at
  SYS:Utilities/GFOpenCalc, is now GFOpenCalculator 0.4. GFOpenCalc is the spreadsheet. No part installs the calculator.
  Where it is in the spreadsheet's way, the GFOpenCalc part keeps it in SYS:Storage/GFOpenUp-Superseded (above).
- **GFOpenFTP** builds against opengadtools main again (openamigaftp #14). It is still 0.2.1.

Every other part is at its repository's main of 26 October 2026, 02:00 GMT. OpenBrowser Lite (v0.4.1) and
OpenAmigaPython (v2.7.18-ac3) stay pinned to their releases. The OpenBrowser 0.7 companion is unchanged from 0.8.9.
Built on GCC 16 (the os32-gcc16 stove, GFOpenAmigaGCC 16.2.0-open3), with the late-combine pass off where it applies.
ACROMFix is built by its own build-amiga.sh on the os32 stove (GCC 6.5).
