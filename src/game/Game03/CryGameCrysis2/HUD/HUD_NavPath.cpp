#include "StdAfx.h"
#include "HUD_NavPath.h"

#include "Player.h"
#include "PlayerNavPath.h"
#include "HUD.h"
#include "Graphics/2DRenderUtils.h"
#include "HUDMissionObjectiveSystem.h"


//////////////////////////////////////////////////////////////////////////


CHUD_NavPath::CHUD_NavPath()
: m_enabled(false)
{
}



CHUD_NavPath::~CHUD_NavPath()
{
}



void CHUD_NavPath::Update(float frameTime)
{
	if(!m_enabled)
		return;

	CPlayer* pLocalPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
	if(!pLocalPlayer)
		return;

	CPlayerNavPath* pPath = pLocalPlayer->GetNavPath();
	if(!pPath)
		return;
	
	if(!pPath->HasValidPath())
		return;

	const ColorF color(1.0f, 0.75f, 0.0f, 1.0f);

	const CPlayerNavPathAgent::TNavPath& path = pPath->GetPath();

	IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

	CPlayerNavPathAgent::TNavPath::const_iterator it_path = path.begin();
	CPlayerNavPathAgent::TNavPath::const_iterator end_path = path.end();
	for(; it_path!=end_path; ++it_path)
	{
		pAuxGeom->DrawSphere((*it_path), 0.2f, color);
	}
}



void CHUD_NavPath::Draw()
{
}



void CHUD_NavPath::OnHUDEvent(const SHUDEvent& event)
{
	switch (event.eventType)
	{
	case eHUDEvent_OnObjectiveChanged:
		{
			CHUDMissionObjective* pObjective = static_cast<CHUDMissionObjective*>(event.GetData(0).GetPtr());
			if(pObjective)
				ObjectiveChanged(pObjective);
		}
		break;
	default:
		{
			CRY_ASSERT_MESSAGE(event.eventType==eHUDEvent_OnSuitModeChanged, "HUD_NavPath received unrecognised event");
			m_enabled = static_cast<ENanoSuitMode>(event.eventIntData) == eNanoSuitMode_Tactical;
		}
		break;
	}
}



void CHUD_NavPath::ObjectiveChanged(CHUDMissionObjective* pObjective)
{
	EntityId id = pObjective->GetTrackedEntity();
	if(pObjective->IsSecondary() || !pObjective->IsActivated() || !id)
		return;

	CPlayer* pLocalPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
	if(!pLocalPlayer)
		return;

	CPlayerNavPath* pPath = pLocalPlayer->GetNavPath();
	if(!pPath)
		return;

	if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id))
	{
		pPath->SetTarget(pEntity->GetPos());
	}
}



//////////////////////////////////////////////////////////////////////////