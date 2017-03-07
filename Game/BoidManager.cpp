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

	float randX = rand() % 80 - 40;
	float randY = rand() % 80 - 40;


	//if (_GD->m_dt * 0.2 > ((float)rand() / (float)RAND_MAX))
	//{
		for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
		{
			if (!(*it)->isAlive())
			{
				(*it)->Spawn({ (float)(rand() % 90) - 50 , 10, (float)(rand() % 90) - 50 }); //make random number

				boidsSpawned++;
			
				cout << "X: " << (*it)->GetPos().x << endl;
				cout << "Y: " << (*it)->GetPos().y << endl;
				cout << "Z: " << (*it)->GetPos().z << endl;
				cout << "spawned boid: " <<  boidsSpawned << endl << endl;
				
				break;
			}
		}

		
	//}

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it)->isAlive())
		{
			if (boidsSpawned > 1)
			{	
				Vector3 v1, v2, v3;
				v1 = Seperation(*it);
				v2 = Cohesion(*it);
				v3 = Alignment(*it);

				(*it)->SetVel(((*it)->GetVel() + v1 + v2 + v3) * _GD->m_dt);
				(*it)->MovePos((*it)->GetVel());

				//(*it)->MovePos((Seperation(*it) + Cohesion(*it) + Alignment(*it)) * _GD->m_dt);
				
				
				//(*it)->MovePos(Rules((*it)));
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

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{
			CofM += (*it)->GetPos();				
		}		
	}

	CofM = CofM / (boidsSpawned - 1);

	cohesion_rule = (CofM - _boid->GetPos());

	return cohesion_rule / 5;
}

Vector3 BoidManager::Seperation(Boid * _boid)
{
	Vector3 seperation_rule;

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{
			if (Vector3::Distance((*it)->GetPos(), _boid->GetPos()) < 8.0f )
			{
				seperation_rule -= (((*it)->GetPos() - _boid->GetPos()) );
			}
			//else if (Vector3::Distance((*it)->GetPos(), _boid->GetPos()) > 15.0f)
			//{
			//	seperation_rule = Vector3::Zero;
			//}
		}
	}

	return seperation_rule;
}

Vector3 BoidManager::Alignment(Boid * _boid)
{
	Vector3 alignment_rule = Vector3::Zero;

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{
			alignment_rule += (*it)->GetVel();
		}
	}
	alignment_rule = (alignment_rule / (boidsSpawned - 1));


	return ((alignment_rule - _boid->GetVel()));
}

Vector3 BoidManager::Rules(Vector3 _rule1, Vector3 _rule2, Vector3 _rule3)
{
	all_rules = _rule1 + _rule2 + _rule3;

	return all_rules;
}


