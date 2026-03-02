#pragma once

#include "Knight.h"   //Engine includes

class Demo4iso : public Knight
{
public:
	void Start() override;
	void EndGame() override;

	Demo4iso();

protected:
	PerspectiveCamera* camera = nullptr;
	Texture2D _mapTiles = { 0 };

	int map[10][10];

	Vector2 mapPos = { -1 };
	Vector2 mousePos = { -1 };

	void DrawGUI() override;
	void OnCreateDefaultResources() override;
};