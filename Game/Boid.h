#pragma once

#include "CMOGO.h"

class Boid : public CMOGO
{
public:
	Boid(string _fileName, ID3D11Device* _pd3dDevice, IEffectFactory* _EF);
	~Boid();

	void Spawn(Vector3 _pos);

	virtual void Tick(GameData* _GD) override;
	virtual void Draw(DrawData* _DD) override;

	bool isAlive() { return m_alive; }

protected:

	bool m_alive;
};
