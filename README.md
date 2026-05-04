# Prototype Horde

A 3D Boxhead-style arena shooter built in C99 on raylib 6.0. Targets
desktop (Windows / Linux) and the Anbernic handheld family (RG35XX
Plus/H/40/V running muOS) in a single codebase.

The repo doubles as a small reusable kit. The contents of `lib/` are
designed to be dropped into other raylib projects — see
[`BLUEPRINT.md`](BLUEPRINT.md).

## Status

In active development. Phases 0, 1, and 2 are done: scaffolding,
fixed-timestep loop, memory arenas, level transition system, debug
overlay, renderer (frustum culling + material sorting + cel shading +
blob shadows), fixed isometric camera, and the physics world (AABB /
sphere / capsule colliders, MoveAndCollide, raycast, overlap).

The repo currently runs a single `level_sandbox` that wires a
WASD-driven dummy capsule against the physics world — placeholder
ground for Phase 3 (player + input system).

See [`PLAN.md`](PLAN.md) for the full phase breakdown and
[`CLAUDE.md`](CLAUDE.md) for working style and code standards.

## Targets

- **Desktop** (primary for development): Windows (MSVC) and native
  Linux. OpenGL 3.3.
- **Handheld** (target hardware): Anbernic RG35XX Plus/H/40/V on muOS.
  ARM Cortex-A53, OpenGL ES 3.0 over SDL2.

## Layout

```
lib/                  ← reusable kit modules (see BLUEPRINT.md)
  arena.h
  level_manager.h, .c
  physics.h, .c
  renderer.h, .c

include/              ← project code
  game_types.h, game.h
  camera_types.h, camera.h
  debug_types.h, debug.h
  config.h, layers.h
  level_sandbox.h
  externals/...       ← rmem, rini, rres, raygui, reasings

src/                  ← project implementation
  main.c, game.c
  camera.c, debug.c
  level_sandbox.c

assets/               ← shaders, models, textures
docs/modules/         ← per-module documentation
```

## Building

### Windows (MSVC)

```sh
cmake -B build-windows
cmake --build build-windows --config Debug
```

The Visual Studio solution lives at
`build-windows/Prototype-Horde.sln`. Set `Prototype-Horde` as startup
project. Working directory is set to the repo root so assets resolve.

### Linux (desktop)

```sh
cmake -B build
cmake --build build
```

### RG35XX cross-compile (from Windows)

Requires the Arm GNU Toolchain (`aarch64-none-linux-gnu`) and the
RG35XX sysroot (see `rg35xx-sysroot/`).

```sh
cmake -B build-rg35xx -DBUILD_FOR_RG35XX=ON \
      -DCMAKE_TOOLCHAIN_FILE=rg35xx-toolchain.cmake
cmake --build build-rg35xx
```

The handheld build defines `PLATFORM_HANDHELD`, which switches
`config.h` to handheld resolution / FPS / pool sizes.

## Dependencies

- **raylib 6.0** — fetched automatically by CMake.
- **rmem.h** — single-header memory pools (vendored under
  `include/externals/rmem/`).
- **raygui.h** — single-header immediate UI for the debug overlay
  (vendored under `include/raygui/`).
- **reasings.h, rini.h, rres.h** — vendored, used opportunistically.

No system package install is needed beyond a C99 toolchain.

## Documentation

- [`CLAUDE.md`](CLAUDE.md) — full project plan, phases, naming
  conventions, decisions.
- [`BLUEPRINT.md`](BLUEPRINT.md) — how to start a new project from the
  `lib/` kit.
- [`docs/modules/`](docs/modules/) — per-module reference (kit and
  project).

## License

TBD.
