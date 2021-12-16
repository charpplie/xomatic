#include "StdAfx.h"
#include "HUD_Message.h"

CHUD_Message::CHUD_Message()
: m_gameStateMessage(NULL)
{
	// ...
}

void CHUD_Message::Init() 
{
	m_gameStateMessage = GetAsset("GameStateMessage");
	SetMessage("");
}

void CHUD_Message::Draw() 
{
	assert(m_gameStateMessage);
	m_gameStateMessage->Draw();
}

void CHUD_Message::OnHUDEvent(const SHUDEvent& hudMsgEvent) 
{
	assert(m_gameStateMessage);

	switch( hudMsgEvent.eventType )
	{
	case eHUDEvent_OnSetGameStateMessage :		
		SetMessage(static_cast<const char*>(hudMsgEvent.eventPtrData));
		break;
	}
}

void CHUD_Message::SetMessage( const char* pString )
{
	m_gameStateMessage->SetVariable("_root.text_bg.dynamic_text.text", pString );
	m_gameStateMessage->SetVariable("_root.text_fg.dynamic_text.text", pString );
}