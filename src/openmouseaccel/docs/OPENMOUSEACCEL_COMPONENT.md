# OpenMouseAccel Component Contract

OpenMouseAccel interprets low-speed motion as aiming and high-speed motion as travel.

## Hard contracts

1. Motion at or below the precision threshold is 1.0x.
2. The configured multiplier is never exceeded; the public maximum is 10x.
3. Output direction is preserved.
4. Fractional movement is accumulated.
5. A stale or implausibly short event interval cannot create unbounded output.
6. Rapid slow-down returns to the precision region immediately when enabled.
7. A strong direction reversal can reset inherited velocity.
8. Disabled/failure state is raw 1:1 pass-through.

## UI contract

The normal preferences UI exposes:

- Enable
- Precision zone
- Acceleration strength
- Maximum acceleration
- Maximum pointer speed
- Rapid precision recovery
- Presets: Precision / Balanced / Fast / Extreme
- Live test area and current multiplier/speed diagnostics

Advanced mode may expose the exact velocity thresholds, curve exponent, smoothing values and direction-reset angle.
