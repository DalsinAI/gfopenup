# OpenUp 0.8.9 (25 October 2026)

The no-ROM-volume design's OpenUp side, the task manager, printing through OpenPrint, and five parts that come with
every install. The pack Cradle offers new instances.

- **OpenPrefs in SYS:Preferences** (no-ROM-volume P8, #152). One project icon per page, by its plain name (Icons
  and Printing too, #163). The old stubs in SYS:Prefs, SYS:Prefs/OpenPrefs and C:OpenPrefs are archived. The boot's
  OpenPrefs APPLY line stays only where the ROM doesn't apply the settings itself. OpenUpTool 0.8.9 reads `when` and
  `unless` in front of any line (AmigaChrome, OpenKick).
- **With every install:** OpenSnap 1.0, the snapshot commodity (Shift+Right Amiga+S and Shift+Right Amiga+W, #157);
  ShellFont, DejaVu Sans Mono 8 to 24 (#149); ShellIcon, the Shell icon opening 80 x 24 (#151); and on a 68040 or
  68060 with its FPU, OpenFont, openfont.library and DejaVu Sans (#150).
- **OpenPrinter** (#161): our printer.device in front of OpenPrint, with printer.prefs naming OpenPrint's driver
  where there are none. OS 3.2.3's printer drivers, editors and tools are archived, and Uninstall-OpenUp puts them
  back. AmigaChrome instances only.
- **OpenMonitor 0.8.1**, the task manager (#154 to #156, #158, #160, #162): Tasks, Memory, Libraries, Ports, Interrupts,
  Hardware, Speed and Calls, with exact CPU time from OpenExec's task clock. The ROM keeps 0.3.4.
- **Backdrops 0.2** (#159): OpenUp's own desktops with their tones. The Background Pack's art is no longer in it;
  that is an optional install from Cradle's Media Library.
- **OpenUpMenu 0.1.6** (#153) looks for what is installed once, not every two seconds.
- **OpenImage** takes its datatypes from opendatatypes, with OpenZlib's z.library. OpenFiles (#145) and OpenWrite
  use that z.library too, so the pack builds no zlib source.
- Part versions follow their programs: OpenWindows 0.9.2, OpenBlanker 1.2, OpenDock 0.6.10, OpenUpSetup 0.1.6 and
  OpenGPU 0.20.

Every other part is at its repository's main of 25 October 2026, 00:35 BST. Among them: OpenBench 0.1.36, OpenPrefs
2.4 with the Printing page, OpenTitle 0.7.4, OpenSpeaker 0.1.6, OpenRTG 0.24 and Tata 0.5.0. OpenBrowser Lite (v0.4.1)
and OpenAmigaPython (v2.7.18-ac3) stay pinned to their releases, and the OpenBrowser 0.7 companion is unchanged from
0.8.8. Built on GCC 16 (the os32-gcc16 stove, OpenAmigaGCC 16.2.0-open3). OpenSnap needed a small fix to build
there.
