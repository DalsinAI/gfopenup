# Release notes for the next GFOpenUp pack: the system bar's row

For whoever builds the pack after 0.8.28. Paste these sections into that pack's RELEASE-NOTES.md.

## What's new

- **GFOpenTitle 0.8** owns the system bar (gfopenprefs #193): the cog, the network icons and every program's icon are drawn in one row at the right of the screen's title bar, and the clock is in the centre of the title.
- **GFOpenNetwork 0.1** is new, in the GFOpenTitle part: the LAN and Wi-Fi icons as a commodity of their own (Exchange can show, hide or quit it), with the panel and its Network settings button. It is in `SYS:Tools/Commodities/GFOpenNetwork` and starts at boot after GFOpenTitle.
- **GFOpenTitle 0.8.1 and GFOpenNetwork 0.1.1** (gfopenprefs #194, Dale 12 Nov 15:01 "the fonts are still a bit yuk"): the clock, the cog's and the tray's menus, the help line and the network panels draw their text anti-aliased through gfopenlayout.library 19 (OLUI_AmigaText), from the screen font's outline face (sans-serif when it has none). Without library 19, gfopenfont.library or 16 pens, Text() draws it as before. The network panels are as wide as their longest line. The pack doesn't carry gfopenlayout.library: it comes with the card's own LIBS: (gfopenlayout #53).
- **Sound 0.2.4**: GFOpenSpeaker 0.2, the speaker as an item in GFOpenTitle's row (its own window when GFOpenTitle isn't running).

## Tested so far

- Lab t3-bar-20261111 (12 Nov 2026, 01:20-01:22 UK): clock centred; LAN, speaker and cog in one row; the bar keeps its colours with the pointer on it; the LAN and speaker panels and the cog menu open under their icons.
- Lab t3-olui18-gfos (12 Nov 2026, 16:00-16:06 UK, gfopenlayout 19.0, gfopenfont 1.8, 1920x1080): clock, cog menu and network panel anti-aliased, nothing cut off; crops in the project's screen-bar-20261111/bar-aa-text-*-1605.png.
- `tests/test_build_pack.py`: GFOpenNetwork is built and started by the GFOpenTitle part; the Sound part is 0.2.4.
