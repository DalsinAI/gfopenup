# Release notes for the next GFOpenUp pack: Smacker and Bink films

For whoever builds the next pack. Paste these sections into that pack's RELEASE-NOTES.md. Build it with
gfopendatatypes at or after its pull request #1 (GFOpenDatatypes 0.2-rc1), merged.

## What's new

- **GFOpenImage 0.7** brings films from 1990s games (Dale, 22 November 2026: "make sure those new datatypes get
  added to the pack").
  - `smacker.datatype` 47.1 and `DEVS:DataTypes/Smacker`: Smacker films (SMK2 and SMK4) decoded on the 68k with no
    card, with their sound. A program can also open one from memory (`DTST_MEMORY`), as a game does with films
    packed in its own archives.
  - `DEVS:DataTypes/Bink`: Bink films go to `gfopenvideo.datatype`, and FFmpeg decodes them on the services card
    through `media.decode/1`. Bink 2 does not open.
  - Both come from the GFOpenDatatypes pack's manifest, as the part's other datatypes do: no file list changed here.

## Tested so far

- `tests/test_build_pack.py`: the part is 0.7.
- gfopendatatypes #1 (CI green): the Smacker core against FFmpeg on its own test films and Might and Magic VII's 163
  Smacker films; in a copy of Instance-32, every frame of six films through `animation.datatype`, and MM7's Bink
  film's 90 frames through `gfopenvideo.datatype`.

## Known

- **On the open-ROM machine no datatype class opens from disk**, the part's older ones (cdxl, webm, gfopenvideo...)
  included. Checked in a copy of Instance-32 on 22 November 2026, after a clean boot:
  - Its Startup-Sequence assigns `LIBS:` to `SYS:Libs` and adds `GFOSBOOT:Classes` only when there is a GFOSBOOT:
    volume, so `datatypes/<class>.datatype` is not found in `SYS:Classes/DataTypes`.
  - A class opens its superclass the same way (`datatypes/animation.datatype`, `datatypes/picture.datatype`), and
    neither is on the disk. This part leaves the system classes out on purpose (they go with GFOpenLibs).
  - With both put right by hand (`Assign LIBS: SYS:Libs SYS:Classes` and the GFOpenDatatypes pack's
    animation.datatype in `SYS:Classes/DataTypes`), cdxl.datatype and smacker.datatype opened. With either one alone,
    neither did.
