#pragma once

#include <vector>
#include <set>

#include "Component.h"

struct RenderContext
{
	Component* pComponent = nullptr;
	float distance2 = 0;
};

struct CompareDistanceAscending {
	bool operator()(const RenderContext& a, const RenderContext& b) const {
		return a.distance2 < b.distance2;
	}
};

struct CompareDistanceDescending {
	bool operator()(const RenderContext& a, const RenderContext& b) const {
		return a.distance2 > b.distance2;
	}
};

struct RenderQueues {
	vector<RenderContext> Background;
	multiset<RenderContext, CompareDistanceAscending> Geometry;
	multiset<RenderContext, CompareDistanceDescending> AlphaBlending;
	vector<RenderContext> Overlay;

	void Clear() {
		Background.clear();
		Geometry.clear();
		AlphaBlending.clear();
		Overlay.clear();
	}
};

//end of RenderQueues.h
