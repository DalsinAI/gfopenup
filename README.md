# GFOpenUp

GFOpenUp is an add-on pack for **AmigaOS 3.2**. You install AmigaOS 3.2.3
from your own copy, run GFOpenUp's Installer once, and reboot. It adds the
GFOpen family of programs, libraries, datatypes, preferences and icons, set up
to work together. Anything it replaces is kept in
`SYS:Storage/GFOpenUp-Superseded`, and `Uninstall-GFOpenUp` puts the system
back as it was.

GFOpenUp is **not** an operating system and does not include one. It contains
no Kickstart ROM, no AmigaOS files and no Workbench disks: you supply those.

![Tools > GFOpenUp](docs/tools-menu-openup.png)

**Latest release: 0.8.28** (11 November 2026). Packs are on the
[releases page](https://github.com/DalsinAI/gfopenup/releases), and each
release's notes are in `docs/releases/<version>/`.

## What it needs to run

| | Needs |
| --- | --- |
| **Operating system** | AmigaOS 3.2.3 (Hyperion), installed from your own copy. The Installer refuses anything older than 3.2. It is built and tested on 3.2.3; 3.2.0 to 3.2.2 are untested. |
| **ROM** | The Kickstart 3.2 ROM that came with your AmigaOS 3.2, or the GFOpenKick open ROM. A few library parts are marked "needs Kickstart" and are left out on GFOpenKick, which has its own. |
| **CPU** | Tested on a 68040. GFOpenUp's own tools are built for the 68040 with soft float, so they need no FPU; a 68020 or 68030 is untested, and a 68000 or 68010 won't run them. The Installer reads the CPU and FPU and leaves out the parts the machine can't run. |
| **FPU** | Not needed for the pack itself. These parts need a **68040 or 68060 with its FPU** and are left out without one: GFOpenRTG, GFOpenFont, Warp3D and MiniGL, OpenBrowser Lite, OpenBrowser and the maths libraries. |
| **Memory** | Fast RAM is strongly advised. The parts that draw on a true-colour screen (GFOpenGPU, the backdrops, the browsers) are made for an RTG board with its own memory. |
| **Disk** | About 110 MB on SYS: for every part. The backdrops (42 MB), GFOpenGPU (22 MB) and OpenAmigaPython (18 MB) are most of it, and each is optional. |
| **Installer** | `SYS:System/Installer`, which comes with AmigaOS 3.2. |

### Which machines

| Machine | State |
| --- | --- |
| **AmiCradle** (instance or appliance) | The main target, and where every release is tested. Its RTG board (ACRTG), network card, sound board (ACAHI) and cores board are found by `GFOpenUpTool DETECT`, and the parts for them are offered. |
| **Real Amiga** | Supported by the Installer, but not yet tested by testers on real hardware. GFOpenRTG, GFOpenSocket, AmiCradle sharing, the printer part and GFOpenAHI need AmiCradle and are left out. Your Picasso96 or CyberGraphX, and your Roadshow or AmiTCP, stay as they are. |
| **PiStorm** (Emu68) | Found by the Installer, but not yet tested. The same parts are left out as on a real Amiga. |

### What needs AmiCradle

These parts install only on AmiCradle, because they talk to its boards:

- **GFOpenRTG**: Workbench on GFOpenRTG's own 16-bit screen on the ACRTG board.
- **GFOpenSocket**: the network (bsdsocket.library) through AmiCradle's cores.
- **AmiCradle sharing**: the shared clipboard and files.
- **GFOpenPrinter**: printing to AmiCradle's PDF and network printers.
- **GFOpenAHI** (in the Sound part): AHI for the ACAHI sound board.

GFOpenGPU, GFOpenMulticore and GFOpenCrypto run their work on AmiCradle's
other cores when they are there, and on the Amiga's own CPU otherwise.
GFOpenGPU's 3D and SDL 2 games are made for AmiCradle's graphics board.

A network stack (GFOpenSocket, Roadshow or AmiTCP) is needed by GFOpenMail,
GFOpenFTP, GFOpenPuTTY, the browsers and Unofficial Telegram.

## What it installs

You choose the parts in the Installer. Among them:

- **Desktop:** GFOpenWindows (snapping, Amiga+Tab, window places), GFOpenDock,
  GFOpenMenus, GFOpenTitle (logo, free memory, clock and settings on the title
  bar), GFOpenIcons (the Team's own icon set), backdrops for each theme,
  GFOpenBlanker and GFOpenUp Setup (questions at the first start, and the
  startup switches).
- **Files and media:** GFOpenFiles (a two-pane file manager), GFOpenCompress
  (ADF, ADZ, DMS, LhA, zip, tar, gzip), GFOpenTypes (which program opens which
  file), GFOpenImage datatypes (PNG, GIF, Targa, WebP, WebM and more),
  GFOpenView in place of MultiView, GFOpenPlay (a media player).
- **Programs:** GFOpenEdit, GFOpenWrite and GFOffice, GFOpenCalc, GFOpenPresentation, GFOpenMail,
  GFOpenFTP, GFOpenPuTTY, OpenBrowser Lite, OpenAmigaPython, Tata.
- **Libraries:** GFOpenGPU (SDL 2, GL through Mesa), ogles3.library (OpenGL ES
  3.1, AmiCradle only), Warp3D 5.0 and MiniGL on GFOpenGPU,
  GFOpenMUI (our own muimaster.library on GFOpenLayout, where there is no MUI),
  GFOpenTLS and GFOpenCrypto, GFOpenFont, GFOpenMulticore, z.library and the
  open replacements of OS libraries.
- **Developer files:** the GFOpenGPU SDK (headers and link libraries).

`gfopenup.readme` lists every part with its version, and the Installer
explains each one before you tick it. GFOpenUp never overwrites another
network stack, RTG system or MUI without asking.

## Installing

1. Install AmigaOS 3.2.3 and boot it.
2. Unpack `GFOpenUp-<version>.lha` (for example to `RAM:` or `Work:`).
3. Double-click **Install-GFOpenUp** in the GFOpenUp drawer and pick the parts.
4. Reboot.

Running a newer pack's Installer upgrades an older GFOpenUp in place.
`Uninstall-GFOpenUp` removes everything and puts back what was replaced. The
Installer writes what it did to `T:GFOpenUp.log`.

## Building

`python3 tools/build_pack.py --lha` builds every part with its own
repository's scripts and writes `build/pack/GFOpenUp/` and the `.lha`.

It needs:

- Linux with Python 3.
- The GFOpenAmigaGCC cross compiler for AmigaOS 3.2 (`m68k-amigaos-gcc`,
  found through the `STOVE` setting, by default `~/AmiCradle/stoves/os32`).
- Checkouts of the part repositories beside this one, or given with
  `--amicradle`, `--guest`, `--openamigaprint`, `--opengadtools` and the
  other options (`python3 tools/build_pack.py --help`). Some of those
  repositories are not public yet.
- The icon writer: `ACBUILD_ICON_JS=<amicradle>/launcher/acbuild-tools/amiga-icon.js`
  and `BOXIE_ICONS=<amicradle>/resources/icons/collections/boxie`, when
  AmiCradle isn't installed on the build machine.

`tests/` holds the host-side checks. `DESIGN.md` is the design, and
`ICONS.md` covers the icons.

## Licence

MIT, Copyright (c) 2026 The AmiCradle Team (`LICENSE`), for our own files.
Third-party code in the parts keeps its own licence, and each licence ships
with its part in `SYS:Utilities/GFOpenUp/Licences`.

If you use or build on this work, we ask (we do not require) that you credit
The AmiCradle Team, for example "based on GFOpenUp by The AmiCradle Team".

Amiga, AmigaOS, Kickstart and other product names are trademarks of their
respective owners. GFOpenUp is not affiliated with them.

## Contributors

GFOpenUp is created and maintained by [SacredTrees](https://github.com/SacredTrees)
with The AmiCradle Team. Everyone whose work it includes is credited in
[`CONTRIBUTORS.md`](CONTRIBUTORS.md).
