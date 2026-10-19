# OpenUp: how we make icons

The standard working practice for the icons of everything OpenUp installs:
every Open app, every prefs editor, every drawer of our own. A thread that
adds an app or an editor adds its icon this way, in the same pull request.

## 1. The rule

**Every OpenUp application and prefs editor has a GlowIcon with animation.**
"Animation" is what Workbench gives a GlowIcon: two frames, one at rest and
one shown while the icon is chosen. Our second frame is never just the first
one lit up. Something in the picture moves, so clicking an icon feels alive:

| Icon | At rest | Chosen |
|---|---|---|
| Drawers (OpenUp, OpenPrint, OpenBrowser, OpenWrite) | Our own drawer, our badge at its foot on the right | The badge plays its own second frame (on our own drawer, files show too) |
| OpenFiles | Two panes, a file chosen on the right | The choice moves to the left pane |
| OpenView | Picture, magnifier beside it | Magnifier over the picture |
| OpenEdit | Pad and pencil | Pencil moves on, a fresh line written |
| OpenPlay | Film with a play button | Play turns to pause |
| OpenWrite | Laid-out page, a pen nib | Nib moves on, a line signed |
| OpenBrowser | Globe in its ring | Meridians turn |
| OpenCompress | Box, arrow out | Lid lifts, arrow further out |
| Printers and Queue | Page in the tray | Page further out, light on |
| Install-OpenUp | Arrow over the box | Arrow sinks into it |
| Uninstall-OpenUp | Red arrow lifting out of the box | Arrow out |
| Dock | Four buttons, one raised | The next button pops up |
| Menus | Menu, a row lit | Row lower down lit, the pointer follows |
| Sound | Speaker, three waves | Four waves, louder |
| TitleBar | Title bar and clock | Clock hands move on |
| Look | Window of swatches, brush | Brush dips into the red, swatch chosen |
| Windows | Three windows stacked | Front window slides out and goes active |
| OpenTypes | Documents, arrow to a program | Arrow moves on |
| OpenUp-Setup | Box and wand | Star flares and sparkles |
| ZuneTest | A window of gadgets | A gadget pressed |
| OpenMonitor | Monitor: the CPU's and GPU's graphs over the cores' bars, a meter at its foot | The load climbs: graphs up, cores full, the needle in the red |
| KindsGallery (opengadtools) | Three tiles: a slider, a button, check boxes (one ticked) | Knob slides on, button pressed, the other box ticked |
| OpenMeters (openamigabench) | A frosted panel: the date and time over three bars, partly filled, the middle one in the accent | A reading later: the bars move on, the last past 90 per cent in the warning colour, the clock's hand on |

## 2. Format

- **OS 3.5 GlowIcon** (FORM ICON with FACE and two IMAG chunks), written by
  amigachrome's `scripts/amiga_icon.py` (`write_os35`). The file also carries
  the classic DiskObject with a 4-colour picture, so OS 3.1 still shows
  something.
- **Drawn at 64 x 64 pixels** (a drawer at 82 x 60, Gorilla's drawer's
  size), **shown at 85 per cent**: the pack build scales every picture by
  `ICON_SCALE`, as Gorilla's own are scaled. Both frames are the same size.
- **Palette:** up to 255 colours per frame, each frame its own palette,
  index 0 see-through. Pictures are dithered to RGB565 first
  (`dither_rgb565`) so they look the same on a 16-bit RTG screen as on a
  24-bit one.
- **Alpha is on or off.** A GlowIcon has no partial transparency: a pixel
  with alpha 110 or more is solid, anything less is see-through. Draw with
  that in mind; soft shadows end up as a solid edge or nothing.
- **The chosen frame gets the glow:** the pack build draws the OS 3.5
  selection outline (`glow_selected`) round the second frame.
- **The icon keeps what it is.** The pack replaces only the pictures; type,
  default tool, tool types, stack and tool window come from the part that
  makes the icon.

## 3. Style

**From 8 October 2026 to OpenUp 0.8.3 the house style was Gorilla's**, the
AROS set OpenUp then gave the system's own icons. Our icons were redrawn to
sit with it, in our own drawing (nothing traced from Gorilla), and the kit
(`tools/gstyle.py`) still draws that way. Since 0.8.4 the system's icons are
OpenIcons look B, our own set (section 8):

- **A bold black outline** round every shape, about two pixels at 64.
- **Light from the top left**, soft gradients, a gentle gloss on the upper part.
- **A soft shadow** below and to the right.
- **Mostly front-on** pages and windows; things that stand (disks, the
  trashcan, drawers) a little from above. Drawers of our own are our own
  drawer with our badge at its foot (`drawer-NAME`).
- **Prefs editors** carry the prefs mark: a small blue monitor with a
  question mark, bottom left.
- **85 per cent** of 64 when shown (the look approved on the canvas).

Every icon in `icons/open` was redrawn this way on 8 October 2026, with our
own kit (`tools/gstyle.py`), from the pilot on the "OpenLook default theme
candidates" canvas.

The style until 8 October 2026, kept for the record: AmigaOS 4.1's, in our own artwork, so the icons sat with the
modern icon set (amigachrome's Boxie and OS4 pack):

- **2:1 isometric.** Objects stand on the ground, seen from the front left;
  x runs right-down, y left-down, z up.
- **Light from the top left.** Top faces lightest, left faces the base
  colour, right faces darker, a gloss on the upper left of the top face and
  a dark outline (the base colour at a third of its brightness).
- **A soft shadow** below and to the right of the object.
- **Bold, simple shapes** that read at 64 pixels: one object, or one object
  and one tool (a magnifier, a pencil, a wand). No text.
- **The palette** is in `tools/open_icons_art.py` (BLUE, TEAL, ORANGE, GREEN,
  RED, GOLD, PURPLE, PAPER, GREY, DARK, INK). Use it before inventing a new
  colour.
- **Drawers of our own** are Boxie's blue filing cabinet with the app's own
  icon as a badge at its foot.
- **Both frames are cropped and scaled alike,** so only the moving part
  moves. `frames()` in the drawing script does this.

## 4. Where the files live

| What | Where |
|---|---|
| The drawing kit | `tools/gstyle.py`: masks, gradients, the outline, gloss and shadow, `finish()` (both frames cropped and scaled alike), the prefs mark, the page, the magnifier (MIT, ours) |
| The drawing script | `tools/open_icons_art.py`: one `NAME(sel)` function per icon, `sel` the chosen frame, listed in `ICONS`; drawer badges in `BADGES` |
| The pictures | `icons/open/NAME.png` and `icons/open/NAME-selected.png`, 64 x 64 RGBA, committed |
| Drawers | `icons/open/badge-NAME.png` (the badge alone, in the drawer's 82 x 60 frame) and `drawer-NAME.png` (on a drawer of our own drawing), both frames |
| The pack's use of them | `tools/build_pack.py`, `open_icons()`: its jobs list maps each icon file a part makes to a picture name |
| The icon codec and layout | amigachrome `scripts/amiga_icon.py` and `scripts/icon_theme.py` (passed in with `--amigachrome`) |

Redraw everything with:

```bash
python3 tools/open_icons_art.py icons/open    # needs Pillow and numpy
```

OpenBrowser draws its own the same way in openamigabrowser's
`tools/make-icons.py` (art in `icons/art/`, with `-selected` frames).

KindsGallery, OpenGadTools' showcase, is drawn here (`kindsgallery` in the
drawing script) and its two pictures are copied to opengadtools'
`art/icons`; its `lib/build.sh` writes `KindsGallery.info` beside the program
with `tools/make_icon.py`, the same way the pack writes ours (85 per cent,
hard alpha, dithered, the glow on the chosen frame).

OpenMeters, OpenBench's desktop widget, is drawn here too (`openmeters`, on
the frosted glass of `frosted_glass`, as the dock's shelf looks). Its two
pictures are copied to openamigabench's `meters/art`; its `meters/build.sh`
writes `OpenMeters.info` beside the program (`meters/make_icon.py`) with the
tooltypes the program's own MAKEICON writes. When the pack ships OpenBench,
its `SYS:WBStartup/OpenMeters.info` is a job in `open_icons()` with the
picture name `OpenMeters`, so it keeps the part's tooltypes and stack.

## 5. How the pack build picks them up

`build_pack.py` makes every part, then calls `open_icons()` with:

- **jobs:** a list of (icon file, picture name). For each job it reads the
  icon the part made, loads `NAME.png` and `NAME-selected.png`, scales them
  to 85 per cent, hardens the alpha, dithers both frames, adds the glow to
  the second, and writes the icon back as a GlowIcon. For a drawer of our
  own, the two frames are `drawer-NAME`, all ours (since 0.8.4: no
  drawer of another set under them). A missing icon file is skipped (a part may not
  make it in every build). A missing `-selected.png` falls back to the rest
  frame with the glow, but the rule above says every icon has one.
- **drawers:** a map of each drawer of our own to the icons in it.

## 6. Where icons sit

- **In drawers we share with the system** (Prefs, Utilities, Tools,
  WBStartup), icons ship **unplaced**: Workbench puts them where there is
  room, so they never land on top of the stock OS 3.2 icons.
- **In drawers of our own** (OpenUp, OpenPrint, OpenBrowser) the icons are
  **laid out**: a grid, sorted with drawers first, by amigachrome's
  `icon_theme.layout()`, and the drawer window is sized to fit them.
- **Drawer windows open in steps** (left 120 + 40 per drawer, top 80 + 30),
  so two drawers opened together don't sit exactly over each other.

## 7. Adding an app or a prefs editor

1. Add `NAME(sel)` to `tools/open_icons_art.py`, drawing both frames with
   the kit, and list it in `ICONS` (a drawer: a badge in `BADGES`).
2. Run the script and commit `icons/open/NAME.png` and `NAME-selected.png`.
3. Add the icon file the part makes, and its picture name, to the jobs list
   in `open_icons()`; add it to `drawers` if it lives in a drawer of our own.
4. Build the pack and look at both frames (ACBuild's Icons tab, or
   `amiga_icon.pictures()` on the built `.info`).
5. Put the new icon in the table in section 1.

## 8. OpenIcons: the system's icons, and what is still to draw

**OpenIcons** is the Team's own icon set (amigachrome-guest `icons/openicons`,
MIT): 1,988 icons drawn new from vector sources by its generator. Look B
(up to 32 colours, soft gradient, bevel and shine) is the look the OpenKick
ROM's own default icons have (amigachrome-guest #336). Since OpenUp 0.8.4 it
gives the system's icons their pictures (Dale, 16 October 2026: "we should
remove them and use ours"), in place of AROS's Gorilla set, which OpenUp
0.6.17 to 0.8.3 fetched at build time. No AROS picture is in the pack.

- **Made at build time, never committed.** The set's `.info` files are its
  generator's build output. `tools/openicons_part.py` runs the generator
  from the amigachrome-guest checkout the pack is built with
  (`build_pack.py --guest`, `--no-openicons` to leave it out): that commit's
  manifest, look and `.info` writer, nothing fetched from outside our
  repositories. The Part file names the commit.
- **New faces, the icons' own selves.** For each icon the system has (the
  SYS: drawers, OS 3.2.3's programs, prefs editors and commodities, the
  monitors, printers, keymaps and DOS drivers, the DefIcons file types in
  `ENVARC:Sys`), the part holds OpenIcons' icon and a `face` line:
  OpenUpTool keeps the icon's type, default tool, tool types, stack and
  drawer window and changes only its picture, keeping the original in
  `SYS:Storage/OpenUp-Superseded` for UNINSTALL. An icon the machine hasn't
  got is left alone, and so is one OpenUp faced before that you changed
  since (0.8.4).
- **Drawers in a second place, and OpenUp's own drawers (OpenIcons 0.2).**
  The part also faces the drawers the manifest names where a machine has them
  elsewhere: OS 3.2.3's `Prefs/Presets/Backdrops`, `Prefs/Presets/Pointers`,
  `Storage/WBStartup` and RECONCILE's `Prefs/Classic`, and AmigaChrome's
  `Apps`, `Games` and `Demos` on SYS: or Work: (`ELSEWHERE` in
  `tools/openicons_part.py`). OpenUp's own drawers (OpenUp, OpenPrint,
  OpenWrite, OpenDemos, OpenGPU-SDK, Developer, Docs) are made as OpenIcons
  look B drawers with their program's motif at pack-build time
  (`own_drawers`, after `open_icons()`), in place of the Gorilla-era
  `drawer-NAME` pictures and Boxie's blue drawer; each keeps its window and
  place. The apps' own animated icons (section 1) are unchanged.
- **Not the default icons or the volumes.** The OpenKick ROM's icon.library
  has its own (RAM, hard disk, floppy, CD, RAD, card, Kickstart, drawer,
  tool, project, trashcan), and a `def_<kind>.info` in `ENVARC:Sys` or
  SYS:'s own `Disk.info` wins over them. The part faces none of them, and
  `OpenUpTool OLDLOOK` (the Installer runs it before the parts) takes each
  out: OS 3.2.3's own as installed (`def_RAM`, `def_harddisk`, `def_disk`,
  `def_drawer`, `def_tool`, `def_project`, `def_trashcan`, `def_cd0`,
  `def_rad`, `def_card`, `def_kickstart`, `def_sys`, `def_work`,
  `SYS:Disk.info`) and the faces OpenUp gave them. Every original stays in
  `SYS:Storage/OpenUp-Superseded`, listed, for UNINSTALL; one you changed
  stays where it is. The DefIcons file types (`def_ascii`, `def_picture`...)
  stay, with OpenIcons' pictures.
- **Gorilla's faces out (0.8.4).** OLDLOOK gives each icon still showing the
  Gorilla picture OpenUp gave it its original back, and the OpenIcons part
  then faces it. Since the pack carries no Gorilla picture, OLDLOOK knows
  them by signature (`src/gorilla_sigs.h`: numbers only, from
  `OpenUpTool ICONSIG` over every GorillaIcons part kept; OS 3.2.3's own
  default icons likewise in `src/os323_default_sigs.h`). "Still as OpenUp
  gave it" is 0.8.3's rule: the icon written again into T: from its own
  picture over the original's fields is the same, byte for byte, as the
  file, but for its place, a window icon's stack and the pointers an icon
  file stores. It also takes out `SYS:Storage/Icons/Gorilla` and the
  GorillaIcons licence note.
- **Kinds.** The part's `Kinds` drawer has OpenIcons' disk, drawer, tool,
  project and trashcan, with no face line: `OpenUpTool ICONS` gives an old
  icon OpenIcons has no picture for by name the one of its kind.

**The icon library** is `icons/LIST.csv`: 640 rows, every Amiga thing that
wants an icon (disks, drawers, prefs, the OS's tools and commodities, our
apps, file types, situations, glyphs, pointers, and the programs people
install), with what each is, what to draw, a priority (P1 ships with OpenUp
on 3.2.3) and, as a name only, the Gorilla picture it corresponded to (up to 0.8.3). The names follow what
Aminet's icon packs (pix/icon) cover.

On 8 October 2026 every row that is an icon was drawn in the house style
(602 rows, 621 icons with the "A / B" names): `tools/icon_library_art.py`
draws each from a recipe of things in `tools/gvocab.py` (a page, a drawer,
a globe, about 170 of them, each an icon of its own or a badge on another),
both frames, and writes them as PNG icons for AmigaChrome's icon library
(`resources/icons/collections/openup-gorilla-style`, rank 3, so the new icon
set picks them before Boxie's). The `Library` column says where each went.
The title bar glyphs and pointers are drawn by OpenLook and OpenWindows, not
as icons; four situations (the selected frame, an AppIcon, Leave Out, the
dock's dot) are not pictures of their own.

```bash
python3 tools/icon_library_art.py OUT --sheet   # needs Pillow, numpy and an amigachrome checkout beside this one
```

Programs that aren't ours get a picture of what they do, not their logo.
