# GFOpenUp 0.8.7 (20 October 2026)

Three fixes from installing 0.8.6 on Instance-32, which runs the GFOpenKick ROM.

- **GFOpenLibs and the GFOpenKick ROM.** GFOpenUpTool DETECT now tells the GFOpenKick ROM by its header mark ('OKR1' at
  $F80014), and the Installer reads it. On GFOpenKick, utility.library and input.device, which OS 3.2's LoadModule
  loads from S:Pre-LoadModule-Startup, are left out. An S:Pre-LoadModule-Startup an earlier GFOpenUp wrote is taken
  out. Their parts say `needs Kickstart`, so GFOpenUpTool INSTALL leaves them out on GFOpenKick too. Under 0.8.6 that
  startup stopped Instance-32's boot with "device not mounted". The startup also comes only with one of those two
  now: mathieeedoubbas.library or OpenRexx alone no longer installs it.
- **Your Font and IControl prefs stay.** OLDLOOK no longer puts back the stock Font.prefs or deletes IControl.prefs.
  Up to 0.8.6 it did that wherever their bytes still matched what GFOpenUp 0.8.1 wrote, even if the user had chosen
  them since. The originals stay in SYS:Storage/GFOpenUp-Superseded for Uninstall-GFOpenUp.
- **GFOpenRTG on a Novice upgrade.** GFOpenRTG is ticked where GFOpenUp's list has the part, and on the GFOpenKick ROM with
  an ACRTG board, where Workbench runs on GFOpenRTG already. A Novice install then brings the pack's gfopenrtg.library
  0.19 instead of leaving 0.17. On GFOpenKick the Installer doesn't ask about the screen mode.
- **OpenBrowser companion.** `--openbrowser` takes only the archive pinned in `OPENBROWSER`: OpenBrowser 0.6 from
  openamigabrowser 12a7380 (WebKit patches 0001-0032: the 68k JavaScript JIT, the HTTP disk cache, XML through
  OXP), archive sha256 2f697d1e…
- GFOpenUpTool 0.8.7. Every other part is at its repository's main, as in 0.8.6. OpenBrowser Lite (v0.4.1) and
  OpenAmigaPython (v2.7.18-ac3) stay pinned to their latest releases.
