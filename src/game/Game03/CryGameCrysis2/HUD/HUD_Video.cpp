#include "StdAfx.h"
#include "HUD_Video.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUD_UnifiedAsset.h"


//////////////////////////////////////////////////////////////////////////


CHUD_Video::CHUD_Video()
: m_objectRoot(NULL)
, m_objectComs(NULL)
{
}



CHUD_Video::~CHUD_Video()
{
}



void CHUD_Video::Init()
{
	HUD_FLASVAROBJ_REG( GetAsset(), "Root_Video", m_objectRoot );
	HUD_FLASVAROBJ_REG( GetAsset(), "Incoming_Comms", m_objectComs );
	m_objectRoot->SetVisible(false);
	m_objectComs->SetVisible(false);
	SHUDEvent event(eHUDEvent_HUDElementVisibility);
	event.ReserveData(3);
	event.AddData(SHUDEventData(false));
	event.AddData(SHUDEventData((int)eHUDElement_transmissions));
	event.AddData(SHUDEventData((int)eHUDElement_incoming));
	CHUD::CallEvent(event);
}



void CHUD_Video::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot);
	HUD_FLASHOBJ_SAFERELEASE(m_objectComs);
}


//////////////////////////////////////////////////////////////////////////