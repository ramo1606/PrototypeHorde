# debug

Debug overlay built on raygui. Toggleable panels (F1 to show/hide, F2-F7 to
toggle each panel), a frametime/memory built-in panel, a renderer stats
panel, a physics stats panel, and a hook for 3D gizmo callbacks per slot.

The whole module compiles to no-ops in release builds via the `DEBUG_ENABLED`
macro.

## Types

- `DebugPanelDrawFn` — `void (*)(float x, float y, float w, float h)`. A 2D
  panel renderer. Called inside the debug overlay's draw pass with a
  rectangle to fill.
- `DebugRender3DFn` — `void (*)(Game* game)`. A 3D gizmo renderer. Called
  inside `BeginMode3D`. Useful for collider wireframes, raycast visualizers,
  spawn markers.
- `DebugPerfStats` — POD aggregate of all per-frame stats the overlay
  displays. The game fills it once per frame and hands it off via
  `DebugSetPerfStats`. The struct is the only coupling point between the
  game and the overlay.

## Public API

| Function | Purpose |
|---|---|
| `DebugInit()` | Set up internal state and register the three built-in panels. |
| `DebugShutdown()` | Currently a no-op; reserved for future cleanup. |
| `DebugRegisterPanel(slot, name, fn)` | Bind a 2D panel callback to a slot (0..5). |
| `DebugRegister3D(slot, fn)` | Bind a 3D gizmo callback to a slot (0..5). |
| `DebugUpdate(game)` | Read F-keys, update FPS history, toggle panels. |
| `DebugRender3D(game)` | Run all active 3D gizmo callbacks. Call inside `BeginMode3D`. |
| `DebugRender(game)` | Draw the hint bar, level info, and active 2D panels. Call after `EndMode3D`. |
| `DebugSetPerfStats(stats)` | Push a fresh stats snapshot. Called once per frame by the game. |
| `DebugIsVisible()` | Check whether the overlay is currently visible. |

## Slot system

Six slots numbered 0..5, each mapped to F2..F7 respectively. Each slot
holds:
- A 2D panel callback (`drawFn`).
- An optional 3D gizmo callback (`render3DFn`).
- An `active` flag toggled by its F-key.

Both callbacks share the same active flag — pressing F4 toggles both the
physics 2D panel and the physics collider wireframes at the same time.

Built-in slots:
- 0 (F2): Performance — frametime, FPS graph, arena usage bars.
- 1 (F3): Renderer — renderable count, draw list size, frustum cull ratio.
- 2 (F4): Physics — collider count, pair tests, contacts, triggers.

Slots 3-5 (F5-F7) are reserved for game-side panels (gameplay, AI, spawn).

## Visibility model

`DebugUpdate` watches F1 and toggles the global `visible` flag. While
`visible == false`, panels still respond to F2..F7 toggles internally but
nothing is drawn. A small "F1: Debug" hint stays in the corner so the user
knows the overlay exists.

## FPS graph

A 120-sample ring buffer of `GetFPS()` values is drawn as a colored bar
chart. Color thresholds compare against `RENDER_FPS` from `config.h`:
≥95% green, ≥75% yellow, otherwise red. Useful for spotting hitches that
average frametimes hide.

## Release build

`DEBUG_ENABLED` is defined by CMake only on Debug configurations
(`target_compile_definitions(... $<$<CONFIG:Debug>:DEBUG_ENABLED>)`).
The headers do **not** force-define it — that would re-enable the
overlay in Release builds.

When `DEBUG_ENABLED` is not defined, the header replaces every public
function with a macro that expands to nothing (or `false` for
`DebugIsVisible`). No code from `debug.c` is compiled. The game's call
sites need no `#ifdef` guards — the cost is zero in release.
