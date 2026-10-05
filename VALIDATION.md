# OpenUp 0.1: validation on an AmigaChrome instance

5 October 2026. We: "we should start openup 0.1 so we can validate it for
amigachrome instances".

## What was tested

Phase 1's test (DESIGN.md section 5): a fresh OS 3.2.3 scratch copy, plus
the pack, reaches the network and prints after one reboot, and the
uninstaller returns it to clean.

**The machine:**
- A scratch copy of an OS 3.2.3 instance, with its own drives, a private
  install and its own ports:
  - AC090 68040 with FPU, 256 MB;
  - an ACRTG card, Workbench at 800x600 in 32-bit;
  - the OpenSocket card fitted (the network on).
- **Fresh, for OpenUp:** the 46 files of the Open family already on its
  drive were moved off it first:
  - OpenSocket;
  - OpenPrint and OpenView;
  - accontrol, ACClip and ACDrop;
  - OpenPrint's spooler.

  So it was OS 3.2.3 (with AmiSSL) as OpenUp expects to find it.

**How:** three boots, driven by a script on DH1:
1. **Install:** the calls `Install-OpenUp` makes, in order:
   - `OpenUpTool DETECT`;
   - `INSTALL` for each part;
   - `SETTING` for the look ("Open light");
   - `STARTUP`.
2. **Check, after the reboot:**
   - `resolve example.com` and `httpget` through OpenSocket;
   - OpenPrint's spooler status test;
   - `OpenLook STATUS`;
   - the GadTools gallery on screen;
   - exec's task list.
3. **Uninstall:** `OpenUpTool UNINSTALL`, then a reboot.

**The comparison:** every file on the system drive (1,575) was hashed before
the install and again after the uninstall.

## Results

| Check | Result |
| --- | --- |
| Detect | "This is an AmigaChrome instance, with a 68040 and an FPU. RTG: an ACRTG card. Network: the OpenSocket card is fitted." |
| Install | OpenUp 0.1 (3 files), OpenSocket 4.2 (23), OpenPrint 0.3 (18), AmigaChrome sharing 1.2 (5), OpenLook 0.1 (4): 54 files in the manifest |
| Network, after one reboot | `resolve example.com` answered with its two addresses, through OpenSocket |
| Printing | OpenPrint's spooler test: "PASS: virtual PDF printer is ready"; OAPSpooler running from WBStartup |
| Sharing | ACClip and ACDrop running from WBStartup |
| The look | OpenLook running from OpenUp's block in S:User-Startup, the gallery drawn in the Open theme |
| Uninstall | "54 files removed, 0 put back as they were" |
| **Clean** | **The system drive after the uninstall is identical to before the install: all 1,575 files** |

**Second run, with the background pack** (the same machine, reset to the
same 1,575 files):

| Check | Result |
| --- | --- |
| Backgrounds | 16 backdrops installed; `OpenUpTool BACKDROP` chose `Open-light-1600x1200.png` for the 800x600 screen and set it, keeping the old WBPattern.prefs |
| On screen | Workbench's desktop in the Open theme's sky, the gallery in the Open look (`docs/validation-open-look.png`) |
| Uninstall | "70 files removed, 2 put back as they were" (WBPattern.prefs in ENVARC: and ENV:). The backdrop picture is in use until Workbench has read the old setting again, so uninstall now waits for it (up to 10 seconds) |
| **Clean** | **Identical to before the install again: all 1,575 files.** This also proves that files OpenUp replaces are kept and put back |

**Third run, with OpenTypes** (5 October 2026): OpenTypes installed in
`SYS:Prefs` (2 files), listed the kinds of file from there after the reboot,
and the uninstall left the drive identical again (72 files removed, 2 put
back).

**Fourth run, with OpenCompress** (5 October 2026): `C:OpenCompress`
installed (1 file), tested a zip as good from C: after the reboot, and the
uninstall left the drive identical again.

**Fifth run, OpenUp 0.2: the look in one go** (5 October 2026): OpenLook
0.3 with the five themes (Open, Graphite, Ember, Clear, Classic), the
screens' font set to CGTriumvirate 13 (`OpenUpTool FONT`), title bars two
pixels taller (`OpenUpTool TITLEBARS 2 2`, IControl's Title Bar Extra
Height), and OpenCompress 0.3 as a Workbench program in SYS:Utilities with
its own icon. Installed (98 files in the manifest), after the reboot the
Workbench came up in the Open look with the new font and title bars,
`OpenCompress t` passed an LhA archive from the Shell's path and its window
opened an ADF, and the uninstall left all 1,575 files on the drive
identical, Font.prefs and IControl.prefs (none before) included.

## Still to check

- **The Installer's own windows** (`Install-OpenUp` and `Uninstall-OpenUp`
  run by Installer with a person clicking). The test called OpenUpTool the
  way the scripts do.
- **`httpget`** saved only 17 bytes of example.com, so the web page itself
  is to look at. The name lookup shows the network works.
- **A printed page,** to PDF and to an IPP printer, through Prefs/Printer
  (OpenUp Setup will set the printer).
- **Replacing older Open-family files** on a system that has them (the
  keep-and-restore path itself is proven by WBPattern.prefs, above).
