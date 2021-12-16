#ifndef __HUD_REVIVE_CYCLE_H__
#define __HUD_REVIVE_CYCLE_H__

#include "HUDObject.h"

//-----------------------------------------------------------------------------------------------------

class CHUD_ReviveCycle : public CHUDObject
{
public:

	CHUD_ReviveCycle();
	virtual ~CHUD_ReviveCycle();

	void Init();
	void Update(float frameTime);
	void Draw();
	void Reload();

	void OnHUDEvent(const SHUDEvent& event);

private:

	void Create();
	void Destroy();

	float m_remainingReviveCycleTime;
	bool m_bVisible;
};


#endif // ~__HUD_REVIVE_CYCLE_H__

