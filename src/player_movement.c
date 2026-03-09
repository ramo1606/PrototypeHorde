/*******************************************************************************************
*
*   player_movement.c — Player Movement Component Implementation
*
********************************************************************************************/
#include "player_movement.h"
#include "camera_tps.h"
#include "actor.h"
#include "game.h"
#include "memory.h"
#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <math.h>

/* Threshold below which input magnitude is considered zero */
#define INPUT_DEAD_ZONE 0.1f

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helper Functions (static)
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * NormalizeAngle (static)
  *
  *   Wraps an angle into the range [-PI, +PI]. This is essential for the shortest-
  *   arc rotation calculation: without it, a character at 350° trying to reach 10°
  *   would rotate 340° the long way instead of 20° the short way.
  *------------------------------------------------------------------------------------*/
static float NormalizeAngle(float angle)
{
    angle = fmodf(angle, 2.0f * PI);
    if (angle > PI)  angle -= 2.0f * PI;
    if (angle < -PI) angle += 2.0f * PI;
    return angle;
}

/*------------------------------------------------------------------------------------
 * ReadMovementInput (static)
 *
 *   Reads WASD keys and returns a 2D input vector.
 *
 *   Convention:
 *     inputX: +1 = right (D), -1 = left (A)
 *     inputY: +1 = forward (W), -1 = backward (S)
 *
 *   Returns raw integer values {-1, 0, +1} per axis. Normalization happens
 *   after combining axes, so diagonals don't exceed magnitude 1.
 *
 *   NOTE: This reads Raylib keys directly. When Phase 1 (Input System) is
 *   implemented, this should be replaced with InputAction queries.
 *------------------------------------------------------------------------------------*/
static Vector2 ReadMovementInput(void)
{
    float inputX = 0.0f;
    float inputY = 0.0f;

    if (IsKeyDown(KEY_W)) inputY += 1.0f;
    if (IsKeyDown(KEY_S)) inputY -= 1.0f;
    if (IsKeyDown(KEY_D)) inputX += 1.0f;
    if (IsKeyDown(KEY_A)) inputX -= 1.0f;

    return (Vector2) { inputX, inputY };
}

/*------------------------------------------------------------------------------------
 * ApplyShortestArcRotation (static)
 *
 *   Rotates the actor's yaw toward targetYaw by at most turnSpeed * dt radians,
 *   always taking the shortest path around the circle.
 *
 *   Algorithm:
 *   1. Compute the signed angular difference (target - current)
 *   2. Normalize to [-PI, +PI] to ensure shortest arc
 *   3. Clamp the step to turnSpeed * dt
 *   4. If remaining delta is smaller than the step, snap to target (no jitter)
 *
 *   This gives constant-speed rotation (radians/sec), not exponential decay
 *   like lerp. The result is predictable and framerate-independent.
 *------------------------------------------------------------------------------------*/
static void ApplyShortestArcRotation(Actor* owner, float targetYaw,
    float turnSpeed, float deltaTime)
{
    Vector3 rot = owner->root.rotation;
    float delta = NormalizeAngle(targetYaw - rot.y);

    float maxStep = turnSpeed * deltaTime;

    if (fabsf(delta) <= maxStep)
    {
        rot.y = targetYaw;
    }
    else
    {
        rot.y += (delta > 0.0f ? 1.0f : -1.0f) * maxStep;
    }

    ACTOR_SetRotation(owner, rot);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Component Update
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * PlayerMovementUpdate (static)
  *
  *   Per-tick update (runs at fixed timestep, update order 10).
  *
  *   Step 1 — Read input
  *     WASD → 2D vector (inputX, inputY). If magnitude < dead zone, no movement.
  *
  *   Step 2 — Transform to world direction
  *     moveDir = cameraForward * inputY + cameraRight * inputX
  *     Both camera vectors are planar (Y=0, normalized), so moveDir is always
  *     in the XZ plane. Normalize to prevent diagonal speed boost.
  *
  *   Step 3 — Move actor
  *     newPos = pos + moveDir * moveSpeed * dt
  *     No gravity, no collision response — that comes in Phase 4-5.
  *
  *   Step 4 — Compute target yaw and rotate
  *     targetYaw = atan2(moveDir.x, -moveDir.z)
  *     The -Z accounts for the engine's forward convention (-Z = forward).
  *     Apply shortest-arc rotation so the character turns to face moveDir.
  *------------------------------------------------------------------------------------*/
static void PlayerMovementUpdate(Component* self, float deltaTime)
{
    assert(self != NULL);
    if (self->type != COMPONENT_TYPE_PLAYER_MOVEMENT) return;

    PlayerMovement* pm = (PlayerMovement*)self;
    Actor* owner = self->owner;

    /* ── Step 1: Read input ── */
    Vector2 input = ReadMovementInput();
    float inputMag = Vector2Length(input);

    if (inputMag < INPUT_DEAD_ZONE)
    {
        pm->isMoving = false;
        return;
    }

    pm->isMoving = true;

    /* Normalize to prevent diagonal speed boost */
    input = Vector2Scale(input, 1.0f / inputMag);

    /* ── Step 2: Transform to world direction via camera ── */
    Vector3 camFwd = CAMERA_TPS_GetPlanarForward(pm->camera);
    Vector3 camRight = CAMERA_TPS_GetPlanarRight(pm->camera);

    Vector3 moveDir;
    moveDir.x = camFwd.x * input.y + camRight.x * input.x;
    moveDir.y = 0.0f;
    moveDir.z = camFwd.z * input.y + camRight.z * input.x;

    /* Safety normalize (should already be ~1.0 but floating point) */
    float moveMag = Vector3Length(moveDir);
    if (moveMag > 0.001f)
    {
        moveDir = Vector3Scale(moveDir, 1.0f / moveMag);
    }

    /* ── Step 3: Move actor ── */
    Vector3 pos = owner->root.position;
    pos = Vector3Add(pos, Vector3Scale(moveDir, pm->moveSpeed * deltaTime));
    ACTOR_SetPosition(owner, pos);

    /* ── Step 4: Rotate to face movement direction ── */
    pm->targetYaw = atan2f(-moveDir.x, -moveDir.z);
    ApplyShortestArcRotation(owner, pm->targetYaw, pm->turnSpeed, deltaTime);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * PLAYER_MOVEMENT_Create
  *
  *   Factory function — allocates from the component pool, initializes with
  *   update order 10 (runs before mesh/camera/colliders), and stores a
  *   non-owning reference to the CameraTPS for direction queries.
  *
  *   Default speeds:
  *     moveSpeed = 8.0  → comfortable jog speed for a TPS character
  *     turnSpeed = 15.0 → fast turn, BotW-style (near-instant but not snapping)
  *------------------------------------------------------------------------------------*/
PlayerMovement* PLAYER_MOVEMENT_Create(Actor* owner, CameraTPS* camera)
{
    assert(owner != NULL);
    assert(camera != NULL);

    PlayerMovement* pm = (PlayerMovement*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(PlayerMovement));
    if (!pm) return NULL;

    COMPONENT_Init(&pm->base, owner, COMPONENT_TYPE_PLAYER_MOVEMENT, 10);
    pm->base.Update = PlayerMovementUpdate;

    pm->camera = camera;
    pm->moveSpeed = 8.0f;
    pm->turnSpeed = 15.0f;
    pm->targetYaw = owner->root.rotation.y;
    pm->isMoving = false;

    return pm;
}

/* Set movement speed in units/second. */
void PLAYER_MOVEMENT_SetMoveSpeed(PlayerMovement* pm, float speed)
{
    assert(pm != NULL);
    pm->moveSpeed = speed;
}

/* Set rotation speed in radians/second. Higher = snappier turns. */
void PLAYER_MOVEMENT_SetTurnSpeed(PlayerMovement* pm, float speed)
{
    assert(pm != NULL);
    pm->turnSpeed = speed;
}

/* Returns true if the player has movement input this tick. */
bool PLAYER_MOVEMENT_IsMoving(PlayerMovement* pm)
{
    assert(pm != NULL);
    return pm->isMoving;
}