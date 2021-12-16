#ifndef ___HUD_ASSCOREELEMENT___
#define ___HUD_ASSCOREELEMENT___

#include "HUD/HUDObject.h"

#include "GameRulesModules/GameRulesCombiCaptureObjective.h"

class CGameRules;
struct IItemParamsNode;

class CHUD_AsScoreElement : public CHUDObject
{
private:

	struct STerminalInfo
	{
		uint8  beingCaptured : 1;
	};

	enum EMessageStates
	{
		ECTFHud_Neutral = 0,
		ECTFHud_Friendly,
		ECTFHud_Enemy,
		ECTFHud_Off,
	};

public:

	CHUD_AsScoreElement();
	virtual ~CHUD_AsScoreElement();

	void Update	(float frameTime);
	void Draw		();

	void Initialize(const IItemParamsNode * object );

	void OnHUDEvent(const SHUDEvent& event);

private:

	void Reset();

	void UpdateData( void );
	void UpdateFlash( void );
	void BooleanFlashUpdates( void );
	void UpdateGameStatePrompt( void );

private:

	STerminalInfo  m_terminalInfos[CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES];
	STerminalInfo  m_terminalInfosLastFrame[CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES];
	EntityId  m_terminalEids[CGameRulesHoldObjectiveBase::HOLD_OBJECTIVE_MAX_ENTITIES];

	IHUDAsset* m_pPlayer;

	int m_ownTeamScore;
	int m_enemyTeamScore;
	int m_ownScore;
	int m_ownTeam;
	int m_roundTime;
	int m_scoreLimit;

	int m_numTerminalIdxs;

	float m_overallProgress;

	bool m_isAttacking;

	bool m_isResetUpdate;

	uint8 m_gameStatePromptNeedsUpdate : 1;
	uint8 m_overallProgressUpdated : 1;
	uint8 m_localDownloading : 1;
	uint8 m_localDownloadedLastFrame : 1;
};


#endif // ___HUD_ASSCOREELEMENT___
