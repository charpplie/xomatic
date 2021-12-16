#ifndef __SATSTRIKEPERK_H__
#define __SATSTRIKEPERK_H__

#include "EMPStrikePerk.h"

#include "SShootHelper.h"
#include "SatelliteStrike.h"
#include "Player.h"
#include "PlayerProgression.h"

class SatStrikePerk:
	public EMPStrikePerk
{
public:
	SET_PLAYER_PLUGIN_NAME(SatStrikePerk);

	SatStrikePerk() { m_color = ColorF(1.0f, 0.0f, 0.0f, 0.25f); m_announcement = "SatStrike"; m_audioSignal = "UnlockSatStrike";}
	virtual ~SatStrikePerk() {}

protected:
	static const int k_numberOfStrikes = 3;

	virtual void FireStrike(Vec3 firePos)
	{
		Vec3 dir = Vec3(0.0f, 0.0f, -1.0f);
		for(int i = 0; i < k_numberOfStrikes; i++)
		{
			Vec3 currentStrikePos = firePos;
			CSatelliteStrike::StrikeTargetPosition(i, k_numberOfStrikes, currentStrikePos);
			SShootHelper::Shoot(m_ownerPlayer->GetEntityId(), "satstrike", "frag", currentStrikePos, dir, 200);
			if(m_ownerPlayer->IsClient())
			{
				CPlayerProgression::GetInstance()->Event(EPP_SatStrike);
			}
		}
	}
};

#endif