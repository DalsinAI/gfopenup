# Autobuild

How LumaVAULT's nightly build (bigguy, 23:00 to 06:00) builds this repository. The form is in lumavault-server
`docs/AUTOBUILD.md`.

## gfopenup-pack

The whole pack from every repository's main, as `<base>.<date>`: tools/nightly_pack.py puts the repositories the pack
is built from beside this checkout and runs tools/build_pack.py. Steps whose repositories haven't moved are reused,
so only a night after a busy day is long. Move `--base` on when a release is cut. `--fonts` is bigguy's copy of the
DejaVu 2.37 files the pack pins: Debian's fonts-dejavu 2.37-8 is a different file from Ubuntu's 2.37-8build1.

- frequency: always
- kind: pack
- script: tools/nightly_pack.py
- args: --base 0.8.31 --fonts /srv/samsung/nightly/pins/dejavu-2.37
- keep: out/pack/*.lha out/COMMITS.txt
- minutes: 300
