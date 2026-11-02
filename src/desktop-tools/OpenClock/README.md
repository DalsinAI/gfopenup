# GFOpenClock 0.1

The clock on the desktop, in the GFOpen family's look: an analogue face drawn from
GFOpenLook's theme, a digital line under it, and the date. It takes over
`SYS:Utilities/Clock` (GFOpenUp's `RECONCILE` retires the OS's one).

    GFOpenClock [DIGITAL | ANALOGUE] [SECONDS]

## What it shows

- **Analogue**, **Digital** or **Both** (the View menu). The face follows the
  window when it is resized.
- **Seconds** on or off, **24 hour** or am/pm, and the **date** line.
- *Save settings* keeps them in `ENVARC:GFOpenClock/Clock`, so the window comes
  back as it was left; `ENV:GFOpenClock/Clock` is read at the start.

## How it is built

68040, no FPU and no maths library: the hands come from a table of sines for the
60 positions, scaled by 1024, so an A1200 with no FPU draws them as fast as an
AC090 does. The face, the marks and the text are the theme's colours through
`ogt_draw`, so light and dark follow GFOpenLook with no settings of its own.

A tick comes from `timer.device` four times a second; the window is drawn again
only when the time shown has changed.

## Build

    OGT=/path/to/gfopengadtools src/desktop-tools/OpenClock/build.sh
