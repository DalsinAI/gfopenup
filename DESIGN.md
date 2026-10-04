# OpenUp: design

Dale, 4 October 2026:
- "one 'upgrade' or some cool name for us installing it on a base 3.2.3
  install"
- "so it's: install 3.2.3, run our 'upgrade' or 'unifier' or 'extender' pack"
- "and it's ready to boot on a PiStorm, our appliance, real Amiga"

This is the Open family as one thing a user installs: the platform of
`OPEN_SDK_DESIGN.md`, and the mission test, delivered.

## 1. The promise

1. Install AmigaOS 3.2.3 as Hyperion ships it (the user's own copy).
2. Run one Installer from the pack.
3. Reboot. RTG works on every monitor, and so do the network, MUI programs,
   printing, mail, archives and file types, with nothing more to read.

**The same pack** works on a real Amiga (with whatever cards it has), on a
PiStorm (Emu68), in an AmigaChrome instance, and on the appliance.

## 2. What it finds, and what it installs

| Machine | Found by | Display | Network | Also |
| --- | --- | --- | --- | --- |
| AmigaChrome instance, the appliance | Dalsin $DA15 boards in expansion.library's list: ACRTG (product 9), the OpenSocket card (6), ACStorage | OpenRTG with `ACRTG.card`, `ACRTG.chip` and OpenGPU's `ACRTG.gpu` | OpenSocket's host backend | accontrol: the shared clipboard, files and pace |
| PiStorm | Emu68's own resources | The PiStorm's RTG through OpenRTG once its driver exists; until then the installed Picasso96 stays. VideoCore later for OpenGPU and OpenMedia | OpenSocketDirect over `wifipi.device` or `genet.device` | |
| A real Amiga | The boards in expansion.library's list, the CPU and FPU, the chipset | OpenRTG where it has a driver for the card, and AGA through OpenRTG's AGA card; otherwise the installed Picasso96 or CyberGraphX stays | OpenSocketDirect over the card's SANA-II driver, or the installed Roadshow or AmiTCP if the user keeps it | |

**The rest is the same everywhere:**
- `zunemaster.library`, plus the `muimaster.library` passthrough where no MUI
  is installed;
- OpenGadTools' classes and the look;
- OpenPrint and OpenView, OpenTypes, OpenCompress, OpenMail, and OpenBrowser
  when it's ready;
- the SDL and Mesa runtime libraries.

AmiSSL is another project's: the Installer offers it as it is published,
with its notices.

## 3. Rules

- **The base OS stays as shipped.** The pack adds. Anything it replaces goes
  to `SYS:Storage/OpenUp-Superseded`, listed in a log, and one uninstaller
  puts everything back.
- **It never overwrites another stack, RTG system or MUI without asking.**
  These are the rules of OpenSocket, OpenRTG and OpenMUI.
- **Novice and Expert.** A Novice user gets the recommended answer to every
  question; an Expert sees each choice.
- **Startup changes are marked.** They go in a marked block in `S:User-Startup`.
  The one exception is `C:OpenRTG`, early in `S:Startup-Sequence` on
  machines whose board has no boot ROM. It too is marked, with the old file
  kept.
- **Re-running a newer pack upgrades in place.** A manifest in
  `ENVARC:OpenAmiga/` lists what is installed, with versions; ACBuild's
  AmigaChrome Tools tab can read it.
- **Errors say what to do next,** in plain words (the mission test).

## 4. Where it ships

- **Aminet:** one LhA archive.
- **Real machines:** an ADF and an HDF.
- **AmigaChrome:** the Dalsin tools drive in instances, and Cradle's recipes
  for new OS 3.2.3 instances. Instances and real machines get the same pack.
- **The appliance:** its first build carries it, demonstrating every Open
  standard.

## 5. Phases

| Phase | Delivers | Done when |
| --- | --- | --- |
| 1 | The Installer, machine detection, the manifest and the uninstaller; what is ready now: OpenSocket, OpenPrint and OpenView, and accontrol's tools in AmigaChrome | A fresh OS 3.2.3 scratch copy, plus the pack, prints and reaches the network after one reboot; the uninstaller returns it to clean |
| 2 | Each Open part as it lands: OpenRTG, `zunemaster.library` and the passthrough, OpenGadTools, OpenTypes, OpenCompress, OpenMail, OpenBrowser | Each works after the pack is re-run |
| 3 | PiStorm and real-Amiga detection, with testers' hardware | The same pack on a PiStorm and on a real A1200 with an RTG card |
| 4 | Cradle's OS 3.2.3 recipe applies the pack; the appliance ships it | A new instance from Cradle comes up with the Open family |

**Name and repository:** OpenUp (Dale, 4 October 2026: "OpenUp it is"),
`DalsinAI/openamigaup`.
