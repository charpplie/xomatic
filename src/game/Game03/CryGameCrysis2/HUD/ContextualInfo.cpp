#include "StdAfx.h"
#include "ContextualInfo.h"

#include "IAgent.h"
#include "HUDAsset.h"
#include "GameCVars.h"
#include "IAIObject.h"


//////////////////////////////////////////////////////////////////////////


CContextualInfo::CContextualInfo()
: m_health(0.0f)
, m_alertness(0)
, m_pAsset(NULL)
{
}



CContextualInfo::~CContextualInfo()
{
}



void CContextualInfo::Update(const float x, const float y)
{
	if(!g_pGameCVars->hud_ContextualHealthIndicator)
		return;

	if(!m_pAsset)
		return;

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_target);
	if(!pEntity)
		return;

	m_pAsset->SetVariable( "Contextual._x", x);
	m_pAsset->SetVariable( "Contextual._y", y);

	if(!gEnv->bMultiplayer)
	{
		if(IAIObject* pAIObject = pEntity->GetAI())
		{
			if(IAIActorProxy* pAIActorProxy = pAIObject->GetProxy())
			{
				const int alertness = pAIActorProxy->GetAlertnessState();
				if(m_alertness != alertness)
				{
					m_alertness = alertness;
					m_pAsset->Invoke("Contextual.Alertness.gotoAndStop", m_alertness+1);
				}
			}
		}
	}

	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_target);
	if(!pActor)
		return;

	float health = (float)pActor->GetHealth();
	float maxhealth = (float)pActor->GetMaxHealth();

	float ratio = health / max(maxhealth, 1.0f);
	if(m_health != ratio)
	{
		m_health = ratio;
		m_pAsset->Invoke("Contextual.Health.gotoAndStop", int_round(ratio * 100.0f));
	}
}



void CContextualInfo::SetTarget(const EntityId entity)
{
	if(gEnv->bMultiplayer)
		return;

	if(!g_pGameCVars->hud_ContextualHealthIndicator)
		return;

	m_target = entity;
	if(m_pAsset)
	{
		m_pAsset->SetVariable("Contextual._visible", m_target!=0);
	}
}



void CContextualInfo::SetSuitMode(const ENanoSuitMode mode)
{
	if(m_pAsset)
	{
		m_pAsset->SetVariable("Contextual.Health._visible", mode==eNanoSuitMode_Power);
		m_pAsset->SetVariable("Contextual.Alertness._visible", mode==eNanoSuitMode_Stealth);
	}
}


//////////////////////////////////////////////////////////////////////////