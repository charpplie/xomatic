/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a "combination capture" objective.
		One team's goal is to be in proximity of one or more of multiple "capture"
		points for a specified combined duration of time.
		The other team must prevent them.

	-------------------------------------------------------------------------
	History:
	- 16:12:2009  : Created by Thomas Houghton

*************************************************************************/

#ifndef _GAME_RULES_COMBI_CAPTURE_OBJECTIVE_H_
#define _GAME_RULES_COMBI_CAPTURE_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/GameRulesHoldObjectiveBase.h"
#include "GameRulesModules/IGameRulesRoundsListener.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesTypes.h"

class CGameRulesCombiCaptureObjective :	public CGameRulesHoldObjectiveBase,
																				public IGameRulesRoundsListener,
																				public SGameRulesListener
{
private:
	typedef CGameRulesHoldObjectiveBase BaseType;
public:
	CGameRulesCombiCaptureObjective();
	~CGameRulesCombiCaptureObjective();

	// IGameRulesSimpleEntityObjective
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void EnableCompletion(int teamId, bool enable) {}
	virtual void EnableScoring(int teamId, bool enable);

	virtual bool IsComplete(int teamId);

	virtual void OnHostMigration(bool becomeServer) {}
	// ~IGameRulesSimpleEntityObjective

	// IGameRulesClientConnectionListener
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesRoundsListener
	virtual void OnRoundStart();
	virtual void ClRoundsNetSerializeReadState(int newState, int curState) {}
	// ~IGameRulesRoundsListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// SGameRulesListener (interface)
	virtual void SvOnTimeLimitExpired();
	// ~SGameRulesListener

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId);
	// ~IGameRulesClientConnectionListener

protected:
	typedef std::vector<EntityId> TEntityIdVec;

	struct SCaptureEntity : public BaseType::IHoldEntityAdditionalDetails
	{
		void Reset()
		{
			m_captureAudio = INVALID_SOUNDID;
			m_capturedAudio = INVALID_SOUNDID;
			m_currentIcon = EGRMO_Unknown;
			m_iconTransitionAmount = 0.f;
			m_needIconUpdate = false;
			m_capturing = false;
		}

		tSoundID m_captureAudio;
		tSoundID m_capturedAudio;

		EGameRulesMissionObjectives m_currentIcon;

		float m_iconTransitionAmount;

		bool m_needIconUpdate;
		bool m_capturing;
	};

	typedef CryFixedStringT<32> TFixedString;

	// CGameRulesHoldObjectiveBase
	virtual void OnInsideStateChanged(SHoldEntityDetails *pDetails);
	virtual void OnNewHoldEntity(SHoldEntityDetails *pDetails, int index);
	virtual void OnRemoveHoldEntity(SHoldEntityDetails *pDetails);
	virtual void OnNetSerializeHoldEntity(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags, SHoldEntityDetails *pDetails, int index) {};
	// ~CGameRulesHoldObjectiveBase

	void ClUpdateMainHUD(const bool siteBeingCaptured, const bool localPlayerIsCapturing);
	void ClUpdateSiteHUD(SHoldEntityDetails *pDetails, const int currActiveIndex);

	void CleanUpEntity(SHoldEntityDetails *pDetails);
	void AwardPlayerPoints(TEntityIdVec *pEntityVec, EGRST scoreType);
	void UpdateIcon(SHoldEntityDetails * pDetails, bool force);
	void UpdateCaptureProgress(SHoldEntityDetails *pDetails, float frameTime);
	void ClSiteStartCapturing(SHoldEntityDetails *pDetails);
	EGameRulesMissionObjectives GetIcon(SHoldEntityDetails *pDetails);

	void ReadAudioSignal(XmlNodeRef node, const char* name, CAudioSignalPlayer* signalPlayer);

	SCaptureEntity m_additionalInfo[HOLD_OBJECTIVE_MAX_ENTITIES];

	TFixedString m_shouldShowIconFunc;

	CAudioSignalPlayer m_capturingSignalTeam;
	CAudioSignalPlayer m_capturingSignalOpponent;

	EGameRulesMissionObjectives m_ourCapturePoint;
	EGameRulesMissionObjectives m_theirCapturePoint;
	EGameRulesMissionObjectives m_usCapturingPoint;
	EGameRulesMissionObjectives m_themCapturingPoint;

	int m_iconPriority;
	int m_numActiveEntities;
	int m_attackingTeamId;

	float m_combiProgress;
	float m_goalCombiCaptureTime;

	bool m_contestable;
	bool m_scoringEnabled[2];
	bool m_useIcons;
	bool m_allowMultiPlayerCaptures;

	const static int k_maxAudioLength = 96;
	char m_capturingAudio[k_maxAudioLength];
	char m_capturedAudio[k_maxAudioLength];
};

#endif // _GAME_RULES_COMBI_CAPTURE_OBJECTIVE_H__
