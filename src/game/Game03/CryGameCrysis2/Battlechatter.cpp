/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description:
Plays sounds for nearby actors
**************************************************************************/

#include "StdAfx.h"
#include "Battlechatter.h"

#include "Player.h"
#include "GameRules.h"

#include "Audio/AudioSignalPlayer.h"
#include "Utility/CryWatch.h"

static AUTOENUM_BUILDNAMEARRAY(s_battlechatterName, BATTLECHATTER_LIST);

#define BATTLECHATTER_DATA_FILE "Scripts/Sounds/BattlechatterData.xml"

//how often it scans for local player (possible to change) and checks for possible speech for them
const static float k_nearestPlayerScantimeSeconds = 0.741f;
const static int k_maxInvalidSignals = 2;

#if !defined(_RELEASE)
static int bc_debug = 0;
static int bc_signals = 0;
static int bc_signalsDetail = -1;
#endif
static float bc_minTimeBetweenBattlechatter = 4.5f;
static float bc_maxTimeCanDitto = 1.5f;

#if !defined(_RELEASE)
struct SBattlehchatterAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const { return BC_Last; };
	virtual const char* GetValue( int nIndex ) const { return s_battlechatterName[nIndex]; };
};

static SBattlehchatterAutoComplete s_battlechatterAutoComplete;
#endif
//-------------------------------------------------------
CBattlechatter::CBattlechatter()
{
	m_clientPlayer = NULL;
	m_lastTimePlayed = 0.0f;
	m_lastPlayed = BC_Null;
	m_nearestPlayerId = 0;
	m_nearestPlayerTimer = k_nearestPlayerScantimeSeconds;

	m_voice.clear();
	m_nextActorVoiceIndex = 0;

	memset(&m_range[0], 0, sizeof(m_range));
	m_maxRange = 0;


#if !defined(_RELEASE)
	REGISTER_CVAR(bc_debug, 0, 0, "Enable/Disables battlechatter debug messages");
	REGISTER_CVAR(bc_signals, 0, 0, "Enables watch for nth voice showing battlechatter signal counts");
	REGISTER_CVAR(bc_signalsDetail, -1, 0, "Enable watch battlechatter signal names and signalIds - set to larger than signal max to show all");
	REGISTER_CVAR(bc_minTimeBetweenBattlechatter, bc_minTimeBetweenBattlechatter, 0, "time between battlechatter per person!");
	REGISTER_CVAR(bc_maxTimeCanDitto, bc_maxTimeCanDitto, 0, "max time for a play to say ditto");

	if (gEnv->pConsole)
	{
		gEnv->pConsole->AddCommand("bc_play", CmdPlay, VF_CHEAT, CVARHELP("play battlechatter and nearby player"));
		gEnv->pConsole->RegisterAutoComplete("bc_play", & s_battlechatterAutoComplete);
	}
#endif

	Init();
}

//-------------------------------------------------------
CBattlechatter::~CBattlechatter()
{
	CGameRules* pGameRules = g_pGame->GetGameRules();
	if(pGameRules)
	{
		pGameRules->UnRegisterKillListener(this);
	}
}

//-------------------------------------------------------
void CBattlechatter::Init()
{

	InitVoices();
	
	InitData();

	CGameRules* pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterKillListener(this);
}

//-------------------------------------------------------
void CBattlechatter::InitVoices()
{
	int voiceCount = 0;
	SVoice voice;
	while(voice.Init(voiceCount))
	{
		CryLog("[Battlechatter] Successfully loaded voice %d", voiceCount + 1);
		m_voice.push_back(voice);
		voice.clear();
		m_nextActorVoiceList.push_back(voiceCount);
		voiceCount++;
	}

	//mark order you hear people in random
	for(int i=0; i < voiceCount; i++)
	{
		int idx = cry_rand() % voiceCount;
		if(idx != i)
		{
			std::swap(m_nextActorVoiceList[i],m_nextActorVoiceList[idx]);
		}
	}
}

//-------------------------------------------------------
void CBattlechatter::InitData()
{
	XmlNodeRef xmlData = GetISystem()->LoadXmlFile(BATTLECHATTER_DATA_FILE);

	if(xmlData)
	{
		for(int i = 0; i < BC_Last; i++)
		{
			XmlNodeRef chatter = xmlData->getChild(i);
			CRY_ASSERT_MESSAGE(strcmpi(chatter->getTag(), s_battlechatterName[i]) == 0, ("Expected %s, not %s", s_battlechatterName[i], chatter->getTag()));
			chatter->getAttr("range", m_range[i]);
			m_maxRange = max(m_maxRange, m_range[i]);
		}
	}
}

//-------------------------------------------------------
CBattlechatter::SVoice::SVoice()
{
	memset(m_chatter, 0, sizeof(m_chatter));
}

//-------------------------------------------------------
void CBattlechatter::SVoice::clear()
{
	for(int i = 0; i < BC_Last; i++)
	{
		m_chatter[i].m_signal.clear();
	}
}


//-------------------------------------------------------
bool CBattlechatter::SVoice::Init(int voiceIndex)
{
	int validChatterCount = 0;
	for(int chatterIndex = 0; chatterIndex < BC_Last; chatterIndex++)
	{
		SVoice::SSignalList signalList;
		if(signalList.Init(voiceIndex, chatterIndex))
		{
			m_chatter[chatterIndex] = signalList;
			validChatterCount++;
		}
	}

	const int k_minChatterCount = (BC_Last/2);
	const bool validVoice = validChatterCount > k_minChatterCount;
#if !defined(_RELEASE)
	if(validVoice == false && validChatterCount > 0)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[Battlechatter] Tried to load voice %d but only found %d chatter types (%d needed)", voiceIndex + 1, validChatterCount, k_minChatterCount);
	}
#endif
	return validVoice;
}


//-------------------------------------------------------
CBattlechatter::SVoice::SSignalList::SSignalList()
{
	m_signal.clear();
}

//-------------------------------------------------------
bool CBattlechatter::SVoice::SSignalList::Init(int voiceIndex, int chatterIndex)
{
	int invalidSignals = 0;
	for(int signalIndex = 0; ; signalIndex++)
	{
		string signalName = "";
		signalName.Format("%s_TM%d_%d", s_battlechatterName[chatterIndex], voiceIndex + 1, signalIndex + 1);

		TAudioSignalID signal = g_pGame->GetGameAudio()->GetSignalID(signalName, false);	//doesn't warn (as will always find invalid signals)
		if(signal != INVALID_AUDIOSIGNAL_ID)
		{
			m_signal.push_back(signal);
		}
		else
		{
			invalidSignals++;
			if(invalidSignals > k_maxInvalidSignals)
			{
				return m_signal.size() > 0;
			}
		}
	}
	return m_signal.size() > 0;
}

//-------------------------------------------------------
void CBattlechatter::SetLocalPlayer(CPlayer* pPlayer)
{
	m_clientPlayer = pPlayer;
}

//-------------------------------------------------------
void CBattlechatter::Update(const float dt)
{
	if(m_clientPlayer)
		UpdateNearestPlayer(dt);

#if !defined(_RELEASE)
	if(bc_debug > 0)
	{
		if(bc_debug == 1)
		{
			const int voiceCount = m_voice.size();
			if(voiceCount > 0)
			{
				CryWatch("Battlechatter Actor Voice Test Assignment");
				IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
				while (CActor* pActor = (CActor*)pIter->Next())
				{
					SVoiceInfo *pInfo = GetVoiceInfo(pActor->GetEntityId());
					CryWatch("EntityId %d - Voice %d Last spoke %.2f", pActor->GetEntityId(), pInfo->m_voiceIndex, pInfo->m_lastTimePlayed);
				}
			}
			else
			{
				CryWatch("No voices found!");
			}

		}
		else if(bc_debug == 2)
		{
			CryWatch("Battlechatter ShouldPlay");
			CryWatch("Client %p", m_clientPlayer);
			CryWatch("Nearest Player %d (%.2f)", m_nearestPlayerId, m_nearestPlayerTimer);
			CryWatch("Last played %d", m_lastPlayed);
			CryWatch("Max Range %.2f", m_maxRange);
			if(m_clientPlayer)
				CryWatch("ShouldClientHear %d", ShouldClientHear());

			IActorIteratorPtr actorIt = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
			IActor *actor;

			while (actor=actorIt->Next())
			{
				CryWatch("	%s - ShouldPlayForActor %d (%.2f)", actor->GetEntity()->GetName(), ShouldPlayForActor(actor->GetEntityId(), m_maxRange), DistanceToActor(actor->GetEntityId()));
			}
		}
		else if(bc_debug == 3)
		{
			CryWatch("Ranges - max (%.2f)", m_maxRange);
			for(int i = 0; i < BC_Last; i++)
			{
				CryWatch("\t%d - %s is %.2f", i, s_battlechatterName[i], m_range[i]);
				if(i == bc_signalsDetail)
				{
					gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags( e_Mode3D | e_AlphaBlended | e_DrawInFrontOff | e_FillModeSolid | e_CullModeNone );
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawCylinder(m_clientPlayer->GetEntity()->GetWorldPos(), Vec3(0, 0, 1), m_range[i]/2.0f, 1.0f, ColorB(0, 100, 255, 55));
				}
			}
		}
	}

	if(bc_signals > 0)
	{
		int voiceIndex = bc_signals - 1;
		const int voiceCount = m_voice.size();
		CryWatch("Voice %d (of %d)", voiceIndex + 1, voiceCount);
		if(voiceIndex >= 0 && voiceIndex < m_voice.size())
		{
			for(int chatterIndex = 0; chatterIndex < BC_Last; chatterIndex++)
			{
				const int signalCount = m_voice[voiceIndex].m_chatter[chatterIndex].m_signal.size();
				CryWatch("\t%d - %s - %d", chatterIndex, s_battlechatterName[chatterIndex], signalCount);
				if(chatterIndex == bc_signalsDetail || bc_signalsDetail > BC_Last)
				{
					for(int signalIndex = 0; signalIndex < signalCount + k_maxInvalidSignals; signalIndex++)
					{
						string signalName = "";
						signalName.Format("%s_TM%d_%d", s_battlechatterName[chatterIndex], voiceIndex + 1, signalIndex + 1);

						TAudioSignalID signal = g_pGame->GetGameAudio()->GetSignalID(signalName, false);
						CryWatch("\t\t%s - %d", signalName.c_str(), signal);
					}
				}
			}
		}
	}
#endif
}

//-------------------------------------------------------
void CBattlechatter::Event(EBattlechatter requestedChatter, EntityId actorId)
{
	CRY_ASSERT_MESSAGE(requestedChatter > BC_Null && requestedChatter < BC_Last, "invalid chatter index");

	float currentTime = gEnv->pTimer->GetCurrTime();
	
	EBattlechatter chatter = requestedChatter;
	if(m_lastPlayed == chatter && CanPlayDitto(currentTime))
	{
		chatter = BC_Ditto;
	}

	if(m_clientPlayer && m_lastPlayed != chatter && ShouldClientHear() && ShouldPlayForActor(actorId, m_range[chatter]))
	{
		SVoiceInfo* pInfo = GetVoiceInfo(actorId);
		if(pInfo && !PlayedRecently(pInfo, currentTime))
		{
			Play(pInfo, chatter, actorId, currentTime);

			m_lastTimePlayed = currentTime;
			m_lastPlayed = requestedChatter;	//stores requested chatter (not Dittos, so you can have multiple Dittos)
		}
	}
}

//-------------------------------------------------------
CBattlechatter::SVoiceInfo* CBattlechatter::GetVoiceInfo(EntityId actorId)
{
	const int voiceCount = m_voice.size();
	if(voiceCount > 0)
	{
		ActorVoiceMap::iterator it = m_actorVoice.find(actorId);

		if(it == m_actorVoice.end())	//add new voice if we need one
		{
			int actorVoiceIndex = m_nextActorVoiceList[m_nextActorVoiceIndex];
			SVoiceInfo voiceInfo(actorVoiceIndex);

			it = m_actorVoice.insert(ActorVoiceMap::value_type(actorId, voiceInfo)).first;


			m_nextActorVoiceIndex = (m_nextActorVoiceIndex + 1) % voiceCount;
		}

		return &it->second;
	}
	return NULL;
}

//-------------------------------------------------------
void CBattlechatter::Play(SVoiceInfo* pInfo, EBattlechatter chatter, EntityId actorId, float currentTime)
{
	const SVoice::SSignalList &list = m_voice[pInfo->m_voiceIndex].m_chatter[chatter];
	int validSignalCount = list.m_signal.size();
	if(validSignalCount > 0)
	{
		int index = cry_rand() % validSignalCount;
		CAudioSignalPlayer::JustPlay(list.m_signal[index], actorId);
		pInfo->m_lastTimePlayed = currentTime;
	}
}

//-------------------------------------------------------
void CBattlechatter::OnEntityKilled(const HitInfo &hitInfo)
{
	if(m_clientPlayer && hitInfo.shooterId && hitInfo.targetId)
	{
		if(m_clientPlayer->IsFriendlyEntity(hitInfo.shooterId) && !m_clientPlayer->IsFriendlyEntity(hitInfo.targetId))
		{
			Event(BC_EnemyKilled, hitInfo.shooterId);
		}
	}
}

//-------------------------------------------------------
bool CBattlechatter::CanPlayDitto(const float currentTime) const
{
	return (m_lastTimePlayed + bc_maxTimeCanDitto) > currentTime;
}

//-------------------------------------------------------
bool CBattlechatter::PlayedRecently(const SVoiceInfo* pInfo, const float currentTime) const
{
	return (pInfo->m_lastTimePlayed + bc_minTimeBetweenBattlechatter) > currentTime;
}

//-------------------------------------------------------
bool CBattlechatter::ShouldClientHear() const
{
	//have health and not spectating
	return m_clientPlayer->GetHealth() > 0 && m_clientPlayer->GetSpectatorMode() == CActor::eASM_None;
}

//-------------------------------------------------------
bool CBattlechatter::ShouldPlayForActor(EntityId actorId, float range) const
{
	//only chatter for friendlies (and not self)
	if(actorId != m_clientPlayer->GetEntityId() && m_clientPlayer->IsFriendlyEntity(actorId))
	{
		if(DistanceToActor(actorId) < range)
		{
			//alive and on a team
			if(g_pGame->GetGameRules()->IsPlayerActivelyPlaying(actorId, true))
			{
				return true;
			}
		}
	}

	return false;
}

//-------------------------------------------------------
float CBattlechatter::DistanceToActor(EntityId actorId) const
{
	const static float k_maxDistanceToActor = 1024.0f;

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
	if(pActor)
	{
		Vec3 playerPos = m_clientPlayer->GetEntity()->GetWorldPos();
		return playerPos.GetDistance(pActor->GetEntity()->GetWorldPos());
	}

	return k_maxDistanceToActor;
}

//-------------------------------------------------------
void CBattlechatter::UpdateNearestPlayer(const float dt)
{
	m_nearestPlayerTimer -= dt;
	if(m_nearestPlayerTimer < 0.0f)
	{
		m_nearestPlayerTimer = k_nearestPlayerScantimeSeconds;
		SetNearestPlayer(m_nearestPlayerId, GetNearestHearbleActor());
		UpdateNearestPlayerBattlechatter();
	}
}

//-------------------------------------------------------
void CBattlechatter::UpdateNearestPlayerBattlechatter()
{
	if(m_nearestPlayerId)
	{
		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_nearestPlayerId);
		if(pActor && pActor->IsPlayer())
		{
			CPlayer* pNearPlayer = static_cast<CPlayer*>(pActor);

			IEntity* pNearEntity = pNearPlayer->GetEntity();
			IEntity* pClientEntity = m_clientPlayer->GetEntity();
			
			float dot = pClientEntity->GetForwardDir().Dot(pNearEntity->GetForwardDir());
			//if client and nearest player are facing in similar direction and near player is moving forwards
			if(dot > 0.9f && GetForwardAmount(pNearPlayer) > 0.0f)
			{
				Event(BC_MovingUp, m_nearestPlayerId);
			}

			//using whoever your nearest friendly is targetting
			EntityId targettingId = pNearPlayer->GetCurrentTargetEntityId();
			if(targettingId)
			{
				IActor* pTargetActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targettingId);
				if(pTargetActor && !m_clientPlayer->IsFriendlyEntity(targettingId))
				{
					Vec3 dir = pTargetActor->GetEntity()->GetWorldPos() - pNearEntity->GetWorldPos();
					float xy = (fabsf(dir.x) + fabsf(dir.y)) / 2.0f;
					float z = fabsf(dir.z);

					EBattlechatter visual = BC_Visual;
					//above or below 45 degs
					if(z > xy)
					{
						visual = dir.z > 0 ? BC_VisualHigh : BC_VisualLow;
					}
					Event(visual, m_nearestPlayerId);
				}
			}
		}
	}
}

//-------------------------------------------------------
float CBattlechatter::GetForwardAmount(CPlayer* pPlayer)
{
	IPhysicalEntity* pPhysicalEntity = pPlayer->GetEntity()->GetPhysics();
	if (pPhysicalEntity)
	{
		pe_status_living status;
		pPhysicalEntity->GetStatus(&status);
		return status.vel.Dot(pPlayer->GetEntity()->GetForwardDir());
	}

	return 0.0f;
}

//-------------------------------------------------------
void CBattlechatter::SetNearestPlayer(EntityId prevId, EntityId newId)
{
	CRY_ASSERT(prevId == m_nearestPlayerId);
	m_nearestPlayerId = newId;
}

//-------------------------------------------------------
EntityId CBattlechatter::GetNearestHearbleActor()
{
	EntityId closestActorId = 0;
	float closestDistance = 1024.0f;
	IActorIteratorPtr actorIt = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	IActor *actor;

	while (actor=actorIt->Next())
	{
		EntityId actorId = actor->GetEntityId();
		if(ShouldPlayForActor(actorId, m_maxRange))
		{
			float actorDistance = DistanceToActor(actorId);
			if(actorDistance < closestDistance)
			{
				closestActorId = actorId;
				closestDistance = actorDistance;
			}
		}
	}

	return closestActorId;
}

#if !defined(_RELEASE)
//static-------------------------------------------------------
void CBattlechatter::CmdPlay(IConsoleCmdArgs* pCmdArgs)
{
	float minDistance = 10000.0f;
	CPlayer* clientPlayer = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	EntityId closestActorId = clientPlayer->GetEntityId();
	CActor *pClosestActor = clientPlayer;

	Vec3 clientPos = clientPlayer->GetEntity()->GetWorldPos();

	IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	while (CActor* pActor = (CActor*)pIter->Next())
	{
		if(pActor->GetEntityId() != clientPlayer->GetEntityId())
		{
			float distance = clientPos.GetDistance(pActor->GetEntity()->GetWorldPos());
			if(distance < minDistance)
			{
				minDistance = distance;
				closestActorId = pActor->GetEntityId();
				pClosestActor = pActor;
			}
		}
	}

	EBattlechatter chatter = BC_Reloading;
	if(pCmdArgs->GetArgCount() > 1)
	{
		const char* chatterName = pCmdArgs->GetArg(1);
		for(int i = 0; i < BC_Last; i++)
		{
			if(strcmpi(s_battlechatterName[i], chatterName) == 0)
			{
				chatter = (EBattlechatter) i;
			}
		}
	}

	SVoiceInfo* pInfo = g_pGame->GetGameRules()->GetBattlechatter()->GetVoiceInfo(closestActorId);
	if(pInfo)
	{
		CryLogAlways("bc_play - Entity %s at %.0fm away with signal %s", pClosestActor->GetEntity()->GetName(), minDistance, s_battlechatterName[chatter]);	
		const float currentTime = gEnv->pTimer->GetCurrTime();
		g_pGame->GetGameRules()->GetBattlechatter()->Play(pInfo, chatter, closestActorId, currentTime);
	}
	else
	{
		CryLogAlways("bc_play - Unable to find voice info - No voices loaded");
	}
}
#endif
