# GFOpenUp 0.8.26-rc1 (9 November 2026) -- release candidate 1

This pack is GFOpenUp 0.8.25 with the parts merged since then: the blanker's roll call, menus drawn off screen, the drawer menu drawn off screen, smaller printed PDFs, and the installer fixes (Reinstall, a safe Uninstall, and the Setup wizard's step-2 hang).

**Not in this pack: 0.8.26's SDL2.module** (gfopenrtg #217, gfopenup #213, `docs/releases/0.8.26/`). That pack (`e693f86c...`) was made on another machine, and neither it nor the module (`4e98af54...`) was reachable here. This candidate is built on 0.8.25, so `LIBS:GFOpenGPU/SDL2.module` is still 0.8.25's (`8d6e9570...`). The final 0.8.26 should have both; the next candidate can take the module from that pack.

## What's new

- **GFOpenBlanker's roll call says AmiCradle** (gfopenprefs #186). The first and last cards show the AmiCradle wordmark, and the sign-off reads "The AmiCradle Team" once, not twice. GFOpenBlanker stays 1.4.
- **GFOpenMenus 0.4.8** (gfopenprefs #187). Each menu panel is drawn off screen and then copied to the screen, so menus no longer flicker when they open or when the highlight moves.
- **The drawer window's menu (the three dots) is drawn off screen** (gfopengadtools #100), and redrawn only when something changes. It is in GFOpenBench, which has the drawer windows built in. GFOpenBench stays 0.1.44.
- **Smaller printed pictures** (gfopenprint #48). GFOpenView and OAVWorker scale a picture down to 300 dpi at its printed size, and never up. They write it Flate-compressed. A 3840x2228 picture on A4 was a 25 MB PDF; it is now about 4 to 5 MB. When a printer stops taking a job (less than 64 KiB in 30 seconds), the upload ends and says so. Upload progress moves every 64 KiB.
- **Reinstall, and an Uninstall that asks** (gfopenup #214, GFOpenUpTool 0.8.17).
  - When GFOpenUp is installed already, Install-GFOpenUp offers Reinstall. It is the default when the pack is the version installed. Reinstall puts every installed part in again, writes back missing or changed files, and keeps settings and startup lines.
  - Uninstall-GFOpenUp asks first at every level, Novice too, and Abort changes nothing. On the GFOpenKick ROM it warns that the desktop and the programs go too, and points to Reinstall instead.
  - INSTALL records each file's size and CRC-32. UNINSTALL leaves a file that has changed since GFOpenUp wrote it.
- **GFOpenUp Setup 0.1.8: step 2 no longer locks the Amiga** (gfopenprefs #188). Advanced, Next, Back, Next on "Your desktop" made a ring of the window's gadgets, and Intuition followed it forever. Both copies of the wizard have the fix: `SYS:Prefs/GFOpenUp-Setup`, and the copy built into GFOpenBench, which is the one WBStartup runs on GFOS.
- Install-GFOpenUp also moves the old OpenPNG, OpenTGA and OpenGIF datatype descriptors to SYS:Storage when it installs GFOpenImage (gfopenup #202: merged before 0.8.25, but not in its pack). Uninstall puts them back.

## Parts changed against 0.8.25

| Part | Change |
|---|---|
| GFOpenBlanker | still 1.4: `Tools/Commodities/GFOpenBlanker` |
| GFOpenMenus | 0.4.7 to 0.4.8: `C:GFOpenMenus` |
| GFOpenBench | still 0.1.44: `System/GFOpenBench` and `System/OpenBench` (the same file), with gfopengadtools #100 and gfopenprefs #188 |
| GFOpenPrint | still 0.4: `C:GFOpenPrint`, `C:GFOpenView`, `C:OAVWorker`, and the copies of GFOpenView in the drawer and at `SYS:Utilities/MultiView` |
| GFOpenUpSetup | 0.1.7 to 0.1.8: `Prefs/GFOpenUp-Setup` |
| GFOpenUp | 0.8.25 to 0.8.26-rc1: `C/GFOpenUpTool` (0.8.17) and its copy in the part, `Install-GFOpenUp` and both copies of `Uninstall-GFOpenUp` (from `pack/` at a844c0f), the readme, and the part |

`tools/build_pack.py` now names GFOpenMenus 0.4.8 and GFOpenUpSetup 0.1.8.

## How it was built

The pack is 0.8.25's released archive (sha256 `341278af...`). Only the 19 members above changed; the other 2,547 are byte for byte 0.8.25's. The script that made it (`mkpack.py`, beside the build) checks the base archive's sha256, that each new program is a hunk file that differs from 0.8.25's, and that each version line it changes reads as expected.

Each program was built from its repository's main with the os32-gcc16 stove, as the 0.8.25 pack's were. GFOpenBlanker rebuilt this way from the commit before #186 is byte-identical to 0.8.25's, and so is GFOpenBench from the commit before gfopengadtools #100, so in those two only the merged changes differ.

| File | From | Bytes | sha256 |
|---|---|---|---|
| GFOpenBlanker | gfopenprefs `86a1973`, `Blanker/build.sh` | 17,304 | `5b58da41...` |
| GFOpenMenus | gfopenprefs `86a1973`, `Menus/engine/build.sh` | 87,908 | `1e1f7ace...` |
| GFOpenBench | gfopenbench `2d42bd7`, `desktop/build-amiga.sh` (gfopengadtools `01c1fd9`, gfopenprefs `86a1973`, gfopenfilemanager `c871819`, gfopentata `1834f63`) | 344,048 | `ea6104f3...` |
| GFOpenPrint | gfopenprint `5405009`, Cradle stove_build | 81,824 | `ad9255f3...` |
| GFOpenView | the same | 102,060 | `1eef898d...` |
| OAVWorker | the same | 93,656 | `0a91fdd7...` |
| GFOpenUp-Setup | gfopenprefs `86a1973`, `Setup/build.sh` | 33,020 | `fb698ad1...` |
| GFOpenUpTool | gfopenup `a844c0f`, Cradle stove_build | 109,060 | `e5f4b477...` |

0.8.25's GFOpenPrint, GFOpenView and OAVWorker were older builds than gfopenprint main before #48, so these three also bring gfopenprint's other merged changes since then, such as GFOpenView's ARexx port (#47). The spooler, the driver, oapspool.device, OAPPrinters and OAPDiscover are 0.8.25's.

GFOpenDrawer (`SYS:Utilities/GFOpenDrawer`, the standalone drawer program) is 0.8.25's. A rebuild would bring a month of other gfopengadtools changes, and the drawer windows people use are GFOpenBench's.

| | |
|---|---|
| Archive | `GFOpenUp-0.8.26-rc1.lha`, 63,353,774 bytes |
| sha256 | `bd1f91b5743f692841426a38a7f6eae5b0101db14fe9bdafa4c0de3257de9b01` |

## Tested

- `lhasa -t`: all 2,566 files test clean. Against 0.8.25, exactly the 19 members above differ, and none was added or removed.
- Lab `t5-up0826rc1-20261109`, a copy of Instance-40 (GFOpenKick ROM, GFOpenBench desktop, 1920x1200 RTG), with the pack in DH1:GFOpenUp:
  - `GFOpenUpTool INSTALL` of GFOpenUp, GFOpenBlanker, GFOpenMenus, GFOpenBench, GFOpenPrint and GFOpenUpSetup changed exactly the new files. Then a reboot.
  - **Blanker:** `GFOpenBlanker NOW`. The first card shows the AmiCradle wordmark over "A homage", and the last shows it over "The AmiCradle Team", once.
  - **Menus:** Right Amiga+Space opened the desktop's menus, and the arrow keys moved through the titles and opened Window > Snapshot.
  - **Drawer menu:** in the System drawer, the three-dots button opened the whole menu (sort, show all files, icon size, thumbnails).
  - **Print to PDF:** a 3840x2228 PNG (16.6 MB) opened in GFOpenView, then ARexx `SAVEPDF`. The PDF is 5,056,097 bytes and took about 6 seconds. It holds one 2180x1265 Flate image at 300 dpi (`pdfimages -list`), and Ghostscript renders it.
  - **Setup wizard:** the copy WBStartup started (GFOpenBench's). Advanced, Next, pick Modern, Back, Next, Back, Next: no lock. The list kept Modern, the agent answered throughout, and Later closed it.
  - **Reinstall:** Install-GFOpenUp with OS 3.2's Installer 47.19 at Novice said "GFOpenUp 0.8.26-rc1 is installed already, so this reinstalls it". Proceed reinstalled 44 parts with no problem in T:GFOpenUp.log.
- Not tested: the flicker itself (single-frame screenshots can't show it); the mouse-held right-button menu (the lab's input sends press and release together); a printer that stalls; Uninstall-GFOpenUp.

## Known issues

- 0.8.25's known issues still stand.
- 0.8.26's SDL2.module isn't in this candidate (see the top).
