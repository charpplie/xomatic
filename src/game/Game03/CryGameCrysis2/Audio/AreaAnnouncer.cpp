/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------

	Plays Announcements based upon AreaBox triggers placed in levels

History:
- 25:02:2010		Created by Ben Parbury
*************************************************************************/

#include "StdAfx.h"
#include "Audio/AreaAnnouncer.h"

#include "Actor.h"
#include "GameRules.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "Utility/DesignerWarning.h"


#define ANNOUNCEMENT_NOT_PLAYING_TIME -1.0f

#if defined(USER_benp)
int static aa_debug = 1;
int static aa_peopleNeeded = 1;
#else
int static aa_debug = 0;
int static aa_peopleNeeded = 3;
#endif

int static aa_enabled = 1;

//---------------------------------------
CAreaAnnouncer::CAreaAnnouncer()
{
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);

	REGISTER_CVAR(aa_peopleNeeded, aa_peopleNeeded, 0, "Number of people needed to play area announcement");
	REGISTER_CVAR(aa_enabled, aa_enabled, 0, "Stops area announcements being played or updated");
	
#if !defined(_RELEASE)
	REGISTER_CVAR(aa_debug, aa_debug, 0, "Enable/Disables Area announcer debug messages");

	if (gEnv->pConsole)
	{
		gEnv->pConsole->AddCommand("aa_play", CmdPlay, VF_CHEAT, CVARHELP("play area announcement"));
		gEnv->pConsole->AddCommand("aa_reload", CmdReload, VF_CHEAT, CVARHELP("Init area announcement"));
	}
#endif
}

//---------------------------------------
CAreaAnnouncer::~CAreaAnnouncer()
{
	if(gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("aa_peopleNeeded");
		gEnv->pConsole->UnregisterVariable("aa_enabled");
#if !defined(_RELEASE)
		gEnv->pConsole->UnregisterVariable("aa_debug");
		gEnv->pConsole->RemoveCommand("aa_play");
		gEnv->pConsole->RemoveCommand("aa_reload");
#endif
	}

	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if(pGameRules)
	{
		pGameRules->UnRegisterRevivedListener(this);
	}
}

//---------------------------------------
void CAreaAnnouncer::Init()
{
	Reset();

	//Scan for areas
	IEntityClass* pTargetClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AreaBox");
	CRY_ASSERT_MESSAGE(pTargetClass, "Unable to find Target class AreaBox");

	if(pTargetClass)
	{
		IEntityIt* it = gEnv->pEntitySystem->GetEntityIterator();
		while ( !it->IsEnd() )
		{
			IEntity* pEntity = it->Next();
			if(pEntity->GetClass() == pTargetClass)
			{
				//check entityName
				IEntityAreaProxy *pArea = (IEntityAreaProxy*)pEntity->GetProxy(ENTITY_PROXY_AREA);
				if (pArea)
				{
					TAudioSignalID signal = g_pGame->GetGameAudio()->GetSignalID(pEntity->GetName(), false);
#if !defined(_RELEASE)
					if(aa_debug)
					{
						CryLogAlways("Found area '%s' with signal %d", pEntity->GetName(), signal);
					}
#endif
					if(signal != INVALID_AUDIOSIGNAL_ID)
					{
						SAnnouncementArea area;
						area.m_areaProxyId = pEntity->GetId();
						area.m_signal = signal;
#if !defined(_RELEASE)
						cry_strncpy(&area.m_name[0], pEntity->GetName(), SAnnouncementArea::k_maxNameLength);
#endif
						
						DesignerWarning(m_areaList.size() < k_maxAnnouncementAreas, "Too many AreaAnnouncer area boxes loaded");
						if(m_areaList.size() < k_maxAnnouncementAreas)
						{
							m_areaList.push_back(area);
						}
						else
						{
							CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Too many AreaAnnouncer area boxes loaded");
						}
					}
				}
			}
		}
		it->Release();
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterRevivedListener(this);
}

//---------------------------------------
void CAreaAnnouncer::Reset()
{
	m_areaList.clear();

	m_currentAnnouncement.Clear();

	m_multipleContactSignal = g_pGame->GetGameAudio()->GetSignalID("AreaAnnouncementMultipleContacts");
	m_andSignal = g_pGame->GetGameAudio()->GetSignalID("AreaAnnouncementAnd");
}

//---------------------------------------
void CAreaAnnouncer::Update(const float dt)
{
	if(!aa_enabled)
		return;

	m_currentAnnouncement.Update(dt);

#if !defined(_RELEASE)
	if(aa_debug)
	{
		const EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
		SAnnouncement announcement;
		bool result = BuildAnnouncement(clientId, &announcement);
		CryWatch("Build announcement %d", result);
	}
#endif

}

//---------------------------------------
void CAreaAnnouncer::OnLoadingComplete(ILevel *pLevel)
{
	Init();
}

//---------------------------------------
void CAreaAnnouncer::EntityRevived(EntityId entityId)
{
	if(!aa_enabled)
		return;

	const EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(entityId == clientId)
	{
		SAnnouncement announcement;
		if(BuildAnnouncement(clientId, &announcement))
		{
			PlayAnnouncement(&announcement);
		}
	}
}

//---------------------------------------
bool CAreaAnnouncer::BuildAnnouncement(const EntityId clientId, SAnnouncement* announcement)
{
	const int k_areaCount = m_areaList.size();
	if(k_areaCount)
	{
		IActorSystem* pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
		CActor* pClientActor = (CActor*)pActorSystem->GetActor(clientId);
		if(pClientActor)
		{
			int actorCount[k_maxAnnouncementAreas];
			memset(&actorCount, 0, sizeof(actorCount));

			IActorIteratorPtr pIter = pActorSystem->CreateActorIterator();

			while (CActor* pActor = (CActor*)pIter->Next())
			{
				if(!pClientActor->IsFriendlyEntity(pActor->GetEntityId()) && pActor->GetHealth() >= 0 && pActor->GetSpectatorMode() == CActor::eASM_None)
				{
					for(int i = 0; i < k_areaCount; i++)
					{
						IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_areaList[i].m_areaProxyId);
						if(pEntity)
						{
							IEntityAreaProxy *pArea = (IEntityAreaProxy*)pEntity->GetProxy(ENTITY_PROXY_AREA);
							if(pArea && pArea->CalcPointWithin(pActor->GetEntity()->GetWorldPos(), true))
							{
								actorCount[i]++;
								break;
							}
						}
					}
				}
			}

			GenerateAnnouncement(&actorCount[0], k_areaCount, announcement);

			return (announcement->m_parts >= 2);
		}
	}

	return false;
}

//---------------------------------------
void CAreaAnnouncer::GenerateAnnouncement(const int* actorCount, const int k_areaCount, SAnnouncement* announcement)
{
	int maxActorArea = -1;	//busiest area
	int maxActorCount = 0;

	int nextActorArea = -1;	//2nd busiest area
	int nextActorCount = 0;

	for(int i = 0; i < k_areaCount; i++)
	{

#if !defined(_RELEASE)
		if(aa_debug)
		{
			CryWatch("%s - %d", m_areaList[i].m_name, actorCount[i]);
		}
#endif

		if(actorCount[i] > maxActorCount)
		{
			nextActorArea = maxActorArea;	//just set to previous
			nextActorCount = maxActorCount;

			maxActorArea = i;
			maxActorCount = actorCount[i];
		}
	}

	CRY_ASSERT(maxActorCount >= nextActorCount);
	if(maxActorCount >= aa_peopleNeeded)
	{

#if !defined(_RELEASE)
		if(aa_debug)
		{
			CryWatch("Max - %s - %d", m_areaList[maxActorArea].m_name, actorCount[maxActorArea]);
		}
#endif

		announcement->AddParts(m_multipleContactSignal, m_areaList[maxActorArea].m_signal);
	}

	if(nextActorCount >= aa_peopleNeeded)
	{
#if !defined(_RELEASE)
		if(aa_debug)
		{
			CryWatch("Next - %s - %d", m_areaList[nextActorArea].m_name, actorCount[nextActorArea]);
		}
#endif

		announcement->AddParts(m_andSignal, m_areaList[nextActorArea].m_signal);
	}
}

//---------------------------------------
void CAreaAnnouncer::PlayAnnouncement(SAnnouncement *announcement)
{
	memcpy(&m_currentAnnouncement, announcement, sizeof(m_currentAnnouncement));
	m_currentAnnouncement.Play();
}

//---------------------------------------
CAreaAnnouncer::SAnnouncement::SAnnouncement()
{
	Clear();
}

//---------------------------------------
void CAreaAnnouncer::SAnnouncement::AddParts(TAudioSignalID prefix, TAudioSignalID area)
{
	if(m_parts + 2 <= k_maxParts)
	{
		AddPart(prefix);
		AddPart(area);
	}
}

//---------------------------------------
void CAreaAnnouncer::SAnnouncement::AddPart(TAudioSignalID signal)
{
	CRY_ASSERT(m_parts >= 0 && m_parts < k_maxParts);
	m_part[m_parts] = signal;
	m_parts++;
}

//---------------------------------------
void CAreaAnnouncer::SAnnouncement::Play()
{
	m_playedTime = 0.0f;
	
	float currentTime = 0.0f;
	for(int i = 0 ; i < m_parts; i++)
	{
		m_partTimes[i] = currentTime;
		currentTime += CAudioSignalPlayer::GetSignalLength(m_part[i]);
	}
}

//---------------------------------------
bool CAreaAnnouncer::SAnnouncement::IsPlaying()
{
	return m_playedTime != ANNOUNCEMENT_NOT_PLAYING_TIME;
}

//---------------------------------------
void CAreaAnnouncer::SAnnouncement::Update(const float dt)
{
	if(IsPlaying())	
	{
		float originalPlayTime = m_playedTime;
		m_playedTime += dt;
		for(int i = 0 ; i < m_parts; i++)
		{
			if(originalPlayTime <= m_partTimes[i] && m_playedTime > m_partTimes[i])
			{
				CAudioSignalPlayer::JustPlay(m_part[i]);
				if(i == m_parts - 1)	//finished playing
				{
					Clear();	//stop and reset
				}
				return;
			}
		}
	}
}

//---------------------------------------
void CAreaAnnouncer::SAnnouncement::Clear()
{
	m_parts = 0;
	memset(&m_part, 0, sizeof(m_part));
	memset(&m_partTimes, 0, sizeof(m_partTimes));
	m_playedTime = ANNOUNCEMENT_NOT_PLAYING_TIME;
}

#if !defined(_RELEASE)
//static-------------------------------------------------------
void CAreaAnnouncer::CmdPlay(IConsoleCmdArgs* pCmdArgs)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	CAreaAnnouncer* pAA = pGameRules->GetAreaAnnouncer();

	if(pAA->m_areaList.size() > 0)
	{
		int randA = cry_rand() % pAA->m_areaList.size();
		int randB = cry_rand() % pAA->m_areaList.size();

		pAA->m_currentAnnouncement.AddParts(pAA->m_multipleContactSignal, pAA->m_areaList[randA].m_signal);
		pAA->m_currentAnnouncement.AddParts(pAA->m_andSignal, pAA->m_areaList[randB].m_signal);

		pAA->m_currentAnnouncement.Play();

		CryLogAlways("Playing - Multiple Contacts at 'the %s' and 'the %s'", pAA->m_areaList[randA].m_name, pAA->m_areaList[randB].m_name);
	}
	else
	{
		CryLogAlways("Unable to play because there aren't any areas!");
	}
}

//static-------------------------------------------------------
void CAreaAnnouncer::CmdReload(IConsoleCmdArgs* pCmdArgs)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	CAreaAnnouncer* pAA = pGameRules->GetAreaAnnouncer();
	pAA->Init();
}
#endif
