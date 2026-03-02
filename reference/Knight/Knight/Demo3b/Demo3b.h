#pragma once

#include "Knight.h"

class Demo3b : public Knight
{
public:
	void Start() override;

protected:
	void Update(float ElapsedSeconds) override;

private:
	void PlaceCharacters();
};

