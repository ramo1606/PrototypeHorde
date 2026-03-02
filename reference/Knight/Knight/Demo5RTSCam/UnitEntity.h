#pragma once

#include "Knight.h"

typedef struct  
{
	char Name[32];
	Vector3 position;
	Vector2 LabelPos;
	bool selected;
} UnitEntity;
