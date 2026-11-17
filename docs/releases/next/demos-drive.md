# GFOpenDemos on the Demos drive

The standard build's drives are System, Work (for applications), Games, Demos and Media (Dale, 17 November 2026).

- GFOpenDemos (part 0.2) installs in `Demos:GFOpenDemos` where a volume named Demos is mounted, and in
  `SYS:Utilities/GFOpenDemos` elsewhere, as before. Each home has its own launch scripts.
- Where the Demos drive is there, an earlier install's files in `SYS:Utilities/GFOpenDemos` are taken out, so the
  demos are in one place.
- GFOpenUpTool 0.8.21 knows the Part file conditions `when DemosDrive` and `unless DemosDrive`. It reads the DOS
  list, so no "Please insert volume Demos" requester comes up where there is none.
- The Installer counts GFOpenDemos as installed in either home.
