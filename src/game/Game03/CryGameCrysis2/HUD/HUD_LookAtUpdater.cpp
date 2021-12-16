#include "StdAfx.h"
#include "HUD_LookAtUpdater.h"

#include "HUD.h"
#include "HUDState.h"
#include "IInteractor.h"
#include "IWorldQuery.h"


#define LOOKATUPDATER_TIME_TO_SPOT 1.5f

//////////////////////////////////////////////////////////////////////////


CHUD_LookAtUpdater::CHUD_LookAtUpdater()
: m_lookAtEntity(0)
, m_usableEntity(0)
, m_interactionType(CPlayer::eInteraction_None)
, m_viewDistance(-1)
, m_spottingTimer(0.0f)
{
	Update(0.0f);
}



CHUD_LookAtUpdater::~CHUD_LookAtUpdater()
{
}



void CHUD_LookAtUpdater::Update(float frameTime)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;

	IGameObject* pGameObject = pActor->GetGameObject();
	if(!pGameObject)
		return;

	EntityId lookAt = pGameObject->GetWorldQuery()->GetLookAtEntityId();
	if(lookAt!=m_lookAtEntity)
	{
		SHUDEvent event;
		event.eventType = eHUDEvent_OnLookAtChanged;
		event.AddData(SHUDEventData((int)lookAt));

		CHUD::CallEvent(event);

		m_lookAtEntity = lookAt;
		m_spottingTimer = 0.0f;
	}

	if(lookAt)
	{
		float timer = m_spottingTimer + frameTime;
		if(timer > LOOKATUPDATER_TIME_TO_SPOT)
		{
			SHUDEvent event(eHUDEvent_OnEntitySpotted);
			event.AddData(SHUDEventData((int)lookAt));
			CHUD::CallEvent(event);
			timer -= LOOKATUPDATER_TIME_TO_SPOT;
		}
		m_spottingTimer = timer;
	}

	CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
	if(!pPlayer)
		return;

	const CPlayer::SInteractionInfo& interactionInfo = pPlayer->GetCurrentInteractionInfo();
	if((interactionInfo.interactiveEntityId != m_usableEntity) || (interactionInfo.interactionType != m_interactionType))
	{
		SHUDEvent event;
		event.eventType = eHUDEvent_OnUsableChanged;
		event.eventIntData = (int)interactionInfo.interactiveEntityId;
		event.eventIntData2 = (int)interactionInfo.interactionType;

		CHUD::CallEvent(event);

		m_usableEntity = interactionInfo.interactiveEntityId;
		m_interactionType = interactionInfo.interactionType;
	}

	const CCamera& camera = gEnv->pRenderer->GetCamera();
	IPhysicalEntity *pTargetPhysics = pPlayer->GetEntity()->GetPhysics();

	m_raycastHelper->CastRay(camera.GetPosition(), camera.GetViewdir()*200.f, ent_all, rwi_stop_at_pierceable|rwi_ignore_back_faces, &pTargetPhysics, 1);
	const ray_hit *pHit = m_raycastHelper->GetRayHit();

	int distance = int_round(pHit->dist);
	if(pHit->dist<=0.0f)
		distance = -1;
	if(m_viewDistance != distance)
	{
		m_viewDistance = distance;
		SHUDEvent event;
		event.eventType = eHUDEvent_OnViewDistanceChanged;
		event.AddData(SHUDEventData(m_viewDistance));
		CHUD::CallEvent(event);
	}
}


//////////////////////////////////////////////////////////////////////////