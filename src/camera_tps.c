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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helper Functions (static)
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * ComputeIdealPos (static)
  *
  *   Computes the "ideal" camera position: behind and above the player.
  *
  *       idealPos = playerPos - forward * horzDist + (0, vertDist, 0)
  *
  *   This gives a classic over-the-shoulder third-person view. The camera
  *   orbits behind the player based on the player's facing direction.
  *------------------------------------------------------------------------------------*/

static Vector3 ComputeIdealPos(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    Vector3 pos = owner->root.position;
    Vector3 fwd = ACTOR_GetForward(owner);

    pos = Vector3Subtract(pos, Vector3Scale(fwd, ctps->horzDist));
    pos.y += ctps->vertDist;
    return pos;
}

/*------------------------------------------------------------------------------------
 * ComputeTarget (static)
 *
 *   Computes the camera's look-at target: a point in front of the player.
 *
 *       target = playerPos + forward * targetDist
 *
 *   targetDist pushes the look-at point forward so the camera looks slightly
 *   ahead of the player, which feels more natural during movement.
 *------------------------------------------------------------------------------------*/

static Vector3 ComputeTarget(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    Vector3 fwd = ACTOR_GetForward(owner);
    return Vector3Add(owner->root.position, Vector3Scale(fwd, ctps->targetDist));
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
  * CameraTPSUpdate (static)
  *
  *   Per-tick update using a critically-damped spring for smooth camera motion.
  *
  *   Critically-Damped Spring:
  *       A spring-damper system that approaches the target position as fast as
  *       possible WITHOUT oscillating (overshooting). This is the sweet spot
  *       between underdamped (bouncy) and overdamped (sluggish).
  *
  *       dampening = 2 × √(springConstant)
  *       acceleration = -springConstant × displacement - dampening × velocity
  *       velocity += acceleration × dt
  *       position += velocity × dt
  *
  *       The dampening coefficient of 2×√k is the exact value for critical damping
  *       (from the solution to the second-order ODE: mx'' + cx' + kx = 0).
  *
  *   After computing the spring position, we apply wall clamping to avoid
  *   geometry intersection, then update the Camera3D struct and push to renderer.
  *
  *   Reference: "Game Programming in C++" by Sanjay Madhav, Chapter 9
  *------------------------------------------------------------------------------------*/
static void CAMERA_TPS_Update(Component* self, float deltaTime)
{
    assert(self != NULL);
    if(self->type != COMPONENT_TYPE_CAMERA_TPS) return;
    CameraTPS* ctps = (CameraTPS*)self;

    /* ── Step 1: Compute critical damping coefficient ── */
    /* For critical damping: c = 2√k (no oscillation, fastest convergence) */
    float dampening = 2.0f * sqrtf(ctps->springConstant);

    /* ── Step 2: Compute spring forces ── */
    Vector3 idealPos = ComputeIdealPos(ctps);

    /* displacement = current - ideal (how far we are from where we want to be) */
    Vector3 diff = Vector3Subtract(ctps->actualPos, idealPos);

    /* acceleration = -k * displacement - c * velocity (spring + damper forces) */
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
  *   spring parameters, and snaps to the ideal position.
  *
  *   Default parameters:
  *       horzDist = 6     → 6 units behind the player
  *       vertDist = 4     → 4 units above
  *       targetDist = 3   → look-at 3 units ahead
  *       springConstant = 64 → stiff tracking (responds quickly)
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
