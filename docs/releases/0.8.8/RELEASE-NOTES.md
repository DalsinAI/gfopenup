# OpenUp 0.8.8 (21 October 2026)

OpenBrowser 0.7 as the companion, and OpenRTG with a version C:Version can read.

- **OpenBrowser companion 0.7.** `--openbrowser` takes only the archive pinned in `OPENBROWSER`: OpenBrowser 0.7 from
  openamigabrowser 4717764 (0.7's version string, #43, with only a lab script after it). Over 0.6 (12a7380) it adds
  WebKit patches 0033-0035 and the engine's idle quit:
  - 0033: each JavaScript thread keeps its own 68k interpreter registers, so a web worker no longer corrupts the
    main thread's. reddit loads again (it stopped about 50 s in, when reCAPTCHA started its worker).
  - 0034: the 68k JavaScript JIT runs get/put_by_val, array lengths and calls inside loops in line.
  - 0035: the JIT's compares, new objects, polymorphic calls and scope variables, and a GC fix: since 0031 a GC at
    the wrong moment could clear live caller slots and crash.
  - WebBrowserEngine quits after 60 s idle and gives back about 120-270 MB; `ENV:WebBrowserIdleQuit` sets the
    seconds, 0 means never.

  The archive's sha256 goes into the pin once it is built.
- **OpenRTG 0.19.1.** openrtg.library has a `$VER:` string (openamigartg #103), so C:Version finds it, and its
  version goes up from 0.19 (#105) so the update brings it to machines that have 0.19.
- OpenUpTool 0.8.8. Every other part is at its repository's main, as in 0.8.7. OpenBrowser Lite (v0.4.1) and
  OpenAmigaPython (v2.7.18-ac3) stay pinned to their latest releases.
