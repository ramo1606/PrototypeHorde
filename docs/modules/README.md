# Module reference

Per-module documentation. Two tiers:

- **Kit** — lives in `lib/`, reusable across raylib projects, no
  knowledge of `Game` or any host type. Drop-in via
  [`../../BLUEPRINT.md`](../../BLUEPRINT.md).
- **Project** — lives in `include/` + `src/`, specific to Prototype
  Horde. Wires the kit, defines collision layers, owns gameplay state.

| Module | Tier | Files | Doc |
|---|---|---|---|
| arena | kit | `lib/arena.h` | [arena.md](arena.md) |
| renderer | kit | `lib/renderer.{h,c}` | [renderer.md](renderer.md) |
| physics | kit | `lib/physics.{h,c}` | [physics.md](physics.md) |
| level_manager | kit | `lib/level_manager.{h,c}` | [level_manager.md](level_manager.md) |
| game | project | `include/game.h`, `src/game.c` | [game.md](game.md) |
| camera | project | `include/camera.h`, `src/camera.c` | [camera.md](camera.md) |
| debug | project | `include/debug.h`, `src/debug.c` | [debug.md](debug.md) |

Concrete levels (e.g. `level_sandbox`) are not documented here — they
are short host-side files that follow the `Level` vtable described in
[level_manager.md](level_manager.md). Read the source directly.

For the broader rationale of kit-vs-project boundaries, see
[`../../CLAUDE.md`](../../CLAUDE.md) and
[`../../BLUEPRINT.md`](../../BLUEPRINT.md).
