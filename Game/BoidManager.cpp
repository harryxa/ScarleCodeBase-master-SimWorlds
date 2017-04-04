#include "BoidManager.h"
#include "Boid.h"

#include <ctime>
#include <iostream>

BoidManager::BoidManager(int _numBoids, std::string _boidFileName, ID3D11Device * _pd3dDevice, IEffectFactory * _EF)
{
	srand(time(NULL));

	for (int i = 0; i < _numBoids; i++)
	{
		m_Boids.push_back(new Boid(_boidFileName, _pd3dDevice, _EF));
	}
}

BoidManager::~BoidManager()
{
	for (auto& boid : m_Boids)
	{
		delete boid;
	}
}

void BoidManager::Tick(GameData * _GD)
{
	Vector3 forwardMove = Vector3::Forward;

	//spawn boids and assign tags
	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		//if boid is not alive then spawn, increase boidsSpawned count and give a tag and set to alive in spawn function
		if ((*it)->isAlive() == false)
		{
			for (int tag = 0; tag < boidsSpawned + 1; tag++)
			{
				(*it)->setBoid_tag(tag);
			}

			(*it)->SetVel(Vector3(0, 0, 0));
			boidsSpawned++;
			(*it)->Spawn({ (float)(rand() % 90) - 50 , (float)(rand() % 90) - 50,  (float)(rand() % 90) - 50 }); //make random number
			break;
		}

		if ((*it)->getBoid_tag() <= enemycount)
		{
			(*it)->setEnemy(true);
			(*it)->SetScale(5);
		}
		if ((*it)->getBoid_tag() >= enemycount)
		{
			(*it)->setEnemy(false);
			(*it)->SetScale(1);
		}
	}

	//apply rules to all boids
	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it)->isAlive())
		{
			if (boidsSpawned > 1)
			{
				Vector3 v1_prey, v1_predator, v2_prey, v2_predator, v3_prey, v3_predator, v4, v5_prey;

				//prey rules
				v1_prey = Seperation(*it)  * _GD->m_dt / seperation_modifier;
				v2_prey = Cohesion(*it)  * _GD->m_dt / cohesion_modifier;
				v3_prey = Alignment(*it)  * _GD->m_dt / alignment_modifier;
				v5_prey = PreyScatter(*it);

				//preditor rules
				v1_predator = Seperation(*it)  * _GD->m_dt / seperation_modifier_pred;				
				v2_predator = Cohesion(*it)  * _GD->m_dt / cohesion_modifier_pred;				
				v3_predator = Alignment(*it)  * _GD->m_dt / alignment_modifier_pred;

				//shared rules
				v4 = BoundPosition(*it);				

				if ((*it)->getEnemy() == false)
				{
					//sets boid a velocity
					(*it)->SetVel((*it)->GetVel() + v1_prey + v2_prey + v3_prey + v4 + v5_prey);
				}
				if ((*it)->getEnemy() == true)
				{
					(*it)->SetVel((*it)->GetVel() + v1_predator + v2_predator + v3_predator + v4);
				}
				//limits the speed of the boids
				LimitSpeed(*it);

				//moves the boids using velocity
				(*it)->MovePos((*it)->GetVel());
				setDimensionFunction((*it));
			}
			(*it)->Tick(_GD);
		}
	}
}

void BoidManager::Draw(DrawData * _DD)
{
	for (auto& boid : m_Boids)
	{
		if (boid->isAlive())
		{
			boid->Draw(_DD);
		}
	}
}

Vector3 BoidManager::Cohesion(Boid* _boid)
{
	Vector3 CofM = Vector3::Zero;
	Vector3 cohesion_rule = Vector3::Zero;
	int close = 0;

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid && (*it)->isAlive() && (*it)->getEnemy() == false)
		{
			//if boids are within cohesion awareness, calculate CofM
			if (fabs(Vector3::Distance((*it)->GetPos(), _boid->GetPos())) < cohesion_awareness)
			{
				CofM += (*it)->GetPos();
				close++;
			}
		}
	}

	if (close > 0)
	{
		CofM = CofM / (close);
		cohesion_rule = (CofM - _boid->GetPos());
	}
	if (close < 1)
	{
		cohesion_rule = (CofM - _boid->GetPos()) / 10;
	}

	return cohesion_rule;
}

Vector3 BoidManager::Seperation(Boid * _boid)
{
	Vector3 seperation_rule;

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{
			if (fabs(Vector3::Distance((*it)->GetPos(), _boid->GetPos())) <= seperation_awareness)
			{
				seperation_rule -= (*it)->GetPos() - _boid->GetPos();
			}
		}
	}

	return seperation_rule;
}

Vector3 BoidManager::PreyScatter(Boid * _boid)
{
	Vector3 prey_rule;

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it)->getEnemy() == true)
		{
			if (fabs(Vector3::Distance((*it)->GetPos(), _boid->GetPos())) <= prey_scatter_awareness)
			{
				prey_rule -= (*it)->GetPos() - _boid->GetPos();
			}
		}
	}

	return prey_rule / 20;
}

Vector3 BoidManager::Alignment(Boid * _boid)
{
	Vector3 alignment_rule = Vector3::Zero;

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{
			if (fabs(Vector3::Distance((*it)->GetPos(), _boid->GetPos())) <= 20.0f)
			{
				alignment_rule += (*it)->GetVel();
			}
		}
	}
	alignment_rule = (alignment_rule / (boidsSpawned - 1));

	return (alignment_rule - _boid->GetVel());
}

//limits the velocity of the boids
void BoidManager::LimitSpeed(Boid * _boid)
{
	float vLimit = speed_limit;
	float bvelocity = fabs(_boid->GetVel().x) + fabs(_boid->GetVel().y) + fabs(_boid->GetVel().z);

	if ((fabs(_boid->GetVel().x) + fabs(_boid->GetVel().y) + fabs(_boid->GetVel().z) > vLimit))
	{
		_boid->SetVel(_boid->GetVel() / bvelocity * vLimit);
	}
}

//sets the limits to which boids can move
Vector3 BoidManager::BoundPosition(Boid * _boid)
{
	int i = 60;
	int j = 60; // for if i want bounding box to be changable, 1. continuous flow, 2. bounce of walls
	int Xmin = -j, Xmax = j, Ymin = -j, Ymax = j, Zmin = -j, Zmax = j;
	Vector3 bound_rule;

	if (_boid->GetPos().x < Xmin)
	{
		_boid->SetPos(Vector3(i, _boid->GetPos().y, _boid->GetPos().z));
		//bound_rule.x = i;
	}
	else if (_boid->GetPos().x > Xmax)
	{
		_boid->SetPos(Vector3(-i, _boid->GetPos().y, _boid->GetPos().z));
		//bound_rule.x = -i;
	}

	if (_boid->GetPos().y < Ymin)
	{
		_boid->SetPos(Vector3(_boid->GetPos().x, i, _boid->GetPos().z));
		//bound_rule.y = i;
	}
	else if (_boid->GetPos().y > Ymax)
	{
		_boid->SetPos(Vector3(_boid->GetPos().x, -i, _boid->GetPos().z));
		//bound_rule.y = -i;
	}

	if (_boid->GetPos().z < Zmin)
	{
		_boid->SetPos(Vector3(_boid->GetPos().x, _boid->GetPos().y, i));
		//bound_rule.z = i;
	}
	else if (_boid->GetPos().z > Zmax)
	{
		_boid->SetPos(Vector3(_boid->GetPos().x, _boid->GetPos().y, -i));
		//bound_rule.z = -i;
	}

	return bound_rule;
}

float * BoidManager::get_coh_mod()
{
	return &cohesion_modifier;
}

float * BoidManager::get_sep_mod()
{
	return &seperation_modifier;
}

float * BoidManager::get_ali_mod()
{
	return &alignment_modifier;
}

float * BoidManager::get_coh_mod_pred()
{
	return &cohesion_modifier_pred;
}

float * BoidManager::get_sep_mod_pred()
{
	return &seperation_modifier_pred;
}

float * BoidManager::get_ali_mod_pred()
{
	return &alignment_modifier_pred;
}

float * BoidManager::get_speed_limit()
{
	return &speed_limit;
}

float * BoidManager::get_boids_spawned()
{
	return &boidsSpawned;
}

float * BoidManager::set_pred()
{
	return &enemycount;
}


float * BoidManager::get_cohesion_awareness()
{
	return&cohesion_awareness;
}

float * BoidManager::get_seperation_awareness()
{
	return&seperation_awareness;
}

float * BoidManager::get_scatter()
{
	return&prey_scatter_awareness;
}

float * BoidManager::setDimension()
{
	return&dimension;
}

Vector3 BoidManager::setDimensionFunction(Boid * _boid)
{
	Vector3 setd;

	if (dimension == 1)
	{
		_boid->SetPos(Vector3(_boid->GetPos().x, 0, _boid->GetPos().z));
	}
	return setd;
}





