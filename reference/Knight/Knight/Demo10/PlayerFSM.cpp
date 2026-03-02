#include "PlayerFSM.h"
#include "Knight.h"

PlayerFSM::PlayerFSM(SceneActor* Character, ModelComponent* AnimController)
    : FSM(Character, AnimController)
{
    _Character = Character;
    _AnimController = AnimController;
    _States[IDLE] = new PlayerIdleState();
    _States[WALK] = new PlayerWalkState();
    _States[ATTACK] = new PlayerAttackState();
    SetState(IDLE);
}

void PlayerIdleState::Enter(FSM* FiniteStateMachine)
{
    if (FiniteStateMachine->GetPrevState() == PlayerFSM::UNKNOWN)
    {
        FiniteStateMachine->GetAnimController()->SetAnimation(PlayerFSM::IDLE);
    }
    else
    {
        FiniteStateMachine->GetAnimController()->TransitionAnimation(PlayerFSM::IDLE);
    }
}

void PlayerIdleState::Update(FSM* FiniteStateMachine, float DeltaTime)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        FiniteStateMachine->SetState(PlayerFSM::ATTACK);
        return;
    }

    SceneActor* character = FiniteStateMachine->GetCharacter();
    if (IsKeyDown(KEY_A))
    {
        character->Rotation.y = 90.0f;
        FiniteStateMachine->SetState(PlayerFSM::WALK);
    }
    else if (IsKeyDown(KEY_W))
    {
        character->Rotation.y = 0.0f;
        FiniteStateMachine->SetState(PlayerFSM::WALK);
    }
    else if (IsKeyDown(KEY_D))
    {
        character->Rotation.y = -90.0f;
        FiniteStateMachine->SetState(PlayerFSM::WALK);
    }
    else if (IsKeyDown(KEY_S))
    {
        character->Rotation.y = 180.0f;
        FiniteStateMachine->SetState(PlayerFSM::WALK);
    }
}

void PlayerWalkState::Enter(FSM* FiniteStateMachine)
{
    FiniteStateMachine->GetAnimController()->TransitionAnimation(PlayerFSM::WALK);
}

void PlayerWalkState::Update(FSM* FiniteStateMachine, float DeltaTime)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        FiniteStateMachine->SetState(PlayerFSM::ATTACK);
        return;
    }

    float walkSpeed = 3.0f;
    SceneActor* character = FiniteStateMachine->GetCharacter();
    if (IsKeyDown(KEY_A))
    {
        character->Rotation.y = 90.0f;
        character->Position.x += walkSpeed * DeltaTime;
    }
    else if (IsKeyDown(KEY_W))
    {
        character->Rotation.y = 0.0f;
        character->Position.z += walkSpeed * DeltaTime;
    }
    else if (IsKeyDown(KEY_D))
    {
        character->Rotation.y = -90.0f;
        character->Position.x -= walkSpeed * DeltaTime;
    }
    else if (IsKeyDown(KEY_S))
    {
        character->Rotation.y = 180.0f;
        character->Position.z -= walkSpeed * DeltaTime;
    }
    else
    {
        FiniteStateMachine->SetState(PlayerFSM::IDLE);
    }
}

void PlayerAttackState::Enter(FSM* FiniteStateMachine)
{
    _Timer = 0.5f;
    FiniteStateMachine->GetAnimController()->TransitionAnimation(FSM::ATTACK);
}

void PlayerAttackState::Update(FSM* FiniteStateMachine, float DeltaTime)
{
    _Timer -= DeltaTime;
    if (_Timer <= 0.0f)
    {
        FiniteStateMachine->SetState(PlayerFSM::IDLE);
    }
}

