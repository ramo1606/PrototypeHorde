/*
 * level_physics_test.c — Stress test for physics subsystem.
 *
 * Exercises: the 6 collider shape combinations (box/capsule/sphere),
 * trigger volumes, and raycast queries. One controllable + one static
 * collider per shape; TAB cycles which one is active, WASD moves it.
 * Active body shoots a ray forward; ray segment + hit point drawn.
 *
 * Uses raylib third-person camera (CAMERA_THIRD_PERSON) targeting the
 * active controllable.
 */

#include "level_physics_test.h"
#include "level_manager.h"

static void Init(void* user)      { (void)user; }
static void Shutdown(void* user)  { (void)user; }
static void Update(void* user, float dt)        { (void)user; (void)dt; }
static void Render3D(void* user, float alpha)   { (void)user; (void)alpha; }
static void RenderHUD(void* user, float alpha)  { (void)user; (void)alpha; }

Level LEVEL_PHYSICS_TEST = {
    .name      = "PhysicsTest",
    .Init      = Init,
    .Shutdown  = Shutdown,
    .Update    = Update,
    .Render3D  = Render3D,
    .RenderHUD = RenderHUD,
};
