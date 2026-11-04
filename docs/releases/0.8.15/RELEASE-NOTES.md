# GFOpenUp 0.8.15 (4 November 2026)

GFOpenUp 0.8.14 with one change, by Dale's decision of 4 November: **new windows open in the centre of the screen
until they are snapped, and the snapshot gadget shows in title bars, by default.**

## What's new

- **GFOpenWindows 0.12 and the Windows page 0.8** (gfopenprefs #173): centring is on and the snapshot gadget follows
  it (on), for new installs and for upgrades. The page writes `centre.v 2` beside the `centre` and
  `snapshot.gadget` lines, and only lines with that mark count as a choice: the Windows page of 0.8.14 wrote
  `centre off` on every Use or Save whether it had been touched or not, so its lines are read as never set. Switch
  either off on the Windows page and it stays off.
- GFOpenUp's pack and Installer write no Windows settings of their own, so nothing else changes: a new install has
  no settings file, and an upgrade's file has no mark (Instance-32's, copied from OpenPrefs by OLDNAMES, has no
  centre lines at all).
- GFOpenUpTool 0.8.15: the pack's version. The GFOpenWindows part is 0.12.

Everything else is as in 0.8.14 (docs/releases/0.8.14): GFOpenPrefs 2.7, the wheel scrolling the list last clicked,
Tata 0.7.0's known fixes, the open ROM's disk libraries where missing, themes and profiles, the backdrop kept on an
upgrade. Every other part is built from the same commits as 0.8.14, again on the os32-gcc16 stove (GFOpenAmigaGCC
16.2.0-open6).

## Tested

Fresh lab copies, the pack placed in DH1:GFOpenUp as Cradle places it, its own Install-GFOpenUp (Novice), then a reboot:

- **Instance-32** (OS 3.2.3 disk, GFOpenUp 0.8.11 before; lab t5-up0815-32; OS 3.2.3's Installer): installed;
  GFOpenWindows 0.12 running; its settings file is the one OLDNAMES copied (no centre lines). A new Shell asked for
  at 0,0 opened centred (640,407 on 1920 x 1080) with the snapshot pin in its title bar; a click on the pin stored
  `"CON:T" 640 407 640 200 snap`; Shift-click took it out again. The view (Advanced), the look and the backdrop
  (Cradle's AmigaChrome-Prismatic-light) are as before.
- **Instance-40** (open ROM, no OS disk, 0.8.11 before; lab t5-up0815-40; OpenInstaller 1.0.0-rc2): installed; a
  new Shell at 0,0 opened centred (640,463 on 1920 x 1200) with the pin; moved to 200,750 and pinned, it was stored
  with `snap` and reopened at 200,750; Shift-click forgot it and it opened centred again.
- Against 0.8.14's labs, the install changed the same files apart from GFOpenWindows, GFOpenPrefs, GFOpenUpTool and
  their Part files.

With centring on, places GFOpenWindows remembered without a snapshot (Remember places) are no longer used: a window
opens centred until it is snapped. Instance-32 has 72 such places from before.
