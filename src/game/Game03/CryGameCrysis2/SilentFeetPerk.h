#ifndef __SILENTFEETPERK_H__
#define __SILENTFEETPERK_H__

#include "IPerk.h"

class SilentFeetPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(SilentFeetPerk);

	SilentFeetPerk();
	virtual ~SilentFeetPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual const void* GetData(EPlayerPlugInData dataType);

protected:
	bool m_muteFootsteps;
	bool m_muteJumping;
};

#endif
