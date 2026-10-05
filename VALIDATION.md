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

## Still to check

- **The Installer's own windows** (`Install-OpenUp` and `Uninstall-OpenUp`
  run by Installer with a person clicking). The test called OpenUpTool the
  way the scripts do.
- **`httpget`** saved only 17 bytes of example.com, so the web page itself
  is to look at. The name lookup shows the network works.
- **A printed page,** to PDF and to an IPP printer, through Prefs/Printer
  (OpenUp Setup will set the printer).
- **Replacing files:** a system that already had older Open-family files,
  where the originals are kept and put back. The code does it; this test's
  machine had none, so nothing was replaced.
