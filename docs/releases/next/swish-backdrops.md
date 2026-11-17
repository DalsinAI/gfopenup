# Release notes for the next GFOpenUp pack: the Swish backdrops

For whoever builds the next pack. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **The Swish backdrops** come in the Backdrops part (0.4), in `SYS:Prefs/Presets/Backdrops/GFOpenUp/`. They are lit silk folds and glass ribbons (gfopengadtools #124, drawn from code, our own art).
  - There are seven: `Swish-Crimson`, `Swish-Midnight`, `Swish-Pearl`, `Swish-Sunset`, `Swish-Glass`, `Swish-Glass-Light` and `Swish-Chrome`. Each has its `.tone` file (Pearl and Glass-Light are light, the rest dark).
  - Each comes at the Ethereal ones' 16 screen sizes, from 640x480 to 1920x1200: 8-bit up to 1280 wide, 16-bit above.
  - They are 33 to 91 KB at 640x480 and 165 to 645 KB at 1920x1080, 22.9 MB for all 112.
- **A new install's backdrop is Glass Light** (`Swish-Glass-Light`). Install-GFOpenUp's "Which backdrop?" offers it first, and a Novice install gets it. The themes' desktops follow it in the list as before. The icons are not changed.

## Tested so far

- gfopengadtools `tools/make_swish_backdrops.py` made all 112; the 1920x1080 ones match the pictures shown on 11 November to within the 16-bit dither.
- `tests/test_build_pack.py`: the part is 0.4, keeps a Swish picture with its `.tone`, and the Installer script offers Glass Light first and names `Swish-Glass-Light` for it.
- Not yet run on an Amiga: the Installer question with eight choices, and `GFOpenUpTool BACKDROP .../Swish-Glass-Light`.
