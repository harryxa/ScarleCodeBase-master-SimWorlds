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


	if (_GD->m_dt * 0.2 > ((float)rand() / (float)RAND_MAX))
	{
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

		
	}

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it)->isAlive())
		{
			if (boidsSpawned > 1)
			{
				(*it)->MovePos(Seperation((*it)));
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

	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{			
			CofM += (*it)->GetPos();
		}		
	}

	CofM = CofM / (boidsSpawned - 1);

	cohesion_rule = (CofM - _boid->GetPos()) / 300;

	return cohesion_rule;
}

Vector3 BoidManager::Seperation(Boid * _boid)
{
	for (vector<Boid *>::iterator it = m_Boids.begin(); it != m_Boids.end(); it++)
	{
		if ((*it) != _boid)
		{
			/*if ( ( ( (*it)->GetPos().x - _boid->GetPos().x) < 5) && ( ( (*it)->GetPos().y - _boid->GetPos().y) < 5) && ( ( (*it)->GetPos().z - _boid->GetPos().z) < 5))*/

			if((fabs((*it)->GetPos().x - _boid->GetPos().x)) < 10)
			{
				seperation_rule.x -= ((*it)->GetPos().x - _boid->GetPos().x);
			}

			//if ((((*it)->GetPos().x - _boid->GetPos().x) < 2))
			//{
			//	seperation_rule.x -= ((*it)->GetPos().x - _boid->GetPos().x);
			//}

			//if ((((*it)->GetPos().z - _boid->GetPos().z) < 2))
			//{
			//	seperation_rule.z -= ((*it)->GetPos().z - _boid->GetPos().z);
			//}

			//if ((((*it)->GetPos().y - _boid->GetPos().y) < 2))
			//{
			//	seperation_rule.y -= ((*it)->GetPos().y - _boid->GetPos().y);
			//}
		}
	}
	return seperation_rule / 10000000;
}

Vector3 BoidManager::Alignment(Boid * _boid)
{
	return alignment_rule;
}
