#include "StdAfx.h"
#include "HUD_AIUpdater.h"

#include "HUD.h"
#include "IAISystem.h"

//////////////////////////////////////////////////////////////////////////


CHUD_AIUpdater::CHUD_AIUpdater()
{
	Update(0.0f);
}



CHUD_AIUpdater::~CHUD_AIUpdater()
{
}



void CHUD_AIUpdater::Update(float frameTime)
{
	SAIDetectionLevels levels;
	gEnv->pAISystem->GetDetectionLevels(0, levels);

	int awareness = int_round( max(levels.puppetExposure, levels.puppetThreat) * 100.0f );
	if(awareness != m_currentAwarenessLevel)
	{
		m_currentAwarenessLevel = awareness;
		SHUDEvent awarenessChangedEvent;
		awarenessChangedEvent.eventType = eHUDEvent_OnAIAwarenessChanged;
		awarenessChangedEvent.eventIntData = awareness;
		CHUD::CallEvent(awarenessChangedEvent);
	}
}


//////////////////////////////////////////////////////////////////////////