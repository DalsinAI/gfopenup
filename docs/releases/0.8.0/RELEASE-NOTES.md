Install AmigaOS 3.2.3, run the pack, reboot: the Open family, set up. Every app is MIT licensed and free. Version 0.8 is for AmigaChrome instances; real Amigas and the PiStorm come next.

0.8.0 brings the first of the Team's own rewrites of AmigaOS's libraries, as opt-in switches, with everything the Team merged since 0.7.9. Every part is rebuilt from the current sources.

**OpenLibs: the open libraries, one switch each**
- The Installer has a sixth page, "Open libraries, in place of OS 3.2's own". Each library is off until you tick it. An upgrade keeps each switch as it was, so an upgrade changes nothing here unless you change a switch.
- A library is offered only when its test against OS 3.2's own library, call for call, is clean, and it builds as its own build script says. Three are:
  - utility.library 47.100 (tags, hooks, dates, 64-bit sums): UtilityABITest, 722 checks, 0 differ. Needs a 68020 or later.
  - mathieeedoubbas.library 47.100 (double-precision maths): MathIEEEABITest, 731,616 checks, 0 differ. Needs a 68040 or 68060 with its FPU.
  - input.device 50.2 (openamigainput #9): the keyboard and the mouse, with AmigaChrome's pointer placed exactly where the host's is. Its checks against OS 3.2's own input.device all pass. Needs a 68020 or later. `InputProbe INFO` in a Shell says which input.device runs.
- On AmigaChrome, utility.library's and the maths' calls run on the x86 or ARM64 cores.
- utility.library and input.device are in the ROM, so they can't simply be replaced in LIBS:. OS 3.2's own LoadModule loads them from S:Pre-LoadModule-Startup, before anything else has started, and exec starts them in place of the ROM's: the Amiga restarts once by itself as it starts. mathieeedoubbas.library goes in LIBS:, and OS 3.2's own is kept in SYS:Storage/OpenUp-Superseded.
- Unticking a library puts OS 3.2's own back: its files go (OpenUpTool REMOVE, new), the original comes back, and a ROM one is gone from the next start. The startup goes with the last library. A Pre-LoadModule-Startup of another program's is never written over; the ROM libraries are left out then.
- Not offered yet, until their tests are clean or they can stand in OS 3.2's place: the IEEE transcendental maths (some answers differ in the last bits), mathieeesingbas (in the ROM; its build needs an FPU, so it has no module yet), layers, keymap and locale (a few differences left), datatypes, icon and expansion (not a whole replacement yet), and the libraries and devices whose tests against OS 3.2 have not run yet.

**The rest**
- OpenUpTool 0.8.0: REMOVE takes one part out and puts back what it replaced.
- OpenDateTime 0.2.2: KEEP no longer spins after a Ctrl-E.
- SDL 2 (OpenGPU's SDL2.module and the developer files): translated Amiga text input, the right modifier keys and focus.
- The Installer's pages say "of 6".

**Checked when the pack was built**
- Every part's declared version is its program's version string, OpenLibs' libraries too.
- `fpcr_check` finds no GCC 16 FPCR clash in the libraries and programs it checks.
- In the Team's labs, copies of an AmigaChrome instance with 0.7.9: an upgrade with OpenUpTool UPDATE and no library ticked (nothing of the OS changed); utility.library alone, input.device alone, mathieeedoubbas.library alone and all three, each after a power-on (OpenBench, ARexx, OpenUp, Ken's Labyrinth and OpenDateTime INFO all worked); and all three switched off again (OS 3.2's own back, nothing of OpenLibs left).

**Known**
- On AmigaChrome, with utility.library or input.device switched on, a reset (not a power-on) logs a Guru 00028001 as exec starts again; AmigaChrome's task guard restarts the machine, and LoadModule loads the libraries again. The Amiga comes up with them each time, after two extra restarts.

The OpenBrowser pack carries the same OpenBrowser as 0.7.6's.

Every file OpenUp adds or replaces is listed in ENVARC:OpenAmiga/OpenUp.manifest, and Uninstall-OpenUp takes it all out again.

MIT, Copyright (c) 2026 Dalsin Limited; third-party code keeps its own licence (Mesa: MIT; SDL 2: Zlib; PuTTY: MIT; BearSSL: MIT; Mozilla's certificate list: MPL-2.0; the Gorilla icons: GNU GPL version 2). The licence texts are in SYS:Utilities/OpenUp/Licences.
