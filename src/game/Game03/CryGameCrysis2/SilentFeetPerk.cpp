#include "StdAfx.h"
#include "SilentFeetPerk.h"

#include "Player.h"
#include "Perk.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"


//Tiers
//One - No sound/Sonar Vision when running or walking
//Two - No sound/Sonar Vision when sprinting
//Three - No sound/Sonar Vision when jumping

SilentFeetPerk::SilentFeetPerk()
{
	m_muteFootsteps = false;
	m_muteJumping = false;
}

void SilentFeetPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_OverrideFootstepSoundScale:
		{
			float* pSoundScale = static_cast<float*>(data);
			const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
			(*pSoundScale) *= perkVars->perk_silentFeet_scaleFootstepSounds;

			CCCPOINT(Perk_SilentFeet_AdjustFootstep);
			break;
		}
	default:
		{
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}


const void* SilentFeetPerk::GetData(EPlayerPlugInData dataType)
{
	switch(dataType)
	{
	case EPD_MuteFootsteps:
		{
			m_muteFootsteps = IsTier(eTierTwo) || !m_ownerPlayer->IsSprinting();
			CCCPOINT_IF(m_muteFootsteps, Perk_SilentFeet_DisableSonarVision);
			return &m_muteFootsteps;
		}
	case EPD_MuteJumping:
		{
			m_muteJumping = IsTier(eTierThree);
			return &m_muteJumping;
		}
	}

	return NULL;
}
