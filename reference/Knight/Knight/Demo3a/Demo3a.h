#pragma once

#include "Knight.h"

class Demo3a : public Knight
{
public:
	void Start() override;

protected:
	void Update(float ElapsedSeconds) override;
};

