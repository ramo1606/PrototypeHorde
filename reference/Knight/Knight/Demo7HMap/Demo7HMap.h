#pragma once

#include "Knight.h"   //Engine includes

#include "HMapTerrainModelComponent.h"

class Demo7HMap : public Knight
{
public:
	void Start() override;
	void EndGame() override;

	PerspectiveCamera* pMainCamera = NULL;
	SceneActor* pTerrain = NULL;

	Image image = { 0 };
	Texture2D texture = { 0 };

protected:

	void DrawFrame() override;
	void DrawGUI() override;
	void OnCreateDefaultResources() override;
};


