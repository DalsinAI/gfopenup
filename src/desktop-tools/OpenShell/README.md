# GFOpenShell 0.1

GFOpenShell reimagines the Amiga Shell **window**, not the Shell language.

The goal is to preserve Amiga semantics while giving GFOpenUp a shell interface
that is pleasant on a modern display.

See `docs/DESIGN.md` for the full interaction design.

## Headline features

- tabbed independent Shell sessions;
- current-path bar;
- proper editable command line;
- selectable/searchable transcript;
- command history and history search;
- completion popup;
- Workbench drag/drop path insertion;
- Open Shell Here integration;
- save transcript;
- useful return-code/busy status;
- configurable copy/break keyboard behaviour;
- normal Workbench window snapshotting.

## Architecture

The UI talks to a deliberately narrow Shell transport interface. The transport
is responsible for creating and driving a real Amiga Shell process. GFOpenShell
does not interpret commands itself.

## Suggested repository location

    GFOpenUp/
      apps/
        GFOpenShell/
