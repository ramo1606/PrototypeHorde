#pragma once

#include "Knight.h"

class Demo9a : public Knight
{
public:
	void Start() override;
	void EndGame() override;
protected:
	void Update(float DeltaTime) override;
	void DrawGUI();
};

