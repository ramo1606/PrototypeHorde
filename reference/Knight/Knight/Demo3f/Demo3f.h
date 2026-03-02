#pragma once

#include "Knight.h"
#include "ObjectPool.h" 
#include "Bullet.h"

class Demo3f : public Knight
{
private:
	ObjectPool<Bullet> _bulletPool;

public:
	void Start() override;

protected:
	void Update(float ElapsedSeconds) override;
	void DrawFrame();
	void DrawGUI();
};

