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
	Vector3 Seperation(Boid* _boid);
	Vector3 Alignment(Boid* _boid);


	vector<Boid*> GetBoids() { return m_Boids; }

	int		GetBoidsSpawned() { return m_boidsSpawned; }
	void	SetBoidsSpawned(int _boidsSpawned) { m_boidsSpawned = _boidsSpawned; }

protected:

	int m_boidsSpawned;

private:
	vector<Boid*> m_Boids;
	int boidsSpawned = 0;
};

