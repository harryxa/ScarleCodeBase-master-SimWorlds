#pragma once

#include "CMOGO.h"
#include "BoidManager.h"

class Boid : public CMOGO
{
public:
	Boid(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);
	~Boid();

	void Spawn(Vector3 _pos);

	void Despawn();

	virtual void Tick(GameData* _GD) override;
	virtual void Draw(DrawData* _DD) override;

	bool isAlive() { return m_alive; }
	void setAlive(bool _m_alive) { m_alive = _m_alive; }

	bool getEnemy();
	int getBoid_tag();

	void setBoid_tag(int m_boid_tag) { boid_tag = m_boid_tag; }
	void setEnemy(bool m_enemy) { enemy = m_enemy; }
	

protected:

	bool m_alive;
	Vector3 m_up;

private:

	bool enemy = false;
	int boid_tag;



};

