Install AmigaOS 3.2.3, run the pack, reboot: the Open family, set up. Every app is MIT licensed and free. Version 0.8 is for AmigaChrome instances; real Amigas and the PiStorm come next.

0.8.1 adds six more of the Team's open libraries to OpenLibs. Everything else is as in 0.8.0.

**OpenLibs: six more libraries, for any Amiga**
- The Installer's sixth page now offers nine libraries. Like the first three, each is off until you tick it, an upgrade keeps each switch as it was, and unticking one puts OS 3.2's own back.
- The new six go in LIBS: in place of OS 3.2's own, which is kept in SYS:Storage/OpenUp-Superseded. They are built for the 68000, so any Amiga can have them, and no restart is needed beyond the next start.
- Each passed its test against OS 3.2's own, call for call, in a lab copy of an AmigaChrome instance, both beside OS 3.2's library and swapped in for it:
  - version.library 47.100: VersionABITest, 36 checks, 0 differ.
  - nonvolatile.library 47.100 (saved settings and high scores, in OS 3.2's own storage): NonVolatileABITest, 266 checks, 0 differ. Items OS 3.2's library wrote read back the same.
  - realtime.library 47.100 (the music clock): RealTimeABITest, 709 checks, 0 differ. It ticks at the same rate on the same CIA timer as OS 3.2's.
  - amigaguide.library 47.100: AmigaGuideABITest, 272 checks, 0 differ.
  - rexxsyslib.library 47.100: RexxSysLibABITest, 1,417 checks beside OS 3.2's and 1,331 swapped in, 0 differ. The ARexx interpreter lives inside OS 3.2's library, so the open one is a face over it: the Installer copies OS 3.2's own to LIBS:OS32/rexxsyslib.library first, and that copy goes again when the library is unticked or OpenUp is uninstalled.
  - lowlevel.library 47.100 (joysticks, CD32 pads, the keyboard and timers for games): LowLevelABITest, 109 checks, 0 differ.

**Known**
- lowlevel.library: OS 3.2's own answers for ports 2 and 3 were not the same from one boot to the next, so the open library follows the best fit there. In the lab, a program started straight after a full LowLevelABITest run hung twice, with either library; reboot between test runs.
- Everything in 0.8.0's Known list still holds.

Every file OpenUp adds or replaces is listed in ENVARC:OpenAmiga/OpenUp.manifest, and Uninstall-OpenUp takes it all out again.

MIT, Copyright (c) 2026 AmigaChrome Team; third-party code keeps its own licence, as in 0.8.0. The licence texts are in SYS:Utilities/OpenUp/Licences.
