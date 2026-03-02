#pragma once

#include "Knight.h"
#include <vector>
#include <memory>

using namespace std;

class EnemyFSM;
class BehaviourTree;

class Blackboard
{
public:
	float DistanceFromPlayer;

public:
	SceneActor* NPC;
	EnemyFSM* FSM;
	SceneActor* Player;
	Vector3 Velocity;
	float WalkSpeed;
	float MaxSpeed;
};

typedef enum 
{
	BT_SUCCESS,
	BT_FAILURE,
	BT_RUNNING
} ETreeStatus;

class TreeNode
{
public:
	TreeNode() : _BT(nullptr) {}
	TreeNode(BehaviourTree* BT);
	virtual ~TreeNode();
	virtual ETreeStatus Update(float DeltaTime);
	BehaviourTree* GetBehaviourTree() { return _BT; }

protected:
	BehaviourTree* _BT;
};

class CompositeNode : public TreeNode 
{
protected:
	vector<std::shared_ptr<TreeNode>> _children;

public:
	void AddChild(shared_ptr<TreeNode> childNode);
};

class Sequence : public CompositeNode 
{
public:
	ETreeStatus Update(float DeltaTime) override;
};

class Selector : public CompositeNode 
{
public:
    ETreeStatus Update(float DeltaTime) override;
};

class GuardTask : public TreeNode
{
public:
	GuardTask(BehaviourTree* BT) : TreeNode(BT) {};
	ETreeStatus Update(float DeltaTime) override;
};

class GotoHeroTask : public TreeNode
{
public:
	GotoHeroTask(BehaviourTree* BT) : TreeNode(BT) {};
	ETreeStatus Update(float DeltaTime) override;
};

class AttackTask : public TreeNode
{
private:
	float _Interval = 0.0f;
public:
	AttackTask(BehaviourTree* BT) : TreeNode(BT) {};
	ETreeStatus Update(float DeltaTime) override;
};

class BehaviourTree
{
private:
	Blackboard _blackboard;
	shared_ptr<Selector> _root;

private:
	void Steering(float DeltaTime);
public:
	BehaviourTree(SceneActor* NPC, EnemyFSM* NPC_FSM, SceneActor* Player);
	void Update(float DeltaTime);
	Blackboard& GetBlackboard() { return _blackboard; }
};

