#ifndef ___HUD_AONSCOREELEMENT___
#define ___HUD_AONSCOREELEMENT___

#include "HUD/HUDObject.h"

class CGameRules;

class CHUD_AONScoreElement : public CHUDObject
{
private:

public:

	CHUD_AONScoreElement();
	virtual ~CHUD_AONScoreElement();

	void Update(float frameTime);
	void Draw();

	virtual void Init();

	void OnHUDEvent(const SHUDEvent& event);

private:

	void UpdateData();
	void UpdateFlash(int roundTime, int ownScore, int ownRank, bool suddenDeath, int numStats, int numAlive, int numEliminated, int lives);

	void SetRemainingText(const int slot, const char func[], const EntityId list[], const int count);

private:

	IHUDAsset* m_pPlayer;

	int m_roundTime;
	int m_ownScore;
	int m_ownRank;
	bool m_suddenDeath;
	int m_numStats;
	int m_numAlive;
	int m_numEliminated;
	int m_lives;

	bool m_firstUpdate;
};


#endif // ___HUD_AONSCOREELEMENT___
