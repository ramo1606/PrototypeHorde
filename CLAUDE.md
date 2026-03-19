# Boxhead TPS — CLAUDE.md

Plan completo de proyecto. Referencia autoritativa para la arquitectura, fases, y tareas.

---

## Contexto

Juego TPS inspirado en Boxhead 2Play con mecánicas modernas simplificadas.
Target principal: Anbernic RG35XX H (Allwinner H700, Mali-G31 MP2, 1 GB RAM, 640×480).
Target secundario: PC (resolución variable, 60 FPS).
Experiencia monojugador. Multiplayer local 4P en PC como extensión post-release.

### Decisiones Arquitectónicas Clave

- **C99 idiomático estilo raylib:** structs planos, funciones libres con prefijo `MODULO_` en mayúsculas (`GAME_Init`, `RENDERER_AddMesh`, `PHYS_WORLD_RayCast`), sin herencia.
- **Raylib 5.5 + raygui:** usar todo lo que ofrezca, no reinventar la rueda.
- **CMake + FetchContent:** raylib se descarga automáticamente. Targets para PC (Desktop/OpenGL 3.3) y handheld (SDL/OpenGL ES 3.0). `BUILD_FOR_RG35XX` flag.
- **Librerías externas (header-only):**
  - `rmem.h` — MemPool (arenas de propósito general), ObjPool (pools fijos para entidades), BiStack.
  - `rini.h` — lectura/escritura de archivos .ini para settings y high scores.
  - `reasings.h` — funciones de easing para cámara, UI, transiciones.
  - `raygui.h` — UI inmediata para debug overlay y menús temporales.
- **Fixed-timestep 60 Hz** (`FIXED_TIMESTEP = 1/60`). Render desacoplado a `RENDER_FPS` (30 handheld, 60 PC). Spiral-of-death protection vía `MAX_DELTA_TIME` (0.25s) y `MAX_UPDATES_PER_FRAME` (5).
- **3 Memory Arenas** (rmem MemPool) por lifetime: permanente, nivel, scratch (por frame).
- **ObjPool** de rmem para entidades de tamaño fijo: enemigos, proyectiles, deployables, pickups. O(1) alloc/free.
- **Resource Manager con enums:** carga centralizada, una sola instancia por recurso, lookup O(1).
- **Colisiones:** AABB, esfera, cápsula, ray. 6 combinaciones de test. Sin OBB, sin mesh colliders.
- **Renderer centralizado:** registro/desregistro, frustum culling (Gribb-Hartmann), sorting por material, draw list + stats.
- **Physics World centralizado:** registro de colliders, detección, queries (raycast, overlap, `RayCastIgnore` para disparos).
- **Levels como structs estáticos de function pointers** (`extern Level LEVEL_X`). Reciben `Game*` en todos los callbacks. No heap-allocados.
- **Level Manager con transiciones:** state machine IDLE → FADING_OUT → FADING_IN. Efectos como function pointers (`TransitionEffectFn`). Swap del nivel ocurre con pantalla cubierta.
- **Game struct heap-allocated** en `main()` (~50KB+, evita stack overflow en handheld).
- **Debug overlay con raygui:** compilación condicional (`#ifdef DEBUG_ENABLED`). F1 toggle, F2-F7 paneles. Separación `DEBUG_Render3D()` (gizmos dentro de BeginMode3D) y `DEBUG_Render()` (paneles 2D). FPS graph con historial.
- **Game UI custom sin raygui:** atlas de texturas, fuentes custom, animaciones con reasings.
- **Cel shading** con luz direccional única. Outline opcional (inverted hull). Sombras blob.
- **Assets:** KayKit Character Animations + KayKit Prototype Bits.
- **Arenas de juego:** planas con obstáculos AABB.
- **IA:** steering behaviors (seek + obstacle avoidance). Sin navmesh, sin A*.

### Estructura de Directorios

```
CMakeLists.txt
include/external/
  rmem/rmem.h
  rini/rini.h
  reasings/reasings.h
  raygui/raygui.h
src/
  main.c                    — Entry point. Game heap-allocated.
  external_impl.c           — RMEM_IMPLEMENTATION + RINI_IMPLEMENTATION
  config.h                  — Constantes de plataforma y gameplay
  arena.h                   — Wrapper sobre rmem MemPool (typedef GameArena)
  game.h / game.c           — Orquestador, game loop, subsistemas embebidos
  level.h                   — Struct Level con function pointers tipados
  level_manager.h / .c      — Lifecycle + transiciones con state machine
  resource.h / .c           — Resource manager con enum→filepath
  debug_ui.h / .c           — Debug overlay (raygui), paneles + gizmos 3D
  renderer.h / .c           — Pipeline de render centralizado
  physics.h / .c            — Physics world, colliders, queries
  player.h / .c             — Estado y lógica de jugador
  enemy.h / .c              — Pool de enemigos, IA
  weapon.h / .c             — Definiciones de armas, sistema de disparo
  projectile.h / .c         — Pool de proyectiles
  pickup.h / .c             — Pool de pickups
  score.h / .c              — Combo, multiplicador, thresholds
  camera.h / .c             — Cámara third-person / over-the-shoulder
  arena_map.h / .c          — Datos de arenas de juego (geometría, spawns)
  input.h / .c              — Abstracción de input por jugador
  game_ui.h / .c            — HUD y menús custom (sin raygui)
levels/
  level_menu.h / .c
  level_gameplay.h / .c
  level_results.h / .c
assets/
  models/ textures/ shaders/ sounds/ fonts/
```

### Naming Convention

```
Módulo         Prefijo            Ejemplo
─────────────────────────────────────────────────
Game           GAME_              GAME_Init(), GAME_Run()
Level Manager  LEVEL_MGR_         LEVEL_MGR_TransitionTo()
Renderer       RENDERER_          RENDERER_AddMesh()
Physics        PHYS_WORLD_        PHYS_WORLD_RayCast()
Resource       RESOURCE_          RESOURCE_Load()
Debug          DEBUG_             DEBUG_Render3D()
Arena          ARENA_             ARENA_Create(), ARENA_ALLOC()
Score          SCORE_             SCORE_AddKill()
Player         PLAYER_            PLAYER_Update()
Enemy          ENEMY_             ENEMY_Spawn()
Weapon         WEAPON_            WEAPON_TryShoot()
Transition     TRANSITION_        TRANSITION_Fade()
```

---

## Fase 0 — Scaffolding, Game Loop y Fundaciones ✅

**Objetivo:** Base ejecutable: loop correcto, memoria gestionada, assets validados, niveles intercambiables con transiciones, debug visible.

**Criterio de completitud:** Un programa que abre ventana, corre a timestep fijo con stats visibles en el debug overlay (FPS graph, arena usage), puede alternar entre dos niveles de prueba con fade/wipe transitions, y muestra un modelo KayKit animado cargado via resource manager.

### Tarea 0.1 — Estructura de proyecto y build system ✅

CMakeLists.txt con FetchContent para raylib 5.5. Targets para PC (`Desktop/OpenGL 3.3`) y handheld (`BUILD_FOR_RG35XX`, `SDL/OpenGL ES 3.0`). `DEBUG_ENABLED` automático en builds Debug. Librerías externas en `include/external/`. `external_impl.c` define `RMEM_IMPLEMENTATION` y `RINI_IMPLEMENTATION`.

### Tarea 0.2 — Memory arenas con rmem ✅

`arena.h` — wrapper fino sobre rmem `MemPool`. `typedef MemPool GameArena`. Macros `ARENA_ALLOC(arena, type)` y `ARENA_ALLOC_ARRAY(arena, type, count)`. Funciones inline `ARENA_Create`, `ARENA_Destroy`, `ARENA_Reset`, `ARENA_GetFreeMemory`.

Tres arenas instanciadas por valor en `Game`:
- **Permanente** (2 MB): tablas de definiciones, config. Nunca reseteada.
- **Nivel** (8 MB): pools de entidades, geometría. Reseteada en `LEVEL_MGR_ApplySwap()`.
- **Scratch** (1 MB): temporales por frame. Reseteada cada frame en `GAME_Run()`.

### Tarea 0.3 — Config y constantes por plataforma ✅

`config.h` con `#ifdef PLATFORM_HANDHELD` (resolución 640×480, `RENDER_FPS` 30, pools reducidos) vs PC (1280×720, `RENDER_FPS` 60, pools grandes). `UPDATE_RATE` 60, `FIXED_TIMESTEP` 1/60, `MAX_DELTA_TIME` 0.25s. Constantes de gameplay compartidas.

### Tarea 0.4 — Game loop con fixed timestep ✅

`GAME_Run()` en `game.c`. Acumulador clampeado por `MAX_DELTA_TIME`. Drena en pasos de `FIXED_TIMESTEP`. Spiral-of-death: cap en `MAX_UPDATES_PER_FRAME`, descarta tiempo restante. Alpha de interpolación = remainder / FIXED_TIMESTEP. Frametime stats con reset cada segundo (min/avg/max). Game struct heap-allocated en `main()` con patrón `malloc → GAME_Init → GAME_Run → GAME_Shutdown → free`.

### Tarea 0.5 — Resource manager ✅

`RESOURCE_Init/Shutdown/Load/Unload/LoadGroup/UnloadGroup`. Enum `ResourceID` como índice directo a arrays paralelos. Tabla `resource_table[]` mapea cada enum a filepath. Flag `loaded[]` previene doble carga. Getters con assert si no está cargado.

### Tarea 0.6 — Validación de assets KayKit

Usando `RESOURCE_Load()`, cargar un modelo GLTF de KayKit con sus animaciones. Dibujarlo con `DrawModel()`. Ciclar animaciones con `UpdateModelAnimation()`. Verificar escala, orientación de ejes, texturas/colores.

**Criterio:** El modelo se ve correctamente, las animaciones transicionan sin artefactos, funciona en ambos targets.

### Tarea 0.7 — Level manager con transiciones ✅

`level.h` — `struct Level` con function pointers tipados (`LevelInitFn`, etc.). Todos reciben `Game*`. Levels declarados como `extern Level LEVEL_X` (structs estáticos, zero allocation).

`level_manager.h/c` — State machine de transiciones:
- `TRANSITION_IDLE → FADING_OUT → FADING_IN → IDLE`.
- `LEVEL_MGR_TransitionTo()` con efectos y duración configurables.
- `LEVEL_MGR_SwitchTo()` como convenience con defaults (fade, 0.4s).
- `ApplySwap()` ocurre con pantalla cubierta: shutdown viejo → `ARENA_Reset(&game->level)` → init nuevo.
- Efectos built-in: `TRANSITION_Fade`, `TRANSITION_WipeLeft`, `TRANSITION_WipeRight`.
- Guards: ignora si NULL, si ya transitioning, si mismo nivel.
- `LEVEL_MGR_HandleInput/UpdateLevel/RenderLevel` delegan al nivel activo.

Dos niveles de prueba: Level A (azul, fade) y Level B (rojo, wipe).

### Tarea 0.8 — Debug overlay ✅

`debug_ui.h/c` envuelto en `#ifdef DEBUG_ENABLED`:
- F1 toggle visibilidad, F2-F7 paneles individuales.
- `DEBUG_RegisterPanel(slot, name, drawFn)` para paneles 2D.
- `DEBUG_Register3D(slot, render3DFn)` para gizmos 3D (separados del 2D).
- `DEBUG_Render3D(game)` — llamar dentro de BeginMode3D.
- `DEBUG_Render(game)` — llamar después de EndMode3D.
- **Panel F2 — Performance:** FPS + graph (historial 120 frames), frametime stats, ticks/frame, alpha, barras de uso de arenas, info de transición activa.
- Release: macros vacías, zero cost.

---

## Fase 1 — Renderer y Cámara

**Objetivo:** Ver la escena con cel shading, frustum culling, y cámara funcional.

**Criterio de completitud:** Modelos KayKit con cel shading y sombras blob, cámara third-person siguiendo un jugador falso movible con teclado, frustum culling descartando objetos fuera de vista, stats del renderer visibles en debug overlay F3.

### Tarea 1.1 — Módulo renderer: registro y dibujo básico

Implementar `renderer.h/c`:

- Array de meshes registrados (`MeshComponent*` o `Renderable`). Capacidad `MAX_RENDERABLES`.
- `RENDERER_AddMesh(mc)` / `RENDERER_RemoveMesh(mc)`.
- Draw list separada (meshes que pasan culling) con `DrawEntry` (mesh + distSq a cámara).
- `RENDERER_DrawFrame(renderer, game)` → build draw list → sort → draw.
- Stats internas: `statsDrawn`, `statsCulled`. Registrar en debug overlay panel F3.

### Tarea 1.2 — Interpolación en el renderer

- Cada renderable almacena `transformPrev` y `transformCurr`.
- `RENDERER_DrawFrame()` interpola linealmente posición y rotación usando alpha antes de dibujar.
- Al inicio de cada tick lógico, copiar current a previous para todos los renderables.

### Tarea 1.3 — Frustum culling

- Extraer los 6 planos del frustum de la view-projection matrix (Gribb-Hartmann).
- `RENDERER_IsAABBInFrustum()`, `RENDERER_IsSphereInFrustum()`, `RENDERER_IsPointInFrustum()` como funciones públicas reutilizables.
- Antes de dibujar cada renderable, testear su bounding box/sphere contra los planos.
- Contador de culled por frame en debug overlay.

### Tarea 1.4 — Sorting por material

- Antes de dibujar, ordenar draw list por material/shader ID.
- `qsort` con comparador por material. Con pocos materiales distintos (KayKit), el sort es prácticamente gratis.

### Tarea 1.5 — Cel shader

- Vertex shader: transforma normales a world space, pasa al fragment.
- Fragment shader: dot(normal, lightDir) cuantizado a 3-4 bandas, multiplicado por color de vértice/textura. Una luz direccional, término ambient configurable.
- Asignar el shader via `RESOURCE_GetShader(RES_SHADER_CEL)`.

### Tarea 1.6 — Cámara third-person

Implementar `camera.h/c`:

- Struct `GameCamera` con: target, offset configurable, smoothing factor.
- Dos modos: FOLLOW (detrás y arriba del jugador) y AIM (over-the-shoulder).
- Transición suave entre modos interpolando offset con reasings (`EaseSineInOut`).
- Colisión de cámara con escenario: raycast desde target hacia posición de cámara, si impacta, acercar.
- Actualiza el `Camera3D` de raylib que el renderer usa.

### Tarea 1.7 — Sombras blob

- Quad oscuro semitransparente en el suelo debajo de cada actor con renderable.
- Se escala según altura del actor (si salta, se encoge y aclara).

---

## Fase 2 — Physics World

**Objetivo:** Detección de colisiones con las 6 combinaciones de primitivas, queries de raycast y overlap, debug draw de colliders.

**Criterio de completitud:** Un modelo se mueve por una arena con obstáculos AABB, colisiona correctamente, debug draw muestra colliders, stats de physics en panel F4.

### Tarea 2.1 — Structs de colliders

Definir en `physics.h`:

- `ColliderAABB`, `ColliderSphere`, `ColliderCapsule`, `ColliderRay`.
- `Collider` (union + tipo + layer + owner_id + active flag).
- Enum `CollisionLayer`: PLAYER, ENEMY, PROJECTILE, SCENERY, PICKUP, TRIGGER, DEPLOYABLE.
- `CollisionInfo`: hit, distance, point, normal, collider, actor. (Adoptado del proyecto anterior).
- `ContactInfo`: actorA/B, colliderA/B, contactPoint, contactNormal, penetration.

### Tarea 2.2 — Registro y pool de colliders

- Array fijo de colliders en la arena de nivel. Capacidad `MAX_COLLIDERS`.
- `PHYS_WORLD_AddBox/RemoveBox`, `PHYS_WORLD_AddSphere/RemoveSphere`.
- `PHYS_WORLD_UpdateTransform(handle, position, rotation)`.

### Tarea 2.3 — Tests de colisión: 6 combinaciones

Funciones puras sin estado:

1. `TestRayVsAABB` → bool + CollisionInfo. (Validar contra `GetRayCollisionBox`).
2. `TestSphereVsAABB` → bool + ContactInfo.
3. `TestRayVsCapsule` → bool + CollisionInfo.
4. `TestCapsuleVsAABB` → bool + ContactInfo.
5. `TestCapsuleVsCapsule` → bool + ContactInfo.
6. `TestSphereVsCapsule` → bool + ContactInfo.

### Tarea 2.4 — Detección broad-phase y resolución

`PHYS_WORLD_Update()`:

- Brute-force filtrado por layers (tabla de bits). Opción de sweep-and-prune como estrategia alternativa.
- Generar lista de ContactInfo (en arena scratch).
- Resolución: push-out por normal × depth. Dinámico vs estático: solo mueve dinámico.

### Tarea 2.5 — Queries: raycast y overlap

- `PHYS_WORLD_RayCast(world, ray, maxDist, layerMask, outHit)` → hit más cercano.
- `PHYS_WORLD_RayCastIgnore(world, ray, maxDist, layerMask, ignore, outHit)` → ignora un actor (para que el jugador no se dispare a sí mismo).
- `PHYS_WORLD_OverlapSphere(world, center, radius, layerMask, outActors, maxResults)`.
- Resultados usan arena scratch.

### Tarea 2.6 — Debug draw de colliders + panel F4

- Toggle en panel F4 del debug overlay.
- `DEBUG_Register3D()` para dibujar wireframes dentro de BeginMode3D.
- Verde = estático, amarillo = dinámico, rojo = trigger.
- Panel F4 stats: colliders activos por tipo/layer, tests por frame, contacts resueltos.

---

## Fase 3 — Jugador Completo

**Objetivo:** Character controller con movimiento, salto, aim, animaciones, registrado en renderer y physics.

**Criterio de completitud:** Jugador con movimiento libre, salto, modo aim con cámara over-the-shoulder, animaciones correctas, colisiones con escenario, cel shading.

### Tarea 3.1 — Input abstraction

`input.h/c`:

- Struct `PlayerInput`: moveDir, aimDir, jump, shoot, aim, switchWeapon, use.
- `INPUT_Read(int slot)` → PlayerInput. Dead zones para sticks analógicos.

### Tarea 3.2 — Movimiento libre

- Movimiento en XZ según moveDir × velocidad × dt. Velocidad constante (sin aceleración, fiel a Boxhead).
- Rotación visual del modelo para encarar dirección de movimiento.
- Actualiza collider cápsula en physics world. Physics resuelve colisiones con escenario.

### Tarea 3.3 — Salto

- Velocidad vertical = PLAYER_JUMP_FORCE al presionar jump.
- Gravedad decrementa velocidad vertical cada tick.
- Si Y <= 0: aterriza. Flag `isGrounded` para evitar doble salto.
- Flag `isInvulnerable` durante primeros N frames (configurable, inicialmente 0).

### Tarea 3.4 — Modo aim

- Al mantener aim: velocidad × `PLAYER_AIM_SPEED_MULT` (0.6).
- Rotación sigue aimDir en vez de moveDir. Cámara transiciona a modo AIM.
- Retícula visible (crosshair con `RESOURCE_GetTexture(RES_TEX_CROSSHAIR)` o DrawLine).

### Tarea 3.5 — Animaciones del jugador

State machine: IDLE, RUN, JUMP, AIM_IDLE, AIM_WALK, DIE. Transiciones explícitas. Cada estado mapea a animación KayKit. `UpdateModelAnimation()` cada frame. Tiempos mínimos para evitar flickering.

### Tarea 3.6 — Registro en renderer y physics

- Al crear: `RENDERER_AddMesh()` + `PHYS_WORLD_AddCapsule()`.
- Cada tick: sincroniza transforms.
- Al destruir: desregistra ambos.
- Este patrón se copia exacto para enemigos, proyectiles, deployables, pickups.

---

## Fase 4 — Arma Básica y Enemigo Básico

**Objetivo:** Core loop mínimo: mover, disparar, matar, morir.

**Criterio de completitud:** El jugador dispara zombies con pistola, los zombies persiguen y atacan, el jugador puede morir.

### Tarea 4.1 — Struct de arma y sistema de disparo genérico

`weapon.h/c`:

- `WeaponDef`: nombre, tipo (HITSCAN, PROJECTILE, DEPLOYABLE, AIRSTRIKE), stats, function pointer `OnShoot`.
- `WEAPON_TryShoot(weapon, player)` → chequea cooldown y munición, llama a `OnShoot`.

### Tarea 4.2 — Pistola (hitscan)

- Handler `ShootHitscan`: `PHYS_WORLD_RayCastIgnore()` desde posición del arma en aimDir, ignora al jugador, layer mask ENEMY | SCENERY.
- Si impacta ENEMY: aplica daño. Pistola: cadencia media, daño medio, munición infinita, spread 0.

### Tarea 4.3 — Pool de enemigos y spawn

`enemy.h/c`:

- ObjPool de `Enemy` (rmem). Cada uno: position, health, state (INACTIVE, ALIVE, DYING, DEAD), type, renderer/physics handles.
- `ENEMY_Spawn(position, type)` → alloca de ObjPool, registra en renderer y physics.
- `ENEMY_Kill(id)` → estado DYING, animación muerte, después desregistra y devuelve a ObjPool.

### Tarea 4.4 — Zombie básico: IA seek

- Dirección = normalize(player.pos - zombie.pos). Mover × zombie_speed × dt.
- Si distancia < ZOMBIE_MELEE_RANGE: daño al jugador cada ZOMBIE_ATTACK_COOLDOWN ticks.
- Obstacle avoidance: raycast corto hacia delante, si impacta SCENERY, desviarse perpendicular.

### Tarea 4.5 — Salud del jugador y game over

- Player HP. Enemigos reducen en melee. HP <= 0 → DEAD, game over.
- HUD temporal: `DrawText()` con HP.

### Tarea 4.6 — Paneles de debug F5 y F6

- **Panel F5 — Gameplay:** enemigos activos/total pool, HP, arma equipada, ObjPool stats.
- **Panel F6 — Spawn/Cheats:** botones raygui para spawnear enemigos, kill all, god mode, toggle colisiones.

---

## Fase 5 — Score, Oleadas y Progresión

**Objetivo:** Oleadas crecientes, combo counter, desbloqueos.

**Criterio de completitud:** Oleadas automáticas con dificultad creciente, combo con multiplicador, thresholds de desbloqueo definidos, HUD mostrando score/combo/oleada.

### Tarea 5.1 — Score y combo system

`score.h/c` — `SCORE_AddKill()`, combo_count, combo_multiplier, combo_timer. Tabla de multiplicador.

### Tarea 5.2 — Thresholds de desbloqueo

Tabla (array de structs) mapea score/combo a desbloqueos: `{threshold, unlock_id}`.

### Tarea 5.3 — Sistema de oleadas

Struct `Wave`: {enemy_type, count} + delays. Wave manager spawnea según wave actual, avanza cuando todos muertos.

### Tarea 5.4 — Spawn points y lógica de posición

Arena define spawn points (bordes). Spawner elige los más alejados del jugador.

### Tarea 5.5 — HUD temporal

`DrawText()` para score, combo, multiplicador, oleada, HP, arma, munición.

---

## Fase 6 — Arsenal Completo

**Objetivo:** Todas las armas de Boxhead 2Play.

**Criterio de completitud:** Cada arma tiene su comportamiento, deployables funcionan, explosiones en cadena de barriles, weapon switching fluido.

### Tarea 6.1 — Pool de proyectiles

`projectile.h/c` — ObjPool. Cada proyectil: position, direction, speed, damage, aoe_radius, ttl, handles. Detonación con AOE: `PHYS_WORLD_OverlapSphere()`.

### Tarea 6.2 — Uzi

Hitscan, cadencia alta, spread pequeño, daño bajo.

### Tarea 6.3 — Shotgun

Múltiples raycasts (6-8) en cono. Cadencia baja, daño alto, rango corto.

### Tarea 6.4 — Rockets

Proyectil recto. Al impactar: AOE. Daño decreciente con distancia.

### Tarea 6.5 — Granadas

Proyectil con arco parabólico. Detona por timer. Rebota contra paredes.

### Tarea 6.6 — Railgun

Hitscan penetrante: recolecta TODOS los impactos. Cadencia muy baja, daño altísimo.

### Tarea 6.7 — Pool de deployables

ObjPool. Cada deployable: position, type, health, handles, trigger collider.

### Tarea 6.8 — Minas

Collider esfera como trigger. Enemigo entra → detona AOE. Máximo simultáneo configurable.

### Tarea 6.9 — Barriles

AABB obstáculo. Destructible. Explosión en cadena via `PHYS_WORLD_OverlapSphere()` iterativo con cola.

### Tarea 6.10 — Fake walls

AABB que bloquea ENEMY pero no PLAYER (layer filtering). Destructible.

### Tarea 6.11 — Turrets

Deployable autónomo con esfera de detección. Cada N ticks: hitscan al enemigo más cercano. TTL o munición limitada.

### Tarea 6.12 — Airstrike

Timer + posición marcada. Al completar: AOE masiva. Usos muy limitados.

### Tarea 6.13 — Weapon switching e inventario

Array de armas desbloqueadas. Ciclar con L1/R1. HUD muestra arma actual. Delay al cambiar.

---

## Fase 7 — Todos los Enemigos

**Objetivo:** 4 tipos con IA diferenciada y oleadas balanceadas.

### Tarea 7.1 — Zombie rápido (Devil)

Misma IA seek. Velocidad mayor, más HP. Validar tunneling a alta velocidad.

### Tarea 7.2 — Zombie a distancia (Mummy)

Mantiene distancia. Dispara proyectil (layer ENEMY_PROJECTILE que colisiona con PLAYER).

### Tarea 7.3 — Zombie tanque

Lento, HP altísimo, daño alto. Cápsula mayor. Resistencia parcial a AOE.

### Tarea 7.4 — Panel de debug de IA (F7)

`DEBUG_Register3D()`: líneas target, estados (SEEK/AVOID/ATTACK/RETREAT/IDLE), vectores de steering.

### Tarea 7.5 — Balanceo de oleadas

Composición gradual. Curva de dificultad alineada con desbloqueo de armas. **Iterativo, requiere playtesting.**

---

## Fase 8 — Pickups y Upgrades

**Objetivo:** Sistema de progresión intra-partida.

### Tarea 8.1 — Pool y spawn de pickups

ObjPool. Cada pickup: position, type, renderable (rotando), trigger esfera.

### Tarea 8.2 — Pickups de munición y salud

AMMO restaura munición. HEALTH restaura HP. Spawn aleatorio o drop de enemigos.

### Tarea 8.3 — Upgrade system

Niveles de upgrade por arma. Mejora stats. Desbloqueo automático por combo/score thresholds.

### Tarea 8.4 — Feedback visual

Texto flotante (+AMMO, UPGRADE!). Pool fijo de floating texts. Efecto de partículas.

---

## Fase 9 — Niveles, Menús y Flow

**Objetivo:** Experiencia completa de principio a fin.

### Tarea 9.1 — Menú principal

Implementado como Level (`LEVEL_MENU`). Opciones con raygui (temporal). Usa `LEVEL_MGR_TransitionTo()` para navegar.

### Tarea 9.2 — Selección de arena

Preview visual. Selección y confirmación → transiciona al gameplay.

### Tarea 9.3 — Múltiples arenas

3-4 arenas con layouts diferentes. Cada arena: struct con AABBs, spawn points, modelos de escenario.

### Tarea 9.4 — Pantalla de resultados

Score final, oleada, kills por arma, combo máximo, tiempo. Opciones: reintentar, menú. Transición con fade.

### Tarea 9.5 — Transiciones entre niveles

Ya implementadas en el Level Manager (Fase 0). Aquí se configuran los efectos específicos: fade para menú→gameplay, wipe para gameplay→results, etc.

### Tarea 9.6 — Pantalla de pausa

Overlay. Toggle con Start/Escape. Resume, Restart, Quit to Menu. Game loop sigue renderizando (escena congelada).

### Tarea 9.7 — Settings persistentes con rini

`rini_load("settings.ini")` al inicio. Volumen, sensibilidad, controles. `rini_save()` al cambiar. High scores en `scores.ini`.

---

## Fase 10 — Polish Visual, Audio y UI Final

**Objetivo:** Juego pulido con feel, audio, y UI estilizada.

### Tarea 10.1 — Outline shader (opcional)

Inverted hull o Sobel post-proceso. Testear en handheld, desactivar si excesivo.

### Tarea 10.2 — Sistema de partículas

Pool fijo. Billboard quads. Tipos: explosión, chispas, muzzle flash, humo, confetti.

### Tarea 10.3 — Screen shake

Offset aleatorio de cámara. Magnitud proporcional a cercanía/potencia. Decae con `EaseExpoOut` (reasings).

### Tarea 10.4 — Hit feedback

Flash blanco (shader uniform). Número de daño flotante. Knockback cosmético. Flash de daño en bordes de pantalla.

### Tarea 10.5 — Audio

Raylib audio. Efectos por arma. Música de fondo. Vía `RESOURCE_Load(RES_SND_*)`. Volumen configurable.

### Tarea 10.6 — Game UI custom: HUD

`game_ui.h/c` — Atlas de texturas, fuente custom. Barra de HP (verde→rojo). Iconos de armas + munición. Combo counter animado (pulso con `EaseBounceOut`). Indicador de oleada. Indicadores de daño direccional. Todo con `DrawTexturePro()` y `DrawTextEx()`.

### Tarea 10.7 — Game UI custom: menús

Reemplazar menús raygui de Fase 9. Transiciones con reasings (`EaseBackOut` para elementos que entran deslizando).

---

## Fase 11 — Optimización para Handheld

**Objetivo:** 30 FPS estables en la RG35XX H.

### Tarea 11.1 — Profiling en hardware real

Panel F2 del debug overlay + `GetTime()`. Identificar bottleneck: CPU vs GPU.

### Tarea 11.2 — Cap de enemigos dinámico

Reducir `MAX_ENEMIES` en config handheld. Encolar excedentes.

### Tarea 11.3 — LOD de animaciones

Enemigos lejanos: `UpdateModelAnimation()` cada 2-3 frames.

### Tarea 11.4 — Batching / Instancing

Evaluar `DrawMeshInstanced()` para grupos de enemigos idénticos.

### Tarea 11.5 — Shader simplificado para handheld

Versión sin cuantización, sin outline. Controlado por config.h.

### Tarea 11.6 — Resolución de render reducida

Renderizar a 320×240 en `RenderTexture2D`, escalar a 640×480. Look low-res + cel shading puede ser estético.

---

## Fase 12 — Multiplayer Local (PC only, post-release)

**Objetivo:** 4 jugadores en split-screen en PC.

### Tarea 12.1 — PlayerManager multi-slot

Expandir a 4 slots. Mapear gamepads. Gestionar join/leave.

### Tarea 12.2 — Split-screen

4 viewports con `BeginScissorMode()` o render textures. Layout: 2P horizontal, 3-4P quadrants.

### Tarea 12.3 — 4× render pass

Renderer dibuja 4 veces con 4 frustums independientes.

### Tarea 12.4 — IA multi-target

Enemigos eligen jugador más cercano. Recalculan cada N ticks.

### Tarea 12.5 — HUD por jugador

Cada viewport tiene su propio HUD posicionado dentro del viewport.

### Tarea 12.6 — Balanceo multiplayer

Más jugadores = más enemigos, más HP, mejores drops. Requiere playtesting.

---

## Resumen de Dependencias

```
Fase 0 (loop + arenas + resources + levels + transitions + debug) ✅
  └→ Fase 1 (renderer + cámara + cel shading)
       └→ Fase 2 (physics world + colliders + queries)
            └→ Fase 3 (jugador completo)
                 └→ Fase 4 (pistola + zombie básico = core loop)
                      └→ Fase 5 (score + oleadas + progresión)
                           └→ Fase 6 (arsenal completo)
                           └→ Fase 7 (todos los enemigos)
                                └→ Fase 8 (pickups + upgrades)
                                     └→ Fase 9 (menús + flow completo)
                                          └→ Fase 10 (polish + audio + UI final)
                                               └→ Fase 11 (optimización handheld)
                                                    └→ Fase 12 (multiplayer PC)
```

Cada fase produce un ejecutable funcional. No hay fases de "infraestructura invisible".