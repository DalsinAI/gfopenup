# Restart: OpenUp

_Written 6 October 2026 at about 23:55 UTC, while all work is paused on @SacredTrees's word (23:28 UTC). Read this first when work resumes; the newest capsule and the live PR list win if they disagree._

## What this repo is

OpenUp: install AmigaOS 3.2.3, run one pack, and it's ready, on a real Amiga, a PiStorm or the AmigaChrome appliance. It bundles every Open app as a part.

## Where it stands

OpenUp 0.6.13 (OpenEdit, OpenFiles 0.2.1, OpenView 0.4, network icons) is shipped and installed on Instances 30 and 31. Merged for 0.6.14 but not cut: the animated GlowIcons and the icon standard ICONS.md (#32), Workbench as a backdrop (#33), OpenFiles 0.2.2 and OpenDock 0.2.

## Merged lately

- #33 (9686a36, 2026-10-06): OpenDock part 0.2: Workbench as a backdrop by default
- #32 (00bdf83, 2026-10-06): The Open apps' own animated GlowIcons, their drawers laid out, and the icon standard
- #31 (540138e, 2026-10-06): OpenUp 0.6.13: OpenEdit, OpenFiles 0.2.1, OpenView 0.4, network icons
- #30 (83f6955, 2026-10-06): Credit who made OpenUp: CONTRIBUTORS.md
- #29 (177b7af, 2026-10-06): OpenUp 0.6.12: Sound and OpenSpeaker, OpenTitle 0.3, OpenView in MultiView's place
- #28 (41fdd24, 2026-10-06): OpenView in MultiView's place, OpenFiles and OpenView on the dock

## Open pull requests

- None.

## Next step

1. Cut OpenUp 0.6.14 and attach the .lha as a GitHub release (tag v0.6.14) so the .deb can bundle it.
2. Install on Instances 30/31 only on "update instances".

## Waiting on @SacredTrees

- "update instances" once 0.6.14 is cut.

## Who owns it

Main Discourse (builds and packs every release).

## Capsules

Restart capsules for this repo's workstreams, in amigachrome's `capjumps/` shelf:

- [`20261006_AmigaChrome_OpenUp_MainDiscourse_Restart_Capsule.zip`](https://github.com/DalsinAI/amigachrome/tree/main/capjumps)
- [`20261006_AmigaChrome_Icon_Pack_Restart_Capsule.zip`](https://github.com/DalsinAI/amigachrome/tree/main/capjumps)

Team rules that still hold: commits as SacredTrees with no co-author lines; third-party code only on "yes with review" (licence checked, commit and sha256 pinned, fetched at build, never committed); deploys with deploy_dev.py only, on a typed line.
