# OpenUp: where the stack stands

4 October 2026. Dale asked: "an assessment of the 'openup' stack".

## In short

- **The idea is sound, and on AmigaChrome the base is real:** networking,
  printing, mail reading and the sharing tools already run on OS 3.2.3.
- **The pack itself is not written yet:** the Installer, machine detection,
  the manifest, the uninstaller and Setup.
- **Of the 18 parts below:**
  - five work today;
  - three work in part;
  - the rest are designed but not built.
- **A PiStorm and a real Amiga** are blocked on two drivers (network and
  RTG) and on testers with hardware.
- **So OpenUp can ship first for AmigaChrome instances,** and grow on real
  machines part by part, as its phase table already says.

## The parts

| Part | What OpenUp needs from it | State today | AmigaChrome | PiStorm | Real Amiga |
| --- | --- | --- | --- | --- | --- |
| **The pack:** Installer, detection, manifest, uninstaller, Setup, startup switches | The pack itself | Designed (DESIGN.md) | Not built | Not built | Not built |
| **OpenSocket** (`bsdsocket.library` 4.2) | Networking | Working: 53 of 53 qualification checks, on Instances 23 and 24 | Yes (host backend) | No: OpenSocketDirect isn't built. Roadshow, or Emu68's own stack, stays | No: the same |
| **OpenPrint and OpenView** | Printing to PDF and IPP, and viewing | Working on OS 3.2.3: GadTools window, Installer, OS 3.2 icons | Yes | Should work (needs the network for IPP) | The same; a paper print is still to test |
| **OpenMail** | Mail | Reads IMAP (Gmail with an app password). Sending, POP3 and the desk are being built now | Yes, reading | Needs the network | Needs the network |
| **AmigaChrome's sharing tools** (accontrol, ACClip, ACDrop) | The clipboard and files shared with the PC | Working | Yes | Not used | Not used |
| **AmiSSL** (another project's, Apache 2.0) | TLS for mail, printing and the browser | Working; it may be shipped with its notices | Yes | Yes | Yes |
| **OpenRTG** | Screens on every monitor, boot to RTG | 0.2: the library, with its modes in the display database, on ACRTG. No screens of its own yet, no `RESIDENT`, and ACRTG is its only driver | In part (Picasso96 still opens the screens) | No driver | No AGA card or board drivers yet |
| **OpenGadTools** (themes and the look) | The look, and the controls our programs use | Themes, their specification and a checker done; the library starts now; the look patch is not built | Starting | Starting | Starting |
| **Smooth text** | Antialiased fonts | Designed. AAText (MIT, another project's) can be offered now | Offer AAText | Offer AAText | Offer AAText |
| **OpenBrowser** | The web, and Microsoft sign-in for OpenMail | The JavaScript engine runs, and a text browser works; WebCore is in progress; its nine libraries build | In part | Fits (Emu68 has an FPU) | 68040 or 68060 with FPU only |
| **OpenMUI** (Zune, and the `muimaster.library` passthrough) | MUI programs without buying MUI | Placeholder | — | — | — |
| **OpenTypes** (in OpenPrefs) | The right program on double-click | Designed | — | — | — |
| **OpenCompress** | Archives | Designed | — | — | — |
| **OpenFiles** | File manager | Spec 0.3 | — | — | — |
| **SDL and Mesa** | Games and 3D | Designed | — | — | — |
| **OpenUSB** | USB | Designed, on the open Poseidon | — | Through Emu68's `xhci.device` | Through the cards' drivers |
| **OpenUp Picks and Update** | Programs to add, updates | Designed (DESIGN.md section 6) | — | — | — |
| **The appliance** | The machine that ships it | Hardware brief and boot-screen design; no build | — | — | — |

## The risks, largest first

1. **Breadth.** Eighteen parts move at once, and only a few are finished.
   Shipping each one as it lands (phase 2 of the design) keeps OpenUp
   useful early.
2. **The Zune port** is the biggest unknown. "MUI programs without buying
   MUI" is a headline promise, and nobody has tried `zunemaster.library` on
   OS 3.2 yet. A short trial port will show its size before it's promised
   anywhere.
3. **The look patch** changes how Intuition draws every window. "Off is
   off" must hold, three details are still to check on OS 3.2.3
   (OpenGadTools section 2d), and it must be tried on real hardware before
   it's on by default.
4. **No real hardware has been tested.** We need testers:
   - a PiStorm A1200 (Pi 4 or CM4);
   - an A1200 with and without an RTG card;
   - a Zorro machine.
5. **Startup files.** An Installer that changes them on many kinds of
   machine is a support risk. The manifest, the marked block, the
   uninstaller and a test matrix are what keep it safe, so they come
   first, not last.
6. **Networking on real machines.** OpenSocketDirect has to be built, or
   OpenUp must rely on what is installed: Roadshow, or Emu68's own
   `bsdsocket.library` on a PiStorm. That choice is Dale's.
7. **Licences:**
   - fine: OS 3.2.3 is the user's own copy; AmiSSL is Apache 2.0; AAText
     is MIT; Poseidon is APL 1.1; DejaVu has the Bitstream Vera licence;
   - **Picks:** each program's licence must be checked before it is
     offered.

## What I'd do, in order

1. **OpenUp 0.1 for AmigaChrome instances:**
   - **The pack:** the Installer with the manifest, the marked startup
     block, the uninstaller, and detection of the Dalsin boards;
   - **What it installs:** OpenSocket 4.2, OpenPrint and OpenView,
     OpenMail (once it sends), the sharing tools, AmiSSL, and AAText as an
     option.
   - Cradle's OS 3.2.3 recipe then applies it, which is phase 4's
     "a new instance comes up with the Open family".
2. **OpenGadTools:** the library and themes, used by OpenMail now; then the
   look patch. OpenUp 0.2 brings the themes.
3. **A Zune trial**, to settle OpenMUI early.
4. **OpenRTG:** its own screens, then `RESIDENT`, the boot-to-RTG option.
5. **The PiStorm path:**
   - a decision on networking (OpenSocketDirect, or Emu68's stack);
   - a tester;
   - then OpenUp on a PiStorm (phase 3).
6. **The rest** as each part lands.
