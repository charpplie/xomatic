/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Implements various IGameStatistics callbacks and listens to
				 various game subsystems, forwarding necessary events to stats
				 recording system.

	-------------------------------------------------------------------------
	History:
	- 02:11:2009  : Created by Mark Tully

*************************************************************************/

#include "StdAfx.h"
#include "StatsRecordingMgr.h"
#include "IGameStatistics.h"
#include "StatHelpers.h"
#include "GameRules.h"
#include "Weapon.h"
#include "Player.h"
#include "XMLStatsSerializer.h"
#include "ITelemetryCollector.h"

static const char	*k_stats_suitMode_any="any";
static const char	*k_stats_suitMode_tactical="tactical";
static const char	*k_stats_suitMode_infiltration="infiltration";
static const char	*k_stats_suitMode_combat="combat";
static const char	*k_stats_suitMode_agility="agility";

CStatsRecordingMgr::CStatsRecordingMgr() :
	m_sessionTracker(NULL),
	m_roundTracker(NULL),
	m_hostMigrationEventOccured(false)
{
	gEnv->pNetwork->AddEventListener(this, "CStatsRecordingMgr");

	m_gameStats=g_pGame->GetIGameFramework()->GetIGameStatistics();
	CRY_ASSERT_MESSAGE(m_gameStats,"CStatsRecordingMgr instantiated without IGameStatistics being instantiated first, initialise IGameStatistics first - fatal");

	m_gameStats->SetStatisticsCallback(this);

	//Diesel cut
	///m_serializer=new CXMLStatsSerializer(g_pGame->GetIGameFramework()->GetIGameStatistics(), this);
	assert(m_serializer);

	//m_statsDirectory.Format(".\\%s/StatsLogs", gEnv->pCryPak->GetAlias("%USER%"));

	string	tmpStr = "%USER%/StatsLogs";
	char path[ICryPak::g_nMaxPath];
	path[sizeof(path) - 1] = 0;
	gEnv->pCryPak->AdjustFileName(tmpStr, path, ICryPak::FLAGS_PATH_REAL | ICryPak::FLAGS_FOR_WRITING);

	m_statsDirectory=path;

	gEnv->pCryPak->MakeDir(m_statsDirectory.c_str());


//	m_profileStatsBinder.BindCallback(functor(*this, &CMissionStatistics::OnStatsReceived));
//	m_profileStatsBinder.BindErrback(functor(*this, &CMissionStatistics::OnStatsError));

	IGameStatistics* pGS = g_pGame->GetIGameFramework()->GetIGameStatistics();
	pGS->RegisterSerializer(m_serializer);

	SGameStatDesc gameSpecificEvents[eGSE_Num - eSE_Num] = {
		GAME_STAT_DESC(eGSE_Melee,				"melee"				),
		GAME_STAT_DESC(eGSE_MeleeHit,			"melee_hit"			),
		GAME_STAT_DESC(eGSE_Firemode,			"firemode"			),
		GAME_STAT_DESC(eGSE_NanoSuitMode, "nanosuit_mode" ),
		GAME_STAT_DESC(eGSE_NanoSuitActivate, "nanosuit_activate"),
		GAME_STAT_DESC(eGSE_NanoSuitEnergy,"nanosuit_energy" ),
		GAME_STAT_DESC(eGSE_HitReactionAnim, "hitReaction_anim" ),
		GAME_STAT_DESC(eGSE_DeathReactionAnim, "deathReaction_anim" ),
	};
	m_gameStats->RegisterGameEvents(gameSpecificEvents, eGSE_Num - eSE_Num);

	SGameStatDesc gameSpecificStates[eGSS_Num - eSS_Num] = {
		GAME_STAT_DESC(eGSS_MapPath,			"map_path"			),
		GAME_STAT_DESC(eGSS_Perks,				"perks"					),
		GAME_STAT_DESC(eGSS_Attachments,	"attachments"		),
		GAME_STAT_DESC(eGSS_TeamName,			"name"					),
		GAME_STAT_DESC(eGSS_RoundBegin,		"begin"					),
		GAME_STAT_DESC(eGSS_RoundEnd,			"end"						),
		GAME_STAT_DESC(eGSS_Config,				"config"				),
	};
	m_gameStats->RegisterGameStates(gameSpecificStates, eGSS_Num - eSS_Num);

	SGameScopeDesc gameScopes[eGSC_Num] = {
		GAME_SCOPE_DESC(eGSC_Session,	"stats_session",	"logged_session_stats"	),
		GAME_SCOPE_DESC(eGSC_Round,		"round",			"logged_round_stats"	),
	};
	m_gameStats->RegisterGameScopes(gameScopes, eGSC_Num);


	SGameElementDesc gameElements[eGSEL_Num] = {
		GAME_ELEM_DESC(eGSEL_Team,		eGNLT_TeamID,		"team",					"logged_team_stats"		),
		GAME_ELEM_DESC(eGSEL_Player,	eGNLT_ChannelID,	"player",				"logged_stats"			),
		GAME_ELEM_DESC(eGSEL_AIActor,	eNLT_EntityID,		"ai_actor",				"logged_stats"			),
		GAME_ELEM_DESC(eGSEL_Entity,	eNLT_EntityID,		"entity",				"logged_stats"			),
	};
	m_gameStats->RegisterGameElements(gameElements, eGSEL_Num);

	SetNewSessionId();

	// default to always recording everything (can be overridden from configs)
	for(int i=0; i<eSE_Num; ++i)
	{
		m_eventConfigurations[i] = eSERT_Always;
	}
}

CStatsRecordingMgr::~CStatsRecordingMgr()
{
	gEnv->pNetwork->RemoveEventListener(this);

	m_gameStats->SetStatisticsCallback(NULL);
	delete m_serializer;
}

bool CStatsRecordingMgr::IsTrackingEnabled()
{
	ITelemetryCollector		*tc=static_cast<CGame*>(gEnv->pGame)->GetITelemetryCollector();
	bool					enabled=tc && tc->ShouldSubmitTelemetry();
	return ((enabled && gEnv->bMultiplayer && gEnv->bServer && !m_hostMigrationEventOccured && g_pGameCVars->g_telemetry_gameplay_enabled) ||
			(!gEnv->bMultiplayer && g_pGameCVars->g_telemetryEnabledSP != 0));
}

void CStatsRecordingMgr::BeginSession()
{
	if (IsTrackingEnabled())
	{
		if (CGameRules *pGameRules = g_pGame->GetGameRules())
			pGameRules->AddHitListener(this);

		m_gameStats->PushGameScope(eGSC_Session);
		CRY_ASSERT(m_sessionTracker);

		// record the initial configuration
		if(m_sessionTracker)
		{
			m_sessionTracker->StateValue(eGSS_Config, g_pGameCVars->g_telemetryConfig);
		}
	}
	
	SetNewSessionId();
}

void CStatsRecordingMgr::EndSession()
{
	if (m_sessionTracker)
	{
		EndRound();
		m_gameStats->PopGameScope(eGSC_Session);
		assert(m_sessionTracker==NULL);
	}

	if (CGameRules *pGameRules = g_pGame->GetGameRules())
		pGameRules->RemoveHitListener(this);
	
	SetNewSessionId();
}

void CStatsRecordingMgr::BeginRound()
{
	if (IsTrackingEnabled())
	{
		CRY_ASSERT_MESSAGE(m_sessionTracker,"BeginRound() called when no session is open");
		if (!m_sessionTracker)
		{
			BeginSession();
		}
		CRY_ASSERT_MESSAGE(m_gameStats->GetScopeID()==eGSC_Session,"stats error, session scope not active as aspected");
		m_gameStats->PushGameScope(eGSC_Round);
		assert(m_roundTracker);		// tracker will have been assigned and setup by callback
									// the reason for doing it in the callback rather than here is to allow the push to work from lua
	}
}

void CStatsRecordingMgr::StateRoundWinner(
	int			inWinningTeam)
{
	if (m_roundTracker)
	{
		m_roundTracker->StateValue(eSS_Winner,inWinningTeam);
	}
}

void CStatsRecordingMgr::EndRound()
{
	if (m_roundTracker)
	{
		StopTrackingAllPlayerStats();		// players cache their IStatsTracker which are about to be popped, stop tracking
		m_roundTracker->StateValue(eGSS_RoundEnd, gEnv->pTimer->GetFrameStartTime().GetMilliSecondsAsInt64());
		m_gameStats->PopGameScope(eGSC_Round);
		m_roundTracker=NULL;
	}
}

void CStatsRecordingMgr::AddTeam(
	int			inTeamId,
	string		inTeamName)
{
	if (IsTrackingEnabled())
	{
		IStatsTracker* track = m_gameStats->AddGameElement(SNodeLocator(eGSEL_Team, eGSC_Session, eGNLT_TeamID, inTeamId));
		CRY_ASSERT(track);
		if(track)
		{
			track->StateValue(eGSS_TeamName, inTeamName.c_str());		// FIXME SStatAnyValue is missing constructor for string
		}
	}
}
 								
// state the basic player values needed once a tracker is added for a player
void CStatsRecordingMgr::StateCorePlayerStats(
	IActor			*inPlayerActor)
{
	// when using modules, this could actually be moved to the OnPlayerRevived() handler in the stats module. but when using lua we still need it to run so call it here
	// if the stats tracker has been created for the player
	if ( IStatsTracker *tracker = GetStatsTracker(inPlayerActor) )
	{
		EntityId		eid=inPlayerActor->GetEntityId();

		tracker->StateValue( eSS_PlayerName,	inPlayerActor->GetEntity()->GetName());
		tracker->StateValue( eSS_ProfileId,		CStatHelpers::GetProfileId(inPlayerActor->GetChannelId()) );
		tracker->StateValue( eSS_EntityId,		static_cast<int>(eid) );
		int teamId = g_pGame->GetGameRules()->GetTeam(eid);
		if(!gEnv->bMultiplayer)
		{
			// todo: save species here as well (and possibly entity class)
			//	to differentiate between blackops / marines / aliens...
			teamId = inPlayerActor->IsPlayer() ? 1 : 0;
		}
		tracker->StateValue( eSS_Team,			teamId );

		//tracker->Event( eSE_Revive );

		StateActorWeapons(inPlayerActor);
		StateActorPerks(inPlayerActor);
	}
}

// saves the current set of perks for an actor to the stats tracker
void CStatsRecordingMgr::StateActorPerks(
	IActor				*inActor)
{
	// this is also called in SP for AI
	if (inActor->IsPlayer())
	{
		if ( IStatsTracker *tracker = GetStatsTracker(inActor) )
		{
			CPlayer		*player=static_cast<CPlayer*>(inActor);
			CPerk		*pmgr=CPerk::GetInstance();
			XmlNodeRef	perksNode=0;

			for(int i = 0; i < ePerk_Last; i++)
			{
				if (player->IsPerkSet(EPerks(i)))
				{
					const SPerkData	*perk=pmgr->GetPerkData(i);
					const char		*perkName=perk->GetIDName();
					EPerkCategory	cat=perk->m_slot;
					const char		*suitModeRestriction=k_stats_suitMode_any;

					switch (cat)
					{
						case ePerkSlot_Agility:
							suitModeRestriction=k_stats_suitMode_agility;
							break;

						case ePerkSlot_Combat:
							suitModeRestriction=k_stats_suitMode_combat;
							break;

						case ePerkSlot_Infiltration:
							suitModeRestriction=k_stats_suitMode_infiltration;
							break;

						case ePerkSlot_Tactical:
							suitModeRestriction=k_stats_suitMode_tactical;
							break;

						case ePerkSlot_Reward:
							// don't output this one, it's time limited
							continue;
							break;

						case ePerkSlot_Any:
							suitModeRestriction=k_stats_suitMode_any;
							break;

						default:
							CryLog("Unknown perk category encountered when outputting perk stats");
							break;
					}

					// add weapon
					if (!perksNode)
					{
						perksNode=m_gameStats->CreateStatXMLNode();
					}

					XmlNodeRef		childNode=GetISystem()->CreateXmlNode("perk");
					childNode->setAttr( "name", perkName);
					childNode->setAttr( "required_suit_mode", suitModeRestriction);
					perksNode->addChild(childNode);
				}

				if (perksNode)
				{
					tracker->StateValue( eGSS_Perks, m_gameStats->WrapXMLNode(perksNode));
				}
			}
		}
	}
}

// saves the current set of weapons for an actor to the stats tracker
void CStatsRecordingMgr::StateActorWeapons(
	IActor				*inActor)
{
	XmlNodeRef			resNode=0;
	IStatsTracker		*tracker=GetStatsTracker(inActor);
	IItemSystem			*is=g_pGame->GetIGameFramework()->GetIItemSystem();

	CRY_ASSERT_MESSAGE(tracker,"actor not currently in stats tracker, weapons won't be recorded - call ordering problem?");

	if (tracker)
	{
		IInventory *pInv = inActor->GetInventory();

		CRY_ASSERT_MESSAGE(pInv,"expected actor to have an inventory");

		int			invCount=pInv->GetCount();

		if (invCount>0)
		{
			for (int invIndex=0; invIndex<invCount; invIndex++)
			{
				EntityId		eid=pInv->GetItem(invIndex);
				IItem			*item=is->GetItem(eid);
				assert(item);
				IWeapon			*weap=item->GetIWeapon();
				if (weap)
				{
					// add weapon
					if (!resNode)
					{
						resNode=m_gameStats->CreateStatXMLNode();
					}

					XmlNodeRef		childNode=GetISystem()->CreateXmlNode("weapon");
					childNode->setAttr( "name", static_cast<CWeapon*>(weap)->GetName());
					childNode->setAttr( "amount", 1 );
					childNode->setAttr( "initial_loadout", 1);
					resNode->addChild(childNode);
				}
			}
		}
		else
		{
			CryLog("Stats recording - empty inv for actor '%s'",inActor->GetActorClass());
		}

		if (resNode)
		{
			tracker->StateValue( eSS_Weapons, m_gameStats->WrapXMLNode(resNode));
		}
	}
}

// returns the stats tracker (if any) for the given actor
IStatsTracker* CStatsRecordingMgr::GetStatsTracker(
	IActor			*inActor)
{
	IStatsTracker	*result=NULL;

	CActor		*pActor = static_cast<CActor*>(inActor);
	result = pActor->m_statsTracker;
	
	return result;
}

void CStatsRecordingMgr::StopTrackingAllPlayerStats()
{
	CGameRules::TPlayers		players;
	IActorSystem				*as=g_pGame->GetIGameFramework()->GetIActorSystem();

	// AI aren't in the gamerules player list,
	//	so use actors here instead (the previous approach didn't work in SP)
	IActorSystem *pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
	IActorIteratorPtr it = pActorSystem->CreateActorIterator();
	while (IActor *pActor = it->Next())
	{
		StopTrackingStats(pActor);
	}
}

void CStatsRecordingMgr::StartTrackingStats(
	IActor		*inActor)
{
	if (IsTrackingEnabled())
	{
		CActor			*pActor=static_cast<CActor*>(inActor);

		if (inActor->IsPlayer())
		{
			CRY_ASSERT_TRACE(!pActor->m_statsTracker,("already tracking stats for player %s\n",inActor->GetEntity()->GetName()));
			m_gameStats->AddGameElement(SNodeLocator(eGSEL_Player,eGSC_Round,eGNLT_ChannelID,inActor->GetChannelId()),NULL);
			CRY_ASSERT(pActor->m_statsTracker);
		}
		else
		{
			CRY_ASSERT_TRACE(!pActor->m_statsTracker,("already tracking stats for actor %s\n",inActor->GetEntity()->GetName()));
			m_gameStats->AddGameElement(SNodeLocator(eGSEL_AIActor,eGSC_Round,eNLT_EntityID,inActor->GetEntityId()),NULL);
			CRY_ASSERT(pActor->m_statsTracker);
		}
	}
}

void CStatsRecordingMgr::StopTrackingStats(
	IActor		*inActor)
{
	CActor			*pActor=static_cast<CActor*>(inActor);

	if (pActor->m_statsTracker)
	{
		m_gameStats->RemoveElement(m_gameStats->GetTrackedNode(pActor->m_statsTracker));
		CRY_ASSERT(pActor->m_statsTracker==NULL);
	}
}

void CStatsRecordingMgr::SaveSessionData(XmlNodeRef node)
{
#if 0	// TODO do something similar
	CRY_ASSERT(m_pMasterSrvSender.get());
	if(!m_pMasterSrvSender.get())
	{
		GameWarning("[CMissionStatistics] Saving session data with no connection to MasterServer");
		return;
	}
#endif

	static const int MAX_SAVE_ATTEMPTS = 20;

	node->setAttr("version", 1);	// todo: is this needed?

	SFileVersion ver = gEnv->pSystem->GetFileVersion();
	node->setAttr("build_number", ver.v[0]);

	string filePath;
	string timeLabel;
	time_t offsetSeconds = 0;

	while (offsetSeconds < MAX_SAVE_ATTEMPTS)
	{
		timeLabel = GetTimeLabel(offsetSeconds);
		filePath.Format("%s/Game_%s.xml", m_statsDirectory.c_str(), timeLabel.c_str());

		if (!gEnv->pCryPak->IsFileExist(filePath.c_str()))
			break;

		++offsetSeconds;
	}

	bool ret = false;
	if (MAX_SAVE_ATTEMPTS == offsetSeconds)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Too many attempts to find statistics session name. Something wrong has happen!");
	}
	else
	{
		static const int k_chunkSize=12*1024;		// on consoles, xmlnode::savetofile won't write in chunks larger than ~15000 bytes

		FILE	*file=gEnv->pCryPak->FOpen( filePath.c_str(),"wt");
		bool	success=false;

		if (file)
		{
			success=node->saveToFile(filePath.c_str(),12*1024,file);
			gEnv->pCryPak->FClose(file);
			if (success)
			{
				ITelemetryCollector		*tc=static_cast<CGame*>(gEnv->pGame)->GetITelemetryCollector();
				if (tc)
				{
					tc->SubmitFile(filePath.c_str());
				}
			}
#if 0
			m_pMasterSrvSender->OnEndSession(filePath.c_str());
#endif
		}
		else
		{
			CryLogAlways("Failed to save statslog file %s",filePath.c_str());
		}
	}
}

//////////////////////////////////////////////////////////////////////////

string CStatsRecordingMgr::GetTimeLabel(time_t offsetSeconds) const
{
	CryFixedStringT<128> timeStr;
	time_t ltime;

	time( &ltime );
	ltime += offsetSeconds;
	tm *today = localtime( &ltime );
	strftime(timeStr.m_str, timeStr.MAX_SIZE, "%y-%m-%d_%H%M%S", today);

	return string(timeStr.c_str());
}

//////////////////////////////////////////////////////////////////////////

void CStatsRecordingMgr::OnNodeAdded(const SNodeLocator& locator)
{
	IStatsTracker *tracker = g_pGame->GetIGameFramework()->GetIGameStatistics()->GetTracker(locator);
	CRY_ASSERT(tracker);
	if(!tracker) return;

	if(locator.isScope()) // Scope pushed
	{
		switch(locator.scopeID)
		{
			case eGSC_Round:
				{
					CRY_ASSERT_MESSAGE(!m_roundTracker,"BeginRound() called when a round is already in progress");
					if (m_roundTracker)
					{
						EndRound();
					}
					m_roundTracker=tracker;
					tracker->StateValue(eGSS_RoundBegin, gEnv->pTimer->GetFrameStartTime().GetMilliSecondsAsInt64());
				}
				break;

			case eGSC_Session:
				{
					CRY_ASSERT_MESSAGE(!m_sessionTracker,"BeginSession() called when session already active");
					if (m_sessionTracker)
					{
						EndSession();
					}
					m_sessionTracker=tracker;
				}
				break;
		}
	}
	else // Element added
	{
		switch(locator.elemID)
		{
			case eGSEL_Player:
				{
					CRY_ASSERT(locator.scopeID == eGSC_Round);
					CRY_ASSERT(locator.locatorType == eGNLT_ChannelID);

					tracker->Event( eSE_Lifetime, "begin" );

					int		channel=locator.locatorValue;
					IActor	*actor=g_pGame->GetGameRules()->GetActorByChannelId(channel);
					CRY_ASSERT_TRACE(actor,("Failed to find actor for channel %d",channel));
					if (actor)
					{
						if (actor->IsPlayer())
						{
							CPlayer	*player=static_cast<CPlayer*>(actor);
							CRY_ASSERT_MESSAGE(player->m_statsTracker==NULL,"setting stats tracker for player, but one is already set?!");
							player->m_statsTracker=tracker;
							StateCorePlayerStats(actor);
						}
					}
				}
				break;

			case eGSEL_AIActor:
				{
					CRY_ASSERT(locator.scopeID == eGSC_Round);
					CRY_ASSERT(locator.locatorType == eNLT_EntityID);

					tracker->Event( eSE_Lifetime, "begin" );

					EntityId id = locator.locatorValue;
					CActor	*pActor=static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id));
					CRY_ASSERT_TRACE(pActor,("Failed to find actor %d",id));
					if (pActor)
					{
						CRY_ASSERT(!pActor->IsPlayer());
						CRY_ASSERT_MESSAGE(pActor->m_statsTracker==NULL,"setting stats tracker for AI, but one is already set?!");
						pActor->m_statsTracker=tracker;
						StateCorePlayerStats(pActor);	// should still be ok for AI... perks are ignored for non-players
					}
				}
				break;
		}

	}
}

//////////////////////////////////////////////////////////////////////////

void CStatsRecordingMgr::OnNodeRemoved(const SNodeLocator& locator, IStatsTracker* tracker)
{
	CRY_ASSERT(tracker);
	if(!tracker) return;

	if(locator.isScope()) // Scope popped
	{
		switch(locator.scopeID)
		{
			case eGSC_Round:
				{
					CRY_ASSERT_MESSAGE(tracker==m_roundTracker,"Stats messed up, ended a round that wasn't the round being tracked?!");
					tracker->StateValue(eGSS_RoundEnd, gEnv->pTimer->GetFrameStartTime().GetMilliSecondsAsInt64());
					m_roundTracker=NULL;
				}
				break;
			case eGSC_Session:
				{
					CRY_ASSERT_MESSAGE(tracker==m_sessionTracker,"Stats messed up, ended a session that wasn't the session being tracked?!");
					m_sessionTracker=NULL;
				}
				break;
		}

	}
	else // Element removed
	{
		switch(locator.elemID)
		{
			case eGSEL_Player:
				{
					CRY_ASSERT(locator.scopeID == eGSC_Round);
					tracker->Event( eSE_Lifetime, "end" );

					int		channel=locator.locatorValue;
					IActor	*actor=g_pGame->GetGameRules()->GetActorByChannelId(channel);
					if (actor && actor->IsPlayer())
					{
						CPlayer		*player=static_cast<CPlayer*>(actor);
						CRY_ASSERT_MESSAGE(player->m_statsTracker==tracker || player->m_statsTracker==NULL,"stats tracker mismatch for player");
						player->m_statsTracker=NULL;
					}
				}
				break;

			case eGSEL_AIActor:
				{
					CRY_ASSERT(locator.scopeID == eGSC_Round);
					tracker->Event( eSE_Lifetime, "end" );

					EntityId id = locator.locatorValue;
					CActor	*pActor=static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id));
					if (pActor)
					{
						CRY_ASSERT_MESSAGE(pActor->m_statsTracker==tracker || pActor->m_statsTracker==NULL,"stats tracker mismatch for AI actor");
						pActor->m_statsTracker=NULL;
					}
				}
				break;
		}
	}
}

// the stats recording mgr changes the telemetry collectors session id when sessions begin and end
void CStatsRecordingMgr::SetNewSessionId()
{
	ITelemetryCollector		*tc=static_cast<CGame*>(gEnv->pGame)->GetITelemetryCollector();
	if (tc)
	{
		tc->SetNewSessionId();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnInitiate(SHostMigrationInfo& hostMigrationInfo)
{
	m_hostMigrationEventOccured = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnReconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnFinalise(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatsRecordingMgr::OnTerminate(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CStatsRecordingMgr::OnHit(const HitInfo& hit)
{
	// todo: record non-actor hits - eg vehicles.
	IActor		*actor=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hit.targetId);
	if (actor)
	{
		CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
		if(!pMgr || !pMgr->ShouldRecordEvent(eSE_Hit, actor))
			return;

		IGameStatistics		*gs=g_pGame->GetIGameFramework()->GetIGameStatistics();
		if (IStatsTracker *tracker=GetStatsTracker(actor))
		{
			const char *weaponClassName = "unknown weapon";
			if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(hit.weaponId))
			{
				weaponClassName=pEntity->GetClass()->GetName();
			}

			CGameRules		*gr=g_pGame->GetGameRules();
			const char		*ht=gr->GetHitType(hit.type);

#if 0		// material type doesn't ever seem to be set for c2 characters
			ISurfaceType *pSurfaceType=gr->GetHitMaterial(hit.material);
			if (pSurfaceType)
			{
				node->setAttr("material_type", pSurfaceType->GetType());
			}
			else
			{
				node->setAttr("material_type", "unknown material");
			}
#endif

			CryFixedStringT<64> hit_part("unknown part");

			if (hit.partId >= 0)
			{
				ICharacterInstance* pCharacter = actor->GetEntity()->GetCharacter(0);
				if (pCharacter)
				{
					// 1000 is a magic number in CryAnimation that establishes the number of the first attachment identifier
					// see CAttachmentManager::PhysicalizeAttachment
					const int FIRST_ATTACHMENT_PARTID = 1000;
					if (hit.partId >= FIRST_ATTACHMENT_PARTID)
					{
						IAttachmentManager* pAttchmentManager = pCharacter->GetIAttachmentManager();
						IAttachment* pAttachment = pAttchmentManager->GetInterfaceByIndex(hit.partId - FIRST_ATTACHMENT_PARTID);
						if (pAttachment)
						{
							hit_part = "attachment "; 
							hit_part += pAttachment->GetName();
						}
					}
					else
					{
						ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
						const char* szJointName = pSkeletonPose->GetJointNameByID(pSkeletonPose->getBonePhysParentOrSelfIndex(hit.partId));
						if (szJointName && *szJointName)
							hit_part = szJointName;
					}
				}
			}

			tracker->Event(eSE_Hit,new CHitStats(hit.projectileId, hit.targetId, hit.damage, ht ? ht : "unknown hit type", weaponClassName, hit_part));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
EStatisticEventRecordType GetRecordType(const XmlString& name)
{
	EStatisticEventRecordType type = eSERT_Always;
	if(name == "never")
		type = eSERT_Never;
	else if(name == "players")
		type = eSERT_Players;
	else if(name == "ai")
		type = eSERT_AI;

	return type;
}

//////////////////////////////////////////////////////////////////////////
void CStatsRecordingMgr::LoadEventConfig(const char* configName)
{
	IGameStatistics* pGS = g_pGame->GetIGameFramework()->GetIGameStatistics();
	if(!pGS)
		return;

	if(m_sessionTracker)
	{
		GameWarning("Can't change config during gameplay");
		return;
	}

	// load an xml file containing a list of event types to enable or disable
	string fileName;
	fileName.Format("Libs/Telemetry/%s.xml", configName);
	XmlNodeRef node = GetISystem()->LoadXmlFile(fileName.c_str());
	if(!node || strcmp("TelemetryConfig", node->getTag()))
	{
		GameWarning("Failed to load telemetry config file %s", configName);
		return;
	}

	XmlNodeRef eventsNode = node->findChild("Events");
	if(eventsNode)
	{
		// unless otherwise specified, enable everything
		XmlString defaultEnableName = "always";
		eventsNode->getAttr("default", defaultEnableName);
		EStatisticEventRecordType defaultEnable = GetRecordType(defaultEnableName);

		// set all events to this value
		for(size_t eventIndex = 0; eventIndex < eGSE_Num; ++eventIndex)
		{
			m_eventConfigurations[eventIndex] = defaultEnable;
		}

		// override specific events
		int numEvents = eventsNode->getChildCount();
		for(int i=0; i<numEvents; ++i)
		{
			XmlNodeRef eventNode = eventsNode->getChild(i);
			XmlString name;
			XmlString enableName;
			EStatisticEventRecordType enable = defaultEnable;
			if(eventNode && eventNode->getAttr("name", name) && eventNode->getAttr("enabled", enableName))
			{
				size_t eventID = pGS->GetEventIDBySerializeName(name.c_str());
				if(eventID != INVALID_STAT_ID)
				{
					assert(eventID < eGSE_Num);
					enable = GetRecordType(enableName);
					m_eventConfigurations[eventID] = enable;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

bool CStatsRecordingMgr::ShouldRecordEvent(size_t eventID, IActor* pActor /*=NULL*/) const
{
	assert(eventID < eGSE_Num);
	EStatisticEventRecordType type = m_eventConfigurations[eventID];
	if(type == eSERT_Always)
		return true;
	else if(type == eSERT_Never)
		return false;
	else
	{
		// if config is 'ai' or 'player' but actor isn't specified,
		//	always record
		if(!pActor)
		{
			assert(false);
			return true;
		}

		return (pActor->IsPlayer() == (type == eSERT_Players));
	}
}