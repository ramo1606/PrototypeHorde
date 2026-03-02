#pragma once

#include "ShadowMapRenderPass.h"

class LoDShadowMapRenderPass : public ShadowMapRenderPass
{
	public:
		LoDShadowMapRenderPass(float cutof, ShadowSceneLight* l, int id);
		void Render() override;
	protected:
		float CutOffDistance = 25.0f * 25.0f; //distance beyond which shadows are not rendered

		int ShouldRenderShadow(const RenderContext &);
};

//End of LoDShadowMapRenderPass.h