/*
	SaveLoad Icon/Text hud object, Jan Mueller, 2010
*/

#include "StdAfx.h"
#include "HUD_SaveLoadDisplay.h"

const static float MIN_TIME_SAVE_LOAD_DISPLAY = 3.0f;

//////////////////////////////////////////////////////////////////////////
CHUD_SaveLoadDisplay::CHUD_SaveLoadDisplay() : m_fSaveLoadTimer(0.0f)
{

}

//////////////////////////////////////////////////////////////////////////
CHUD_SaveLoadDisplay::~CHUD_SaveLoadDisplay()
{

}

//////////////////////////////////////////////////////////////////////////
void CHUD_SaveLoadDisplay::Update( float frameTime )
{
	if(m_fSaveLoadTimer > 0.0f)
	{
		if(gEnv->pSystem->IsSerializingFile())
		{
			m_fSaveLoadTimer = MIN_TIME_SAVE_LOAD_DISPLAY;
		}
		else
		{
			//disable save/load icon after timeout
			const float fCurTimer = m_fSaveLoadTimer-frameTime;
			if(fCurTimer <= 0.0f)
			{
				GetAsset2D()->SetVariable("SaveProgress._visible", false);
			}
			m_fSaveLoadTimer = fCurTimer;
		}

	}
}

//////////////////////////////////////////////////////////////////////////
void CHUD_SaveLoadDisplay::Draw( void )
{

}

//////////////////////////////////////////////////////////////////////////
void CHUD_SaveLoadDisplay::Init( void )
{
	
}

//////////////////////////////////////////////////////////////////////////
void CHUD_SaveLoadDisplay::OnHUDEvent( const SHUDEvent& event )
{
	if(event.eventType == eHUDEvent_OnGameSave)
	{
		GetAsset2D()->SetVariable("SaveProgress._visible", true);
		m_fSaveLoadTimer = MIN_TIME_SAVE_LOAD_DISPLAY;
	}
}

//////////////////////////////////////////////////////////////////////////
void CHUD_SaveLoadDisplay::PreDelete( void )
{

}
