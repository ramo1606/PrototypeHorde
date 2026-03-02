#pragma once

#include "Knight.h"

class Entity
{
public:
	// method to render the entire level
	virtual void Create(Scene* pScene, Entity* pParent) = 0;
	virtual void Update(float elaspedTime);
	SceneActor* Actor;
	int HP;

	Entity();
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
	void Update(float elaspedTime) override;
	void Die();
	void Resurrect();
	EnemyEntity();
	float respawnInterval;
};

class TerrainEntity : public Entity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
};


