# REFERENCES.md — Books, articles, code I want considered

When you (the AI) help me with design, brainstorming, or review, draw
on the material below. If a topic comes up that maps to one of these,
mention the reference so I can read more.

I'll grow this list over time. Add an entry only when I tell you to,
or propose one and ask.

---

## Books

### Game Programming Patterns — Robert Nystrom
- Online: https://gameprogrammingpatterns.com/
- Patterns I use directly: Game Loop, Update Method, Component, Object
  Pool, Service Locator (avoided here — see Service Locator chapter
  for why), Type Object.
- Style: practical, opinionated, C++-flavored but applicable to C.

### Game Engine Architecture (3rd ed.) — Jason Gregory
- Reference for: subsystem layering, memory management strategies,
  fixed-timestep rationale, animation pipeline.
- Heavy. Good for "why" and big picture, less for "show me the code".

### 3D Math Primer for Graphics and Game Development — Fletcher Dunn
- Linear algebra and 3D math from a games perspective.
- Use for: matrix vs quaternion tradeoffs, slerp, frustum math, projection.

---

## Articles / blog posts

### Fix Your Timestep! — Glenn Fiedler
- https://gafferongames.com/post/fix_your_timestep/
- The accumulator pattern in our `GameRun` loop comes from here.
- Spiral-of-death protection too.

### Fast Extraction of Viewing Frustum Planes — Gribb & Hartmann
- The technique used in `RendererExtractFrustumPlanes`.
- PDF circulates online; search the title.

### Casey Muratori — "Computer Architecture, A Quantitative Approach"
- For when performance optimization comes up. Cache lines, branch
  prediction, etc.
- His "Performance-Aware Programming" course covers practical cases.

### Sebastian Lague — YouTube series
- Procedural generation, marching cubes, pathfinding visuals.
- Useful for level/arena generation if we go procedural.

---

## Code references

### raylib examples
- https://github.com/raysan5/raylib/tree/master/examples
- Always look here first when learning a raylib feature.
- Particularly: `models/*`, `shaders/*`, `audio/*`.

### raygui examples
- https://github.com/raysan5/raygui/tree/master/examples
- Reference for: immediate-mode UI patterns, custom styles.

### stb_image / stb_truetype (Sean Barrett)
- https://github.com/nothings/stb
- Reference for: single-header library style, the "OVERRIDES before
  include" idiom we adopted.

### rmem (raylib companion)
- Vendored at `include/externals/rmem/rmem.h`.
- Provides MemPool (our arena), ObjPool (used in Phase 4+ for pools),
  BiStack.

### Boxhead 2Play — original Flash game
- For mechanic reference. Movement feel, weapon balance, wave pacing.
- Find via Flashpoint archive.

---

## Video references

### Handmade Hero — Casey Muratori
- youtube.com/c/MollyRocket
- The first ~50 episodes cover platform layer, fixed timestep,
  software renderer math, and memory arenas. Mostly applicable.

### Tsoding — ad-hoc C streams
- youtube.com/@TsodingDaily
- Useful for: minimalist C style, debugging mindset, terminal workflow.

---

## Conventions / style

### "Practice of Programming" — Brian Kernighan & Rob Pike
- Naming, error handling, debugging discipline. Short and sharp.

### Linux kernel coding style (selectively)
- https://www.kernel.org/doc/html/latest/process/coding-style.html
- For: brace style, function length, comment style. We don't follow it
  literally (we use raylib's PascalCase) but the spirit is similar.

---

## How to use this list

When I'm stuck on a design question, you can say something like:

> "Game Programming Patterns has a chapter on Object Pool that covers
> the trade-off you're hitting (free list vs. flag) — worth a 5-min read
> before deciding."

Don't quote-and-paste from sources. Just point me at the right place.
