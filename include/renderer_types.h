#pragma once

#include "raylib.h"
#include "config.h"
#include <stdbool.h>

#define RENDERER_NEAR_PLANE   0.01f
#define RENDERER_FAR_PLANE    1000.0f

/* A RenderHandle is an index into the renderables array.
 * RENDER_HANDLE_INVALID means "not registered" or "registration failed". */
typedef int RenderHandle;
#define RENDER_HANDLE_INVALID (-1)

typedef struct Renderable
{
    Model   model;
    Matrix  transformCurr;       /* Current tick's world transform           */
    Matrix  transformPrev;       /* Previous tick's transform (interpolation)*/
    Vector3 boundingCenter;      /* Local-space center for culling           */
    float   boundingRadius;
    int     materialID;          /* For sorting (group by shader/texture)    */
    bool    active;
    bool    blobShadow;
    float   blobRadius;          /* Shadow radius on the ground (world units)*/
} Renderable;

/* Plane in Hessian normal form: ax + by + cz + d = 0 */
typedef struct FrustumPlane
{
    float a, b, c, d;
} FrustumPlane;

/* Entry in the sorted draw list: renderable index + sort key. */
typedef struct DrawEntry
{
    int    index;                /* Index into renderables[] */
    int    materialID;
    float  distSq;               /* Squared distance to camera */
    Matrix transform;            /* Cached interpolated transform */
} DrawEntry;

typedef struct Renderer
{
    Camera3D camera;
    Color    clearColor;

    /* Cel shader (Task 1.5) */
    Shader  celShader;
    int     locLightDir;
    int     locAmbient;
    int     locNumBands;
    Vector3 lightDir;
    float   ambient;
    float   numBands;

    /* Blob shadows (Task 1.7) */
    Model   shadowPlane;
    Texture shadowTex;

    /* Renderable pool */
    Renderable renderables[MAX_RENDERABLES];
    int        renderableCount;

    /* Draw list (built each frame after culling) */
    DrawEntry drawList[MAX_RENDERABLES];
    int       drawCount;

    /* Frustum planes (extracted each frame): 0=L 1=R 2=B 3=T 4=N 5=F */
    FrustumPlane frustum[6];

    /* Per-frame stats */
    int statsDrawn;
    int statsCulled;
} Renderer;
