#ifndef __HUD_OBJECTIVEDISPLAY_H__
#define __HUD_OBJECTIVEDISPLAY_H__

#include "HUD/HUDObject.h"

#include "Utility/CryFixedArray.h"

class CHUDMissionObjective;

//////////////////////////////////////////////////////////////////////////


class CHUD_ObjectiveDisplay : public CHUDObject
{

public:

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_Objective				= BIT(0),
		eDF_All							= 0xFFFF
	};

	CHUD_ObjectiveDisplay();
	virtual ~CHUD_ObjectiveDisplay();

	virtual void Update	(float frameTime);
	virtual void Init();
	virtual void PreDelete();


	virtual void OnHUDEvent( const SHUDEvent& event );

private:

	void ObjectiveChanged(const CHUDMissionObjective* pObjective);
	void Show(const bool show);

	const static int k_maxActiveObjectivesSupported = 4;

	typedef CryFixedArray<EntityId, k_maxActiveObjectivesSupported> Objectives;
	Objectives						m_objectives;

	IFlashVariableObject* m_rootObject;

	uint16								m_dirty;

	bool									m_show;
	
};


//////////////////////////////////////////////////////////////////////////

#endif

