# Release notes for the next GFOpenUp pack: the Swish backdrops

For whoever builds the next pack. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **The Swish backdrops** come in the Backdrops part (0.4), in `SYS:Prefs/Presets/Backdrops/GFOpenUp/`. They are lit silk folds and glass ribbons (gfopengadtools #125, drawn from code, our own art).
  - There are seven: `Swish-Crimson`, `Swish-Midnight`, `Swish-Pearl`, `Swish-Sunset`, `Swish-Glass`, `Swish-Glass-Light` and `Swish-Chrome`. Each has its `.tone` file (Pearl and Glass-Light are light, the rest dark).
  - Each comes at the four sizes `GFOpenUpTool BACKDROP` looks for: 1280x1024 (8-bit), 1600x1200, 1920x1080 and 1920x1200 (16-bit). Workbench scales the one nearest the screen's shape.
  - They are 83 to 288 KB at 1280x1024 and 165 to 645 KB at 1920x1080, 9.9 MB for all 28.
- **A new install's backdrop is Glass Light** (`Swish-Glass-Light`). Install-GFOpenUp's "Which backdrop?" offers it first, and a Novice install gets it. The themes' desktops follow it in the list as before. The icons are not changed.

## Tested so far

- gfopengadtools `tools/make_swish_backdrops.py` made all 28; the 1920x1080 ones match the pictures shown on 11 November to within the 16-bit dither.
- `tests/test_build_pack.py`: the part is 0.4, keeps a Swish picture with its `.tone`, and the Installer script offers Glass Light first and names `Swish-Glass-Light` for it.
- On the Pi 5 release board (17 November, RTG 1920x1080): `GFOpenUpTool BACKDROP .../Swish-Glass-Light` answered 0 and Workbench showed the picture at once. GFOpenDock keeps the old backdrop's colour behind its glass until it restarts.
- Not yet run on an Amiga: the Installer question with eight choices.
