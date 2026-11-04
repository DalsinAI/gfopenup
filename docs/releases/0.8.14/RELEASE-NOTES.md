# GFOpenUp 0.8.14 (4 November 2026)

GFOpenUp 0.8.14 brings in the day's work: the wheel, centred windows with a snapshot pin, GFOpenPrefs 2.7, Tata's
known fixes, and what an Amiga on the open ROM was missing. Every part GFOpenUp compiles is built from its
repository's main of 4 November on the os32-gcc16 stove, **GFOpenAmigaGCC 16.2.0-open6** (gfopenasm 1.1.0 as the
assembler), with no compiler cache.

## What's new

- **GFOpenPrefs 2.7** (gfopenprefs #168): a page that can't start says on a requester which library it needs, and
  the window stays on the page before, instead of GFOpenPrefs ending with no word. Theme reads desktop profiles from
  the old drawer (SYS:Prefs/Presets/OpenPrefs) when the new one has none, and Theme and Pointer say what is missing
  when nothing is installed. The Windows page has the centring, the snapshot gadget and "Wheel scrolls"; the Input
  page has Mouse wheel and Middle button (on) and Joy-port wheel (off).
- **GFOpenWindows 0.11** (gfopenprefs #169-#171):
  - new windows can open in the centre of the screen until they are snapped, with a MUI-style snapshot gadget in
    title bars (the pin hollow until snapped, solid after; Shift-click forgets). Both are **off by default**;
  - the wheel scrolls the scroller last clicked in each window (or the one under the pointer), as a drag would;
    programs that scroll themselves are left alone. **On by default**, for new installs and upgrades;
  - the edge pointers' bitmaps are no longer held in Chip RAM.
- **Tata 0.7.0 and its known fixes** (gfopentata #33, gfopenup #188): `S:Tata.fixes`, fix-ups put right in memory
  as a program loads, Lupus Reditus' copper list first. Your own go in ENVARC:Tata/KnownFixes; "Tata FIXES OFF"
  turns them off.
- **The open ROM's disk libraries** (#186): on the GFOpenKick ROM, the GFOpenUp part puts in iffparse, diskfont,
  commodities and lowlevel.library and clipboard, serial and parallel.device, built from amicradle-guest as the
  OpenKick boot disk builds them, **each only where none is**. Under OS 3.2.3's ROM nothing changes.
- **Themes and profiles** (#186): the themes go in SYS:Prefs/Presets/Themes and the desktop profiles in
  SYS:Prefs/Presets/GFOpenPrefs, so the Theme page has its choices. Nothing is applied until one is picked.
- **GFOpenUpMenu 0.1.7** (#183): Ctrl+Alt+Help (Ctrl+Alt+F12 on a PC keyboard) opens GFOpenMonitor.
- **GFOpenMeters 0.2.9** (gfopenbench #74), in GFOpenBench: no Chip RAM for its text.
- **An upgrade keeps your backdrop** (this release): over an OpenUp from before the GF names (0.8.11, as on
  Instance-32), the Installer used to put the theme's backdrop over the one you had. GFOpenUpTool OLDNAMES now keeps
  a copy of WBPattern.prefs, and `BACKDROP <prefix> KEEP` puts your picture back: as it was when it is still there,
  or the same picture from GFOpenUp's drawer when it was one of the old OpenUp's.
- GFOpenUpTool 0.8.14: the pack's version. The GFOpenWindows part is 0.11. GFOpenLibs' input.device is 50.5 (the
  wheel), still off unless picked, and left out on the GFOpenKick ROM, which has its own.

Upgrading from an OpenUp before the GF names works as in 0.8.13: OLDNAMES keeps a copy of the old list and
S:User-Startup in SYS:Storage/GFOpenUp-Superseded/OpenUp-before-GF and copies the old settings to their GF names (the
view, the look, the dock, the windows' places), then the old OpenUp is taken out with its own uninstaller.

Not in this pack: gfopenprefs #165 (GFOpenInput 0.1.4, the Monitor hotkey switch on the Input page) was still open
when it was built; the hotkeys are on and can be turned off in Exchange (GFOpenUp Hotkeys). The wheel and the
middle button reach programs through the Team's input.device 50.5, which comes with the next GFOpenKick ROM
(full32); GFOpenWindows' settings are in place before that.

## Tested

In fresh lab copies, with the pack placed in DH1:GFOpenUp as Cradle places it and its own Install-GFOpenUp (Novice):

- **Instance-32** (OS 3.2.3 disk, open ROM, GFOpenUp 0.8.11 before; labs t5-up0814-32, -32b, -32c), with OS 3.2.3's
  Installer: "GFOpenUp 0.8.14 is installed"; the old OpenUp taken out. After a reboot: the view (Advanced) and the
  look (Open light) carried over by OLDNAMES; GFOpenPrefs 2.7 opens every one of its 23 pages with nothing on the
  Shell; Theme lists the eight themes and three profiles; Windows shows "Wheel scrolls: Area clicked", centring off
  and the snapshot gadget off, and no title bar has the gadget; Input shows Mouse wheel and Middle button on;
  `Tata FIXES` lists lupus-reditus-copper-end, on, from S:Tata.fixes; GFOpenWindows 0.11, GFOpenUpTool 0.8.14.
- **Instance-40** (open ROM, no OS disk, GFOpenUp 0.8.11 before; labs t5-up0814-40, -40b), with OpenInstaller
  1.0.0-rc2 (the open ROM has no Installer): installed; every GFOpenPrefs page opens, including the nine that
  needed iffparse.library; themes and profiles listed; Lupus fixed; the disk libraries and devices it already had
  (from Install-OpenKick) were left alone, and with iffparse.library and serial.device taken away first, the
  GFOpenUp part put both in (47.100 and 45.2, listed as new in the manifest).

Found and fixed in the labs: the backdrop on an upgrade (above). In lab t5-up0814-32c, with the final pack, Instance-32's
own backdrop (AmigaChrome-Prismatic-light, Cradle's) came back as it was.
Still open, cosmetic: over 0.8.11 the dock's buttons for SYS:Utilities/OpenClock and OpenCalc show the gear picture
after the upgrade, though both programs are still there; the old "Look" button points at SYS:Prefs/OpenLook,
which 0.8.11 no longer had either.
