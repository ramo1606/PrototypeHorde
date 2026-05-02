# Boxhead 3D — AGENTS.md

Plan completo de proyecto. Referencia autoritativa para la arquitectura, fases, y tareas.

---

## Contexto

Clon de Boxhead 2Play con assets 3D y cámara isométrica con perspectiva baja.
Mecánicas fieles al original (movimiento 8 direcciones, salto, oleadas crecientes,
arsenal completo, deployables) sobre un motor 3D simple con cel shading.

Target principal: familia Anbernic completa, incluyendo dispositivos sin sticks
analógicos (RG35XX, RG35XX H, RG40XX, etc.). Resolución variable, 30 FPS estables.
Target secundario: PC. Resolución variable, 60 FPS.

Experiencia monojugador + multiplayer local 2-4 jugadores con pantalla compartida
(cámara común que sigue al centroide del grupo).

### Decisiones Arquitectónicas Clave

- **C99 idiomático estilo raylib:** structs planos, funciones libres, sin herencia.
- **Naming PascalCase:** sin guiones bajos. Prefijo de módulo en API pública para
  evitar colisiones con raylib y entre módulos. Static helpers internos en camelCase.
- **Organización por módulo en 3 archivos:** `<module>_types.h` (estructuras),
  `<module>.h` (API), `<module>.c` (implementación). Documentación detallada en
  `docs/modules/<module>.md`.
- **Comentarios minimalistas en código.** Solo el "porqué" no obvio. La explicación
  detallada de algoritmos, complejidad y referencias va en los docs por módulo.
- **Raylib 5.5 + raygui:** usar todo lo que ofrezca, no reinventar la rueda.
- **CMake + FetchContent:** raylib se descarga automáticamente. Targets para PC
  (Desktop/OpenGL 3.3) y handheld (SDL/OpenGL ES 3.0). Flag `BUILD_FOR_RG35XX`.
- **Librerías externas (header-only):**
  - `rmem.h` — MemPool (arenas), ObjPool (pools fijos), BiStack.
  - `rini.h` — lectura/escritura de archivos .ini para settings y high scores.
  - `reasings.h` — funciones de easing para cámara, UI, transiciones.
  - `raygui.h` — UI inmediata para debug overlay.
- **Cámara isométrica fija con perspectiva baja** (~30°). Sin rotación libre, sin
  modo aim. La cámara sigue al jugador (o centroide en multiplayer) con smoothing.
- **Input 8 direcciones + strafe lock:** sin sticks analógicos. D-pad mueve y
  apunta en la misma dirección. Botón L mantiene la mira fija mientras se mueve
  (strafe). Mapeo completo en sección "Input".
- **Fixed-timestep 60 Hz** (`FIXED_TIMESTEP = 1/60`). Render desacoplado a
  `RENDER_FPS` (30 handheld, 60 PC). Spiral-of-death protection vía
  `MAX_DELTA_TIME` (0.25s) y `MAX_UPDATES_PER_FRAME` (5).
- **3 Memory Arenas** (rmem MemPool) por lifetime: permanente, nivel, scratch.
- **ObjPool** de rmem para entidades de tamaño fijo: enemigos, proyectiles,
  deployables, pickups. O(1) alloc/free.
- **Resource Manager con enums:** carga centralizada, una sola instancia por
  recurso, lookup O(1) directo (no búsqueda lineal).
- **Colisiones:** AABB, esfera, cápsula, ray. 6 combinaciones de test.
- **Renderer centralizado:** registro/desregistro, frustum culling
  (Gribb-Hartmann), sorting por material, draw list con stats.
- **Physics World centralizado:** registro de colliders, detección, queries
  (raycast, overlap, ignore variant para disparos).
- **Levels como structs estáticos de function pointers**
  (`extern Level Level_X`). Reciben `Game*` en todos los callbacks. No
  heap-allocados.
- **Level Manager con transiciones:** state machine IDLE → FADING_OUT → FADING_IN.
- **Game struct heap-allocated** en `main()` (~50KB+, evita stack overflow).
- **Debug overlay con raygui:** compilación condicional (`#ifdef DEBUG_ENABLED`).
- **Game UI custom sin raygui:** atlas de texturas, fuentes custom, animaciones
  con reasings.
- **Cel shading** con luz direccional única. Outline opcional. Sombras blob.
- **Assets:** KayKit Character Animations + KayKit Prototype Bits.
- **Arenas planas** con obstáculos AABB (free-form, no grid).
- **IA:** steering behaviors (seek + obstacle avoidance). Sin navmesh, sin A*.

### Filosofía de código

**Simple y legible antes que clever.** Solo introducimos optimizaciones complejas
cuando la ganancia es grande y medible. Un loop O(n) sobre 200 elementos es
preferible a una estructura espacial elaborada. Si profiling indica un cuello,
ahí sí optimizamos.

### Naming Convention

PascalCase para toda la API pública. Prefijo del módulo cuando alguna función
del módulo colisione (típico: `Init`, `Update`, `Shutdown`). Por consistencia,
si un módulo necesita prefijo en alguna función, lo usa en todas sus públicas.

```
Módulo            Estilo                  Ejemplo
─────────────────────────────────────────────────────────────────
Game              GameXxx                 GameInit, GameRun
LevelManager      LevelManagerXxx         LevelManagerTransitionTo
Renderer          RendererXxx             RendererBuildDrawList
Physics           PhysicsXxx              PhysicsRayCast
Resource          ResourceXxx             ResourceLoad
Debug             DebugXxx                DebugRegister3D
Arena             ArenaXxx                ArenaCreate, ArenaReset
Score             ScoreXxx                ScoreAddKill
Player            PlayerXxx               PlayerUpdate
Enemy             EnemyXxx                EnemySpawn
Weapon            WeaponXxx               WeaponTryShoot
Camera            CameraXxx               CameraSetTarget
Input             InputXxx                InputRead
Transition        TransitionXxx           TransitionFade
```

Macros de conveniencia (alloc, etc.) en MAYÚSCULAS con guión bajo:
`ARENA_ALLOC`, `ARENA_ALLOC_ARRAY`. Solo macros, no funciones.

Static helpers dentro de un .c: camelCase sin prefijo (`findFreeSlot`,
`lerpMatrix`, `closestPointOnAABB`).

Tipos: PascalCase (`PhysWorld`, `Renderable`, `CollisionInfo`).

### Estructura de Directorios

```
CMakeLists.txt
include/
  external/
    rmem/rmem.h
    rini/rini.h
    reasings/reasings.h
    raygui/raygui.h
  config.h                    Constantes de plataforma y gameplay
  arena_types.h / arena.h     Wrapper sobre rmem MemPool
  game_types.h / game.h       Orquestador, game loop, subsistemas embebidos
  level_types.h               Struct Level con function pointers
  level_manager_types.h / level_manager.h
  resource_types.h / resource.h
  debug_types.h / debug.h
  renderer_types.h / renderer.h
  physics_types.h / physics.h
  player_types.h / player.h
  enemy_types.h / enemy.h
  weapon_types.h / weapon.h
  projectile_types.h / projectile.h
  pickup_types.h / pickup.h
  score_types.h / score.h
  camera_types.h / camera.h
  arena_map_types.h / arena_map.h
  input_types.h / input.h
  game_ui_types.h / game_ui.h
src/
  main.c                      Entry point. Game heap-allocated.
  external_impl.c             RMEM_IMPLEMENTATION + RINI_IMPLEMENTATION
  game.c
  level_manager.c
  resource.c
  debug.c
  renderer.c
  physics.c
  player.c
  enemy.c
  weapon.c
  projectile.c
  pickup.c
  score.c
  camera.c
  arena_map.c
  input.c
  game_ui.c
levels/
  level_menu.c
  level_gameplay.c
  level_results.c
docs/
  modules/
    arena.md
    physics.md
    renderer.md
    ...
assets/
  models/ textures/ shaders/ sounds/ fonts/
```

**Excepciones:** módulos muy pequeños (ej. `level.h` que solo declara una
struct de function pointers) pueden quedar en un solo archivo header.

### Documentación por módulo

Cada `docs/modules/<module>.md` contiene:

1. **Resumen** del módulo (1 párrafo).
2. **Tabla de funciones públicas** — una línea por función con su propósito.
3. **Secciones detalladas** para funciones complejas o con carga técnica:
   algoritmo paso a paso, complejidad, referencias bibliográficas, edge cases.
4. **Diagrama de uso típico** cuando aplique.

El código se mantiene minimalista. Solo se comenta el "porqué" no obvio. Para
entender el "cómo", el lector va al doc del módulo.

### Input — Mapeo de Botones (Anbernic)

```
D-pad    Mover (8 direcciones)  +  Apuntar (misma dirección)
A        Disparar
B        Saltar
X        Cambiar arma
Y        Usar (deployable / interactuar)
L        Strafe lock — fija la mira en la última dirección apuntada
         mientras se mantiene; D-pad solo mueve
R        Arma secundaria (granada / airstrike)
Start    Pausa
Select   (reservado, debug en builds Debug)
```

En PC: WASD = D-pad, Mouse = apuntar (override del strafe), click izq = A,
Espacio = B, Q = X, E = Y, Shift izq = L, click der = R.

---

## Fase 0 — Scaffolding, Game Loop y Fundaciones ✅

Base ejecutable: loop correcto, memoria gestionada, assets validados, niveles
intercambiables con transiciones, debug visible.

### Tarea 0.1 — Estructura de proyecto y build system ✅
### Tarea 0.2 — Memory arenas con rmem ✅
### Tarea 0.3 — Config y constantes por plataforma ✅
### Tarea 0.4 — Game loop con fixed timestep ✅
### Tarea 0.5 — Resource manager ✅
### Tarea 0.6 — Validación de assets KayKit
### Tarea 0.7 — Level manager con transiciones ✅
### Tarea 0.8 — Debug overlay ✅

### Tarea 0.9 — Refactor a nueva arquitectura

Refactor incremental, módulo por módulo:
- Split en `<module>_types.h` + `<module>.h` + `<module>.c`.
- Renombrado a PascalCase con prefijos de módulo.
- Comentarios minimalistas en código.
- Generación de `docs/modules/<module>.md` por cada módulo refactorizado.
- Compila y corre después de cada módulo.

Orden sugerido (de chico a grande):
1. `arena` (memoria) — el más simple.
2. `resource` — pequeño y bien delimitado.
3. `debug` — pocas dependencias.
4. `camera` — reescrito completo en Tarea 1.6 (cámara iso).
5. `level` + `level_manager` — testeo de transiciones.
6. `renderer` — grande pero bien encapsulado.
7. `physics` — grande, con bugs ya identificados.
8. `game` — tira los hilos del resto.
9. Update de los `level_test_*` al nuevo estilo.

---

## Fase 1 — Renderer y Cámara Isométrica

Ver la escena con cel shading, frustum culling, y cámara isométrica fija
siguiendo al jugador.

**Criterio de completitud:** modelos KayKit con cel shading y sombras blob,
cámara isométrica con perspectiva baja siguiendo al jugador, frustum culling
descartando objetos fuera de vista, stats del renderer en debug overlay F3.

### Tarea 1.1 — Módulo renderer: registro y dibujo básico ✅
### Tarea 1.2 — Interpolación en el renderer ✅
### Tarea 1.3 — Frustum culling ✅
### Tarea 1.4 — Sorting por material ✅ (bug arreglado)
### Tarea 1.5 — Cel shader ✅

### Tarea 1.6 — Cámara isométrica con perspectiva baja [REWRITE]

Reemplaza la cámara orbit/third-person de la versión anterior.

Implementación:
- `Camera3D` con `projection = CAMERA_PERSPECTIVE`, `fovy` bajo (~25-35°).
- Posición fija relativa al target: ángulo de elevación ~30°, ángulo azimut
  fijo (típico iso clásico: 45° en plano XZ).
- Distancia fija configurable. La cámara nunca rota — siempre mira al target.
- Smoothing con `EaseSineOut` o `Lerp` simple sobre `target → desiredTarget`
  para que el seguimiento no sea instantáneo.
- En multiplayer: target = centroide de jugadores vivos. Distancia se ajusta
  según spread del grupo (más lejos cuando están dispersos).
- Sin colisión de cámara con escenario (cámara alta, no se obstruye).

Struct de configuración:
```
distance, elevationDeg, azimuthDeg, fovy, smoothFactor,
multiplayerMinDistance, multiplayerMaxDistance, multiplayerSpreadFactor
```

### Tarea 1.7 — Sombras blob ✅

---

## Fase 2 — Physics World ✅

Detección con 6 combinaciones de primitivas, queries de raycast y overlap,
debug draw, stats en panel F4.

### Tarea 2.1 — Structs de colliders ✅
### Tarea 2.2 — Registro y pool de colliders ✅
### Tarea 2.3 — Tests de colisión ✅ (bug capsule-vs-capsule arreglado)
### Tarea 2.4 — Detección broad-phase y resolución ✅
### Tarea 2.5 — Queries: raycast y overlap ✅
### Tarea 2.6 — Debug draw + panel F4 ✅

### Tarea 2.7 — Mejoras pendientes

- Generation handles (encodear `(generation << 16) | index`) para evitar use-
  after-free silencioso cuando un slot se reutiliza.
- Lista compacta de colliders activos para evitar O(n²) sobre slots vacíos.
- Mover `s_triggerContacts` de buffer estático a alocación en arena scratch.

---

## Fase 3 — Jugador

Character controller con movimiento 8-direcciones, salto, strafe lock,
animaciones, registrado en renderer y physics.

**Criterio de completitud:** jugador con movimiento 8-dir, salto funcional,
strafe lock para fijar dirección de disparo, animaciones correctas,
colisiones con escenario, cel shading.

### Tarea 3.1 — Input abstraction (8 direcciones + strafe lock)

`input_types.h`:
```c
typedef struct PlayerInput {
    Vector2 moveDir;     // 8-dir normalizada (D-pad)
    Vector2 aimDir;      // dirección de mira (= moveDir, o última si strafe)
    bool    fire;        // A
    bool    jump;        // B (edge-triggered)
    bool    switchWeapon;// X
    bool    use;         // Y
    bool    strafe;      // L (held)
    bool    secondary;   // R
    bool    pause;       // Start
} PlayerInput;
```

`InputRead(int slot) → PlayerInput`. Lee D-pad/teclado y construye la struct.
El strafe lock se aplica internamente: si `strafe` está held, `aimDir`
mantiene la última dirección no-strafe; `moveDir` sigue actualizándose libre.

Mapeo PC: WASD=D-pad, Mouse=aim override (cuando hay mouse, `aimDir` lo usa
y se ignora el strafe lock).

### Tarea 3.2 — Movimiento 8 direcciones

Movimiento en XZ según `moveDir × PLAYER_SPEED × dt`. Velocidad constante,
sin aceleración (fiel a Boxhead).

Rotación visual: el modelo rota a `aimDir`. Cuando strafe está held, rota
solo si cambia `aimDir` (manual).

`PhysicsMoveAndCollide` resuelve colisiones contra escenario.

### Tarea 3.3 — Salto

Velocidad vertical = `PLAYER_JUMP_FORCE` al presionar B (edge-triggered) si
`isGrounded`. Gravedad cada tick. Aterrizaje cuando posición Y ≤ 0.
`isGrounded` se determina por flag de `PhysicsMoveAndCollide` (normal.y > 0.7).

Salto da capacidad de esquivar enemigos y saltar barriles bajos.

### Tarea 3.4 — Animaciones del jugador

State machine: IDLE, RUN, JUMP, FALL, FIRE, DIE. Transiciones explícitas.
`UpdateModelAnimation()` cada frame. Sin modo aim — el personaje siempre está
"con arma equipada" en el sentido visual.

### Tarea 3.5 — Registro en renderer y physics

Patrón estándar:
- Crear: `RendererRegister()` + `PhysicsAddCapsule()`.
- Cada tick: sincroniza transforms.
- Destruir: desregistra ambos.

Mismo patrón se reutiliza para enemigos, proyectiles, deployables, pickups.

---

## Fase 4 — Arma Básica y Enemigo Básico

Core loop mínimo: mover, disparar, matar, morir.

### Tarea 4.1 — Struct de arma y sistema de disparo genérico

`weapon_types.h`:
```c
typedef struct WeaponDef {
    const char* name;
    WeaponType type;          // HITSCAN, PROJECTILE, DEPLOYABLE, AIRSTRIKE
    int damage;
    int cooldownTicks;
    int magazineSize;
    int reserveAmmo;
    float spread;
    void (*OnShoot)(struct Player* p, struct Game* g);
} WeaponDef;
```

`WeaponTryShoot(weapon, player) → bool`. Chequea cooldown y munición,
llama a `OnShoot`.

### Tarea 4.2 — Pistola (hitscan)

Handler `shootHitscan` (static). `PhysicsRayCastIgnore` desde el arma en
`aimDir`, ignora al jugador. Layer mask ENEMY|SCENERY. Si impacta ENEMY,
aplica daño.

Pistola: cadencia media, daño medio, munición infinita, spread 0.

### Tarea 4.3 — Pool de enemigos y spawn

`enemy_types.h`: ObjPool de Enemy. Cada uno: position, hp, state
(INACTIVE/ALIVE/DYING/DEAD), type, render handle, physics handle.

`EnemySpawn(position, type) → bool`. `EnemyKill(id)` → estado DYING,
animación muerte, después desregistra.

### Tarea 4.4 — Zombie básico: IA seek

Steering: `dir = normalize(player.pos - zombie.pos)`. Mover × speed × dt.
Si dist < melee_range, daña al jugador cada `ATTACK_COOLDOWN` ticks.
Obstacle avoidance: raycast corto adelante, si impacta SCENERY, desviarse.

### Tarea 4.5 — Salud del jugador y game over

Player HP. Enemigos reducen en melee. HP ≤ 0 → DEAD, game over (transición
a level results).

### Tarea 4.6 — Paneles de debug F5 y F6

- **F5 Gameplay:** enemigos activos, HP, arma, ObjPool stats.
- **F6 Spawn/Cheats:** botones raygui (spawn, kill all, god mode, toggle
  collisions).

---

## Fase 5 — Score, Oleadas y Progresión

### Tarea 5.1 — Score y combo system

`ScoreAddKill()`, combo_count, combo_multiplier, combo_timer, tabla de
multiplicador.

### Tarea 5.2 — Thresholds de desbloqueo

Tabla `{threshold, unlock_id}`. `ScoreCheckUnlocks()` cada tick.

### Tarea 5.3 — Sistema de oleadas

Struct `Wave`: `{enemy_type, count}` + delays. Wave manager spawnea según
wave actual, avanza cuando todos muertos.

### Tarea 5.4 — Spawn points

Arena define spawn points (bordes). Spawner elige los más alejados del
jugador (o del centroide en multiplayer).

### Tarea 5.5 — HUD temporal

`DrawText()` para score, combo, multiplicador, oleada, HP, arma, munición.

---

## Fase 6 — Arsenal Completo

Todas las armas de Boxhead 2Play.

### Tarea 6.1 — Pool de proyectiles
### Tarea 6.2 — Uzi (hitscan, cadencia alta, spread pequeño)
### Tarea 6.3 — Shotgun (múltiples raycasts en cono)
### Tarea 6.4 — Rockets (proyectil + AOE)
### Tarea 6.5 — Granadas (arco parabólico, timer, rebota)
### Tarea 6.6 — Railgun (hitscan penetrante)
### Tarea 6.7 — Pool de deployables
### Tarea 6.8 — Minas (trigger esfera → AOE)
### Tarea 6.9 — Barriles (AABB destructible, explosión en cadena)
### Tarea 6.10 — Fake walls (AABB que bloquea ENEMY pero no PLAYER)
### Tarea 6.11 — Turrets (deployable autónomo)
### Tarea 6.12 — Airstrike (timer + AOE masiva, usos limitados)
### Tarea 6.13 — Weapon switching e inventario

X cicla armas desbloqueadas. HUD muestra arma actual. Delay al cambiar.

---

## Fase 7 — Todos los Enemigos

### Tarea 7.1 — Zombie rápido (Devil)
### Tarea 7.2 — Zombie a distancia (Mummy)
### Tarea 7.3 — Zombie tanque
### Tarea 7.4 — Panel de debug de IA (F7)
### Tarea 7.5 — Balanceo de oleadas (iterativo)

---

## Fase 8 — Pickups y Upgrades

### Tarea 8.1 — Pool y spawn de pickups
### Tarea 8.2 — Pickups de munición y salud
### Tarea 8.3 — Upgrade system
### Tarea 8.4 — Feedback visual (floating text, partículas)

---

## Fase 9 — Niveles, Menús y Flow

### Tarea 9.1 — Menú principal (Level + raygui)
### Tarea 9.2 — Selección de arena
### Tarea 9.3 — Múltiples arenas (3-4 layouts)
### Tarea 9.4 — Pantalla de resultados
### Tarea 9.5 — Transiciones específicas (ya implementado el sistema)
### Tarea 9.6 — Pantalla de pausa
### Tarea 9.7 — Settings persistentes con rini

---

## Fase 10 — Polish Visual, Audio y UI Final

### Tarea 10.1 — Outline shader (opcional)
### Tarea 10.2 — Sistema de partículas
### Tarea 10.3 — Screen shake
### Tarea 10.4 — Hit feedback (flash, número de daño, knockback)
### Tarea 10.5 — Audio (raylib audio)
### Tarea 10.6 — Game UI custom: HUD (atlas, fuente custom, animaciones)
### Tarea 10.7 — Game UI custom: menús

---

## Fase 11 — Optimización para Handheld (tentativa)

Solo si profiling indica que es necesario para mantener 30 FPS estables.

### Tarea 11.1 — Profiling en hardware real
### Tarea 11.2 — Cap de enemigos dinámico
### Tarea 11.3 — LOD de animaciones
### Tarea 11.4 — Batching / Instancing
### Tarea 11.5 — Shader simplificado para handheld
### Tarea 11.6 — Resolución de render reducida

---

## Fase 12 — Multiplayer Local 2-4P (PC)

Pantalla compartida (NO split-screen). Cámara común que sigue al centroide
del grupo. Distancia de cámara se ajusta al spread.

### Tarea 12.1 — PlayerManager multi-slot

Hasta 4 slots. Mapeo de gamepads. Join con un botón en pantalla de menú.

### Tarea 12.2 — Cámara común

Target = centroide de jugadores vivos. Distancia entre
`MULTIPLAYER_MIN_DISTANCE` y `MULTIPLAYER_MAX_DISTANCE` según spread.
Si un jugador se aleja demasiado del centroide, se le marca con flecha en
el borde de la pantalla.

### Tarea 12.3 — IA multi-target

Enemigos eligen jugador más cercano. Recalculan cada N ticks.

### Tarea 12.4 — HUD por jugador

4 esquinas de la pantalla. Cada uno muestra HP, arma, munición de su slot.

### Tarea 12.5 — Balanceo multiplayer

Más jugadores = más enemigos, mejores drops. Iterativo.

---

## Resumen de Dependencias

```
Fase 0 (loop + arenas + resources + levels + transitions + debug) ✅
  └→ Tarea 0.9 Refactor a nueva arquitectura
       └→ Fase 1 (renderer + cámara iso + cel shading)
            └→ Fase 2 (physics) ✅
                 └→ Fase 3 (jugador 8-dir + salto + strafe)
                      └→ Fase 4 (pistola + zombie = core loop)
                           └→ Fase 5 (score + oleadas)
                                └→ Fase 6 (arsenal)
                                └→ Fase 7 (enemigos)
                                     └→ Fase 8 (pickups)
                                          └→ Fase 9 (menús + flow)
                                               └→ Fase 10 (polish)
                                                    └→ Fase 11 (optim handheld, tentativa)
                                                         └→ Fase 12 (MP 2-4P pantalla compartida PC)
```

Cada fase produce un ejecutable funcional. No hay fases de "infraestructura
invisible".
