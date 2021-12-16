/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a capture objective (take and hold)
	-------------------------------------------------------------------------
	History:
	- 21:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_CAPTURE_OBJECTIVE_H_
#define _GAME_RULES_CAPTURE_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/GameRulesHoldObjectiveBase.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesTypes.h"

#ifdef _RELEASE
#define CAPTURE_OBJECTIVE_ADDITIONAL_DEBUG_CODE 0
#else
#define CAPTURE_OBJECTIVE_ADDITIONAL_DEBUG_CODE 1
#endif

class CGameRulesCaptureObjective :	public CGameRulesHoldObjectiveBase
{
private:
	typedef CGameRulesHoldObjectiveBase BaseType;
public:
	CGameRulesCaptureObjective();
	~CGameRulesCaptureObjective();

	// IGameRulesSimpleEntityObjective
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual void EnableCompletion(int teamId, bool enable);
	virtual void EnableScoring(int teamId, bool enable);

	virtual bool IsComplete(int teamId);

	virtual bool IsEntityFinished(int type, int index);
	virtual bool CanRemoveEntity(int type, int index);
	virtual void SetWaveNumber(int num, int waveCount) {}

	virtual void OnHostMigration(bool becomeServer) {}
	// ~IGameRulesSimpleEntityObjective

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId);
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

protected:
	struct SCaptureProgress
	{
		SCaptureProgress()
		{
			Reset();
		}

		void StartCapture(int teamId)
		{
			m_teamId = teamId;
			if (gEnv->bServer)
			{
				// Don't reset progress unless we're the server, will be set from NetSerialize
				m_progress = 0.f;
			}
		}

		void Reset()
		{
			m_teamId = 0;
			m_progress = 0.f;
		}

		int m_teamId;
		float m_progress;
	};

	struct SCaptureEntity : public BaseType::IHoldEntityAdditionalDetails
	{
		SCaptureEntity()
		{
			Reset();
		}

		void Reset()
		{
			m_captureProgress.Reset();

			m_captureAudio = INVALID_SOUNDID;
			m_capturedAudio = INVALID_SOUNDID;

			m_capturedTeamId = 0;
			m_capturelengthIdx = 0;
			m_captures = 0;
			m_scores = 0;
			m_currentIcon = EGRMO_Unknown;

			m_needIconUpdate = false;
			m_isFinished = false;
			m_isOnRadar = false;
		}

		SCaptureProgress m_captureProgress;

		tSoundID m_captureAudio;
		tSoundID m_capturedAudio;

		int m_capturedTeamId;
		int m_capturelengthIdx;
		int m_captures;
		int m_scores;			// Will only be updated on clients if CGameRulesCaptureObjective::m_maxScoresPerEntity is non-zero
		EGameRulesMissionObjectives m_currentIcon;

		bool m_needIconUpdate;
		bool m_isFinished;
		bool m_isOnRadar;
	};

	struct SCaptureLength
	{
		SCaptureLength(int requiredPlayers, float timeToCapture, int requiredCaptures)
		{
			m_requiredPlayers = requiredPlayers;
			m_timeToCapture = timeToCapture;
			m_requiredCaptures = requiredCaptures;
		}

		int m_requiredPlayers;
		int m_requiredCaptures;
		float m_timeToCapture;
	};

	typedef std::vector<SCaptureLength> TCaptureLengthVec;
	typedef CryFixedStringT<32> TFixedString;

	// CGameRulesHoldObjectiveBase
	virtual void OnInsideStateChanged(SHoldEntityDetails *pDetails);
	virtual void OnNewHoldEntity(SHoldEntityDetails *pDetails, int index);
	virtual void OnRemoveHoldEntity(SHoldEntityDetails *pDetails);
	virtual void OnNetSerializeHoldEntity(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags, SHoldEntityDetails *pDetails, int index);
	// ~CGameRulesHoldObjectiveBase

	int DetermineCaptureLengthIndex(int numPlayers, int numCaptures);
	void UpdateIcon(SHoldEntityDetails * pCaptureEntity, bool force);
	void UpdateCaptureProgress(SCaptureEntity *pCaptureEntity, float frameTime);
	void SiteStartCapturing(SHoldEntityDetails *pDetails, int teamId);
	void ClSiteCaptured(SHoldEntityDetails *pDetails);
	EGameRulesMissionObjectives GetIcon(SHoldEntityDetails *pDetails);

	void SvSiteCaptured(SHoldEntityDetails *pDetails);
	void SvSiteHeld(SCaptureEntity *pCaptureEntity, int &team1Count, int &team2Count);

	void ClUpdateHUD(SHoldEntityDetails *pDetails, int &currentActiveIndex);

	void InitCaptureEntityAudio(SHoldEntityDetails *pDetails);
	void UpdateCaptureEntityAudio(SHoldEntityDetails *pDetails);
	void ClearCaptureEntityAudio(SHoldEntityDetails *pDetails);

	void ReadAudioSignal(const XmlNodeRef& node, const char* name, CAudioSignalPlayer* signalPlayer);

	SCaptureEntity m_additionalInfo[HOLD_OBJECTIVE_MAX_ENTITIES];

	TCaptureLengthVec m_captureLengths;

	TFixedString m_friendlyCaptureString;
	TFixedString m_enemyCaptureString;
	TFixedString m_newEntityString;
	TFixedString m_shouldShowIconFunc;

	CAudioSignalPlayer m_captureSignal;

	CAudioSignalPlayer m_capturingSignalTeam;
	CAudioSignalPlayer m_capturingSignalOpponent;

	CAudioSignalPlayer m_capturedSignalTeam;
	CAudioSignalPlayer m_capturedSignalOpponent;

	EGameRulesMissionObjectives m_neutralIcon;
	EGameRulesMissionObjectives m_stealBlueToRedIcon;
	EGameRulesMissionObjectives m_stealRedToBlueIcon;
	EGameRulesMissionObjectives m_initialCaptureBlueIcon;
	EGameRulesMissionObjectives m_initialCaptureRedIcon;

	int m_iconPriority;
	int m_maxCapturesPerEntity;
	int m_maxScoresPerEntity;

	float m_timeToScore;
	float m_scoreTimerLength;
	float m_bleedRate;

	bool m_capturingEnabled[2];		// Todo: support games without 2 teams
	bool m_scoringEnabled[2];		// Todo: support games without 2 teams
	bool m_useScoreTimer;
	bool m_useBleedRate;
	bool m_useIcons;

	const static int k_maxAudioLength = 96;
	char m_capturingAudio[k_maxAudioLength];
	char m_capturedAudio[k_maxAudioLength];
#if CAPTURE_OBJECTIVE_ADDITIONAL_DEBUG_CODE
	float m_overrideCaptureTime;
#endif
};

#endif // _GAME_RULES_CAPTURE_OBJECTIVE_H__
