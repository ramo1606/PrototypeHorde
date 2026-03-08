/*******************************************************************************************
*
*   player_movement.h — Character Movement Component (Camera-Relative)
*
*   Transforms player input (WASD / stick) into world-space movement using the
*   camera's orientation as the frame of reference. The character moves in the
*   direction the camera looks, and rotates smoothly to face the movement direction.
*
*   This is the "Uncharted / BotW exploration mode" pattern:
*       - W moves toward where the camera looks (projected onto XZ plane)
*       - A/D strafe relative to the camera, not the character
*       - The character turns to face the direction of movement
*       - Releasing input stops movement; character keeps last facing direction
*
*   This component is a simplified precursor to the full CharacterMovementComponent
*   planned for Phase 4. It handles input→direction→movement→rotation, but does NOT:
*       - Apply gravity or ground detection
*       - Perform collision response (character walks through walls)
*       - Support multiple movement modes (walk/run/fall/swim)
*
*   Architecture:
*       Component (base) — no SceneComponent needed, modifies actor root directly
*       Holds a CameraTPS* for direction queries (non-owning reference)
*
*   Naming Convention:
*       API:     PLAYER_MOVEMENT_*
*
********************************************************************************************/
#pragma once

#include "component.h"

typedef struct CameraTPS CameraTPS;
typedef struct PlayerMovement PlayerMovement;

/* ── Player Movement Struct ──────────────────────────────────────────────── */
struct PlayerMovement
{
    Component base;             /* Inherited base component (must be first field)      */

    CameraTPS* camera;          /* Camera for direction queries (non-owning reference) */
    float moveSpeed;            /* Movement speed in units/second                      */
    float turnSpeed;            /* Rotation speed in radians/second                    */
    float targetYaw;            /* Desired facing angle (radians)                      */
    bool  isMoving;             /* True if player has movement input this frame        */
};

/* ── Public API ──────────────────────────────────────────────────────────── */

/* Allocate from pool, attach to owner with update order 10.
 * camera must be a valid CameraTPS* — stored as non-owning reference. */
PlayerMovement* PLAYER_MOVEMENT_Create(Actor* owner, CameraTPS* camera);

/* Runtime parameter adjustment. */
void PLAYER_MOVEMENT_SetMoveSpeed(PlayerMovement* pm, float speed);
void PLAYER_MOVEMENT_SetTurnSpeed(PlayerMovement* pm, float speed);

/* Query — useful for animation, debug, and future FSM. */
bool PLAYER_MOVEMENT_IsMoving(PlayerMovement* pm);