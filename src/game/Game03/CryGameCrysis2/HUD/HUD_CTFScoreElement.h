#ifndef ___HUD_CTFSCOREELEMENT___
#define ___HUD_CTFSCOREELEMENT___

#include "HUD/HUDObject.h"

class CGameRules;

class CHUD_CtfScoreElement : public CHUDObject
{
private:

	enum EMessageStates
	{
		ECTFHud_Neutral = 0,
		ECTFHud_Friendly,
		ECTFHud_Enemy,
		ECTFHud_Off,
	};

public:

	CHUD_CtfScoreElement();
	virtual ~CHUD_CtfScoreElement();

	void Update	(float frameTime);
	void Draw		();

	void Initialize(const IItemParamsNode * object );

	void OnHUDEvent(const SHUDEvent& event);

private:

	void UpdateData( void );
	void UpdateFlash( void );
	void UpdateGameStatePrompt(CGameRules* pGameRules, int iTeam, bool bFlagCarrier);

private:

	IHUDAsset* m_pPlayer;

	// score board data.
	int m_ownTeamScore;
	int m_enemyTeamScore;
	int m_ownScore;
	int m_ownTeam;
	int m_roundTime;
	int m_scoreLimit;
	int m_ownFlagState; // 0 or 1 (not-in base or in base
	int m_lastOwnFlagState;
	int m_opponentFlagState; // 0 or 1 (not-in base or in base
	int m_lastopponentFlagState;
	bool m_carryingEnemyFlag;
	bool m_lastCarryingEnemyFlag;

	bool m_friendCarryingEnemyFlag;
	bool m_enemyCarryingFriendlyFlag;
};


#endif // ___HUD_CTFSCOREELEMENT___
