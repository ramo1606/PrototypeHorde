/*******************************************************************************************
*
*   camera_tps.c — Third-Person Spring Camera Implementation
*
********************************************************************************************/
#include "camera_tps.h"
#include "actor.h"
#include "game.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

/* Small offset to prevent camera from sitting exactly on the wall surface */
#define CAMERA_WALL_OFFSET 0.25f

/* Default orbit parameters */
#define DEFAULT_SENSITIVITY  0.003f
#define DEFAULT_PITCH_MIN   (-20.0f * DEG2RAD)
#define DEFAULT_PITCH_MAX   (70.0f  * DEG2RAD)

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helper Functions (static)
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
* ComputeIdealPos (static)
*
*   Computes the "ideal" camera position using spherical orbit coordinates.
*
*   The camera orbits around the player at distance horzDist, with pitch
*   controlling the elevation angle and yaw the horizontal angle:
*
*       idealPos = playerPos
*                - orbitForwardXZ * horzDist * cos(pitch)   (behind player)
*                + (0, vertDist + horzDist * sin(pitch), 0) (above player)
*
*   At yaw=0, pitch=0: camera is directly behind (+Z) and above the player.
*   Positive pitch raises the camera, negative pitch lowers it.
*   vertDist provides a baseline height that persists regardless of pitch.
*------------------------------------------------------------------------------------*/
static Vector3 ComputeIdealPos(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    Vector3 playerPos = owner->root.position;

    Vector3 orbFwd = CAMERA_TPS_GetPlanarForward(ctps);

    float cosPitch = cosf(ctps->pitch);
    float sinPitch = sinf(ctps->pitch);

    Vector3 idealPos;
    idealPos.x = playerPos.x - orbFwd.x * ctps->horzDist * cosPitch;
    idealPos.y = playerPos.y + ctps->vertDist + ctps->horzDist * sinPitch;
    idealPos.z = playerPos.z - orbFwd.z * ctps->horzDist * cosPitch;

    return idealPos;
}

/*------------------------------------------------------------------------------------
 * ComputeTarget (static)
 *
 *   Computes the camera's look-at target: a point ahead of the player along
 *   the camera's orbit forward direction.
 *
 *       target = playerPos + orbitForward * targetDist
 *
 *   Uses the camera's own yaw, NOT the actor's facing direction. This means
 *   the look-at follows the camera's orbit, not the character's orientation.
 *------------------------------------------------------------------------------------*/
static Vector3 ComputeTarget(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    Vector3 orbFwd = CAMERA_TPS_GetPlanarForward(ctps);
    return Vector3Add(owner->root.position, Vector3Scale(orbFwd, ctps->targetDist));
}

/*------------------------------------------------------------------------------------
 * ClampCameraToWorld (static)
 *
 *   Prevents the camera from clipping through world geometry (walls, floors).
 *
 *   Algorithm:
 *   1. Cast a ray from the camera's target point toward the camera position.
 *   2. If the ray hits a collider before reaching the camera, clamp the camera
 *      to the hit point minus a small offset (CAMERA_WALL_OFFSET).
 *   3. Minimum distance is CAMERA_WALL_OFFSET to prevent the camera from
 *      entering the geometry entirely.
 *
 *   Uses PhysWorld's ray cast which tests against all boxes and spheres.
 *   Layer mask 0xFFFFFFFF means it tests against everything.
 *------------------------------------------------------------------------------------*/
static Vector3 ClampCameraToWorld(CameraTPS* ctps, Vector3 cameraPos)
{
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    PhysWorld* world = &owner->game->physWorld;

    Vector3 target = ComputeTarget(ctps);
    Vector3 toCamera = Vector3Subtract(cameraPos, target);
    float dist = Vector3Length(toCamera);
    if (dist < 0.001f) return cameraPos;

    Ray ray = { target, Vector3Scale(toCamera, 1.0f / dist) };
    CollisionInfo hit;

    if (PHYS_WORLD_RayCast(world, ray, dist, 0xFFFFFFFF, &hit))
    {
        float clampedDist = hit.distance - CAMERA_WALL_OFFSET;
        if (clampedDist < CAMERA_WALL_OFFSET) clampedDist = CAMERA_WALL_OFFSET;
        return Vector3Add(target, Vector3Scale(ray.direction, clampedDist));
    }

    return cameraPos;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Spring-Damped Camera Update
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * CAMERA_TPS_Update (static)
  *
  *   Per-tick update using a critically-damped spring for smooth camera motion.
  *
  *   Critically-Damped Spring:
  *       dampening = 2 * sqrt(springConstant)
  *       acceleration = -springConstant * displacement - dampening * velocity
  *       velocity += acceleration * dt
  *       position += velocity * dt
  *
  *   After computing the spring position, we apply wall clamping to avoid
  *   geometry intersection, then update the Camera3D struct and push to renderer.
  *
  *   Reference: "Game Programming in C++" by Sanjay Madhav, Chapter 9
  *------------------------------------------------------------------------------------*/
static void CAMERA_TPS_Update(Component* self, float deltaTime)
{
    assert(self != NULL);
    if (self->type != COMPONENT_TYPE_CAMERA_TPS) return;
    CameraTPS* ctps = (CameraTPS*)self;

    /* ── Step 1: Compute critical damping coefficient ── */
    float dampening = 2.0f * sqrtf(ctps->springConstant);

    /* ── Step 2: Compute spring forces ── */
    Vector3 idealPos = ComputeIdealPos(ctps);

    Vector3 diff = Vector3Subtract(ctps->actualPos, idealPos);

    Vector3 accel = Vector3Subtract(
        Vector3Scale(diff, -ctps->springConstant),
        Vector3Scale(ctps->velocity, dampening)
    );

    /* ── Step 3: Euler integration ── */
    ctps->velocity = Vector3Add(ctps->velocity, Vector3Scale(accel, deltaTime));
    ctps->actualPos = Vector3Add(ctps->actualPos, Vector3Scale(ctps->velocity, deltaTime));

    /* ── Step 4: Wall collision avoidance ── */
    ctps->actualPos = ClampCameraToWorld(ctps, ctps->actualPos);

    /* ── Step 5: Update Camera3D and push to renderer ── */
    ctps->base.camera.position = ctps->actualPos;
    ctps->base.camera.target = ComputeTarget(ctps);
    ctps->base.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&ctps->base);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * CAMERA_TPS_Create
  *
  *   Factory function — allocates from the component pool, initializes the
  *   CameraComponent base, overrides the type and update callback, sets default
  *   spring parameters, initializes orbit angles from the actor's current
  *   rotation, and snaps to the ideal position.
  *
  *   The initial yaw is taken from the actor's Y rotation so the camera starts
  *   behind the actor. At rotation.y=0 (facing -Z), yaw=0 places the camera
  *   at +Z looking toward -Z — directly behind.
  *------------------------------------------------------------------------------------*/
CameraTPS* CAMERA_TPS_Create(Actor* owner)
{
    assert(owner != NULL);

    CameraTPS* ctps = (CameraTPS*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(CameraTPS));
    if (!ctps) return NULL;

    /* Initialize base camera (registers with Actor, attaches to root) */
    CAMERA_COMPONENT_Init(&ctps->base, owner);

    /* Override type and update to our TPS-specific versions */
    ctps->base.scene.base.type = COMPONENT_TYPE_CAMERA_TPS;
    ctps->base.scene.base.Update = CAMERA_TPS_Update;

    /* Spring state */
    ctps->actualPos = (Vector3){ 0 };
    ctps->velocity = (Vector3){ 0 };

    /* Default distances */
    ctps->horzDist = 6.0f;
    ctps->vertDist = 4.0f;
    ctps->targetDist = 3.0f;
    ctps->springConstant = 64.0f;

    /* Orbit: start behind actor's current facing direction */
    ctps->yaw = owner->root.rotation.y;
    ctps->pitch = 0.0f;
    ctps->sensitivity = DEFAULT_SENSITIVITY;
    ctps->pitchMin = DEFAULT_PITCH_MIN;
    ctps->pitchMax = DEFAULT_PITCH_MAX;

    /* Snap to ideal immediately (no spring animation on creation) */
    CAMERA_TPS_SnapToIdeal(ctps);

    return ctps;
}

/*------------------------------------------------------------------------------------
 * CAMERA_TPS_SnapToIdeal
 *
 *   Teleports the camera instantly to its ideal position with zero velocity.
 *   Useful after level loads or player teleports to avoid the camera
 *   visibly springing from a distant location.
 *------------------------------------------------------------------------------------*/
void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps)
{
    assert(ctps != NULL);

    ctps->actualPos = ComputeIdealPos(ctps);
    ctps->velocity = (Vector3){ 0 };

    ctps->base.camera.position = ctps->actualPos;
    ctps->base.camera.target = ComputeTarget(ctps);
    ctps->base.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&ctps->base);
}

/* Set the three distance parameters that define the camera's orbit shape. */
void CAMERA_TPS_SetDistances(CameraTPS* ctps, float horz, float vert, float target)
{
    assert(ctps != NULL);
    ctps->horzDist = horz;
    ctps->vertDist = vert;
    ctps->targetDist = target;
}

/* Set the spring stiffness. Higher values = faster tracking, lower = more cinematic. */
void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant)
{
    assert(ctps != NULL);
    ctps->springConstant = springConstant;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Orbit Control
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * CAMERA_TPS_RotateOrbit
  *
  *   Updates orbit angles from raw mouse/stick input deltas.
  *
  *   Convention:
  *     - Positive deltaYaw  → rotate right (clockwise from above)
  *     - Positive deltaPitch → tilt down (camera rises, looks down at player)
  *
  *   This matches standard mouse behavior: moving the mouse right rotates the
  *   view right, moving it down tilts the view down. If you want inverted Y,
  *   negate deltaPitch before calling this function.
  *
  *   Pitch is clamped to [pitchMin, pitchMax] to prevent the camera from
  *   flipping over the poles. Yaw is unclamped (wraps naturally via trig).
  *------------------------------------------------------------------------------------*/
void CAMERA_TPS_RotateOrbit(CameraTPS* ctps, float deltaYaw, float deltaPitch)
{
    assert(ctps != NULL);

    ctps->yaw += deltaYaw * ctps->sensitivity;
    ctps->pitch += deltaPitch * ctps->sensitivity;

    /* Clamp pitch to prevent flipping */
    if (ctps->pitch < ctps->pitchMin) ctps->pitch = ctps->pitchMin;
    if (ctps->pitch > ctps->pitchMax) ctps->pitch = ctps->pitchMax;
}

/* Set orbit angles directly (radians). Pitch is clamped. */
void CAMERA_TPS_SetOrbitAngles(CameraTPS* ctps, float yaw, float pitch)
{
    assert(ctps != NULL);
    ctps->yaw = yaw;
    ctps->pitch = pitch;

    if (ctps->pitch < ctps->pitchMin) ctps->pitch = ctps->pitchMin;
    if (ctps->pitch > ctps->pitchMax) ctps->pitch = ctps->pitchMax;
}

/* Set sensitivity and pitch clamp range. */
void CAMERA_TPS_SetOrbitParams(CameraTPS* ctps, float sensitivity, float pitchMin, float pitchMax)
{
    assert(ctps != NULL);
    ctps->sensitivity = sensitivity;
    ctps->pitchMin = pitchMin;
    ctps->pitchMax = pitchMax;

    /* Re-clamp current pitch in case range changed */
    if (ctps->pitch < ctps->pitchMin) ctps->pitch = ctps->pitchMin;
    if (ctps->pitch > ctps->pitchMax) ctps->pitch = ctps->pitchMax;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Camera Direction Queries
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * CAMERA_TPS_GetPlanarForward
  *
  *   Returns the camera's horizontal forward direction (Y=0, normalized).
  *
  *   This is the direction the camera "looks" when projected onto the ground
  *   plane. The movement system uses this to convert WASD input into world-
  *   space movement direction:
  *
  *       moveDir = forward * inputY + right * inputX
  *
  *   At yaw=0: returns (0, 0, -1) — forward is -Z, matching engine convention.
  *------------------------------------------------------------------------------------*/
Vector3 CAMERA_TPS_GetPlanarForward(CameraTPS* ctps)
{
    assert(ctps != NULL);
    return (Vector3) {
        sinf(ctps->yaw),
            0.0f,
            -cosf(ctps->yaw)
    };
}

/*------------------------------------------------------------------------------------
 * CAMERA_TPS_GetPlanarRight
 *
 *   Returns the camera's horizontal right direction (Y=0, normalized).
 *
 *   Perpendicular to planar forward, rotated 90° clockwise when viewed from
 *   above. Used by the movement system for strafing input.
 *
 *   At yaw=0: returns (1, 0, 0) — right is +X, matching engine convention.
 *------------------------------------------------------------------------------------*/
Vector3 CAMERA_TPS_GetPlanarRight(CameraTPS* ctps)
{
    assert(ctps != NULL);
    return (Vector3) {
        cosf(ctps->yaw),
            0.0f,
            sinf(ctps->yaw)
    };
}
