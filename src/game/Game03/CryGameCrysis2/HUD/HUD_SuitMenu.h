#ifndef __HUD_SUITMENU_H__
#define __HUD_SUITMENU_H__

#include "HUDObject.h"
#include "NanoSuitDefs.h"

//-----------------------------------------------------------------------------------------------------

class CHUD_SuitMenu : public CHUDObject
{
public:

	CHUD_SuitMenu();
	virtual ~CHUD_SuitMenu();

	virtual void Init();
	virtual void PreDelete();

	virtual void OnHUDEvent(const SHUDEvent& event);

private:

	void OnSuitModeChanged(const ENanoSuitMode suitMode);
	void SetFrame(const ENanoSuitMode suitMode, const int frame);
	void SuitMenuChanged(const bool show);

	IFlashVariableObject* m_menuObject;
	IFlashVariableObject* m_powerObject;
	IFlashVariableObject* m_tacticalObject;
	IFlashVariableObject* m_stealthObject;
	IFlashVariableObject* m_armorObject;
	ENanoSuitMode					m_mode;
};

#endif

