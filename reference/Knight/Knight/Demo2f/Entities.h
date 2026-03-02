#pragma once

#include "Knight.h"

#include <queue>

typedef enum
{
	ApplyDamage = 1,
	LostLife
}  ActionId;

struct ActionEvent
{
	ActionId Id;
	int Value;
};

class Entity
{
public:
	// method to render the entire level
	virtual void Create(Scene* pScene, Entity* pParent) = 0;
	virtual void Update(float elaspedTime);
	virtual void DrawGUI();
	SceneActor* Actor;
	Entity();
};

class AliveEntity : public Entity
{
public:
	int HP;
	AliveEntity();
	void Update(float elaspedTime) override;
	virtual void Die();
	virtual void DoAttack();
	void SetTarget(AliveEntity*);
	void AddAction(ActionId id, int value);

	inline float GetRechargeRatio() { return rechargeTime / attackCooldown; }

protected:
	virtual void DealDamage(int originalValue);
	virtual float GetCooldownTime();

	queue<ActionEvent*> eventsQueue;
	AliveEntity* _target;

	float attackCooldown;
	float rechargeTime;
};

class PlayerEntity : public AliveEntity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
	void Update(float elaspedTime) override;
	void DrawGUI() override;
	void Die() override;
};

class EnemyEntity : public AliveEntity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
	void Update(float elaspedTime) override;
	void DrawGUI() override;
	void Die() override;
};

class TerrainEntity : public Entity
{
public:
	// method to render the entire level
	void Create(Scene* pScene, Entity* pParent) override;
};



