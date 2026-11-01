# GFOpenGames

GFOpenGames is the GFOpenUp gaming workstream: a compact SQLite catalogue and a safe WHDLoad maintenance layer.

## First slice

The working host/Cradle implementation lives in AmiCradle:

- `scripts/opengames_catalog.py`
- `resources/opengames/schema-v1.sql`
- `tests/test_opengames_catalog.py`

It scans a user-owned WHDLoad collection, records each slave and SHA-256, notices neighbouring LHA/ZIP payloads, builds one indexed SQLite catalogue, creates a separate writable user database, and can search the catalogue without walking the game tree.

No WHDLoad binary, game data, MrV2K database or other third-party catalogue is redistributed.

## GFOpenUp policy

GFOpenUp will add an optional GFOpenGames part which:

1. detects an existing WHDLoad and its version;
2. asks before downloading/updating upstream WHDLoad;
3. selects the appropriate WHDLoad VFS build for the detected CPU when available;
4. keeps game payload and save data separate;
5. builds or refreshes `games.sqlite`;
6. installs the native GFOpenGameSelector;
7. stages WHDLoad patch updates and classifies them before touching a live install;
8. records every change in the GFOpenUp manifest for clean uninstall/rollback.

## Patch classes

- **A** - slave only: automatic after hash verification.
- **B** - slave plus icon/readme/metadata: automatic after hash verification.
- **C** - installed game data changes: requires staged data migration.
- **D** - original media/reinstall required: never automatic.

The AmiCradle update helper currently applies A/B only, backs up every changed file, replaces through a temporary sibling, and restores earlier files if a later operation fails.

## Next build

The next vertical slice is the native AmigaOS GFOpenGameSelector using the same schema: list, incremental search, details, favourites and WHDLoad launch. After that GFOpenUp can package it as a normal optional Part.
