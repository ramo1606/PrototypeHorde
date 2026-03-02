
#include "BehaviourTree.h"
#include "EnemyFSM.h"

TreeNode::TreeNode(BehaviourTree* BT)
{
    _BT = BT;
}

TreeNode::~TreeNode()
{
}

ETreeStatus TreeNode::Update(float DeltaTime)
{
    return BT_FAILURE;
}

void CompositeNode::AddChild(std::shared_ptr<TreeNode> childNode)
{
    _children.push_back(childNode);
}

ETreeStatus Sequence::Update(float DeltaTime)
{
    for (const auto& child : _children) 
    {
        ETreeStatus status = child->Update(DeltaTime);
        if (status == BT_FAILURE || status == BT_RUNNING)
        {
            return status;
        }
    }
    return BT_SUCCESS;
}

ETreeStatus Selector::Update(float DeltaTime)
{
    for (const auto& child : _children) 
    {
        ETreeStatus status = child->Update(DeltaTime);
        if (status == BT_SUCCESS || status == BT_RUNNING)
        {
            return status;
        }
    }
    return BT_FAILURE;
}

ETreeStatus GuardTask::Update(float DeltaTime)
{
    auto modelComponent = _BT->GetBlackboard().NPC->GetComponent<ModelComponent>();
    FSM::ECharacterState animState = (FSM::ECharacterState)modelComponent->GetAnimation();

    if (_BT->GetBlackboard().DistanceFromPlayer <= 15.0f)
    {
        return BT_FAILURE;
    }
    else if (animState != EnemyFSM::ECharacterState::IDLE)
    {
        auto bt = _BT->GetBlackboard().FSM;
        bt->SetState(EnemyFSM::ECharacterState::IDLE);
    }
    return BT_RUNNING;
}

ETreeStatus GotoHeroTask::Update(float DeltaTime)
{
    auto blackboard = _BT->GetBlackboard();
    auto player = blackboard.Player;
    auto npc = blackboard.NPC;
    auto modelComponent = npc->GetComponent<ModelComponent>();
    FSM::ECharacterState animState = (FSM::ECharacterState)modelComponent->GetAnimation();
    auto fsm = blackboard.FSM;

    if (blackboard.DistanceFromPlayer <= 3.0f)
    {
        return BT_SUCCESS;
    }
    else
    {
        if (animState != EnemyFSM::ECharacterState::WALK)
        {
            fsm->SetState(EnemyFSM::ECharacterState::WALK);
        }
        return BT_RUNNING;
    }
}

ETreeStatus AttackTask::Update(float DeltaTime)
{
    auto blackboard = _BT->GetBlackboard();
    auto modelComponent = blackboard.NPC->GetComponent<ModelComponent>();
    FSM::ECharacterState animState = (FSM::ECharacterState)modelComponent->GetAnimation();
    auto fsm = blackboard.FSM;

    if (animState != FSM::ECharacterState::ATTACK)
    {
        if (_Interval <= 0.0f)
        {
            fsm->SetState(FSM::ECharacterState::ATTACK);
            _Interval = 1.5f;
        }
        else
        {
            _Interval -= DeltaTime;
            if (_Interval <= 0.0f)
            {
                return BT_SUCCESS;
            }
        }
    }
    return BT_RUNNING;
}

BehaviourTree::BehaviourTree(SceneActor* NPC, EnemyFSM* NPC_FSM, SceneActor* Player)
{
    //Initialize the blackboard
    _blackboard.NPC = NPC;
    _blackboard.FSM = NPC_FSM;
    _blackboard.Player = Player;
    _blackboard.Velocity = Vector3Zero();
    _blackboard.WalkSpeed = 1.5f;
    _blackboard.MaxSpeed = 3.0f;
    //Build up the behaviour tree
    _root = make_shared<Selector>();
    _root->AddChild(make_shared<GuardTask>(this));
    auto sequence = make_shared<Sequence>();
    sequence->AddChild(make_shared<GotoHeroTask>(this));
    sequence->AddChild(make_shared<AttackTask>(this));
    _root->AddChild(sequence);
}

void BehaviourTree::Update(float DeltaTime)
{
    //Calculate the distance from the player and the emnemy
    _blackboard.DistanceFromPlayer = Vector3Distance(_blackboard.NPC->Position, _blackboard.Player->Position);
    //Update the tree
    _root->Update(DeltaTime);
    //Update the finite state machine for animation control
    _blackboard.FSM->Update(DeltaTime);
    //Steering process
    Steering(DeltaTime);
}

void BehaviourTree::Steering(float DeltaTime)
{
    auto modelComponent = _blackboard.NPC->GetComponent<ModelComponent>();
    FSM::ECharacterState animState = (FSM::ECharacterState)modelComponent->GetAnimation();
    
    //Get the vector from Player to NPC
    Vector3 dir = Vector3Subtract(_blackboard.Player->Position, _blackboard.NPC->Position);
    _blackboard.Velocity = Vector3Zero();
    if (Vector3Equals(dir, Vector3Zero()))
    {
        dir = Vector3{ 0.0f, 0.0f, 1.0f };
    }

    //Calculate the movement velocity
	if (animState == FSM::WALK)
	{
		dir = Vector3Normalize(dir);
		_blackboard.Velocity = Vector3Scale(dir, _blackboard.WalkSpeed * DeltaTime);
	}

    //Add Vpush when NPC and Player are close
	if (_blackboard.DistanceFromPlayer < 2.6f)
	{
        auto Vpush = Vector3Scale(Vector3Negate(dir), _blackboard.MaxSpeed * DeltaTime);
		_blackboard.Velocity = Vector3Add(_blackboard.Velocity, Vpush);
	}
    //Move NPC to the new location
	_blackboard.NPC->Position = Vector3Add(_blackboard.NPC->Position, _blackboard.Velocity);
    //Rotate NPC to face Player
	_blackboard.NPC->Rotation.y = RAD2DEG * atan2f(dir.x, dir.z);
}