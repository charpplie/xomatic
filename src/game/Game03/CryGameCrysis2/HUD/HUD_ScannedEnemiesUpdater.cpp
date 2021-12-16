#include "StdAfx.h"
#include "HUD_ScannedEnemiesUpdater.h"

#include "TacticalManager.h"
#include "HUD/HUD.h"

//////////////////////////////////////////////////////////////////////////


CHUD_ScannedEnemiesUpdater::CHUD_ScannedEnemiesUpdater()
{
}



CHUD_ScannedEnemiesUpdater::~CHUD_ScannedEnemiesUpdater()
{
}



void CHUD_ScannedEnemiesUpdater::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_OnSuitModeChanged)
	{
		SetSuitMode(static_cast<ENanoSuitMode>(event.eventIntData));
	}
	else if(event.eventType == eHUDEvent_OnEntityScanned)
	{
		AddEnemy((EntityId)event.GetData(0).GetInt());
	}
}



void CHUD_ScannedEnemiesUpdater::Update(float frameTime)
{
}



void CHUD_ScannedEnemiesUpdater::Draw()
{
}



void CHUD_ScannedEnemiesUpdater::RemoveAllEnemies()
{
	CTacticalManager::TInterestPoints points = g_pGame->GetTacticalManager()->GetTacticalPoints(CTacticalManager::eTacticalEntity_Unit);

	CTacticalManager::TInterestPoints::const_iterator it = points.begin();
	CTacticalManager::TInterestPoints::const_iterator end = points.end();
	for(; it!=end; ++it)
	{
		const CTacticalManager::STacticalInterestPoint &entry = (*it);
		g_pGame->GetHUD()->GetSilhouettes()->ResetSilhouette(entry.m_entityId);
	}
}



void CHUD_ScannedEnemiesUpdater::AddAllEnemies()
{
	CTacticalManager::TInterestPoints points = g_pGame->GetTacticalManager()->GetTacticalPoints(CTacticalManager::eTacticalEntity_Unit);

	CTacticalManager::TInterestPoints::const_iterator it = points.begin();
	CTacticalManager::TInterestPoints::const_iterator end = points.end();
	for(; it!=end; ++it)
	{
		const CTacticalManager::STacticalInterestPoint &entry = (*it);
		if(entry.m_scanned)
		{
			AddEnemy(entry.m_entityId);
		}
	}
}



void CHUD_ScannedEnemiesUpdater::AddEnemy(EntityId entity)
{
	if(m_suitMode!=eNanoSuitMode_Tactical)
		return;

	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entity);
	if(!pActor)
		return;

	g_pGame->GetHUD()->GetSilhouettes()->SetSilhouette(pActor, 0.6015625f, 0.83203125f, 0.71484375f, 1.0f, -1.0f, false);
}



void CHUD_ScannedEnemiesUpdater::SetSuitMode(ENanoSuitMode mode)
{
	if(m_suitMode != mode)
	{
		if(m_suitMode == eNanoSuitMode_Tactical)
		{
			RemoveAllEnemies();
		}

		m_suitMode = mode;

		if(mode == eNanoSuitMode_Tactical)
		{
			AddAllEnemies();
		}
	}
}



//////////////////////////////////////////////////////////////////////////