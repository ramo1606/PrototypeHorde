#pragma once

#include "Component.h"

typedef enum {
	UPWARD_ALIGNED = 1,  //Axis aligned to the Y axis
	SCREEN_ALIGNED = 2		//Aligned to the screen
} BillboardAlignType;

class BillboardComponent : public Component
{
public:

	BillboardComponent();
	~BillboardComponent();

	void Draw(RenderHints *pRH = nullptr) override;

	Texture2D texture = { 0 };
	Rectangle source = { 0 };
	Vector2 size = { 0 };
	Vector2 origin = { 0 };

	Color tint = WHITE;

	BillboardAlignType AlignType = UPWARD_ALIGNED;

	friend SceneActor;
};

//end of BillboardComponent.h