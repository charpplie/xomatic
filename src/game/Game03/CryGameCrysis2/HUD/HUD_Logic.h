#ifndef __HUDLOGIC_H__
#define __HUDLOGIC_H__

#include "HUDObject.h"
#include "NanoSuitDefs.h"

//////////////////////////////////////////////////////////////////////////


class CHUD_Logic : public CHUDObject
{

public:

	CHUD_Logic();
	virtual ~CHUD_Logic();

	virtual void OnHUDEvent(const SHUDEvent& event);

private:

	static IEntityClass* s_binocularsClass;

	void				OnSuitModeChanged(ENanoSuitMode mode);
	void				ShowSuitMenu(bool show);
	void				OnItemSelected(EntityId item);

	string			m_mode;
	bool				m_binoculars;

};


//////////////////////////////////////////////////////////////////////////

#endif

