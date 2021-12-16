#ifndef __TRACKERPERK_H__
#define __TRACKERPERK_H__

#include "IPerk.h"

struct IParticleEffect;

class TrackerPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(TrackerPerk);

	TrackerPerk(const char* particleEffect);
	virtual ~TrackerPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);

protected:
	void SetParticleLifetimeFromPlayerCount();

	IParticleEffect *m_effect;
};

#endif
