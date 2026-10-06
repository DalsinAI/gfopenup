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
| Drawers (OpenUp, OpenPrint, OpenBrowser) | Blue cabinet, badge at its foot | Top drawer pulled open, files showing; the badge plays its own second frame |
| OpenFiles | Two panes, a file chosen on the right | The choice moves to the left pane |
| OpenView | Picture, magnifier beside it | Magnifier over the picture |
| OpenEdit | Pad and pencil | Pencil moves on, a fresh line written |
| OpenPlay | Film with a play button | Play turns to pause |
| OpenBrowser | Globe in its ring | Meridians turn |
| OpenCompress | Box, arrow out | Lid lifts, arrow further out |
| Printers and Queue | Page in the tray | Page further out, light on |
| Install-OpenUp | Arrow over the box | Arrow sinks into it |
| Dock | Four buttons, one raised | The next button pops up |
| Menus | Menu, a row lit | Row lower down lit, the pointer follows |
| Sound | Speaker, three waves | Four waves, louder |
| TitleBar | Title bar and clock | Clock hands move on |
| Look | Window of swatches, brush | Brush dips into the red, swatch chosen |
| Windows | Three windows stacked | Front window slides out and goes active |
| OpenTypes | Documents, arrow to a program | Arrow moves on |
| OpenUp-Setup | Box and wand | Star flares and sparkles |
| ZuneTest | A window of gadgets | A gadget pressed |

## 2. Format

- **OS 3.5 GlowIcon** (FORM ICON with FACE and two IMAG chunks), written by
  amigachrome's `scripts/amiga_icon.py` (`write_os35`). The file also carries
  the classic DiskObject with a 4-colour picture, so OS 3.1 still shows
  something.
- **64 x 64 pixels.** Both frames are the same size.
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

Our look is AmigaOS 4.1's, in our own artwork, so the icons sit with the
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
| The drawing script | `tools/open_icons_art.py`: one `icon_NAME(f=0)` function per icon, `f` the frame (0 at rest, 1 chosen), listed in `ICONS` |
| The pictures | `icons/open/NAME.png` and `icons/open/NAME-selected.png`, 64 x 64 RGBA, committed |
| The pack's use of them | `tools/build_pack.py`, `open_icons()`: its jobs list maps each icon file a part makes to a picture name |
| The icon codec and layout | amigachrome `scripts/amiga_icon.py` and `scripts/icon_theme.py` (passed in with `--amigachrome`) |

Redraw everything with:

```bash
python3 tools/open_icons_art.py icons/open    # needs Pillow
```

OpenBrowser draws its own the same way in openamigabrowser's
`tools/make-icons.py` (art in `icons/art/`, with `-selected` frames).

## 5. How the pack build picks them up

`build_pack.py` makes every part, then calls `open_icons()` with:

- **jobs:** a list of (icon file, picture name). For each job it reads the
  icon the part made, loads `NAME.png` and `NAME-selected.png`, hardens the
  alpha, dithers both frames, adds the glow to the second, and writes the
  icon back as a GlowIcon. A missing icon file is skipped (a part may not
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

1. Add `icon_NAME(f=0)` to `tools/open_icons_art.py`, drawing both frames,
   and list it in `ICONS`.
2. Run the script and commit `icons/open/NAME.png` and `NAME-selected.png`.
3. Add the icon file the part makes, and its picture name, to the jobs list
   in `open_icons()`; add it to `drawers` if it lives in a drawer of our own.
4. Build the pack and look at both frames (ACBuild's Icons tab, or
   `amiga_icon.pictures()` on the built `.info`).
5. Put the new icon in the table in section 1.
