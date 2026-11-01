Install AmigaOS 3.2.3, run the pack, reboot: the GFOpen family, set up. Every app is MIT licensed and free. Version 0.7 is for AmiCradle instances; real Amigas and the PiStorm come next.

0.7.9 carries everything the Team merged since 0.7.6. Every part is rebuilt from the current sources. (0.7.7 and 0.7.8 were GFOpenUpTool's versions, not packs.)

**The screen**
- gfopenrtg.library 0.17: Native screen modes, which take the size of AmiCradle's window. Pick GFOpenRTG.1 Native in ScreenMode, and Workbench follows the window when it is resized.
- An upgrade now brings GFOpenRTG up to date where it is installed, and keeps the screen mode you chose. Up to 0.7.6 an upgrade left GFOpenRTG at its old version, so as not to undo the mode.
- gfopengpu.library 0.15: GFOpenGfx's core draws blits, area fills, ellipses, scrolling and text, in windows too.

**New parts**
- GFOpenBench (a preview, off unless picked): the Team's own Workbench, with the dock and the Open preference editors built in, in SYS:System. It runs where the Startup-Sequence starts SYS:System/GFOpenBench in place of LoadWB, as AmiCradle's own instances do; GFOpenUp doesn't change the Startup-Sequence. On a Native mode it follows the window, and Resize to Window fits Workbench to it.
- GFOpenDateTime (ticked by default in the Installer): the date, the time and the region in one editor in SYS:Prefs. You pick a place, not an offset, and summer time follows by itself; on AmiCradle the place is the host's. Setting the clock gives C:Date and SetClock their own NIL: handles.

**The rest**
- bsdsocket.library 4.3: a break signal stays set after it interrupts a wait, so ping ends.
- gfopenmulticore.library 0.4: version 2 run blocks and OMCF_PAGES; jobs on the cores board are permissive by default.
- GFOpenFiles 0.3.1: a typed path is looked at before DOS can ask for a disk.
- GFOpenTitle 0.6.15: the 64 MiB ACRTG board counts as a GPU.
- Tata 0.4.3: a program AmiCradle's task guard stopped goes on your list.
- OAPSpooler ends on Ctrl-C, so GFOpenBench can quit.
- The Open theme's light mode has a light screen bar.
- GFOpenUpTool 0.7.8: UNINSTALL lets go of the backdrop picture before deleting it, the 64 MiB ACRTG board is an ACRTG board, and every command it runs and waits for gets its own input and output.

**Checked when the pack was built**
- Every part's declared version is its program's version string: all 40 parts, and OpenBrowser in the OpenBrowser pack.
- `fpcr_check` finds no GCC 16 FPCR clash in the libraries and programs it checks.
- In the Team's labs: an upgrade from 0.7.6 with GFOpenUpTool UPDATE (the look and the 1920 x 1080 screen mode kept), a new install, the Native screen mode followed through 10 window resizes and Resize to Window, GFOpenDateTime setting the clock, and OMCTest.

The OpenBrowser pack carries the same OpenBrowser as 0.7.6's.

Every file GFOpenUp adds or replaces is listed in ENVARC:GFOpenAmiga/GFOpenUp.manifest, and Uninstall-GFOpenUp takes it all out again.

MIT, Copyright (c) 2026 AmigaChrome Team; third-party code keeps its own licence (Mesa: MIT; SDL 2: Zlib; PuTTY: MIT; BearSSL: MIT; Mozilla's certificate list: MPL-2.0; the Gorilla icons: GNU GPL version 2). The licence texts are in SYS:Utilities/GFOpenUp/Licences.
