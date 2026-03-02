#pragma once

#include "DepthRenderPass.h"

class LoDDepthRenderPass : public DepthRenderPass
{
	public:
		LoDDepthRenderPass(float cutoff, ShadowSceneLight* l);
		bool OnAddToRender(Component* pSC, SceneObject* pSO) override;

	protected:
		float CutOffDistance = 25.0f * 25.0f; 
};
