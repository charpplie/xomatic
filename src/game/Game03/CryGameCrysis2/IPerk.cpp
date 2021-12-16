#include "StdAfx.h"
#include "IPerk.h"
#include "Player.h"

IPerk::IPerk()
{
	m_perkId = ePerk_Null;
	if(gEnv->bMultiplayer)
	{
		SetTier(eTierTwo);	//defaulting to Tier Two for MS3
	}
	else
	{
		SetTier(eTierThree);	//defaulting to Tier Three for SP so doesn't change what they have currently
	}
	
}

void IPerk::SetPerkID(EPerks perkId)
{
	m_perkId = perkId;
	SetRequiredSuitMode(CPerk::GetInstance()->GetPerkRequiredSuitMode(perkId));
}

void IPerk::SetTier(EPerkTier tier)
{
	CRY_ASSERT_MESSAGE(tier > eTierInvalid && tier < eTierMax, "Invalid tier set");
	m_tier = tier;
	if(m_ownerPlayer)
	{
		CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_SETTINGS_CLIENT);
	}
	InformActiveHasChanged();
}

bool IPerk::IsTier(EPerkTier tier) const
{
	CRY_ASSERT_MESSAGE(tier > eTierInvalid && tier < eTierMax, "Invalid tier check");
	return m_tier >= tier;
}

#if PLAYER_PLUGIN_DEBUGGING
const char * IPerk::DbgGetPerkName()
{
	return CPerk::GetInstance()->GetPerkData(m_perkId)->GetIDName();
}

void IPerk::Leave()
{
	PlayerPluginAssert(! m_ownerPlayer->IsPerkSet(m_perkId), "Class being left but perk is currently set!");
	CPlayerPlugin::Leave();
}

void IPerk::Enter()
{
	PlayerPluginAssert(m_ownerPlayer->IsPerkSet(m_perkId), "Class being entered but perk is currently not set!");
	CPlayerPlugin::Enter();
}
#endif
