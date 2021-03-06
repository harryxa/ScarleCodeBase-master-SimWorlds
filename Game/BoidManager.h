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
	void LimitSpeed(Boid* _boid);
	Vector3 BoundPosition(Boid* _boid);
	Vector3 PreyScatter(Boid* _boid);

	//getters for prey
	float* get_coh_mod();
	float* get_sep_mod();
	float* get_ali_mod();
	float* get_cohesion_awareness();
	float* get_seperation_awareness();
	float* get_scatter();
	
	//getters for pred
	float* get_coh_mod_pred();
	float* get_sep_mod_pred();
	float* get_ali_mod_pred();

	float* get_speed_limit();
	float* get_boids_spawned();
	float* set_pred();
	
	float* setDimension();

	void setDimensionFunction(Boid * _boid);

	vector<Boid*> GetBoids() { return m_Boids; }

protected:

	int m_boidsSpawned;

private:
	vector<Boid*> m_Boids;

	//modifiers for prey
	float cohesion_modifier = 20;
	float seperation_modifier = 1;
	float alignment_modifier = 1.4;

	float prey_scatter_awareness = 6;
	float cohesion_awareness = 10;
	float seperation_awareness = 2.5;

	//modifiers for predator
	float cohesion_modifier_pred = 4;
	float seperation_modifier_pred = 3;
	float alignment_modifier_pred = 3.7;

	float speed_limit = 5;
	
	float boidsSpawned = 0;

	float dimension = 0;

	float enemycount = 0;
	
};

