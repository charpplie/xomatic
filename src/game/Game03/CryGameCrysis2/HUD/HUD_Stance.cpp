#include "StdAfx.h"
#include "HUD_Stance.h"


//////////////////////////////////////////////////////////////////////////


CHUD_Stance::CHUD_Stance()
: m_currentStance(STANCE_NULL)
, m_objectStance(NULL)
{
}



CHUD_Stance::~CHUD_Stance()
{
}



void CHUD_Stance::Init()
{
	HUD_FLASVAROBJ_REG( GetAsset(), "Root_Radar.StanceIndicator", m_objectStance);
}



void CHUD_Stance::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectStance);
}



void CHUD_Stance::Update(float frameTime)
{
	CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
	if(!pActor)
		return;

	const EStance stance = pActor->GetStance();
	if(m_currentStance != stance)
	{
		m_currentStance = stance;
		m_objectStance->GotoAndStop( (stance==STANCE_CROUCH) ? 2 : 1);
	}
}


//////////////////////////////////////////////////////////////////////////