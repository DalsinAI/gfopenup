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
| Drawers (OpenUp, OpenPrint, OpenBrowser, OpenWrite) | Gorilla's drawer, our badge at its foot on the right | The badge plays its own second frame (on our own drawer, files show too) |
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

**From 8 October 2026 the house style is Gorilla's** (section 8): the set
OpenUp ships for the system's own icons. Our icons are redrawn to sit with
it, in our own drawing (nothing traced from Gorilla), and new ones are drawn
that way:

- **A bold black outline** round every shape, about two pixels at 64.
- **Light from the top left**, soft gradients, a gentle gloss on the upper part.
- **A soft shadow** below and to the right.
- **Mostly front-on** pages and windows; things that stand (disks, the
  trashcan, drawers) a little from above. Drawers of our own are Gorilla's
  drawer with our badge at its foot (those are GPL, as Gorilla's drawer is).
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

## 5. How the pack build picks them up

`build_pack.py` makes every part, then calls `open_icons()` with:

- **jobs:** a list of (icon file, picture name). For each job it reads the
  icon the part made, loads `NAME.png` and `NAME-selected.png`, scales them
  to 85 per cent, hardens the alpha, dithers both frames, adds the glow to
  the second, and writes the icon back as a GlowIcon. For a drawer of our
  own, when the pack ships Gorilla, the two frames are `badge-NAME` on
  Gorilla's drawer (fetched and checked as in section 8, so those drawers
  are GPL); without Gorilla (`--no-gorilla`) they are `drawer-NAME`, all ours. A missing icon file is skipped (a part may not
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

## 8. Gorilla: the system's icons, and what is still to draw

**Gorilla** is AROS's icon set (`images/IconSets/Gorilla` in the AROS
repository), under the GNU General Public License. It was chosen on
8 October 2026 for OpenUp's look ("ship gorilla"): complete for a Workbench
(a drawer for every SYS: drawer, the disks, RAM, the Shell, the prefs, the
default icons), soft-shaded, at 60 pixels.

- **Fetched, never committed.** `tools/gorilla_icons.py` fetches it at the
  AROS commit it pins, checks every file it uses against `tools/gorilla.lock`
  (SHA-256), and the pack build (`build_pack.py`, `--no-gorilla` to leave it
  out) makes the GorillaIcons part.
- **New faces, the icons' own selves.** For each icon the system has (the
  SYS: drawers, OS 3.2.3's programs and prefs editors, the default icons in
  `ENVARC:Sys`), the part holds a GlowIcon with only the picture, at 85 per
  cent, and a `face` line: OpenUpTool 0.6 keeps the icon's type, default
  tool, tool types, stack and drawer window and changes only its picture,
  keeping the original in `SYS:Storage/OpenUp-Superseded` for UNINSTALL. An
  icon the machine hasn't got is left alone.
- **GlowIcons for now.** A GlowIcon has no partial transparency, so the soft
  shadows end as a solid edge or nothing. The PNGs go in too
  (`SYS:Storage/Icons/Gorilla`), with Gorilla's LEGAL and the GPL (COPYING):
  they are the set's source under the GPL, and the pictures OpenUp will use
  when it draws PNG icons (openworkbench.library, to come).
- **Licences kept apart.** Gorilla's pictures, and anything drawn from them
  (our drawers on Gorilla's drawer), are GPL and live apart from our own
  icons (MIT). The GPL covers the pictures only, not the programs beside them.

**The icon library** is `icons/LIST.csv`: 640 rows, every Amiga thing that
wants an icon (disks, drawers, prefs, the OS's tools and commodities, our
apps, file types, situations, glyphs, pointers, and the programs people
install), with what each is, what to draw, a priority (P1 ships with OpenUp
on 3.2.3) and the Gorilla picture it corresponds to. The names follow what
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
