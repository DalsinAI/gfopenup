# OpenUp 0.6.14: validation on an AmigaChrome instance

7 October 2026. Phase 1's test (DESIGN.md section 5), repeated with the
0.6.14 release pack (`OpenUp-0.6.14.lha`, sha256 `880835e7…573e253d`), and
the first test of OpenUp Update (phase 5).

**The machine:** a scratch copy of an AmigaOS 3.2.3 instance that had never
had OpenUp (with AmiSSL, as instances have it), its own drives and ports:
AC090 as a 68040 with FPU, an ACRTG card at 800x600, the OpenSocket card
fitted.

**How:** boots driven by a script on DH1, the host hashing every file on the
system drive before the install and after the uninstall:
1. **Install:** what `Install-OpenUp` does with its AmigaChrome defaults
   (every part but OpenRTG and OpenMUI): `DETECT`, `INSTALL` for 21 parts,
   the look's `SETTING`s, `FONT`, `TITLEBARS`, `BACKDROP`, `STARTUP`.
2. **Check, after the reboot:** the network, printing, the look, the tasks.
3. **Uninstall:** `OpenUpTool UNINSTALL`, then a reboot.

| Check | Result |
| --- | --- |
| Detect | "This is an AmigaChrome instance, with a 68040 and an FPU. RTG: an ACRTG card. Network: the OpenSocket card is fitted." |
| Install | 21 parts installed; the font, title bars and backdrop (`Open-light-strip-600.png`) set; the startup block written |
| Network, after one reboot | `resolve example.com`: two addresses; `httpget http://example.com/`: saved the page (OpenSocket's tools in SYS:Tools/OpenSocket) |
| Printing | "PASS: virtual PDF printer is ready and can never report paper out" |
| The look and the rest | OpenLook running; OpenBlanker, OpenWindows, OpenDock, OpenMenus, OpenTitle and OpenSpeaker running from OpenUp's block; OpenUp Setup's first-start questions on screen |
| Uninstall | "271 files removed, 16 put back as they were" |
| **Clean** | **All 1,604 files on the system drive identical to before the install.** (The host's `.amigachrome-meta.json` files, which hold the protection bits, have the same contents, written out differently.) |

**OpenUp Update** (`OpenUpTool UPDATE`, with a 0.6.15 test pack in
`DH1:OpenUp` and its `Cradle-Pack`, as Cradle places it):

| Check | Result |
| --- | --- |
| `UPDATE CHECK` | "OpenUp 0.6.15 is waiting in DH1:OpenUp (0.6.14 is installed)", WARN (5) |
| `UPDATE` | The "OpenUp Update" requester: "OpenUp 0.6.15 is ready to install (0.6.14 is installed now)", buttons "Not now" and "Update" |
| No `Cradle-Pack` | "No newer OpenUp from Cradle…", and on a real Amiga the GitHub releases page; OK (0) |

Not tested here: the Installer's own windows (they need clicks), and the
update's Installer run itself.
