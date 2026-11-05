# GFOpenUp 0.8.17 (5 November 2026)

This release is GFOpenUp 0.8.16 with one change: **MEDIA:**, gfopenservice's new read-only DOS handler, now comes in the GFOpenService part.

## What's new

**MEDIA:** (gfopenservice #28, gfopenup #196). Shell scripts and programs that know nothing of datatypes can decode a file by copying it through `media.decode/1`, from the services card or a paired Cradle:

    Copy MEDIA:ilbm/Work:grad.qoi RAM:grad.iff
    Copy MEDIA:8svx/Work:tone.flac RAM:tone.8svx

- The kinds are `ilbm` (24-bit), `ilbm8` (256 colours), `8svx`, `aiff` and `raw`.
- `,WxH` after the kind caps a picture's size, and a video gives its first frame.
- MEDIA: is read-only.
- The datatypes keep calling the device directly.

gfopenservice's `docs/MEDIA_HANDLER.md` has the details.

## Parts changed against 0.8.16

| Part | Change |
|---|---|
| GFOpenService | 1.1 to 1.2. It adds `L:media-handler` (1.0), its mountlist `DEVS:DOSDrivers/MEDIA` (mounted at boot; a `MEDIA` mountlist of your own stays) and `C:ServiceTest` (0.1, which lists the services and opens one). gfopenservice.device (1.1) and Nursery are unchanged. |
| GFOpenUp | 0.8.16 to 0.8.17: the Installer's and the uninstaller's version lines. GFOpenUpTool is still 0.8.16, because its source hasn't changed. |

The Installer's GFOpenService page and the readme now mention MEDIA:.

## How it was built

The pack is 0.8.16's archive with only those files changed. Every other member is copied from `GFOpenUp-0.8.16.lha` byte for byte, including gfopenservice.device and Nursery, whose source hasn't changed since 1 November. media-handler and ServiceTest were built from gfopenservice main (90dedba) on the os32-gcc16 stove with OpenAmigaGCC 16.2.0-open4. The other parts are 0.8.16's open6 builds.

gfopendatatypes #26 (picture.datatype's pen fallback for PMODE_V43) is not in GFOpenUp. picture.datatype is a system class, and the GFOpenImage part leaves datatypes.library and the system classes out.

## Tested

This was a lab copy of Instance-32 (Instance-32-DT5: OpenKick, ACRTG, the services card). The hand-installed MEDIA: files from the handler's own lab test were set aside first. Then the 0.8.17 pack went in DH1:GFOpenUp, and the pack's own `GFOpenUpTool INSTALL DH1:GFOpenUp/Parts/GFOpenService` ran, followed by a cold boot:

- The install reported "GFOpenService 1.2 installed (5 files changed)" and listed them in `ENVARC:GFOpenAmiga/GFOpenUp.manifest`. `Version` gives media-handler 1.0, ServiceTest 0.1 and gfopenservice.device 1.1.
- After the boot, `MEDIA:` was mounted from `DEVS:DOSDrivers/MEDIA`. `Info` shows it as read-only.
- `Copy MEDIA:ilbm/SYS:DT5/samples/extra/grad.qoi RAM:grad.iff` worked. The copy is 9264 bytes, and datatypes.library takes it as an ILBM picture.
- `ServiceTest` lists echo/1, input.pads/1, media.decode/1, media.cdxl/1, crwin/1 and cpu.m68k/1. It opens echo/1 on the card, and 100 calls ran with 0 bad.
- `ShowConfig` lists the services card, AmiCradle Team $DA15, product 13, at $EE0000.

The handler itself was lab-tested before gfopenservice #28 was merged: 11 conversions (pictures, sounds and a video's first frame), Dir, List and patterns, and its errors.
