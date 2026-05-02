#pragma once

#include "renderer_types.h"

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void RendererInit(Renderer* renderer);
void RendererShutdown(Renderer* renderer);

/* ── Registration ────────────────────────────────────────────────────────── */

RenderHandle RendererRegister(Renderer* renderer, Model model, int materialID);
void         RendererUnregister(Renderer* renderer, RenderHandle handle);

/* ── Transform Updates ───────────────────────────────────────────────────── */

void RendererSetTransform(Renderer* renderer, RenderHandle handle, Matrix transform);
void RendererPreUpdate(Renderer* renderer);

/* ── Blob Shadows ────────────────────────────────────────────────────────── */

void RendererSetBlobShadow(Renderer* renderer, RenderHandle handle,
                           bool enabled, float radius);

/* ── Frame Drawing ───────────────────────────────────────────────────────── */

void RendererBuildDrawList(Renderer* renderer, float alpha);
void RendererDraw3D(Renderer* renderer);

/* ── Camera Control ──────────────────────────────────────────────────────── */

void     RendererSetCamera(Renderer* renderer, Camera3D camera);
Camera3D RendererGetCamera(const Renderer* renderer);
void     RendererSetClearColor(Renderer* renderer, Color color);

/* ── Lighting (Cel Shader) ───────────────────────────────────────────────── */

void RendererSetLightDir(Renderer* renderer, Vector3 dir);
void RendererSetAmbient(Renderer* renderer, float ambient);
void RendererSetNumBands(Renderer* renderer, float numBands);

/* ── Frustum Culling ─────────────────────────────────────────────────────── */

void RendererExtractFrustumPlanes(FrustumPlane planes[6], Matrix m);
bool RendererIsSphereInFrustum(const FrustumPlane planes[6],
                               Vector3 center, float radius);

/* ── Bounding Sphere Helpers ─────────────────────────────────────────────── */

void RendererComputeBoundingSphere(Model model, Vector3* outCenter, float* outRadius);

/* ── Screen Projection ───────────────────────────────────────────────────── */

Vector2 RendererWorldToScreen(const Renderer* renderer, Vector3 worldPos);
