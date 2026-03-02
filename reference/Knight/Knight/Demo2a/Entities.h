#pragma once

#include "Knight.h"

class Entity
{
public:
	// method to render the entire level
	virtual void Create(Scene* pScene, Entity* pParent) = 0;
	virtual void Update(float elapsedTime);
	SceneActor* Actor = nullptr;
};

class PlayerEntity : public Entity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
};

class EnemyEntity : public Entity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
};

class TerrainEntity : public Entity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
};

class PropEntity : public Entity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
};

