#ifndef ___HUD_PSLSCORELEMENT___
#define ___HUD_PSLSCORELEMENT___

#include "HUD/HUDObject.h"

class CHUD_PSLScoreElement : public CHUDObject
{
public :
	CHUD_PSLScoreElement();

	void Init( void );

	void Draw( void );

	void OnHUDEvent(const SHUDEvent& event);

	void Update( float frameTime );

	void UpdateFlash( void );

private :
	IHUDAsset* m_pPSLScoreElement;

	// score board data.
	int m_ownTeamScore;
	int m_enemyTeamScore;
	int m_ownScore;
	int m_ownTeam;
	int m_roundTime;
	int m_scoreLimit;

	bool m_bForceUpdate;
};

#endif // ___HUD_PSLSCORELEMENT___