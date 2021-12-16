#ifndef __SMOKEBLASTPERK_H__
#define __SMOKEBLASTPERK_H__

#include "CriticalEMPPerk.h"

#include "Player.h"

class SmokeBlastPerk:
	public CriticalEMPPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(SmokeBlastPerk);

	SmokeBlastPerk(){ m_weaponName = "smokeblast";  }
	virtual ~SmokeBlastPerk() {}

protected:
	virtual void Explode(const CPerk::SPerkVars * perkVars)
	{
		if(!gEnv->pSystem->IsDedicated())
		{
			EntityId entityId = m_ownerPlayer->GetEntityId();
			SHUDEvent ignoreEntity(eHUDEvent_OnIgnoreEntity);
			ignoreEntity.ReserveData(2);
			ignoreEntity.AddData(static_cast<int>(entityId));
			ignoreEntity.AddData(perkVars->perk_smokeBlast_disableTagNamesSeconds);
			CAudioSignalPlayer::JustPlay("Perk_SmokeBlast", entityId);
		}
		CriticalEMPPerk::Explode(perkVars);
	}
};

#endif