#pragma once

#include "Knight.h"

class Demo1 : public Knight
{
public:
	void Start() override;
	void EndGame() override;

protected:
	void Update(float ElapsedSeconds) override;
	void DrawFrame() override;
};