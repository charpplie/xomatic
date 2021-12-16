#ifndef __PHANTOMPERK_H__
#define __PHANTOMPERK_H__

#include "IPerk.h"

class PhantomPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(PhantomPerk);

	PhantomPerk(const float* suitEnegyScale1, const float* suitEnegyScale2, const float* suitEnegyScale3);
	virtual ~PhantomPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);

protected:
	void SetPhantom(bool enable);
	void SetEntityPhantom(bool enable, IEntity* pEntity);
	const float* m_suitEnergyScale[eTierMax];
};

#endif
