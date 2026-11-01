# GFOpenUp 0.8.4 (16 October 2026)

Our own icons, and no AROS artwork. Dale, 16 October 2026: "we should remove them and use ours".

- **The GFOpenIcons part, in place of GorillaIcons.** GFOpenIcons look B, the Team's own set (amigachrome-guest
  `icons/openicons`, MIT), gives the system's icons their pictures the way GorillaIcons did: the SYS: drawers,
  OS 3.2.3's programs, prefs editors and commodities, the monitors, printers, keymaps and DOS drivers, and the
  DefIcons file types. Each icon keeps its type, default tool, tool types, stack and place; its original goes to
  SYS:Storage/GFOpenUp-Superseded, and Uninstall-GFOpenUp puts it back. Look B is the look the GFOpenKick ROM's own
  default icons have, so the desktop is one set.
- **Where the icons come from.** The set's `.info` files are its generator's build output, never committed. The
  pack build (`tools/openicons_part.py`) runs the generator from the amigachrome-guest checkout it is built with
  (`--guest`), so the pack's icons are that commit's and nothing is fetched from outside our repositories.
  `--no-openicons` leaves the part out.
- **No Gorilla.** The GorillaIcons part and its build-time fetch from AROS's repository (`tools/gorilla_icons.py`,
  `tools/gorilla.lock`) are gone, and so is the badge-on-Gorilla's-drawer for our own drawers: they are our own
  drawer (`drawer-NAME`). The pack ships no AROS picture and no GPL text for one.
- **The upgrade (GFOpenUpTool OLDLOOK, run before the parts):**
  - **Gorilla's faces out.** An icon still showing the Gorilla picture GFOpenUp gave it gets its original back
    (keeping its place and drawer window), and the GFOpenIcons part then faces it. "Still as GFOpenUp gave it" is
    0.8.3's byte-for-byte rule: the icon written again into T: from its own picture over the original's fields
    is the same as the file, but for its place, a drawer's or disk's stack (Workbench's snapshot writes 4096
    where OS 3.2.3's had 8192), and the pointers an icon file stores. The pack has no Gorilla picture to compare
    with, so the picture is known by its signature (`GFOpenUpTool ICONSIG`; `src/gorilla_sigs.h`, numbers only).
  - **The default icons out.** OS 3.2.3's own `ENVARC:Sys/def_RAM`, `def_harddisk`, `def_disk`, `def_drawer`,
    `def_tool`, `def_project`, `def_trashcan`, `def_cd0`, `def_rad`, `def_card`, `def_kickstart`, `def_sys`,
    `def_work` and `SYS:Disk.info` go, so the ROM's own icons show: each where it is the original GFOpenUp kept
    (unchanged but for its place), or, never listed, where its whole signature is OS 3.2.3's as installed
    (`src/os323_default_sigs.h`); and GFOpenUp's own faces of them. The originals go to
    SYS:Storage/GFOpenUp-Superseded, listed, so Uninstall-GFOpenUp puts them back.
  - **Yours stay.** A default icon of your own, or an icon you changed since GFOpenUp gave it a picture, stays as
    it is; the GFOpenIcons part leaves it too, and T:GFOpenUp.log says so.
  - **What GorillaIcons copied goes:** SYS:Storage/Icons/Gorilla and SYS:Utilities/GFOpenUp/Licences/Gorilla.txt.
- **ICONS** gives an old icon GFOpenIcons' picture by name, else GFOpenIcons' disk, drawer, tool, project or trashcan
  by its kind (the part's Kinds drawer).
- `Version C:GFOpenUpTool` says "GFOpenUpTool 0.8.4". New: `GFOpenUpTool ICONSIG <icon or drawer>`.
- On Kickstart 3.2.3 the volumes and the removed default kinds show 3.2.3's own built-in icons.
