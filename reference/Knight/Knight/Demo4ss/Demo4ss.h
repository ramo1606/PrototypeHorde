#pragma once

#include "Knight.h"   //Engine includes

class Demo4ss : public Knight
{
public:
	void Start() override;
	void EndGame() override;

	Demo4ss();

protected:
	float scrollingBack; //The farest background layer
	float scrollingMid;  //The middle layer
	float scrollingFore;  //The closest layer

	Texture2D background;
	Texture2D midground;
	Texture2D foreground;

	void Update(float ElapsedSeconds) override;
	void DrawGUI() override;
	void OnCreateDefaultResources() override;
};
