# GFOpenApt: C:apt, GFOS's parts from the Shell

Dale, 23 November 2026: "side quest , amiga version of apt"; asked of a new design, "is that the soluton ... ?";
then "c: command", "noetwortk up", "needs the repo list", "github being hte source of most".

So this is not a new system. `C:apt` is a second face on GFOpenUpdate (docs/design/GFOPENUPDATE.md): what the
window does with a click, a Shell does with a word. It adds no second index, no second list of what is installed
and no second way to install.

## What it shares

| Need | Where it is, and whose it was |
| --- | --- |
| The index, read and refused when cut short or wrong | `src/openupdate.h` (`oud_index_parts`), GFOpenUpdate's |
| Versions compared, what is offered, the rows and their words | `src/openup_vers.h`, `oud_offers`, `oud_rows`, `oud_row_text` |
| What is installed | `ENVARC:GFOpenAmiga/GFOpenUp.manifest`, GFOpenUpTool's |
| Fetch (httpget), size and SHA-256, unpack (GFOpenCompress), `GFOpenUpTool INSTALL` | `src/openupdate_amiga.h`: GFOpenUpdate's own functions, moved out of `openupdate.c` so both programs call the same ones |
| What a part needs | the `needs part <name>` lines of its Part file, `src/openup_lines.h` (`oul_part_needs`), GFOpenUpTool's |
| Taking a part out | `GFOpenUpTool REMOVE <drawer with its Part file>` |

New: `src/openapt.h` (the command line, the repo list, the second index form, the order of needs; plain C, tested
on the host by `tests/test_openapt.c`) and `src/openapt/openapt.c` (the command, `$VER: GFOpenApt 0.1`).

## The commands

| Command | What |
| --- | --- |
| `apt update` | fetches each source's index and keeps it (`ENVARC:GFOpenUpdate/Lists/<8 hex>.list`); an index that isn't whole is not kept, and the old copy stays |
| `apt list`, `apt list installed`, `apt list upgradable` | from the kept indexes and the manifest, in the window's words: `GFOpenPlay  0.1.6  new: 0.1.7-rc1` |
| `apt search <text>`, `apt show <name>` | the same; names are found in any case |
| `apt install <name>...` | every archive is fetched, weighed (size, SHA-256) and unpacked before anything is installed; a wrong one stops the whole run. What a part needs and the disk hasn't goes in first; parts that need each other in a ring are refused. After each `GFOpenUpTool INSTALL` the manifest is read again: a part it doesn't list at the new version wasn't installed, whatever was printed |
| `apt upgrade [<name>...]` | the installed parts with a newer version (`oud_offers`), or those named |
| `apt remove <name>...` | only where the Part file of the installed version is at hand: the one apt kept when it put the part in (`SYS:Storage/GFOpenApt/<Name>/Part`), else the pack's (`DH1:GFOpenUp/Parts/<Name>`). Never GFOpenUp itself, never a part an installed part needs. Otherwise it says why and the part stays |
| `apt sources` | the repo list, and whether each source has been read |

The network is needed by update, install and upgrade only. Before each fetch apt opens bsdsocket.library, looks
the host up and connects to it (8 seconds at most), and says which of the three failed: "the network isn't up",
"can't be looked up", "can't reach github.com". Return codes are AmigaDOS's: 0 done, 5 something was left as it
was and said so, 10 it couldn't be done, 20 apt couldn't run.

## The repo list

`S:apt.sources` (Dale, 23 November 2026: apt is the command, "extended and controlled through our repo list", and
the list lives in S:). apt reads that file and no other. It is shipped from `pack/apt.sources` and written only
where there is none:

```
github DalsinAI/gfopenup updates     a release by its tag: .../releases/download/updates/GFOpenUpdate.index
github DalsinAI/gfopenbrowser        a repository's newest release: .../releases/latest/download/GFOpenUpdate.index
index https://.../software/index.txt an index anywhere (an address ending "/" holds GFOpenUpdate.index)
off github DalsinAI/gfoffice         listed, not read
```

No GitHub API and no JSON: GitHub's own `releases/latest/download/<file>` address finds the newest release, so
the one thing a repository must publish is a file of a known name. A name two sources have is listed once, at the
newer version. With no `S:apt.sources` apt says so in one line and returns 10: it doesn't make a list up. A wrong
line stops apt and is named by number.

The list ships with GFOpenUp's index and gfosdistribution's `software/index.txt` on, and every repository of
`parts.lock` and `apps.lock` listed but off (`tests/test_apt_sources.py` holds the list to the two locks).

## Not done yet

1. **Nothing here has run on an Amiga.** The logic is tested on the host; the program compiles.
2. **A repository's own release as a source.** For `github <owner>/<repo>` to give anything, each release must
   carry two files: `GFOpenUpdate.index` (`tools/update_index.py` writes it) and, beside it, the archive the index
   names, `<Name>-<version>.lha`, holding one drawer with a Part file. `apps.lock`'s releases are plain archives
   with no Part file; `tools/apps_pack.py` already makes `App<Drawer>` parts of them, so the short way is for the
   apps cut to run `update_index.py` over `GFOpenApps/Parts` and publish the result as one release of gfopenup
   (a line `github DalsinAI/gfopenup apps`). GitHub's "latest" passes over releases marked pre-release.
3. **gfosdistribution's index** is read (tabs: name, version, size, sha256, address, description), but it has no
   last line to show it came whole, as GFOpenUpdate's has. It should gain one (`# end`), and say that a package
   is a part's archive: apt refuses one that unpacks to anything else.
4. **Needs are known only after the download**, from the Part file. For the Store to show them before, the index
   needs them; a `needs` line would make GFOpenUpdate 0.2 refuse the whole index (it takes only `part` lines), so
   it wants `GFOpenUpdate 2` or a second file beside the index.
5. **remove after the window's Update**: the window doesn't keep the Part file, so apt can't remove a part the
   window last updated. The right home for keeping it is `GFOpenUpTool INSTALL` itself.
6. No `autoremove`, no pinning, no asking before it acts; `ENVARC:GFOpenUpdate/Skipped` is the window's and apt
   upgrades past it, as Check now does.
7. **AmiPortAtlas has `C:Atlas`** (UPDATE, SEARCH, INSTALL, UPGRADE, sources of its own) for games and ports: it
   installs by Installer script or by copying a drawer and keeps its own list. Dale, 23 November 2026: apt is the
   command, and replaces Atlas and the others in time. For that apt needs what Atlas has and it hasn't: a package
   that is a drawer or an Installer script and not a part, and the Atlas's JSON index as a source kind.

## What the Store and the updater share with it

The GFOS Store (a GFOpenLayout window on the same catalogue) and GFOpenUpdate need the same four things, and they
are already outside `openapt.c`: the repo list (`oua_sources`), the kept indexes (`oua_read_list`, `oua_merge`:
the Store opens with the network down), the rows (`oua_row`, `oud_rows`) and the steps of an install
(`src/openupdate_amiga.h`, the needs' order `oua_order`). GFOpenUpdate reads one address today (its URL tooltype);
reading the repo list instead is its next step, and then the window, the Store and apt offer the same things.
