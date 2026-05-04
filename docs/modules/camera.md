# camera (project)

Fixed isometric camera with low-angle perspective. Follows a single
target (or a centroid in multiplayer) with frame-rate independent
exponential smoothing. No rotation, no aim mode, no mouse — the angles
are constant for the whole game.

This is **project code**, not a kit module. Other projects pick their
own camera model.

## Types

- `CameraConfig` — POD with all tunables: angles, fovy, distance,
  look-at lift, smoothing speed, multiplayer distance range and spread
  factor.
- `GameCamera` — runtime: the raylib `Camera3D`, the config, the
  target position, the multiplayer spread, and the smoothed look-at.

## Public API

| Function | Purpose |
|---|---|
| `CameraInit(*cam)` | Initialize with `DEFAULT_CONFIG` and a sane starting pose. |
| `CameraUpdate(*cam, dt)` | Smooth toward target, recompute `Camera3D`. Call per visual frame. |
| `CameraSetTarget(*cam, pos)` | Singleplayer: target = position. Resets spread to 0. |
| `CameraSetGroupTarget(*cam, centroid, spread)` | Multiplayer: target = centroid; distance scales with spread. |
| `CameraSetConfig(*cam, config)` | Replace the whole config. |

## Math

The camera offset from the look-at point is spherical:

```
x = distance * cos(elevation) * sin(azimuth)
y = distance * sin(elevation)
z = distance * cos(elevation) * cos(azimuth)
```

Defaults: elevation 30°, azimuth 45°, fovy 30°, distance 14. The fovy
is intentionally narrow — a narrow perspective fovy with the camera
far away approximates an orthographic iso look while keeping minor
parallax cues.

## Smoothing

Frame-rate independent exponential lerp on the look-at point:

```
t = 1 - exp(-smoothSpeed * dt)
currentLookAt = lerp(currentLookAt, desiredLookAt, t)
```

The camera position is recomputed each frame as
`currentLookAt + offset(angles, distance)`. Smoothing the look-at
instead of the position avoids any drift in the orientation; the angle
to the look-at stays fixed.

## Multiplayer

`CameraSetGroupTarget(centroid, spread)` switches behavior:

- `targetPos = centroid` (caller computes from live players).
- `spread > 0` triggers distance scaling:
  ```
  distance = clamp(minDistance + spread * spreadFactor,
                   minDistance, maxDistance)
  ```

So when the group spreads out, the camera pulls back; when they
clump, it stays in. `CameraSetTarget` resets spread to 0 to switch
back to singleplayer behavior.

## Why not orthographic

A true orthographic projection would give the cleanest iso look, but
it strips depth cues that help spatial reasoning during fast play.
Narrow-fovy perspective at long distance is the common compromise:
near-iso silhouettes, slight parallax for feedback.

## Replacing the camera

If a future project wants a different camera (top-down, follow-cam,
free-orbit), the kit doesn't constrain it. The renderer takes the
`Camera3D` as a parameter to `RendererBuildDrawList` and doesn't care
where it came from.
