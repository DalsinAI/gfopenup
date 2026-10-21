# OpenUp 0.8.7 (20 October 2026)

Three fixes from installing 0.8.6 on Instance-32, which runs the OpenKick ROM.

- **OpenLibs and the OpenKick ROM.** OpenUpTool DETECT now tells the OpenKick ROM by its header mark ('OKR1' at
  $F80014), and the Installer reads it. On OpenKick, utility.library and input.device, which OS 3.2's LoadModule
  loads from S:Pre-LoadModule-Startup, are left out. An S:Pre-LoadModule-Startup an earlier OpenUp wrote is taken
  out. Their parts say `needs Kickstart`, so OpenUpTool INSTALL leaves them out on OpenKick too. Under 0.8.6 that
  startup stopped Instance-32's boot with "device not mounted". The startup also comes only with one of those two
  now: mathieeedoubbas.library or OpenRexx alone no longer installs it.
- **Your Font and IControl prefs stay.** OLDLOOK no longer puts back the stock Font.prefs or deletes IControl.prefs.
  Up to 0.8.6 it did that wherever their bytes still matched what OpenUp 0.8.1 wrote, even if the user had chosen
  them since. The originals stay in SYS:Storage/OpenUp-Superseded for Uninstall-OpenUp.
- **OpenRTG on a Novice upgrade.** OpenRTG is ticked where OpenUp's list has the part, and on the OpenKick ROM with
  an ACRTG board, where Workbench runs on OpenRTG already. A Novice install then brings the pack's openrtg.library
  0.19 instead of leaving 0.17. On OpenKick the Installer doesn't ask about the screen mode.
- **OpenBrowser companion.** `--openbrowser` takes only the archive pinned in `OPENBROWSER`: OpenBrowser 0.6 from
  openamigabrowser 12a7380 (WebKit patches 0001-0032: the 68k JavaScript JIT, the HTTP disk cache, XML through
  OXP), archive sha256 2f697d1e…
- OpenUpTool 0.8.7. Every other part is at its repository's main, as in 0.8.6. OpenBrowser Lite (v0.4.1) and
  OpenAmigaPython (v2.7.18-ac3) stay pinned to their latest releases.
