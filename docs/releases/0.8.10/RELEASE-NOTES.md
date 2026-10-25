# OpenUp 0.8.10 (25 October 2026)

The day's parts, rebuilt with GCC 16's late-combine pass off: OpenBench, OpenMenus, OpenDock and OpenRexx's
rexxsyslib.library. The GFOffice Suite comes as three parts, with OpenCalc new. The pack Cradle offers new instances.

- **GCC 16's late-combine pass off** (#167, and each repository's own build). OpenAmigaGCC 16.2.0-open3's late-combine
  pass (GCC PR 123853) can store a copy one byte late. In 0.8.9, OpenUpTool wrote the manifest's first line ("OpenUp",
  a tab, 0.1) a byte late, and OpenMenus named its OpenWBNotify semaphore "". OpenUpTool, OpenUpMenu, ACClip, ACDrop
  and OpenSnap are now built with `-fno-late-combine-instructions`. So are openamigaprefs (#151),
  openamigabench (#57), OpenRexx (amigachrome-guest #519), OpenMUI (openamigamui #16), OpenMulticore
  (openamigamulticore #24) and the GFOffice Suite. This holds until OpenAmigaGCC open4. The store is gone from
  OpenUpTool, OpenMenus, OpenDock and rexxsyslib.library.
- **OpenUpTool 0.8.10**: the "needs part" lines (#166), built with the pass off. Its version is the pack's again.
- **OpenBench 0.1.42** (openamigabench #49 to #57):
  - "No icons on the desktop", when asked;
  - Snapshot;
  - the prefs on one port;
  - a drive mounted while it runs gets its icon;
  - Icons > Leave Out, written to .backdrop.
- **OpenMenus 0.4.6** (openamigaprefs #151; the part, #167).
- **OpenDock 0.6.11** (openamigaprefs #150; the part, #168): it wakes once a second at rest, and ten times a second
  only while the pointer is over it.
- **rexxsyslib.library 48.8** (amigachrome-guest #519): the OpenLibs/rexx part.
- **OpenPrefs 2.6** (openamigaprefs #147 to #151), with "No icons on the desktop" on the Icons page. The OpenTypes and
  OpenDateTime parts follow it.
- **OpenTitle 0.7.6** (openamigaprefs #148, #149): the see-through bar's setting and fade, and the layers check asking
  for 47.106's real table size.
- **The GFOffice Suite** (gfoffice, which was openamigawrite until 25 October; #165, #166):
  - **GFOffice 2.4**, a new part with what the suite's programs share: owf.library 2.4, LIBS:z.library (OpenZlib),
    the dictionary in SYS:Dictionaries and C:OWConvert. OpenWrite and OpenCalc each need it ("needs part GFOffice").
  - **OpenWrite 2.0.13** (from 2.0.1):
    - text measured with the font that draws it;
    - contents pages' right tab stops with dot leaders;
    - bold as wide as it is laid out;
    - Word's cover pages, with their pictures and text boxes where Word puts them;
    - frames that stay put while you type;
    - a lighter undo, and a DOCX round trip;
    - paragraph styles and bookmarks kept.
    It runs on OpenLayout 0.5 (openamigalayout 1375bd7). Its toolbar shows the Chrome icon set (opengadtools #85 to
    #88, `ogt_iconset_attach()`).
  - **OpenCalc 0.1.19**, new: the suite's spreadsheet for XLSX, ODS, XLS and CSV. It has formulas, each cell's own
    font, conditional formatting, comments, data validation, the AutoFilter, and charts on .xlsx sheets drawn
    (columns, bars, lines). It has no Installer choice yet: install it with `OpenUpTool INSTALL Parts/OpenCalc`.
- **OpenFTP** builds against opengadtools main again (openamigaftp #14). It is still 0.2.1.

Every other part is at its repository's main of 25 October 2026, 15:40 BST. OpenBrowser Lite (v0.4.1) and
OpenAmigaPython (v2.7.18-ac3) stay pinned to their releases. The OpenBrowser 0.7 companion is unchanged from 0.8.9.
Built on GCC 16 (the os32-gcc16 stove, OpenAmigaGCC 16.2.0-open3), with the late-combine pass off where it applies.

**Known:** where SYS:Utilities/OpenCalc is already a program (the OpenUp Desktop Tools calculator, which some
instances have by hand), the OpenCalc part can't make its drawer there. OpenUpTool INSTALL then answers 10 but lists
the part in the manifest. Move the calculator away first.
