# GitHub Copilot Instructions for Prototype Horde

This file is the GitHub Copilot equivalent of `CLAUDE.md`. Follow it as the working guide for this repository.

The user is an experienced C++ developer with some game dev experience. Use technical language and prefer concise explanations.
Chat with the user should be in Spanish, but documentation and code must be in English.

The other top-level docs:

- [`README.md`](../README.md) — public description of the project.
- [`PLAN.md`](../PLAN.md) — phases, tasks, current status.
- [`REFERENCES.md`](../REFERENCES.md) — books, articles, code examples to consult.
- [`RAYLIB_STANDARD.md`](../RAYLIB_STANDARD.md) — code standard matching raylib.
- [`BLUEPRINT.md`](../BLUEPRINT.md) — how to start a new project from `lib/`.
- [`docs/modules/`](../docs/modules/) — per-module reference (one file per module).

---

## AI workflow

The fun part for the user is writing code and solving problems. AI is here to think *with* the user, not *for* them. Default behavior:

### Do

- **Discuss, plan, design.** Brainstorm options, surface tradeoffs, propose architecture, sketch APIs.
- **Review.** When asked, review a diff, file, or full branch. Report findings ranked by severity.
- **Investigate.** Read code, search, trace dependencies, and summarize.
- **Generate scaffolding.** Boilerplate the AI is faster at — file templates, config, build glue, doc skeletons — but only when asked.
- **Run shell, build, test.** Diagnostics and verification are AI's fast lane.

### Don't

- **Don't write game logic without explicit ask.** If the task is "add the jump system", discuss the design first unless the user explicitly asks Copilot to implement it.
- **Don't auto-fix what you find in review.** Surface the finding; let the user decide or explicitly ask for the fix.
- **Don't expand scope.** If a task says "rename foo to bar", don't also reorganize the file. Match the request.
- **Don't write speculative code.** No "in case we need it later", though you can mention ideas.
- **Don't write comments unless asked or clearly needed.** Code is for what; comments are for the rare why.

### Triggers that flip you to write-mode

These mean: yes, write code now.

- `implementá X`, `escribí esto`, `hacelo vos`
- `refactor X siguiendo el plan` (when a plan was already agreed)
- `fix this bug` (after root cause was discussed)
- Closing tasks at the boundary between phases (kit work, build glue, doc generation)

### Code reviews

When the user asks for a review:

1. List findings by severity: **blocker / major / minor / nit**.
2. For each: `file:line`, what, why it matters, suggested fix.
3. No prose summary unless asked.
4. Don't apply fixes unless explicitly requested.

### Brainstorming

When the user is exploring options:

1. Give 2-4 alternatives with real tradeoffs.
2. State which one you would pick and why, in one sentence.
3. Stop and wait for the user's direction.

---

## Code standards

### Language

- C99. No C++. No designated-init extensions outside the standard.
- Single source of truth, no header-impl split into more than `.h` + `.c`.
- Follow raylib style and standards.

### File organization

Reusable modules (`lib/`) ship as a single `.h` + `.c` per module. No `_types.h` split for the lib. Each `.h` opens with:

```text
DEPENDENCIES: raylib.h, ...
OVERRIDES (define before include):
  MAX_X (default N)
```

Project code (`include/` + `src/`) can split into `<module>_types.h` + `<module>.h` + `<module>.c` if it helps reasoning, but isn't required.

### Comments

Default to none. Only write a comment when the *why* is non-obvious:

- Hidden constraint or invariant.
- Workaround for a specific bug or library quirk.
- Surprising behavior a future reader would not predict.

If removing the comment would not confuse a future reader, don't write it. Don't restate what the code does.

### Documentation

Long-form explanation lives in `docs/modules/<module>.md`, not in inline comments. Algorithms, complexity, references, and edge cases go there.

### Architectural rules

- **Simple over clever.** Optimize only with measurable gain. A loop over 200 elements beats a fancy spatial structure unless profiling proves otherwise.
- **No speculative abstractions.** Three similar lines is better than a premature wrapper. Build the abstraction when there's a third caller, not before.
- **No error handling for impossible cases.** Trust internal code. Validate at boundaries (user input, file load, GPU upload, network).
- **No backwards-compat shims.** Change the code.
- **One source of truth.** Reusable modules in `lib/` are *the* version. Don't maintain a separate amalgamated copy.

---

## Git workflow

### Branches

Branch off `main`. Naming convention:

- `phase-N/<task>` — work on a phase task (for example `phase-3/input-system`)
- `fix/<short-desc>` — bug fix
- `refactor/<short-desc>` — refactor without behavior change
- `kit/<short-desc>` — change to anything under `lib/`
- `docs/<short-desc>` — docs-only change

### Commits

Imperative mood, present tense. Format:

```text
<scope>: <short summary>

<optional body — what + why, not how>
```

`<scope>` matches the affected area: `renderer`, `physics`, `game`, `docs`, `build`, and so on.

Examples:

- `camera: rewrite as fixed isometric`
- `physics: drop CollisionLayer enum, use int bitfields`
- `docs: split CLAUDE.md into PLAN.md + REFERENCES.md`

One logical change per commit. Don't bundle unrelated work.

### When AI handles git

Copilot should branch, commit, or push **only when explicitly asked**. Triggers:

- `commit esto` / `commit and push`
- `creá una branch para X y commiteá lo que hicimos`
- `pusheá`

If unsure, ask. Never:

- force-push
- push directly to `main`
- skip hooks with `--no-verify`
- amend a commit that was already pushed

### PRs

Title: same format as commits. Body:

```md
## Summary
<bullet points: what changed and why>

## Test plan
- [ ] <how to verify>
```

Don't open a PR unless the user asks.

---

## Project specifics

### Targets

- **Desktop** (development): Windows MSVC / Linux. OpenGL 3.3.
- **Handheld**: Anbernic RG35XX family on muOS, OpenGL ES 3.0 over SDL2. CMake flag `BUILD_FOR_RG35XX=ON` defines `PLATFORM_HANDHELD`.

### Build commands

```sh
# Desktop Debug
cmake -B build-windows
cmake --build build-windows --config Debug

# Desktop Release
cmake --build build-windows --config Release

# Handheld
cmake -B build-rg35xx -DBUILD_FOR_RG35XX=ON \
      -DCMAKE_TOOLCHAIN_FILE=rg35xx-toolchain.cmake
cmake --build build-rg35xx
```

### Where things live

```text
lib/                ← reusable kit (arena, renderer, physics, level_manager)
include/            ← project headers (game, camera, debug, config, layers)
src/                ← project implementation
assets/             ← shaders, models, textures
docs/modules/       ← per-module reference docs
```

### What's a kit module vs project code

Kit modules (`lib/`):

- No knowledge of `Game` or any host type.
- No project-specific enums or constants.
- Self-contained; depend only on raylib (and optionally rmem).
- Defaults overrideable via `#ifndef X` / `#define X N` / `#endif`.

Project code:

- Owns the `Game` type and all subsystem wiring.
- Defines collision layers, gameplay constants, and render glue (cel shader, blob shadows).
- Adapts the kit to this specific game.

If a change in `lib/` would only make sense for Boxhead 3D, it belongs in the project, not the kit.
