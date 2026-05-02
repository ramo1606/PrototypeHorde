# renderer

Centralized 3D rendering pipeline. Owns the camera, the renderable pool,
the cel shader, blob shadow resources, and the per-frame draw list. Game
code registers/unregisters renderables and pushes transforms; the renderer
handles interpolation, frustum culling, material sorting, and submission.

The renderer does NOT manage the `BeginMode3D` / `EndMode3D` boundaries —
the game loop owns the render sequence and decides when 3D vs HUD draws.

## Types

- `RenderHandle` — opaque integer index into the renderable pool.
  `RENDER_HANDLE_INVALID` (-1) means "no handle".
- `Renderable` — one entry in the pool. Holds the model, current and
  previous transform (for interpolation), bounding sphere (for culling),
  material id (for sorting), and blob shadow flags.
- `FrustumPlane` — `(a, b, c, d)` in Hessian normal form.
- `DrawEntry` — one item in the per-frame sorted draw list. Caches the
  interpolated transform so `RendererDraw3D` doesn't recompute it.
- `Renderer` — the whole pipeline state.

## Public API

### Lifecycle

| Function | Purpose |
|---|---|
| `RendererInit(*r)` | Set up camera, cel shader, blob shadow texture/plane, clear pool. |
| `RendererShutdown(*r)` | Unload blob shadow plane (also frees the shadow texture). |

### Registration

| Function | Purpose |
|---|---|
| `RendererRegister(*r, model, materialID)` | Reserve a slot, store model, compute bounding sphere, assign cel shader. Returns a handle or `RENDER_HANDLE_INVALID` if pool is full. |
| `RendererUnregister(*r, handle)` | Free the slot. The model is **not** unloaded — the level owns the model lifetime. |

### Per-tick / per-frame

| Function | Purpose |
|---|---|
| `RendererPreUpdate(*r)` | Copy `transformCurr → transformPrev` for all active renderables. Call once at the start of each fixed tick, before gameplay. |
| `RendererSetTransform(*r, handle, M)` | Set the current tick's world transform. Called by gameplay. |
| `RendererSetBlobShadow(*r, handle, on, radius)` | Toggle blob shadow and set its ground radius. |
| `RendererBuildDrawList(*r, alpha)` | Interpolate, cull, sort. Once per visual frame, **before** `BeginMode3D`. |
| `RendererDraw3D(*r)` | Submit draw calls for the sorted list and the blob shadow pass. **Inside** `BeginMode3D`. |

### Camera

| Function | Purpose |
|---|---|
| `RendererSetCamera(*r, cam)` | Push a fresh `Camera3D` (the renderer never updates the camera itself). |
| `RendererGetCamera(*r)` | Read back the current camera. |
| `RendererSetClearColor(*r, c)` | Background color (the game loop uses it inside `BeginDrawing`). |

### Lighting (cel shader)

| Function | Purpose |
|---|---|
| `RendererSetLightDir(*r, dir)` | Set the directional light vector (auto-normalized). |
| `RendererSetAmbient(*r, a)` | Set ambient level (`[0..1]`). |
| `RendererSetNumBands(*r, n)` | Set the discrete shading band count (e.g. 3.0). |

### Culling utilities

| Function | Purpose |
|---|---|
| `RendererExtractFrustumPlanes(planes, viewProj)` | Gribb-Hartmann extraction from a view-projection matrix. |
| `RendererIsSphereInFrustum(planes, center, radius)` | Conservative sphere-vs-frustum test. |
| `RendererComputeBoundingSphere(model, *outCenter, *outRadius)` | Combined sphere from all meshes' AABBs. Used by `RendererRegister`. |
| `RendererWorldToScreen(*r, worldPos)` | Wrapper over raylib's `GetWorldToScreen`. |

## Frame pipeline

```
PER TICK (fixed timestep):
    RendererPreUpdate(r)              // curr → prev for all active
    [gameplay calls RendererSetTransform(...)]

PER FRAME (visual):
    alpha = accumulator / FIXED_TIMESTEP
    RendererBuildDrawList(r, alpha)   // interpolate + cull + sort

    BeginDrawing()
        ClearBackground(r->clearColor)
        BeginMode3D(r->camera)
            RendererDraw3D(r)         // models + blob shadow pass
            [level Render3D]
            [debug Render3D]
        EndMode3D()
        [HUD, debug 2D, transition overlay]
    EndDrawing()
```

## Interpolation

`Renderable` keeps `transformCurr` and `transformPrev`. Gameplay sets
`transformCurr` during its fixed-tick update; `RendererPreUpdate` (called
**before** gameplay) copies the previous tick's `transformCurr` into
`transformPrev` so we always have the two endpoints to lerp between.

`BuildDrawList` lerps element-wise on the 16 matrix floats. This is wrong
in the strict sense (the correct way is decompose T/R/S, lerp T, slerp R,
lerp S), but at 60 Hz the per-tick angular delta is small enough that
direct element lerp is visually indistinguishable and significantly
cheaper.

## Frustum culling (Gribb-Hartmann)

`RendererExtractFrustumPlanes` builds 6 planes from `view * proj` by
adding/subtracting the matrix rows. After normalization, the signed
distance from a point to a plane is in world units, so we can compare it
to the bounding sphere radius directly.

Plane order: `0=Left, 1=Right, 2=Bottom, 3=Top, 4=Near, 5=Far`.

`RendererIsSphereInFrustum` returns `false` only if the sphere is fully
behind some plane. False positives at frustum corners are accepted —
drawing one extra is fine, missing a visible one is not.

Reference: Gribb & Hartmann, *Fast Extraction of Viewing Frustum Planes
from the World-View-Projection Matrix*.

## Material sorting

After culling, the draw list is sorted by `(materialID, distSq)`:

- **Primary key:** `materialID` ascending — groups draws by shader/texture
  to minimize GPU state changes between draw calls.
- **Secondary key:** `distSq` ascending — within a material, draw closer
  objects first so the GPU's early-Z can reject occluded fragments.

`qsort` over the draw list. The list size is bounded by `MAX_RENDERABLES`
(few hundred), so `O(n log n)` per frame is negligible.

## Bounding sphere

`RendererComputeBoundingSphere` walks all meshes in a model, builds the
combined AABB, then takes the center as the AABB midpoint and the radius
as the distance from the midpoint to the farthest corner. This
overestimates for non-axis-aligned models — accepted because cull
correctness > tightness.

When `BuildDrawList` transforms the sphere, it uses the **max axis scale**
of the transform's 3×3 part as the radius scale. Same trade-off:
overestimates for non-uniform scale.

## Blob shadows (Task 1.7)

A 64×64 radial-gradient texture (dark center, transparent edges) painted
on a unit plane. For each visible renderable with `blobShadow == true`:

- Position the plane at `(world.x, 0.01, world.z)` to avoid z-fighting.
- Scale by `2 * blobRadius * (1 - heightRatio * 0.5)` so it shrinks as
  the entity rises.
- Alpha fades out linearly between height 0 and `SHADOW_MAX_HEIGHT` (6
  units).
- Drawn after models inside `BLEND_ALPHA` so depth testing handles
  occlusion.

The shadow texture is owned by `shadowPlane.materials[0]`; `UnloadModel`
in shutdown frees both, **don't** call `UnloadTexture` separately or
you'll double-free.

## Cel shader (Task 1.5)

Loaded via the resource manager (`RES_SHADER_CEL`). The shader's `matModel`
uniform location is registered into raylib's `SHADER_LOC_MATRIX_MODEL` so
raylib auto-pushes the per-draw model matrix. Three custom uniforms
(`lightDir`, `ambient`, `numBands`) are pushed once per frame in
`RendererDraw3D`.

The shader is assigned to all materials of every registered model. This
mutates the shared model's material array, which is intentional — every
renderable should cel-shade.
