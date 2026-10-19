# OpenUp 0.8.5 (19 October 2026)

Every part built with OpenAmigaGCC 16.2.0-open3, and nothing of the vendor NDK in any of them.

- **One compiler.** Every part, those that took the GCC 6.5 stove until now included, is built with the
  os32-gcc16 stove, which is OpenAmigaGCC 16.2.0-open3 on OpenNDK: our own headers and our own amiga.lib
  (libamiga.a) in every multilib. No part links Commodore's amiga.lib any more.
- **Windowed GL works again.** GL.module's present hook is back to the two fields every GL program was
  built with (openamigartg #101): a GL program in a window, and OpenTensor's Window Breakout scene, run at
  full speed instead of hanging.
- **Newer parts.** OpenFiles 0.4.1, OpenGPU 0.16, OpenMenus 0.4.5, OpenSocket 4.4 (bsdsocket.library),
  OpenUp Setup 0.1.4 and OpenWindows 0.9.1, each part's version now the one its program says.
- **OpenPuTTY** is built for the 68040, as every other part is.
