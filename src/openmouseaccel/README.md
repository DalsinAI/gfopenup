# OpenMouseAccel

OpenMouseAccel is the OpenUp/OpenInput variable pointer-acceleration component.

## Behaviour

- Slow movement is exactly 1:1 for pixel-level precision.
- Faster movement progressively accelerates using a smooth velocity curve.
- Maximum distance amplification is configurable up to 10x.
- Optional maximum output velocity provides a second hard safety limit.
- Rapid precision recovery immediately returns to 1x when the user slows down.
- Direction reversal reduces inherited velocity so fine corrections do not retain the previous flick's acceleration.
- Fractional output is accumulated instead of discarded.
- If disabled or unavailable, raw pointer movement passes through unchanged.

## OpenInput position

Install the filter after raw relative-device normalisation and before pointer-position clamping/application delivery:

```
raw device -> OpenInput normalisation -> OpenMouseAccel -> pointer clamp -> application/desktop
```

`src/oma_filter.c` is intentionally a small shim because the final OpenInput filter ABI is owned by the OpenUp tree. It must be wired into the real filter registration API rather than creating a second input API here.

## Preferences

The intended Amiga preferences program is `OpenMouseAccelPrefs` using MUI. Build `prefs/OpenMouseAccelPrefs.c` with `OMA_USE_MUI` under the OpenUp Amiga/AROS SDK.

Preferences should be stored in:

- `ENV:OpenUp/OpenMouseAccel.prefs`
- `ENVARC:OpenUp/OpenMouseAccel.prefs`

The running OpenInput service should reload or be messaged when settings change so changes can be felt immediately without rebooting.

## Host validation

```sh
make test
make prefs-host
```

These checks validate the portable acceleration engine only; they do not qualify the Amiga OpenInput integration or MUI UI.

## Suggested first OpenUp integration

1. Add the public header to the OpenUp SDK.
2. Add the core/filter objects to OpenInput.
3. Register OpenMouseAccel at the relative-pointer filter stage.
4. Add live config reload/message plumbing.
5. Build `OpenMouseAccelPrefs` with MUI.
6. Test with raw 1-pixel movements, rapid flicks, reversals, high-DPI devices and pointer-capture transitions.
