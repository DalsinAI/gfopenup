# Restart: OpenUp

_Written 6 October 2026 at about 23:55 UTC and brought up to date on 7 October 2026. Read this first when work resumes; the newest capsule and the live PR list win if they disagree._

## What this repo is

OpenUp: install AmigaOS 3.2.3, run one pack, and it's ready, on a real Amiga, a PiStorm or the AmigaChrome appliance. It bundles every Open app as a part.

## Where it stands

_Updated 8 October 2026._ OpenUp 0.6.17 is on GitHub (tag v0.6.17). 0.6.18 is built (this branch, after #55's icon fix): OpenGPU 0.7 (opengpu.library 0.7, OpenGfx's look hook) with OpenLook 0.6, which must ship together, so only OpenGfx patches RectFill and Text; OpenTitle 0.6 with the taskspace; Tata 0.2 as a part left unticked while 0.x (bit 1073741824, the parts word's last bit: one more part needs a second word); AmigaChromeSharing 1.4 (ACDrop 1.3, the Send Out icon); SDL2.module 3 (SDL_GL on GL.module); the installer's wording; and the OS 3.2-style icons, which 0.6.17 left out because build_pack.py gave OpenCrypto's part OpenCompress's name.

Phases (DESIGN.md section 5): 1 done for AmigaChrome (Setup 0.1 and the startup switches included); 2 under way (OpenBrowser and OpenMail next, as parts left unticked); 3 (PiStorm, real A1200) waits; 4 (Cradle's OS 3.2.3 recipes apply the pack) is live in Cradle (8 October 2026, with 0.6.14 registered); 5's OpenUp Update is built (Cradle first; the GitHub step next). OpenUp Picks is no longer a requirement.

## Merged lately

- #36 (3ffbba5, 2026-10-07): OpenWrite 1.0.2 in OpenUp
- #35 (1544854, 2026-10-07): OpenBlanker, installed with every OpenUp
- #34 (ec5d7cd, 2026-10-07): OpenWrite 1.0 as a part
- #33 (9686a36, 2026-10-06): OpenDock part 0.2: Workbench as a backdrop by default
- #32 (00bdf83, 2026-10-06): The Open apps' own animated GlowIcons, their drawers laid out, and the icon standard

## Open pull requests

- None.

## Next step

1. Phase 2 is merged (#39): OpenBrowser's companion archive goes beside the next release.
2. Phase 4: try the first-start install on a new OS 3.2.3 instance made by Cradle.
3. Phase 5: OpenUp Update's GitHub step (a newer release found over the network, where the Amiga has TLS).

## Waiting on @SacredTrees

- The word that OpenPuTTY 0.2 (on OpenCrypto) is ready, then the 0.6.17 release.

## Who owns it

Main Discourse (builds and packs every release).

## Capsules

Restart capsules for this repo's workstreams, in amigachrome's `capjumps/` shelf:

- [`20261006_AmigaChrome_OpenUp_MainDiscourse_Restart_Capsule.zip`](https://github.com/DalsinAI/amigachrome/tree/main/capjumps)
- [`20261006_AmigaChrome_Icon_Pack_Restart_Capsule.zip`](https://github.com/DalsinAI/amigachrome/tree/main/capjumps)

Team rules that still hold: commits as SacredTrees with no co-author lines; third-party code only on "yes with review" (licence checked, commit and sha256 pinned, fetched at build, never committed); deploys with deploy_dev.py only, on a typed line.
