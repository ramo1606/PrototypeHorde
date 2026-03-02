#pragma once

#include "NeuralNetwork.h"
#include "Knight.h"

class TurretController
{
private:
	NeuralNetwork* _ANN;			//Artificial Neural Network
	Scene* _Scene;					//The game scene	
	SceneActor* _Cannon;			//The turret cannon	
	Vector3 _CannonDir = Vector3{ 0.0f, 0.0f, -1.0f }; //The cannon's aiming direction 
	float _CannonRotAngle = 0.0f;	//The cannon's rotation angle 
	float _AttackRange = 15.0f;		//The turret's attack range
	float _FiringArc = 20.0f;		//The turret's firing arc in degrees
	float _TurnSpeed = 8.0f;		//The turret's turning speed
	float _VisionRange;				//The turret's vision range
	Vector3 _FiringDir;				//The firing direction 
private:
	float _IsLoaded;				//The cannon is loaded or not
	SceneActor* _Fireball;			//The Fireball actor
	float _FireballDuration = 1.5f;	//The fireball's lifetime  
	float _FireballSpeed = 15.0f;	//The fireball's speed
public:
	int Trained = 0;				//Indicates whether the ANN is trined
public:
	TurretController(Scene* Scene, SceneActor* Cannon);
	~TurretController();
	
	void InitANN(int Method);	//1-shadow learning, 2-deep learning
	void Train(int Method, int SampleCount = 10000,
		int epochs = 1);
	void Update(float DeltaTime);
};

