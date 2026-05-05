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
- Patterns I use directly: Game Loop, Update Method, Component, Object Pool, Service Locator (avoided here — see Service Locator chapter for why), Type Object.
- Style: practical, opinionated, C++-flavored but applicable to C.

### Game Engine Architecture (3rd ed.) — Jason Gregory
- Reference for: subsystem layering, memory management strategies,
  fixed-timestep rationale, animation pipeline.
- Heavy. Good for "why" and big picture, less for "show me the code".

### 3D Math Primer for Graphics and Game Development — Fletcher Dunn
- Linear algebra and 3D math from a games perspective.
- Use for: matrix vs quaternion tradeoffs, slerp, frustum math, projection.

### Rendering / graphics

#### Real-Time Rendering (4th ed.)
- Reference for: modern real-time rendering pipeline, lighting models,
  visibility, shading, and GPU-era rendering tradeoffs.

#### Learn OpenGL
- Practical reference for: OpenGL fundamentals, coordinate spaces,
  materials, lighting, shadow maps, framebuffers, and post-process flow.

#### Graphics Shaders
- Reference for: shader structure, GPU shading stages, and material /
  effect implementation ideas.

#### Computer Graphics Programming in OpenGL with C++
- Use for: graphics programming patterns, OpenGL implementation details,
  and rendering-oriented C++ examples that can be adapted to C.

#### Fundamentals of Computer Graphics (5th ed.)
- Reference for: graphics fundamentals, transformations, cameras,
  rasterization, shading, ray tracing, and rendering math.

#### Foundations of Game Engine Development, Volume 1: Mathematics
- Use for: engine-facing math foundations, spaces, transforms,
  quaternions, matrices, and geometric reasoning.

#### Foundations of Game Engine Development, Volume 2: Rendering
- Use for: renderer architecture, visibility, lighting, and practical
  engine-side rendering structure.

### Gameplay / engine programming

#### Practical C++ Game Programming with Data Structures and Algorithms — Zhenyu George Li
- Reference for: data-structure choices in gameplay code, performance
  tradeoffs, and practical game-oriented algorithm selection.

#### Game Programming in C++ — Sanjay Madhav
- Good practical reference for: game loop structure, actor/component
  design, animation, input, UI, and engine organization.

#### Game Coding Complete (4th ed.)
- Reference for: full-game architecture, gameplay systems, tools, and
  production-minded engine organization.

#### C++ Game Animation Programming (2nd ed.)
- Use for: animation systems, skeletal animation workflow, blending,
  state transitions, and runtime animation structure.

### Collision / physics

#### Real-Time Collision Detection
- Primary reference for: geometric queries, closest-point tests, broad
  phase / narrow phase thinking, and robust collision math.

#### Game Physics Engine Development (2nd ed.)
- Reference for: rigid body basics, constraint solving, collision
  response, and physics engine structure.

#### Physics for Game Developers (2nd ed.)
- Use for: gameplay-oriented physics intuition, practical formulas, and
  approachable implementation guidance.

### AI

#### Game AI Pro series
- Reference for: production game AI techniques, steering, planning,
  tactical behaviors, and practical shipped-game patterns.

### Language / fundamentals

#### The C Programming Language
- Ground truth for: C fundamentals, idioms, language constraints, and
  keeping core code simple and precise.

### Spanish-language game development references

#### Desarrollo de Videojuegos: Enfoque Practico, Vol. 1
- General reference in Spanish for foundational game development topics.

#### Desarrollo de Videojuegos: Enfoque Practico, Vol. 2
- General reference in Spanish for intermediate game development topics.

#### Desarrollo de Videojuegos: Enfoque Practico, Vol. 3
- General reference in Spanish for advanced game development topics.

#### Desarrollo de Videojuegos: Enfoque Practico, Vol. 4
- General reference in Spanish for specialized / later-stage game
  development topics.

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
