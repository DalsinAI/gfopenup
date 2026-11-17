# GFOpenUpdate: updates offered on the Amiga

Dale, 17 November 2026: "we need to create a GFOS updater that knows what versions of things are installed and
checks dalsinai etc for updates and offers them to the user to update"; "make it a wbstartup commodity that checks
periodically"; "once at start up when the network is up, then periodically that can be set via TOOLTYPE"; "store its
data in envarc:"; "lets give gfopenup a ui, and make a cx".

Until now a fix reached a machine only in a whole new pack: `GFOpenUpTool UPDATE` looks only at a pack Cradle put in
`DH1:GFOpenUp`. DESIGN.md section 9, step 5, left "the GitHub step" as next. This is that step. It adds no second way
to install: what is installed stays GFOpenUpTool's manifest, and a part goes in through `GFOpenUpTool INSTALL`.

## What there is already

| Need | Where it is |
| --- | --- |
| What is installed, with versions | `ENVARC:GFOpenAmiga/GFOpenUp.manifest`, a `part <name> <version>` line each |
| Versions compared (0.8.29-rc1 < 0.8.29-rc2 < 0.8.29) | `src/openup_vers.h`, `ouv_cmp` |
| One part installed from a Parts drawer, older files never over newer | `GFOpenUpTool INSTALL` |
| A part with a release of its own | `parts.lock`, `docs/design/PARTS-ON-MERGE.md` |
| HTTPS on the Amiga | GFOpenSocket with GFOpenTLS |

## What is added

1. **The index.** One small text file published by DalsinAI beside the part archives, written by the cut:

   ```
   GFOpenUpdate 1
   part <Name> <version> <bytes> <sha256> <file>
   GFOpenUpdate end
   ```

   The last line shows it came whole. `<file>` is a plain name beside the index, so an index can't send a download
   anywhere else. Read by `src/openupdate.h`, tested by `tests/test_openupdate.c`.
2. **GFOpenUpdate**, a commodity in `SYS:WBStartup`, put there by the GFOpenUp part. It checks once at startup when
   the network is up, then every `INTERVAL` seconds (a tooltype; 86400 by default, never under 600, `0` for only at
   startup). A check fetches the index and weighs it against the manifest. Only a part that is installed and older
   is offered. Exchange shows it, and its Show opens the window.
3. **The window** (GFOpenLayout, the look every GFOpenUp program has): the parts installed with their versions, the
   newer version beside each one that has one, and Update, Not now and Check now. Update downloads each part's
   archive, checks its size and SHA-256, unpacks it and runs `GFOpenUpTool INSTALL` on it. Nothing is ever installed
   unasked. The "GFOpenUp Update..." menu entry opens this window.
4. **Its data** is in `ENVARC:GFOpenUpdate/`: `LastCheck` (seconds), and `Skipped` (`skip <Name> <version>` lines: a
   version said no to isn't offered again until a newer one comes).

## Steps

| Step | What | State |
| --- | --- | --- |
| 1 | The index's form, what is offered against the manifest, the tooltype and when a check is due, with host tests | Done (`src/openupdate.h`) |
| 2 | The cut writes the index and each part's archive: `tools/update_index.py`, run by `tools/build_pack.py --lha` into `<out>/updates/` | Done; publishing them on the release is the cut's upload |
| 3 | GFOpenUpdate the commodity: startup and timed checks, the fetch, the install | Done: `src/openupdate/openupdate.c`, in `SYS:WBStartup` from the GFOpenUp part; fetches with GFOpenSocket's httpget, checks size and SHA-256, unpacks with GFOpenCompress, installs with `GFOpenUpTool INSTALL` |
| 4 | The window, and the menu entry pointed at it | Done (0.2): GFOpenLayout UI, "GFOS Updates": every part installed with its version and the newer one beside it, Update, Skip these, Check now, Not now; it opens by itself when a timed check finds updates, and from Exchange's Show or GFOpenUp's menu "GFOS Updates...". Its rows and status line are `oud_rows`, `oud_row_text`, `oud_status` in `src/openupdate.h`, host-tested |
| 5 | Its icon, in the icon standard (amicradle `docs/ICON_STANDARD.md`): a Soft 3D drawing with its sheet for Dale | Owed: until then it has the pack's OS 3.2-style tool icon |

## Where it is published

Dale, 17 November 2026: "yes build parts from public releases". The index and the part archives go on this
repository's public releases, so the Amiga needs no token. Built files only, as the pack itself holds: never source,
and never anything `check_private` refuses.
