#pragma once

#include "Knight.h"

#define SHADOWMAP_RESOLUTION 2048

class LitDepthRenderPass : public SceneRenderPass
{
public:

	LitDepthRenderPass();

	bool Create(Scene* sc) override;
	void Release() override;

	void BeginScene(SceneCamera* cam = NULL) override;
	void Render() override;
	void EndScene() override;
	bool OnAddToRender(Component* pSC, SceneObject* pSO) override;

	std::vector<RenderTexture2D> ShadowMaps;
	int shadowMapResolution = SHADOWMAP_RESOLUTION;

	int ShadowMapsLocs[NUM_MAX_LIGHTS] = { 0 };

	Shader depthShader = { 0 };

	OrthogonalCamera *pLightCamera = nullptr;

protected:

	RenderTexture2D LoadShadowmapRenderTexture(int width, int height);
	void UnloadShadowmapRenderTexture(RenderTexture2D target);
};


