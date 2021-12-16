#ifndef ___CHUD_CD_SCORE_ELEMENT___
#define ___CHUD_CD_SCORE_ELEMENT___

#include "HUD/HUDObject.h"

class CHUD_CDScoreElement : public CHUDObject
{
public:

	CHUD_CDScoreElement();

	void Update( float frameTime );
	void Draw( void );

	void OnHUDEvent(const SHUDEvent& event);

	void Init( void );

private:

	void UpdateData( void );
	void UpdateBonusText( void );

	IHUDAsset* m_pCDScoreBoard;

	// score board data.
	float m_ownTeamScore;
	float m_enemyTeamScore;

	int m_team1RoundScore;
	int m_team2RoundScore;
	int m_roundScoreLimit;
	int m_maxTeamScore;

	int m_ownTeam;

	int m_currentWaveNum;
	int m_currentWaveCount;
	int m_currentActiveWaveCount;
};

#endif //___CHUD_CD_SCORE_ELEMENT___
