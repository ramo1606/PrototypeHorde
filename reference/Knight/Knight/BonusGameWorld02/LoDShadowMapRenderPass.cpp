//Render shadow only for objects within certain distance to the view camera
#include "LodShadowMapRenderPass.h"
#include "QuadTreeTerrainComponent.h"

LoDShadowMapRenderPass::LoDShadowMapRenderPass(float cutof, ShadowSceneLight * l, int id)
	: ShadowMapRenderPass(l, id)
{
	CutOffDistance = cutof;
}

/// <summary>
/// ShouldRenderShadow - determine if the object should render shadow based on its distance to the view camera
/// </summary>
/// <param name="rc"></param>
/// <returns></returns>
int LoDShadowMapRenderPass::ShouldRenderShadow(const RenderContext& rc)
{
	//No shadow receiving if the component is set to not receive shadow
	int receiveShadow = rc.pComponent->receiveShadow ? 1 : 0;
	//disable shadow receiving for far away background objects to save performance
	if (rc.distance2 > CutOffDistance)
		receiveShadow = 0;
	return receiveShadow;
}

/// <summary>
/// Render - override standard render function to skip rendering shadow 
///          for objects far away from the view camera
/// </summary>
void LoDShadowMapRenderPass::Render()
{
	//render background first
	vector<RenderContext>::iterator bk = pScene->_RenderQueue.Background.begin();
	while (bk != pScene->_RenderQueue.Background.end()) {
		int receiveShadow = ShouldRenderShadow(*bk);
		SetShaderValue(shadowShader, receiveShadowLoc, &receiveShadow, SHADER_UNIFORM_INT);
		bk->pComponent->Draw(&Hints);
		++bk;
	}

	//render opauqe geometry from nearest to farest
	multiset<RenderContext, CompareDistanceAscending>::iterator opaque = pScene->_RenderQueue.Geometry.begin();
	while (opaque != pScene->_RenderQueue.Geometry.end()) {
		int receiveShadow = ShouldRenderShadow(*opaque);
		SetShaderValue(shadowShader, receiveShadowLoc, &receiveShadow, SHADER_UNIFORM_INT);
		opaque->pComponent->Draw(&Hints);
		++opaque;
	}

	//render alpha blend
	multiset<RenderContext, CompareDistanceDescending>::iterator alpha = pScene->_RenderQueue.AlphaBlending.begin();
	while (alpha != pScene->_RenderQueue.AlphaBlending.end()) {
		rlDisableDepthMask();
		rlDisableBackfaceCulling();
		BeginBlendMode(alpha->pComponent->blendingMode);
		int receiveShadow = ShouldRenderShadow(*alpha);
		SetShaderValue(shadowShader, receiveShadowLoc, &receiveShadow, SHADER_UNIFORM_INT);
		alpha->pComponent->Draw(&Hints);
		EndBlendMode();
		rlEnableDepthMask();
		rlEnableBackfaceCulling();
		++alpha;
	}

	//render overlay first
	vector<RenderContext>::iterator overlay = pScene->_RenderQueue.Overlay.begin();
	while (overlay != pScene->_RenderQueue.Overlay.end()) {
		int receiveShadow = ShouldRenderShadow(*overlay);
		SetShaderValue(shadowShader, receiveShadowLoc, &receiveShadow, SHADER_UNIFORM_INT);
		overlay->pComponent->Draw(&Hints);
		++overlay;
	}
}

//End of LoDShadowMapRenderPass.cpp