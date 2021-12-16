/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 26:10:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GameRulesStandardRounds_h_
#define _GameRulesStandardRounds_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesRoundsModule.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "CryFixedString.h"
#include "GameRulesTypes.h"

class CGameRules;

class CGameRulesStandardRounds :	public IGameRulesRoundsModule,
									public IGameRulesTeamChangedListener
{
public:
	CGameRulesStandardRounds();
	virtual ~CGameRulesStandardRounds();

	// IGameRulesRoundsModule
	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void OnEndGame(int teamId, EntityId playerId, EGameOverReason reason);
	virtual int GetRoundNumber();
	virtual int GetRoundsRemaining();

	virtual int GetPrimaryTeam();

	virtual bool IsInProgress() const								{ return (m_roundState == eGRRS_InProgress); }
	virtual bool IsRestarting() const								{ return (m_roundState == eGRRS_Restarting); }
	virtual bool IsRestartingRound(int round) const	{ return (round == eGRRS_Restarting); }
	// ~IGameRulesRoundsModule

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

protected:
	typedef CryFixedStringT<32> TFixedString;
	typedef std::vector<EntityId> TEntityIdVec;

	struct SEntityDetails
	{
		TEntityIdVec m_currentEntities;
		TFixedString m_activateFunc;
		TFixedString m_deactivateFunc;
		const IEntityClass *m_pEntityClass;
		int m_selectCount;
		bool m_doRandomSelection;
	};

	struct SOnEndRoundStrings
	{
		SOnEndRoundStrings()
		{
			m_reason = EGOR_Unknown;
		}

		EGameOverReason m_reason;

		TFixedString m_roundWinMessage;
		TFixedString m_roundLoseMessage;
		TFixedString m_roundDrawMessage;
	};

	enum ERoundState
	{
		eGRRS_InProgress,		// Round is in progress
		eGRRS_Restarting,		// New round is about to start
		eGRRS_Finished,			// All rounds finished
	};

	typedef std::vector<SEntityDetails> TEntityDetailsVec;

	void StartNewRound(bool isReset);
	void EndRound();
	void ReadEntityClasses(TEntityDetailsVec &classesVec, XmlNodeRef xml, TFixedString &startRoundString);
	void SetupEntities();
	void SetTeamEntities(TEntityDetailsVec &classesVec, int teamId);
	void CallScript(EntityId entityId, const char *pFunction);
	void ActivateEntity(EntityId entityId, int teamId, const char *pActivateFunc, TEntityIdVec &pEntitiesVec);
	void DeactivateEntities(const char *pDeactivateFunc, TEntityIdVec &entitiesVec);
	void CheckForTeamEntity(IEntity *pEntity, int newTeamId, TEntityDetailsVec &entitiesVec);

	void ClDisplayEndOfRoundMessage();

	static void CmdNextRound(IConsoleCmdArgs *pArgs);

	TEntityDetailsVec m_primaryTeamEntities;
	TEntityDetailsVec m_secondaryTeamEntities;

	// strings
	TFixedString m_primaryStartRoundString;
	TFixedString m_secondaryStartRoundString;

	static const int MAX_END_OF_ROUND_STRINGS = 3;
	SOnEndRoundStrings m_endOfRoundStringsDefault;
	SOnEndRoundStrings m_endOfRoundStrings[MAX_END_OF_ROUND_STRINGS];
	int m_endOfRoundStringCount;

	int m_roundNumber;		// Start on round 0
	int m_previousRoundWinnerTeamId;
	EntityId m_previousRoundWinnerEntityId;
	EGameOverReason m_previousRoundWinReason;

	float m_newRoundStartTime;

	ERoundState m_roundState;

	bool m_resetScores;			// Reset scores on round end
};

#endif // _GameRulesStandardRounds_h_