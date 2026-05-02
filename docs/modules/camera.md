# camera

> **Note:** this is the legacy third-person orbit camera. It will be
> replaced in Task 1.6 (CLAUDE.md, Phase 1) with a fixed isometric camera
> with low-angle perspective. The current refactor is mechanical only —
> file split and rename — to keep the codebase consistent. The detailed
> design of the iso camera lives in CLAUDE.md and will be documented here
> when implemented.

Third-person orbit camera. The camera sits on a sphere around a target,
controlled by mouse delta (yaw/pitch). It supports two modes (`FOLLOW` and
`AIM`) with a smooth blend between them, frame-rate independent smoothing,
and direction queries for camera-relative movement.

## Types

- `CameraMode` — enum: `CAMERA_MODE_FOLLOW`, `CAMERA_MODE_AIM`.
- `CameraConfig` — POD with all tunable values (distances, lateral offsets,
  heights, sensitivity, pitch limits, smoothing speed, mode transition
  duration, look-at height).
- `GameCamera` — full state. Holds the configured `CameraConfig`, the
  current orbit angles, the target position, the smoothed position and
  look-at, and a raylib `Camera3D` ready to feed to the renderer.

## Public API

| Function | Purpose |
|---|---|
| `CameraInit(*cam)` | Initialize with `DEFAULT_CONFIG` and a sane starting orbit. |
| `CameraUpdate(*cam, dt)` | Advance mode transition, recompute desired pose, smooth toward it, write `Camera3D`. |
| `CameraRotateByMouse(*cam, dx, dy)` | Apply mouse delta to yaw/pitch. Pitch is clamped. |
| `CameraSetTarget(*cam, pos)` | Set the world position the camera orbits. |
| `CameraGetForwardXZ(*cam)` | Forward direction projected onto XZ. For camera-relative movement. |
| `CameraGetRightXZ(*cam)` | Right direction projected onto XZ. |
| `CameraSetMode(*cam, mode)` | Switch mode; starts a smooth transition between configs. |
| `CameraSetConfig(*cam, config)` | Replace the whole config. |

## Smoothing

`CameraUpdate` uses frame-rate independent exponential smoothing:

```
factor = 1 - exp(-smoothSpeed * dt)
currentPos = lerp(currentPos, desiredPos, factor)
```

This converges at the same rate regardless of frame time. Higher
`smoothSpeed` is snappier.

## Mode transitions

Switching modes captures the current params (which may themselves be in the
middle of a previous transition) as the start point and eases toward the
new mode's params using `EaseSineInOut` over `transitionDuration` seconds.

## Why this will be rewritten

The Boxhead 3D pivot needs a fixed isometric camera that follows the player
(or the centroid in multiplayer) without any free rotation. The orbit/aim
distinction goes away. The new camera will be simpler and is documented
in CLAUDE.md, Task 1.6.
