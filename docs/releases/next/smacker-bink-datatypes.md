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

- **On the GFOpenKick ROM, GFOpenImage's datatypes open from disk.** Until now none did there (Dale, 22 November
  2026: "need the startup sequence fixed i think"). A class opens its superclass as `datatypes/animation.datatype`
  (or `picture.datatype`, `sound.datatype`), and on an open-ROM machine none of these was on the disk or found in the
  ROM by that name. `LIBS:` didn't reach `SYS:Classes` either on a machine whose Startup-Sequence was made before
  amicradle's 17 November one.
  - The part puts the GFOpenDatatypes pack's eight system classes (picture, ilbm, sound, 8svx, text, ascii,
    animation, anim) in `SYS:Classes/DataTypes`, on the GFOpenKick ROM only, and only where none is. Kickstart
    3.2.3's own classes stay as they are. The library itself still stays out.
  - Its startup line, on the GFOpenKick ROM only, is `Assign >NIL: LIBS: SYS:Classes ADD`.
  - The system classes come from gfopendatatypes' `build/library`. A build without it says so, and the part then
    has none.

## Tested so far, the GFOpenKick ROM

In a copy of Instance-32 (lab t5-oolite-20261119), from a clean boot each time:
- As it was: no class opened, cdxl.datatype included (2002, "couldn't open").
- With only the assign, or only animation.datatype on disk: still none.
- With the assign in `S:user-startup`'s GFOpenUp block, where this part's startup line goes, and the eight system
  classes on disk: all 15 classes on the disk opened (PNG, GIF, WebP, Targa, the picture service, sound, WAV, modules,
  documents, CDXL, Smacker, gfopenvideo, WebM, stream, gzip), and MM7's Smacker film loaded all 41 frames.
- Not yet: the part itself installed by GFOpenUpTool on such a machine.
