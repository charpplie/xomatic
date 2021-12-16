#ifndef ___HUD_IASCOREELEMENT___
#define ___HUD_IASCOREELEMENT___

#include "HUD/HUDObject.h"

class CGameRules;

class CHUD_IAScoreElement : public CHUDObject
{
private:

public:

	CHUD_IAScoreElement();
	virtual ~CHUD_IAScoreElement();

	void Update(float frameTime);
	void Draw();

	virtual void Init();

	void OnHUDEvent(const SHUDEvent& event);

private:

	void UpdateData();
	void UpdateFlash(int roundTime, int ownScore, int ownRank);

private:

	IHUDAsset* m_pPlayer;

	int m_roundTime;
	int m_ownScore;
	int m_ownRank;

	bool m_firstUpdate;
};


#endif // ___HUD_IASCOREELEMENT___
