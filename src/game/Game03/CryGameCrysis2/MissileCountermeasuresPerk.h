#ifndef __MISSILECOUNTERMEASURESPERK_H__
#define __MISSILECOUNTERMEASURESPERK_H__

#include "IPerk.h"

class MissileCountermeasuresPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(MissileCountermeasuresPerk);

	MissileCountermeasuresPerk();
	virtual ~MissileCountermeasuresPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual void Update(const float dt);
	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);

protected:
	void ActivateCountermeasures();
	float m_timer;
	uint8 m_numTimesFired;
	bool m_everRead;
};

#endif
