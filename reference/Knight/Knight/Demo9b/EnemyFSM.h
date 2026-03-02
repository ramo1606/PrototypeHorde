#pragma once

#include "FSM.h"

class EnemyFSM : public FSM
{
public:
    EnemyFSM(SceneActor* Character, ModelComponent* AnimController);
};

class EnemyIdleState : public FSMState
{
public:
    void Enter(FSM* FiniteStateMachine) override;
    void Update(FSM* FiniteStateMachine, float DeltaTime) override;
};

class EnemyWalkState : public FSMState
{
public:
    void Enter(FSM* FiniteStateMachine) override;
    void Update(FSM* FiniteStateMachine, float DeltaTime) override {}
};

class EnemyAttackState : public FSMState
{
private:
    float _Timer = 0.0f;
public:
    void Enter(FSM* FiniteStateMachine) override;
    void Update(FSM* FiniteStateMachine, float DeltaTime) override;
};
