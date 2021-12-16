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

#ifndef __STATSRECORDINGMGR_H__
#define __STATSRECORDINGMGR_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IItemSystem.h"
#include "IWeapon.h"
#include "IGameStatistics.h"
#include "IGameRulesSystem.h"

class CXMLStatsSerializer;

//////////////////////////////////////////////////////////////////////////

// Game-specific events
enum EGameStatisticEvent
{
	eGSE_Melee = eSE_Num,
	eGSE_MeleeHit,
	eGSE_Firemode,
	eGSE_NanoSuitMode,
	eGSE_NanoSuitActivate,
	eGSE_NanoSuitEnergy,
	eGSE_HitReactionAnim,
	eGSE_DeathReactionAnim,

	eGSE_Num,
};

//////////////////////////////////////////////////////////////////////////

// Game-specific states
enum EGameStatisticsState
{
	eGSS_MapPath = eSS_Num,
	eGSS_Perks,
	eGSS_Attachments,
	eGSS_TeamName,
	eGSS_RoundBegin,
	eGSS_RoundEnd,
	eGSS_Config,

	eGSS_Num,
};

//////////////////////////////////////////////////////////////////////////

enum EGameStatisticsScopes
{
	eGSC_Session,
	eGSC_Round,

	eGSC_Num,
};

//////////////////////////////////////////////////////////////////////////

enum EGameStatisticsElements
{
	eGSEL_Team,
	eGSEL_Player,
	eGSEL_AIActor,
	eGSEL_Entity,

	eGSEL_Num,
};

enum EGameNodeLocators
{
	eGNLT_TeamID = eNLT_Num,
	eGNLT_ChannelID,

	eGNLT_Num,
};

enum EStatisticEventRecordType
{
	eSERT_Never,		// never record events of this type
	eSERT_Always,		// always record
	eSERT_Players,	// only record if the actor is a player
	eSERT_AI,				// only record if the actor is an AI
};

class CStatsRecordingMgr : public IGameStatisticsCallback, public IHostMigrationEventListener, public IHitListener
{
	protected:
		IGameStatistics		*m_gameStats;
		IStatsTracker		*m_sessionTracker;
		IStatsTracker		*m_roundTracker;
		string				m_statsDirectory;
		CXMLStatsSerializer	*m_serializer;
		bool	m_hostMigrationEventOccured;
		EStatisticEventRecordType m_eventConfigurations[eGSE_Num];

		void				StateActorPerks(
								IActor		*inActor);
		void				StateActorWeapons(
								IActor		*inActor);
		void				StateCorePlayerStats(
								IActor		*inPlayerActor);

		void				SetNewSessionId();


	public:
							CStatsRecordingMgr();
							~CStatsRecordingMgr();

		string				GetTimeLabel(time_t offsetSeconds) const;
		bool				IsTrackingEnabled();

		void				BeginSession();
		void				EndSession();
		void				BeginRound();
		void				StateRoundWinner(
								int			inTeam);
		void				EndRound();
		IStatsTracker		*GetSessionTracker()			{ return m_sessionTracker; }
		IStatsTracker		*GetRoundTracker()				{ return m_roundTracker; }

		IStatsTracker		*GetStatsTracker(
								IActor		*inActor);
		void				StartTrackingStats(
								IActor		*inActor);
		void				StopTrackingStats(
								IActor		*inActor);
		void				StopTrackingAllPlayerStats();
		void				AddTeam(
								int			inTeamId,
								string		inTeamName);

		void				SaveSessionData(XmlNodeRef node);

		bool				ShouldRecordEvent(size_t eventID, IActor* pActor=NULL) const;
		void				LoadEventConfig(const char* configName);

		// IGameStatisticsCallback
		virtual void OnEvent(const SNodeLocator& locator, size_t eventID, const CTimeValue& time, const SStatAnyValue& val) {}
		virtual void OnState(const SNodeLocator& locator, size_t stateID, const SStatAnyValue& val) {}
		virtual void PreprocessScriptedEventParameter(size_t eventID, SStatAnyValue& value) {}
		virtual void PreprocessScriptedStateParameter(size_t stateID, SStatAnyValue& value) {}
		virtual void OnNodeAdded(const SNodeLocator& locator);
		virtual void OnNodeRemoved(const SNodeLocator& locator, IStatsTracker* tracker);

		// IHostMigrationEventListener
		virtual bool OnInitiate(SHostMigrationInfo& hostMigrationInfo);
		virtual bool OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo);
		virtual bool OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo);
		virtual bool OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo);
		virtual bool OnReconnectClient(SHostMigrationInfo& hostMigrationInfo);
		virtual bool OnFinalise(SHostMigrationInfo& hostMigrationInfo);
		virtual bool OnTerminate(SHostMigrationInfo& hostMigrationInfo);

		// IHitListener
		virtual void OnHit(const HitInfo&);
		virtual void OnExplosion(const ExplosionInfo&) {}
		virtual void OnServerExplosion(const ExplosionInfo&) {}
};


#endif // __STATSRECORDINGMGR_H__
