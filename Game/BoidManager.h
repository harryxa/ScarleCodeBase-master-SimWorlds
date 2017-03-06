#pragma once
#include "gameobject.h"
#include <vector>
#include "GameData.h"
#include "Model.h"
#include <string>

using namespace std;
//using namespace DirectX;

class Boid;

class BoidManager : public GameObject
{
public:
	BoidManager(int _numBoids, std::string _boidFileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);
	~BoidManager();

	virtual void Tick(GameData* _GD) override;
	virtual void Draw(DrawData* _DD) override;

	Vector3 Cohesion(Boid* _boid);
	Vector3 cohesion_rule = Vector3::Zero;

	vector<Boid*> GetBoids() { return m_Boids; }

private:
	vector<Boid*> m_Boids;
	int boidsSpawned = 0;
};

