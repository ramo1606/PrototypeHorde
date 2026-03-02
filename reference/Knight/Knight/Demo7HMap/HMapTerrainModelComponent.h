#pragma once

#include "raylib.h"

#include "SceneActor.h"
#include "Component.h"

class HMapTerrainModelComponent : public Component
{
public:

	bool CreateFromFile(Vector3 terrainDimension, Vector2 texTileSize, const char* pHightmapFilePath, const char* pTerrainTexurePath);
	void Draw(RenderHints *pRH = nullptr) override;

	Image hightMapImage = { 0 };
	Model model = { 0 };
	Mesh mesh = { 0 };

	Color tint = WHITE;

	friend SceneActor;

protected:
	Mesh GenMeshHeightmapEx(Image heightmap, Vector3 size, Vector2 texPatchSize);
};

