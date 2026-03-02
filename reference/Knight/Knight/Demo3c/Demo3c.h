#pragma once

#include "Knight.h"

class Demo3c : public Knight
{
public:
	void Start() override;

protected:
	void Update(float ElapsedSeconds) override;
	void DrawGUI() override;
	void EndGame() override;

private:
	void Shuffle();
};

