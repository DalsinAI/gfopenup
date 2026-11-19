# The Media drive is MediaFiles:

- The standard build's DH4: is now the volume MediaFiles, not Media (Dale, 19 November 2026: "change dh4 to
  MediaFiles", "that way it wont happen again"). A volume called Media had the name of the MEDIA: handler that
  GFOpenService mounts, and Mount said "already mounted" at boot on GFOpenDOS (fixed in amicradle-guest #799).
- GFOpenUpTool 0.8.25: `when MediaDrive` and `unless MediaDrive` look for a volume named MediaFiles.
- GFOpenIcons gives MediaFiles:Disk the Media drive's icon and MediaFiles:midi its folder.
- GFOpenBlanker's icon plays MediaFiles:midi/Dance/copper-list.mid.
