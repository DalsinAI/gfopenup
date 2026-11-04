# GFOpenUp

Install AmigaOS 3.2.3, run GFOpenUp, and it's ready to boot on a real Amiga, a PiStorm or the AmiCradle appliance. We, 4 October 2026: "install 3.2.3, run our pack", "and it's ready to boot on a PiStorm, our appliance, real Amiga"; "GFOpenUp it is".

Status, 5 October 2026: **GFOpenUp 0.1 is built, for AmiCradle instances**
(phase 1, plus GFOpenLook). It installs GFOpenSocket, GFOpenPrint and GFOpenView,
AmiCradle's sharing tools, GFOpenLook and the background pack, and its uninstaller puts the system
back exactly as it was: validated on a scratch OS 3.2.3 instance
(`docs/history/VALIDATION.md`).

**Latest: GFOpenUp 0.8.14** (4 November 2026; `docs/releases/0.8.14`): GFOpenPrefs 2.7, GFOpenWindows 0.11
(the wheel scrolls the list last clicked; new windows centred until snapped, with a snapshot pin, both off unless
chosen), Tata's known fixes (`S:Tata.fixes`), the themes and desktop profiles for the Theme page, and on the open
ROM the disk libraries it leaves to a disk, each only where none is. Packs are on the
[releases page](https://github.com/DalsinAI/gfopenup/releases).

GFOpenUp's programs and their settings are in Workbench's Tools menu, under
GFOpenUp and Preferences (`src/openupmenu.c`, `DESIGN.md` section 7):

![Tools > GFOpenUp](docs/tools-menu-openup.png)

**Building it:** `python3 tools/build_pack.py --lha` builds every part with
its own repository's scripts (checkouts beside this one, or given with
`--amicradle`, `--openamigaprint`, `--opengadtools`, `--guest`) and makes
`build/pack/GFOpenUp/` and `GFOpenUp-0.1.lha`. On the Amiga, double-click
Install-GFOpenUp in the drawer. Without an AmiCradle install on the build
machine, point the icon writer at the amicradle repository:
`ACBUILD_ICON_JS=<amicradle>/launcher/acbuild-tools/amiga-icon.js` and
`BOXIE_ICONS=<amicradle>/resources/icons/collections/boxie`. `DESIGN.md` is the design. Section 6 takes in a lot of what AmiKit tries to be: an Amiga that is beautiful and ready the moment it starts, with first-boot Setup, startup switches, themes and free updates.

Part of the GFOpen family (internally, the Great Open Source Reunification
Project): one platform, AmigaOS 3.2.x with the Open libraries, that is easy
to set up, and to build and port for.

## Licence

MIT, Copyright (c) 2026 AmiCradle Team (`LICENSE`), for our own files.
Third-party code keeps its own licence (see `DESIGN.md`).

If you use or build on this work, we ask (we do not require) that you credit
the AmiCradle Team, for example "based on GFOpenUp by the AmiCradle
Team".

## Contributors

GFOpenUp is created and maintained by [SacredTrees](https://github.com/SacredTrees) with The AmiCradle Team, copyright AmiCradle Team. Everyone whose work it includes is credited in [`CONTRIBUTORS.md`](CONTRIBUTORS.md).
