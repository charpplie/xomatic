#ifndef ___CHUD_TIASCOREELEMENT___
#define ___CHUD_TIASCOREELEMENT___

#include "HUD/HUDObject.h"

class CHUD_TiaScoreElement : public CHUDObject
{
public:

	CHUD_TiaScoreElement();

	void Update( float frameTime );
	void Draw( void );

	void Init( void );

private:

	void UpdateData( void );
	void UpdateFlash( void );

	IHUDAsset* m_pTiaScoreBoard;


	// score board data.
	int m_ownTeamScore;
	int m_enemyTeamScore;
	int m_ownScore;
	int m_ownTeam;
	int m_roundTime;
	int m_scoreLimit;
};

#endif //___CHUD_TIASCOREELEMENT___
