#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

typedef struct Game Game;
typedef struct MeshComponent MeshComponent;
typedef struct Renderer Renderer;
typedef struct FrustumPlane FrustumPlane;
typedef struct DrawEntry DrawEntry;

#define RENDERER_MAX_MESHES 1024

#define RENDERER_NEAR_PLANE 0.01
#define RENDERER_FAR_PLANE  1000.0

struct FrustumPlane
{
    float a, b, c, d;
};

struct DrawEntry
{
    MeshComponent* mc;
    float distSq;
};

struct Renderer
{
    Camera3D camera;
    Color    clearColor;

    MeshComponent* meshes[RENDERER_MAX_MESHES];
    int meshCount;

    DrawEntry drawList[RENDERER_MAX_MESHES];
    int drawCount;

    FrustumPlane frustum[6];

    int statsCulled;
    int statsDrawn;
};

void RENDERER_Init(Renderer* renderer);
void RENDERER_Shutdown(Renderer* renderer);
void RENDERER_DrawFrame(Renderer* renderer, Game* game);

void RENDERER_AddMesh(Renderer* renderer, MeshComponent* mc);
void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc);

void RENDERER_SetCamera(Renderer* renderer, Camera3D camera);
Camera3D RENDERER_GetCamera(const Renderer* renderer);
void RENDERER_SetClearColor(Renderer* renderer, Color color);

void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix viewProj);
bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box);
bool RENDERER_IsPointInFrustum(const FrustumPlane planes[6], Vector3 point);
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6], Vector3 center, float radius);

Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos);
bool RENDERER_IsOnScreen(const Renderer* renderer, Vector3 worldPos);