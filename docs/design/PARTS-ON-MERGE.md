# GFOpenUp parts on every merge

Dale, 16 Nov 2026: "how can we have a situation that we have to build an RC to do this. we should have been there
when it was merged". A central pillar of the work.

Today a GFOpenUp pack is only made by an RC cut: daletop clones about 34 repositories and builds about 87 steps
(1.5 hours or more for rc3). gfopenup #267 made a cut build again only the steps whose repositories moved, but a cut
is still the only place a part gets built. This design moves the build to the merge.

## The rule

Every merge to a part repository's main builds that repository's GFOpenUp part and publishes it, built at that
commit. A pack (nightly, or an RC) only puts parts that are already published together. It compiles nothing.

## 1. Where a part builds

GitHub's hosted runners, never the daletop runner.

- **The stove:** `DalsinAI/gfopenup/.github/actions/stove` installs the os32-gcc16 stove on the runner from three
  sources:
  - gfopengcc's public binary and source releases, checked against their SHA256SUMS;
  - the AmigaOS 3.2 NDK from Aminet, checked against the sha256 in gfopengcc scripts/ndk.lock;
  - gfopengcc's own scripts/install-ndk.sh, which puts the NDK in place.

  The result is the folder layout every build script already reads (`$STOVE/prefix/bin/m68k-amigaos-gcc`), and the
  action sets OS32_GCC16 and STOVE. It is kept in the Actions cache by release, so a run after the first one takes
  seconds.
- **Why not a GHCR image:** the NDK isn't free software. A public image may not carry it, and a private image needs
  a registry login on every run. The cache gives the same speed and redistributes nothing.
- **Private repositories:** a part repo checks out its sibling repos at main (gfopenbench needs gfopengadtools,
  gfopenprefs, gfopenfilemanager, gfopentata and gfopenlayout). The workflow's own token can't read other private
  repositories, so this takes a read-only token for the DalsinAI repositories, held as the organisation secret
  `PARTS_READ_TOKEN`.
- **Cost:** hosted minutes are free for public repositories. A private repository's job can name the
  amigachromebuild runner (label guest-m68k) instead. The workflow is the same apart from `runs-on`.
- **The os32 stove (GCC 6.5):** some parts still build with it. Those wait until it has a release like gfopengcc's,
  or until their scripts move to GCC 16 (already the rule: "gcc16 on all stoves").

## 2. What a part repo publishes

`.github/workflows/part.yml`:

- **On a pull request:** builds the part, as a check.
- **On a push to main:** builds the part and publishes it to the repository's rolling pre-release `parts`:
  - `<repo>-<commit12>.tar.gz`: the files build_pack.py takes from that checkout, at their paths in the repository
    (gfopenbench: `build/os3/GFOpenBench`), plus `PART.json`;
  - `PART.json` holds the repository, the full commit, the build time, the stove (`m68k-amigaos-gcc --version`),
    and the commit of every sibling checkout the build read;
  - `latest.json` is the PART.json of the newest build.

  The release keeps the newest 30 builds. An RC's pinned builds are copied to the RC's own release, so pruning
  never loses one.

## 3. Each part has its own RCs (Dale, 17 Nov: "acgame should be their own rcs ... that way they fold in, rather than this huge thing")

A part repository (acgame, gfopenallegro, gfopenbench, gfopenprefs...) cuts its own release candidates, on its own
version line, when its own work is ready. A pack never decides a part's version.

- **Promotion, not a rebuild:** `release.yml` (workflow_dispatch: version such as 1.3-rc1, and a commit, default main
  HEAD) takes that commit's build already in `parts` and publishes it as the repository's own release `v<version>`.
  It marks the release as a pre-release while the version ends in -rcN. The release holds the same tarball and
  PART.json, plus `<Part>-<version>.lha` for an Amiga user who wants only that part. If the commit has no build yet,
  release.yml builds it the same way part.yml does.
- **One release per part repository:** a repository that makes several parts (amicradle-guest's libraries, say)
  names each part in PART.json's `parts` list, and the release carries all of them.
- **The pack pins part releases:** gfopenup keeps `parts.lock`, one line per part:
  `gfopenbench v0.2-rc1`, `acgame v1.0-rc2`, and so on. A GFOpenUp RC is a parts.lock commit plus an assembly run:
  fetch each pinned release, check PART.json, put the pack together and LhA it. Moving one part forward is a
  one-line PR to parts.lock, reviewed like any other, and it changes nothing else in the pack.
- **Nightly:** takes `latest` from every repository's `parts` release instead of parts.lock, so it shows what main
  would give before anyone cuts.

## 4. How a pack is put together

`tools/fetch_parts.py` (next PR) reads parts.lock (a GFOpenUp RC), a freeze list of commits (repo and commit per line,
the form rc.sh writes), or "latest" (nightly). For each repository it downloads that commit's tarball, checks PART.json's commit, and unpacks it into
the checkout. build_pack.py then runs `--prebuilt`: a step whose repository came from a published part is taken as
built, and its outputs are checked against PART.json. Only repositories with no published part yet still build
locally, through #267's step reuse.

- **Nightly:** gfopenup's `nightly.yml` makes `GFOpenUp-nightly.lha` from "latest" and publishes it. Assembly only,
  a few minutes.
- **RC:** pins the commits in freeze.tsv. It fetches, assembles and LhA-packs (jlha, 8 s). No compiling.
- **GFOpenUpTool UPDATE:** it can take the nightly pack the way it takes an RC today. Pulling single parts over the
  network on the Amiga is a later step.

## 5. Order

1. The stove action (this PR), with its own check that compiles and links against the NDK's headers.
2. gfopenbench `part.yml` (first part repo, public).
3. gfopenbench `release.yml` (its own RCs), then gfopenprefs, gfopengadtools, amicradle-guest, gfopenrtg, acgame,
   gfopenallegro and the rest: one PR each with part.yml and release.yml, their paths from build_pack.py's reads.
4. gfopenup `parts.lock`, listing each part's current release.
5. fetch_parts.py and build_pack.py `--prebuilt`.
6. nightly.yml.

## Needs from Dale

- The `PARTS_READ_TOKEN` organisation secret: a fine-grained token with read-only Contents on the DalsinAI
  repositories, for the private sibling checkouts.
- Yes or no to hosted minutes for private part repositories. Without it they use the amigachromebuild runner.

Copyright (c) 2026 AmiCradle Team. MIT licence.
