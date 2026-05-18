#pragma once

/*
 * config.h — Project-specific constants for Boxhead 3D.
 *
 * This file is the host project's config, NOT the kit's. Kit modules
 * (renderer, physics, level_manager, arena) define their own defaults
 * with #ifndef guards; the project overrides them here BEFORE the kit
 * headers see them. game.h is the include point that ensures the order:
 * it pulls config.h before any kit header that consumes the overrides.
 */

/* ── Platform-dependent values ───────────────────────────────────────────── */

#ifdef PLATFORM_HANDHELD
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480
#define RENDER_FPS          30
#define MAX_ENEMIES         50
#define MAX_PROJECTILES     30
#define MAX_DEPLOYABLES     15
#define MAX_PICKUPS         20
#define MAX_PARTICLES       100
#else
#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       720
#define RENDER_FPS          60
#define MAX_ENEMIES         100
#define MAX_PROJECTILES     60
#define MAX_DEPLOYABLES     30
#define MAX_PICKUPS         40
#define MAX_PARTICLES       300
#endif

/* ── Game loop timing ────────────────────────────────────────────────────── */

#define UPDATE_RATE              60
#define FIXED_TIMESTEP           (1.0f / (float)UPDATE_RATE)
#define MAX_DELTA_TIME           0.25f       /* Spiral-of-death clamp */
#define MAX_UPDATES_PER_FRAME    5

/* ── Memory pool sizes (bytes) ───────────────────────────────────────────── */

#define ARENA_PERMANENT_SIZE    (2  * 1024 * 1024)
#define ARENA_LEVEL_SIZE        (8  * 1024 * 1024)
#define ARENA_SCRATCH_SIZE      (1  * 1024 * 1024)

/* ── Module overrides ────────────────────────────────────────────────── */
/* Sized from gameplay maxes. Defined here so rehusable modules see them via
 * the #ifndef guards in renderer.h / physics.h. */

#define RENDERER_MAX_RENDERABLES (MAX_ENEMIES + MAX_PROJECTILES + MAX_DEPLOYABLES \
                                  + MAX_PICKUPS + 64)
#define MAX_COLLIDERS            (RENDERER_MAX_RENDERABLES + 64)

/* ── Gameplay constants ──────────────────────────────────────────────────── */

/* Player */
#define PLAYER_SPEED            5.0f
#define PLAYER_AIM_SPEED_MULT   0.6f
#define PLAYER_JUMP_FORCE       8.0f
#define PLAYER_GRAVITY          20.0f
#define PLAYER_MAX_HP           100
#define PLAYER_CAPSULE_RADIUS   0.35f
#define PLAYER_CAPSULE_HEIGHT   1.4f

/* Enemies */
#define ZOMBIE_SPEED            2.0f
#define ZOMBIE_HP               30
#define ZOMBIE_MELEE_RANGE      1.2f
#define ZOMBIE_MELEE_DAMAGE     10
#define ZOMBIE_ATTACK_COOLDOWN  60

#define DEVIL_SPEED             4.5f
#define DEVIL_HP                50

#define MUMMY_SPEED             1.5f
#define MUMMY_HP                40
#define MUMMY_PREFERRED_DIST    8.0f
#define MUMMY_SHOOT_COOLDOWN    90

#define TANK_SPEED              1.0f
#define TANK_HP                 200
#define TANK_MELEE_DAMAGE       25
#define TANK_AOE_RESIST         0.5f

/* Combo / Score */
#define COMBO_WINDOW_TICKS      180

/* Waves */
#define MAX_WAVES               50
#define MAX_SPAWN_POINTS        16
