# GFOpenUp 0.8.3 (16 October 2026)

The default icons and the volumes' icons are the ROM's. Dale, 16 October 2026: "ram disk needs to have one of our
new icons as standard … so do all the volumes etc, they default to our equivalent set".

- **The GorillaIcons part no longer faces the default icons or the volumes.** The GFOpenKick ROM's icon.library has
  its own default icons (GFOpenIcons look B: RAM, hard disk, floppy, CD, RAD, card, network, Kickstart, drawer, tool,
  project, trashcan, device). A `def_<kind>.info` in ENVARC:Sys, or a volume's own `Disk.info`, wins over them, so
  the part leaves alone `ENVARC:Sys/def_RAM`, `def_Harddisk`, `def_Disk`, `def_CDROM`, `def_Kick`, `def_Drawer`,
  `def_Tool`, `def_Project`, `def_Trashcan`, `SYS:Disk.info` and `Work:Disk.info`. The DefIcons file types
  (`def_ascii`, `def_picture`, `def_music`…) keep their Gorilla faces.
- **An upgrade takes out the faces an earlier GFOpenUp gave them** (GFOpenUpTool OLDLOOK, run before the parts, as in
  0.8.2). Each icon is written again into T: from the original GFOpenUp kept in SYS:Storage/GFOpenUp-Superseded and
  the same Gorilla picture (now in Parts/GorillaIcons/Retired), and the file goes only when it is the same, byte
  for byte, but for two things: the icon's place (where it sits, its drawer window), which Workbench writes on
  every Snapshot (a Snapshot of RAM Disk writes `def_RAM.info` through the `RAM:Disk.info` link), and the memory
  addresses an icon file holds for its pointers, which icon.library reads only as there or not there. An icon you
  changed stays, and T:GFOpenUp.log says so.
- **The originals stay in SYS:Storage/GFOpenUp-Superseded** and Uninstall-GFOpenUp puts them back. They are not put
  back on an upgrade: they are OS 3.2.3's own default icons, and they too would hide the ROM's.
- **`Work:Disk.info`**, which GFOpenUp's list doesn't have on some machines though it carries Gorilla's hard disk,
  goes when it is exactly that picture over its own type and fields.
- The Startup-sequence's `MakeLink RAM:Disk.info TO ENVARC:Sys/def_RAM.info` (OS 3.2.3's own) stays: with no
  def_RAM the link leads nowhere and RAM Disk shows the ROM's icon. On Kickstart 3.2.3 the volumes then show
  3.2.3's own built-in disk icon.
- `Version C:GFOpenUpTool` says "GFOpenUpTool 0.8.3" (0.8.2's said "GFOpenLook": OLDLOOK's marker came first).
