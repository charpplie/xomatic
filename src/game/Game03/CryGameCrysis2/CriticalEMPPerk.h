#ifndef __CRITICALEMPPERK_H__
#define __CRITICALEMPPERK_H__

#include "IPerk.h"

class CriticalEMPPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(CriticalEMPPerk);

	CriticalEMPPerk(){ m_weaponName = "criticalemp"; }
	virtual ~CriticalEMPPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual void Update(const float dt);

	virtual void Explode(const CPerk::SPerkVars * perkVars);

protected:
	float m_timer;

	const char* m_weaponName;
};

#endif
