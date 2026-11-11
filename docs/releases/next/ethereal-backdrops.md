# Release notes for the next GFOpenUp pack: the Ethereal backdrops

For whoever builds the pack after 0.8.28. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **The Ethereal backdrops** come in the Backdrops part (0.3), in `SYS:Prefs/Presets/Backdrops/GFOpenUp/`. They are the Prism picture made softer and lighter: glass loops with fine strands of light over an airy sky (gfopengadtools #101, drawn from code, our own art).
  - There are three: `Ethereal-light`, `Ethereal-dark` and `Ethereal-rose`. Each has its `.tone` file.
  - Each comes at 16 screen sizes, from 640x480 to 1920x1200, so Workbench shows one at the screen's own size and scales nothing.
  - Up to 1280 wide they are 8-bit, 256 colours with an ordered dither. Wider than that they are 16-bit, RGB565 with the same dither. They are 56 KB at 640x480 and 297–627 KB at 1920x1080, 10.5 MB for all 48.

## Tested so far

- gfopengadtools `tools/make_backdrops.py` made all 48, and `build_pack.backdrops_part` keeps them (the Background Pack's filter leaves them in).
- `tests/test_build_pack.py`: the part is 0.3.
