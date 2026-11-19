# Parts can put files on the Media drive

The standard build's Media drive (DH4:) is where pictures and music go (Dale, 17 November 2026).

- GFOpenUpTool 0.8.22 knows the Part file conditions `when MediaDrive` and `unless MediaDrive`, as it knows
  `DemosDrive`. It reads the DOS list, so no "Please insert volume Media" requester comes up where there is none.
- A part uses them to put its files on MediaFiles: where the drive is there. The first is GFOpenSynth's MIDI
  collection, which goes in `MediaFiles:midi`.
