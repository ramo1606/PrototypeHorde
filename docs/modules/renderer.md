# renderer (kit)

Pool of registered models with frame-rate independent interpolation,
Gribb-Hartmann frustum culling, and material/distance sort. The renderer
does **not** own a camera, a clear color, a shader, or any effect.
Callers pass the `Camera3D` to `RendererBuildDrawList`; custom material
setup is done on models by the host before registering; optional effect
passes iterate the public draw list after `RendererDraw3D`.

Lives under `lib/` as part of the reusable kit.

## Dependencies & overrides

```
DEPENDENCIES: raylib.h
OVERRIDES (define before include):
  RENDERER_MAX_RENDERABLES (default 256)
  RENDERER_NEAR_PLANE      (default 0.01f)
  RENDERER_FAR_PLANE       (default 1000.0f)
```

## Types

- `RenderHandle` — opaque integer index into the renderable pool.
  `RENDER_HANDLE_INVALID` (-1) means "no handle".
- `Renderable` — pool entry: model, current/previous transforms,
  bounding sphere, material id.
- `FrustumPlane` — `(a, b, c, d)` Hessian normal form.
- `DrawEntry` — sorted draw list item with cached interpolated
  transform. Public so effect passes can iterate visible items.
- `Renderer` — pool + draw list + frustum + per-frame stats.

## Public API

| Function | Purpose |
|---|---|
| `RendererInit(*r)` | Zero the pool. |
| `RendererShutdown(*r)` | Currently a no-op. |
| `RendererRegister(*r, model, materialID)` | Reserve a slot, store model, compute bounding sphere. Does **not** touch shaders. |
| `RendererUnregister(*r, handle)` | Free the slot. |
| `RendererSetTransform(*r, handle, M)` | Set the current tick's world transform. |
| `RendererPreUpdate(*r)` | Copy `transformCurr → transformPrev`. Call once per fixed tick, **before** gameplay. |
| `RendererBuildDrawList(*r, camera, alpha)` | Interpolate, cull, sort. Once per visual frame, **before** `BeginMode3D`. |
| `RendererDraw3D(*r)` | Submit draw calls. Call **inside** `BeginMode3D`. |
| `RendererExtractFrustumPlanes(planes, viewProj)` | Gribb-Hartmann extraction. |
| `RendererIsSphereInFrustum(planes, c, r)` | Conservative sphere-vs-frustum test. |
| `RendererComputeBoundingSphere(model, *out, *out)` | Combined sphere from all meshes. Used internally by `RendererRegister`. |

## Frame pipeline

```
PER TICK (fixed timestep):
    RendererPreUpdate(r)                  // curr → prev
    [gameplay calls RendererSetTransform(...)]

PER FRAME (visual):
    alpha = accumulator / FIXED_TIMESTEP
    RendererBuildDrawList(r, camera, alpha)

    BeginDrawing()
        ClearBackground(...)              // host-side
        BeginMode3D(camera)
            RendererDraw3D(r)
            [host effect passes — outlines, decals, ground projections, ...]
        EndMode3D()
        ...
    EndDrawing()
```

## Interpolation

Element-wise lerp on the 16 matrix floats. Strictly wrong (correct is
decompose T/R/S, lerp T, slerp R, lerp S), but at 60 Hz the per-tick
angular delta is small enough that this is visually indistinguishable
and significantly cheaper.

## Frustum culling (Gribb-Hartmann)

Six planes from `view * proj` by adding/subtracting matrix rows. After
normalization the signed distance from a point to a plane is in world
units, comparable to the bounding sphere radius directly. Plane order:
`0=Left, 1=Right, 2=Bottom, 3=Top, 4=Near, 5=Far`.

`RendererIsSphereInFrustum` returns `false` only if the sphere is fully
behind some plane. False positives at frustum corners are accepted —
drawing one extra is fine, missing a visible one is not.

Reference: Gribb & Hartmann, *Fast Extraction of Viewing Frustum Planes
from the World-View-Projection Matrix*.

## Material sorting

After culling, the draw list is sorted by `(materialID, distSq)`:

- Primary: `materialID` ascending — groups draws by shader/texture.
- Secondary: `distSq` ascending — front-to-back within a material, helps
  early-Z reject occluded fragments.

`qsort` over the draw list. Bounded by `RENDERER_MAX_RENDERABLES`, so
`O(n log n)` is negligible per frame.

## Bounding sphere

Combined AABB across all meshes; center = AABB midpoint; radius =
distance from midpoint to farthest corner. Overestimates for non-AABB
geometry — accepted because cull correctness > tightness.

When `BuildDrawList` transforms the sphere, it scales the radius by the
**max axis scale** of the transform's 3×3 part. Same trade-off:
overestimates for non-uniform scale.

## Effect pattern (host-side)

Effects like outlines, decals, or other host-side passes iterate the
public draw list:

```c
for (int d = 0; d < r->drawCount; d++) {
    DrawEntry* e = &r->drawList[d];
    /* e->transform is the interpolated world matrix */
    /* e->index   is the slot in r->renderables */
    /* e->distSq  is the squared distance to camera */
    /* draw your effect using e->transform */
}
```

The renderer guarantees only visible (post-cull) entries are present
and the transform is interpolated. Anything else (per-entity flags,
effect parameters, outline colors) is the host's parallel state.
