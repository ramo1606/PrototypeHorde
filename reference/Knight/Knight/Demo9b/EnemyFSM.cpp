#include "EnemyFSM.h"
#include "Knight.h"

EnemyFSM::EnemyFSM(SceneActor* Character, ModelComponent* AnimController)
    : FSM(Character, AnimController)
{
    _Character = Character;
    _AnimController = AnimController;
    _States[IDLE] = new EnemyIdleState();
    _States[WALK] = new EnemyWalkState();
    _States[ATTACK] = new EnemyAttackState();
    SetState(IDLE);
}

void EnemyIdleState::Enter(FSM* FiniteStateMachine)
{
    if (FiniteStateMachine->GetPrevState() == FSM::UNKNOWN)
    {
        FiniteStateMachine->GetAnimController()->SetAnimation(FSM::IDLE);
    }
    else
    {
        FiniteStateMachine->GetAnimController()->TransitionAnimation(FSM::IDLE);
    }
}

void EnemyIdleState::Update(FSM* FiniteStateMachine, float DeltaTime)
{
}

void EnemyWalkState::Enter(FSM* FiniteStateMachine)
{
    FiniteStateMachine->GetAnimController()->TransitionAnimation(FSM::WALK);
}

void EnemyAttackState::Enter(FSM* FiniteStateMachine)
{
    _Timer = 0.5f;
    FiniteStateMachine->GetAnimController()->TransitionAnimation(FSM::ATTACK);
}

void EnemyAttackState::Update(FSM* FiniteStateMachine, float DeltaTime)
{
    _Timer -= DeltaTime;
    if (_Timer <= 0.0f)
    {
        FiniteStateMachine->SetState(FSM::IDLE);
    }
}

