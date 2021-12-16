/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a king of the hill objective (take and hold)
	-------------------------------------------------------------------------
	History:
	- 15:02:2010  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_KING_OF_THE_HILL_OBJECTIVE_H_
#define _GAME_RULES_KING_OF_THE_HILL_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/GameRulesHoldObjectiveBase.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesTypes.h"

class CGameRulesKingOfTheHillObjective :	public CGameRulesHoldObjectiveBase
{
private:
	typedef CGameRulesHoldObjectiveBase BaseType;
public:
	CGameRulesKingOfTheHillObjective();
	~CGameRulesKingOfTheHillObjective();

	// IGameRulesSimpleEntityObjective
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual void EnableCompletion(int teamId, bool enable) {}
	virtual void EnableScoring(int teamId, bool enable);

	virtual bool IsComplete(int teamId) { return false; }

	virtual bool IsEntityFinished(int type, int index)	{ return false; }
	virtual bool CanRemoveEntity(int type, int index)		{ return true; }
	virtual void SetWaveNumber(int num, int waveCount) {}

	virtual void OnHostMigration(bool becomeServer) {}
	// ~IGameRulesSimpleEntityObjective

	// IGameRulesClientConnectionListener
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

protected:
	struct SKotHEntity : public BaseType::IHoldEntityAdditionalDetails
	{
		SKotHEntity()
		{
			Reset();
		}

		void Reset()
		{
			m_capturedAudio = INVALID_SOUNDID;

			m_currentIcon = EGRMO_Unknown;

			m_scoringTeamId = 0;

			m_timeSinceLastScore = 0.f;
			m_scoreTimerLength = 0.f;

			m_isOnRadar = false;
			m_needsIconUpdate = false;
		}

		tSoundID m_capturedAudio;

		EGameRulesMissionObjectives m_currentIcon;

		int m_scoringTeamId;

		float m_timeSinceLastScore;
		float m_scoreTimerLength;

		bool m_isOnRadar;
		bool m_needsIconUpdate;
	};

	typedef CryFixedStringT<32> TFixedString;

	// CGameRulesHoldObjectiveBase
	virtual void OnInsideStateChanged(SHoldEntityDetails *pDetails);
	virtual void OnNewHoldEntity(SHoldEntityDetails *pDetails, int index);
	virtual void OnRemoveHoldEntity(SHoldEntityDetails *pDetails);
	// ~CGameRulesHoldObjectiveBase

	void SvSiteChangedOwner(SHoldEntityDetails *pDetails);
	void ClSiteChangedOwner(SHoldEntityDetails *pDetails, int oldTeamId);

	void UpdateIcon(SHoldEntityDetails *pDetails, bool force);
	EGameRulesMissionObjectives GetIcon(SHoldEntityDetails *pDetails);

	float CalculateScoreTimer(int playerCount);

	void ClUpdateHUD(SHoldEntityDetails *pDetails);

	void InitEntityAudio(SHoldEntityDetails *pDetails);
	void UpdateEntityAudio(SHoldEntityDetails *pDetails);
	void ClearEntityAudio(SHoldEntityDetails *pDetails);

	void ReadAudioSignal(const XmlNodeRef& node, const char* name, CAudioSignalPlayer* signalPlayer);

	SKotHEntity m_additionalInfo[HOLD_OBJECTIVE_MAX_ENTITIES];

	TFixedString m_friendlyCaptureString;
	TFixedString m_enemyCaptureString;
	TFixedString m_friendlyLostString;
	TFixedString m_enemyLostString;
	TFixedString m_newEntityString;
	TFixedString m_shouldShowIconFunc;

	CAudioSignalPlayer m_captureSignal;
	CAudioSignalPlayer m_capturedSignalTeam;
	CAudioSignalPlayer m_capturedSignalOpponent;

	EGameRulesMissionObjectives m_neutralIcon;
	EGameRulesMissionObjectives m_friendlyIcon;
	EGameRulesMissionObjectives m_hostileIcon;

	int m_iconPriority;

	float m_scoreTimerMaxLength;
	float m_scoreTimerAdditionalPlayerMultiplier;

	bool m_scoringEnabled[2];		// Todo: support games without 2 teams
	bool m_useIcons;

	const static int k_maxAudioLength = 96;
	char m_capturedAudio[k_maxAudioLength];
};

#endif // _GAME_RULES_KING_OF_THE_HILL_OBJECTIVE_H_

