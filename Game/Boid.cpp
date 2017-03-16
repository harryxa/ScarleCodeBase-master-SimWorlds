#include "Boid.h"

Boid::Boid(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF) : CMOGO(_fileName, _pd3dDevice, _EF)
{
	m_alive = false;

	m_fudge = Matrix::CreateRotationY(XM_PIDIV4);//gives local rotation

	m_up = Vector3::Transform(Vector3::Up, m_fudge.Invert() * m_worldMat) - m_pos;

}


Boid::~Boid()
{
}

void Boid::Spawn(Vector3 _pos)
{
	m_alive = true;
	m_pos = _pos;
	//do stuff

}

void Boid::Despawn()
{
	m_alive = false;
	m_pos = Vector3::Zero;
}

void Boid::Tick(GameData * _GD)
{
	if (m_alive)
	{
		
		Matrix scaleMat = Matrix::CreateScale(m_scale);
		Matrix rotTransMat = Matrix::CreateWorld(m_pos, m_vel, m_up);
		Matrix transMat = Matrix::CreateTranslation(m_pos);
		m_worldMat = m_fudge * scaleMat * rotTransMat * transMat;
		
	}
}



void Boid::Draw(DrawData * _DD)
{
	if (m_alive)
	{
		CMOGO::Draw(_DD);
	}
}


