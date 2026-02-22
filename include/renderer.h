#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

typedef struct Game Game;
typedef struct MeshComponent MeshComponent;

#define RENDERER_MAX_MESHES 1024

/* Near/far clip planes — must match Raylib's BeginMode3D defaults
 * (RL_CULL_DISTANCE_NEAR=0.01, RL_CULL_DISTANCE_FAR=1000.0 in rlgl.h) */
#define RENDERER_NEAR_PLANE 0.01
#define RENDERER_FAR_PLANE  1000.0

 /*
  * Frustum plane: normal (a,b,c) + distance (d).
  * A point P is on the positive side if: a*Px + b*Py + c*Pz + d >= 0
  */
typedef struct
{
    float a, b, c, d;
} FrustumPlane;

/*
 * Draw entry — one mesh to draw, used for sorting.
 */
typedef struct
{
    MeshComponent* mc;
    float          distSq;  /* Squared distance to camera (for future use) */
} DrawEntry;

typedef struct
{
    Camera3D camera;
    Color    clearColor;

    MeshComponent* meshes[RENDERER_MAX_MESHES];
    int            meshCount;

    DrawEntry drawList[RENDERER_MAX_MESHES];
    int       drawCount;

    FrustumPlane frustum[6];

    /* Stats */
    int statsCulled;
    int statsDrawn;
} Renderer;

void RENDERER_Init(Renderer* renderer);
void RENDERER_Shutdown(Renderer* renderer);

void RENDERER_DrawFrame(Renderer* renderer, Game* game);

/* Mesh registration (called by MeshComponent create/destroy) */
void RENDERER_AddMesh(Renderer* renderer, MeshComponent* mc);
void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc);

/* Camera control */
void RENDERER_SetCamera(Renderer* renderer, Camera3D camera);
void RENDERER_SetClearColor(Renderer* renderer, Color color);

/* Frustum utilities */
void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix viewProj);
bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box);