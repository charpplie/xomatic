/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Perk icon data
-------------------------------------------------------------------------
History:
- 2:10:2009: Created by Tim Furnish (split out of Perk.cpp)

*************************************************************************/

#include "StdAfx.h"
#include "PerkIconData.h"
#include "PerkDbgDisplay.h"

CPerkIconData * CPerkIconData::s_localPlayerIconData = NULL;
EntityId CPerkIconData::s_localPlayerEntityId = 0;

void CPerkIconData::AddIcon(EPerks perk)
{
	for (int i = 0; i < k_numberOfPerkIcons; ++ i)
	{
		if (m_icons[i].m_perkID == ePerk_Null)
		{
			m_icons[i].m_perkID = perk;

			// This structure should already have been cleared, so there's no need to set these again... let's just sanity check 'em. [TF]
			CRY_ASSERT(m_icons[i].m_drainAmount == 0.f);
			CRY_ASSERT(m_icons[i].m_pulse == 0.f);
			CRY_ASSERT(m_icons[i].m_alpha == 0.f);
			CRY_ASSERT( (m_icons[i].m_flags & SSinglePerkIconInfo::eSSPIIF_Ticked) == 0 );
			CRY_ASSERT(m_icons[i].m_promptInfo.GetType() == kCITV_none);
			return;
		}
	}

	// OK, couldn't find an empty slot for the new icon. Find something which is vanishing and use that slot!
	SSinglePerkIconInfo * leastVisible = NULL;

	for (int i = 0; i < k_numberOfPerkIcons; ++ i)
	{
		if (m_icons[i].m_flags & SSinglePerkIconInfo::eSSPIIF_Vanishing)
		{
			if (leastVisible == NULL || m_icons[i].m_alpha < leastVisible->m_alpha)
			{
				leastVisible = & m_icons[i];
			}
		}
	}

#ifndef _RELEASE
	CPerk * perkInstance = CPerk::GetInstance();
	CRY_ASSERT_MESSAGE(leastVisible, string().Format("Too many active perks, can't add icon for perk %d! About to crash horribly!", perk));
	CryLogAlways ("[WARNING] Activating perk icon '%s', all perk icon slots in use, immediately clearing vanishing icon for perk icon '%s', visibility was %.2f!", perkInstance->GetPerkData(perk)->GetIDName(), perkInstance->GetPerkData(leastVisible->m_perkID)->GetIDName(), leastVisible->m_alpha);
#endif

	leastVisible->Clear();
	leastVisible->m_perkID = perk;
}

CPerkIconData::SSinglePerkIconInfo * CPerkIconData::Find(EPerks perk)
{
	for (int i = 0; i < k_numberOfPerkIcons; ++ i)
	{
		if (m_icons[i].m_perkID == perk && !(m_icons[i].m_flags & SSinglePerkIconInfo::eSSPIIF_Vanishing) )
		{
			return & m_icons[i];
		}
	}

	CRY_ASSERT_MESSAGE(0, string().Format("Perk %d not in icon set!", perk));
	return NULL;
}

void CPerkIconData::RemoveIcon(EPerks perk)
{
	Find(perk)->m_flags |= SSinglePerkIconInfo::eSSPIIF_Vanishing;
}

void CPerkIconData::SetIconDrainAmount(EPerks perk, float drainAmount, bool usable)
{
	SSinglePerkIconInfo * icon = Find(perk);

	if ( usable != ((icon->m_flags & SSinglePerkIconInfo::eSSPIIF_Usable)>0) )
	{
		if(usable)
		{
			icon->m_pulse = 1.0f;
			icon->m_flags |= SSinglePerkIconInfo::eSSPIIF_Usable;
		}
		else
		{
			icon->m_flags ^= SSinglePerkIconInfo::eSSPIIF_Usable;
		}
	}

	icon->m_drainAmount = drainAmount;
}

void CPerkIconData::Tick(float frameTime)
{
	CPerk * perkInstance = CPerk::GetInstance();

	for(int i = 0 ; i < k_numberOfPerkIcons; i++)
	{
		CPerkIconData::SSinglePerkIconInfo * iconInfo = & m_icons[i];
		if (iconInfo->m_perkID != ePerk_Null)
		{
			const SPerkData * perkData = perkInstance->GetPerkData(iconInfo->m_perkID);
			PerkDbgDisplay("Perk icon %d = %s (%s, alpha=%.2f)", i, perkData->GetIDName(), (iconInfo->m_flags & SSinglePerkIconInfo::eSSPIIF_Vanishing) ? "vanishing" : "alive", iconInfo->m_alpha);
			if(iconInfo->m_pulse > 0.0f)
			{
				iconInfo->m_pulse -= (frameTime / perkInstance->GetVars()->perk_icon_pulse_time);
			}

			if (iconInfo->m_flags & SSinglePerkIconInfo::eSSPIIF_Vanishing)
			{
				iconInfo->m_alpha -= frameTime;
				if (iconInfo->m_alpha <= 0.f)
				{
					iconInfo->Clear();
				}
			}
			else if ( iconInfo->m_flags & SSinglePerkIconInfo::eSSPIIF_Ticked )
			{
				iconInfo->m_alpha = min(iconInfo->m_alpha + frameTime, 1.f);
			}
			else
			{
				iconInfo->m_flags |= SSinglePerkIconInfo::eSSPIIF_Ticked;
			}
		}
	}
}

CControllerInputRenderInfo * CPerkIconData::GetIconInputPrompt(EPerks perk)
{
	return & Find(perk)->m_promptInfo;
}

void CPerkIconData::Reset()
{
	for (int i = 0; i < k_numberOfPerkIcons; ++ i)
	{
		m_icons[i].Clear();
	}
}

void CPerkIconData::SSinglePerkIconInfo::Clear()
{
	m_perkID = ePerk_Null;
	m_pulse = 0.f;
	m_promptInfo.Clear();
	m_drainAmount = 0.f;
	m_alpha = 0.f;

	// Flags
	m_flags = eSSPIIF_Usable;
}

//---------------------------------------------------------------------
CPerkIconData::CPerkIconData()
{
	assert (s_localPlayerIconData == NULL);
	s_localPlayerIconData = this;

	Reset();
}

CPerkIconData::~CPerkIconData()
{
	assert (s_localPlayerIconData == this);
	s_localPlayerIconData = NULL;
}

/*static*/ void CPerkIconData::SetLocalPlayerEntityId(EntityId who)
{
	if (who == 0)
	{
		CRY_ASSERT_MESSAGE (s_localPlayerEntityId != 0, "CPerkIconData already knows there's no local player!");
		s_localPlayerIconData->Reset();
	}
	else
	{
		CRY_ASSERT_MESSAGE (s_localPlayerEntityId == 0, string().Format("CPerkIconData thinks entity %d is local player, now overwriting this with entity %d", s_localPlayerEntityId, who));
	}

	s_localPlayerEntityId = who;
}

/*static*/ CPerkIconData * CPerkIconData::GetForEntity(EntityId who)
{
	assert (who);
	return (who == s_localPlayerEntityId) ? s_localPlayerIconData : NULL;
}
