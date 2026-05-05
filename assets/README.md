# Assets

Game assets shipped with Prototype Horde. CMake copies this directory
verbatim to the build output, so paths used at runtime are relative to
the repo root (e.g. `assets/characters/player.glb`).

## Layout convention

Follows raylib's recommendation (see
[`RAYLIB_STANDARD.md §8`](../RAYLIB_STANDARD.md)): group by context
and load timing, not by file type.

```
assets/
  audio/
    fx/                long_jump.wav, pistol_fire.wav, ...
    music/             main_theme.ogg, wave_loop.ogg
  screens/
    logo/              logo.png
    title/             title.png
    gameplay/          background.png, hud_atlas.png
  characters/          player.glb, zombie_basic.glb, ...
  weapons/             pistol.glb, shotgun.glb, ...
  arenas/              arena_01.glb, arena_02.glb
  common/              font_main.ttf, gui.png, palette.png
  data/                waves.rini, weapons.rini
```

## Naming rules

- `snake_case` for files and directories.
- No spaces, no special characters.
- Descriptive names — the filename should explain the asset without
  opening it.
- Group what loads together (a level loads from a single subdirectory
  when possible).

## What goes where

- **`audio/fx`** — short, one-shot sound effects.
- **`audio/music`** — looping tracks.
- **`screens/<name>`** — assets owned by a single screen / state.
- **`characters/`, `weapons/`, `arenas/`** — gameplay-shared models.
- **`common/`** — shared across the whole game (fonts, GUI atlas).
- **`data/`** — config and definition tables (rini, rres, json).
