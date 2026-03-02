#pragma once

#include "Knight.h"
#include "MazeGenerator.h"

class Demo3e : public Knight
{
private:
	PerspectiveCamera* _camera;
	MazeGenerator* _mazeGenerator = nullptr;

public:
	void Start() override;
	void EndGame() override;

protected:
	void Update(float ElapsedSeconds) override;
	void DrawFrame() override;
	void DrawGUI() override;
	void DrawMazeBoundaryWalls(Vector3 startPos, int row, int col);
};

