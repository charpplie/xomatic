/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	Silhouettes manager

-------------------------------------------------------------------------
History:
- 20:07:2007: Created by Julien Darre

*************************************************************************/

#include "StdAfx.h"
#include "IActorSystem.h"
#include "IEntitySystem.h"
#include "IItemSystem.h"
#include "IVehicleSystem.h"
#include "HUDSilhouettes.h"
#include "Item.h"

#include "Actor.h"
#include <IUIDraw.h>

#include "NanoSuitDefs.h"
#include "GameCVars.h"
#include "Game.h" 


bool CHUDSilhouettes::s_hasNotYetWarnedAboutVectorSize = true;

//-----------------------------------------------------------------------------------------------------

CHUDSilhouettes::CHUDSilhouettes() : m_nanoVisionActive(true)
{
	CRY_TODO(01, 12, 2009, "Performance: consider using dynamic sized vector w/ reserve instead resize.");
	m_silhouettesVector.resize(32);
}

//-----------------------------------------------------------------------------------------------------

CHUDSilhouettes::~CHUDSilhouettes()
{
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetVisionParams(EntityId uiEntityId,float r,float g,float b,float a)
{
	// When quick loading, entities may have been already destroyed when we do a ShowBinoculars(false)
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(uiEntityId);
	if(!pEntity)
		return;

	// Some actor accessories may not have render proxy
	IEntityRenderProxy *pEntityRenderProxy = static_cast<IEntityRenderProxy *>(pEntity->GetProxy(ENTITY_PROXY_RENDER));
	if(!pEntityRenderProxy)
		return;

	pEntityRenderProxy->SetVisionParams(r,g,b,a);
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetFlowGraphSilhouette(IEntity *pEntity,float r,float g,float b,float a,float fDuration)
{
	if(!pEntity)
		return;

	//Beni - Do not return, we might need to update the color, and SetShilhouette() will do the rest 
	//if(GetFGSilhouette(pEntity->GetId()) != m_silhouettesFGVector.end())
	//{
		//return;
	//}

	SetSilhouette(pEntity, r, g, b ,a, fDuration, true);
	m_silhouettesFGVector[pEntity->GetId()] = Vec3(r,g,b);
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::ResetFlowGraphSilhouette(EntityId uiEntityId)
{
	std::map<EntityId, Vec3>::iterator it = GetFGSilhouette(uiEntityId);
	if(it != m_silhouettesFGVector.end())
	{
		m_silhouettesFGVector.erase(it);
		ResetSilhouette(uiEntityId);
		return; 
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetSilhouette(IEntity *pEntity,float r,float g,float b,float a,float fDuration, bool bFlowGraphRequested /*= false*/)
{
	if(!CanUseSilhouettes(bFlowGraphRequested))
		return;

	if(!pEntity)
		return;

	SSilhouette *pSilhouette = NULL;

	// First pass: is that Id already in a slot?
	for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
	{
		SSilhouette *pSil = &(*iter);
		if(pEntity->GetId() == pSil->uiEntityId)
		{
			pSilhouette = pSil;
			break;
		}
	}

	if(!pSilhouette)
	{
		// Second pass: try to find a free slot
		for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
		{
			SSilhouette *pSil = &(*iter);
			if(!pSil->bValid)
			{
				pSilhouette = pSil;
				break;
			}
		}
	}

	if(s_hasNotYetWarnedAboutVectorSize)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CHUDSilhouettes: Too many silhouettes active, vector size should be increased!");
		s_hasNotYetWarnedAboutVectorSize = false;
	}

	if(pSilhouette)
	{
		pSilhouette->uiEntityId	= pEntity->GetId();
		pSilhouette->fTime = fDuration;
		pSilhouette->bValid			= true;
		pSilhouette->r = r;
		pSilhouette->g = g;
		pSilhouette->b = b;
		pSilhouette->a = a;

		SetVisionParams(pEntity->GetId(),r,g,b,a);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetSilhouette(IActor *pActor,float r,float g,float b,float a,float fDuration,bool bHighlightCurrentItem,bool bHighlightAccessories)
{
	if(!pActor)
		return;

	SetSilhouette(pActor->GetEntity(),r,g,b,a,fDuration);

	if(bHighlightCurrentItem)
	{
		SetSilhouette(pActor->GetCurrentItem(),r,g,b,a,fDuration,bHighlightAccessories);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetSilhouette(IItem *pItem,float r,float g,float b,float a,float fDuration,bool bHighlightAccessories)
{
	if(!pItem)
		return;

	SetSilhouette(pItem->GetEntity(),r,g,b,a,fDuration);
	
	if(bHighlightAccessories)
	{
		const CItem::TAccessoryMap *pAccessoryMap = static_cast<CItem *>(pItem)->GetAttachedAccessories();
		for(CItem::TAccessoryMap::const_iterator iter=pAccessoryMap->begin(); iter!=pAccessoryMap->end(); ++iter)
		{
			SetSilhouette(gEnv->pEntitySystem->GetEntity((*iter).second),r,g,b,a,fDuration);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetSilhouette(IVehicle *pVehicle,float r,float g,float b,float a,float fDuration)
{
	if(!pVehicle)
		return;

	SetSilhouette(pVehicle->GetEntity(),r,g,b,a,fDuration);
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::ResetSilhouette(EntityId uiEntityId)
{
	for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
	{
		SSilhouette *pSilhouette = &(*iter);

		if(pSilhouette->uiEntityId == uiEntityId && pSilhouette->bValid)
		{
			std::map<EntityId, Vec3>::iterator it = GetFGSilhouette(pSilhouette->uiEntityId);
			if(it != m_silhouettesFGVector.end())
			{
				Vec3 color = it->second;
				SetVisionParams(uiEntityId, color.x, color.y, color.z, 1.0f);
			}
			else
			{
				SetVisionParams(pSilhouette->uiEntityId,0,0,0,0);
				pSilhouette->bValid = false;
			}

			return;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::SetType(int iType)
{
	// Exit of binoculars: we need to reset all silhouettes
	if(0 == iType)
	{
		for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
		{
			SSilhouette *pSilhouette = &(*iter);

			if(pSilhouette->bValid)
			{
				std::map<EntityId, Vec3>::iterator it = GetFGSilhouette(pSilhouette->uiEntityId);
				if(it != m_silhouettesFGVector.end())
				{
					Vec3 color = it->second;
					SetVisionParams(pSilhouette->uiEntityId, color.x, color.y, color.z, 1.0f);
				}
				else
				{
					SetVisionParams(pSilhouette->uiEntityId,0,0,0,0);
					pSilhouette->bValid = false;
				}
			}

		}
	}

	gEnv->p3DEngine->SetPostEffectParam("CryVision_Type",(float)(1-iType));
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::Update(float frameTime)
{
	for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
	{
		SSilhouette *pSilhouette = &(*iter);

		if(pSilhouette->bValid && pSilhouette->fTime != -1)
		{
			pSilhouette->fTime -= frameTime;
			if(pSilhouette->fTime < 0.0f)
			{
				SetVisionParams(pSilhouette->uiEntityId,0,0,0,0);
				pSilhouette->bValid = false;
				pSilhouette->fTime = 0.0f;
			}
			else if (pSilhouette->fTime < 1.0f)
			{
				// fade out for the last second
				float scale = pSilhouette->fTime ;
				scale *= scale;
				SetVisionParams(pSilhouette->uiEntityId,pSilhouette->r*scale,pSilhouette->g*scale,pSilhouette->b*scale,pSilhouette->a*scale);
			}
		}
	}

/*
	//Iterate again to render the debug info 
	//It is done like this, for efficiency using the UIDraw and AuxRenderGeometry (this should be redone completely anyways)
	IRenderAuxGeom* pAuxRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags oldFlags = pAuxRenderer->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	pAuxRenderer->SetRenderFlags(newFlags);

	for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
	{
		SSilhouette *pSilhouette = &(*iter);

		if(pSilhouette->bValid && pSilhouette->fTime != -1)
		{
			//Debug infiltration mode extra info
			if(pSilhouette->debugInfiltrationInfo.bEnabled)
			{
				pSilhouette->debugInfiltrationInfo.fTime -= frameTime;
				if(pSilhouette->debugInfiltrationInfo.fTime <= 0.0f)
				{
					pSilhouette->debugInfiltrationInfo.bEnabled = false;
				}
				else
				{
					DrawDebugInfiltrationInfo(*pSilhouette);
				}
			}
			else if(pSilhouette->debugCombatInfo.bEnabled)
			{
				pSilhouette->debugCombatInfo.fTime -= frameTime;
				if(pSilhouette->debugCombatInfo.fTime <= 0.0f)
				{
					pSilhouette->debugCombatInfo.bEnabled = false;
				}
				else
				{
					DrawDebugCombatInfo(*pSilhouette);
				}
			}
		}
	}

	pAuxRenderer->SetRenderFlags(oldFlags);
*/

	if(m_nanoVisionActive)
		UpdateNanoVision(frameTime);
}

//-----------------------------------------------------------------------------------------------------

void CHUDSilhouettes::Serialize(TSerialize &ser)
{
	if(ser.IsReading())
		m_silhouettesFGVector.clear();

	int amount = m_silhouettesFGVector.size();
	ser.Value("amount", amount);
	if(amount)
	{
		EntityId id = 0;
		Vec3 color;
		if(ser.IsWriting())
		{
			std::map<EntityId, Vec3>::iterator it = m_silhouettesFGVector.begin();
			std::map<EntityId, Vec3>::iterator end = m_silhouettesFGVector.end();
			for(; it != end; ++it)
			{
				ser.BeginGroup("FlowGraphSilhouette");
				id = it->first;
				color = it->second;
				ser.Value("id", id);
				ser.Value("color", color);
				ser.EndGroup();
			}
		}
		else if(ser.IsReading())
		{
			for(int i = 0; i < amount; ++i)
			{
				ser.BeginGroup("FlowGraphSilhouette");
				ser.Value("id", id);
				ser.Value("color", color);
				ser.EndGroup();
				if(IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id))
					SetFlowGraphSilhouette(pEntity, color.x, color.y, color.z, 1.0f, -1.0f);
			}
		}
	}
}

//----------------------------------------------------
bool CHUDSilhouettes::CanUseSilhouettes(bool bFlowGraphRequested /*= false*/) const
{
	//Allow always from FlowGraph
	if(bFlowGraphRequested)
		return true;

	IActor* pActor = g_pGame->GetIGameFramework()->GetClientActor();
	CActor* pCActor = static_cast<CActor*>(pActor);

	if(pCActor && pCActor->GetNanoSuit() && pCActor->GetNanoSuit()->GetGameParams().GetState() != eNanoSuitState_Disabled)
	{
		return true;
	}
	else if(pActor)
	{
		return true;
	}

	return false;
}

//------------------------------------------------------
void CHUDSilhouettes::UpdateNanoVision(float frameTime)
{
	const float updateTime = 1.0f;
	const float maxEnemyDistance = 100.0f;

	if(gEnv->pTimer->GetAsyncCurTime() - m_lastNanoVisionUpdate < updateTime)
		return;

	m_lastNanoVisionUpdate = gEnv->pTimer->GetAsyncCurTime();

	IActor* pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor == NULL)
		return;

	CActor *pActor = static_cast<CActor*>(pClientActor);
	ENanoSuitMode suitMode = pActor->GetActorSuitGameParameters().GetMode();
	
	if(suitMode == eNanoSuitMode_Tactical)
	{
		UpdateNanoVisionTactical(frameTime);
	}

/*
	ESuitMode suitMode = eSuitMode_Last;
	float fHealth = 0.0f;

	{
		CActor2* pClientActor2 = static_cast<CActor2*>(pClientActor);
		const ESuitMode *clientSuitMode = pClientActor2->GetFrameworkData<ESuitMode>(ComponentDataRegistry::Nanosuit.mode);
		if(clientSuitMode)
			suitMode = *clientSuitMode;
		const float *pHealth = pClientActor2->GetFrameworkData<float>(ComponentDataRegistry::State.health);
		if(pHealth)
			fHealth = *pHealth;
	}

	IActorIteratorPtr pActorIt = g_pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	if(pActorIt == NULL)
		return;

	Matrix34 camMatrix = gEnv->pSystem->GetViewCamera().GetMatrix();
	while (IActor *pActor = pActorIt->Next())
	{
		if(pActor->IsClient())
			continue;

		if(fHealth <= 0)
			continue;

		if(IEntity* pEntity = pActor->GetEntity())
		{
			// Hidden
			if(pEntity->IsHidden())
				continue;

			const Vec3 actorPos = pEntity->GetWorldPos();
			Vec3 diff = (actorPos - camMatrix.GetTranslation());

			// Too far away
			if(diff.len2() > maxEnemyDistance * maxEnemyDistance)
				continue;

			// In view
			diff.Normalize();
			if(diff.Dot(camMatrix.GetColumn1()) < 0.0f)
				continue;

			if(suitMode == eSuitMode_Combat)
				SetSilhoutteInCombatMode(pClientActor->GetEntity(), pEntity);
			else if(suitMode == eSuitMode_Infiltration)
				SetSilhouetteInInfiltrationMode(pClientActor->GetEntity(), pEntity);
			else if(suitMode == eSuitMode_Tactical)
				SetSilhouetteInTacticalMode(pClientActor->GetEntity(), pEntity);

		}
	}
*/
}

//--------------------------------------------------------------------
void CHUDSilhouettes::UpdateNanoVisionTactical(float frameTime)
{
	IActor* pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor == NULL)
		return;

	const float sizeX = 10.0f;
	const float sizeY = 20.0f;
	const float sizeZup = 8.25f;
	const float sizeZdown = 1.75f;
	const Vec3  playerPos = pClientActor->GetEntity()->GetWorldPos();

	IItemSystem* pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
	assert(pItemSystem);

	SEntityProximityQuery query;
	query.box = AABB(Vec3(playerPos.x - sizeX, playerPos.y - sizeY, playerPos.z-sizeZdown),
		Vec3(playerPos.x +sizeX, playerPos.y + sizeY, playerPos.z + sizeZup));

	gEnv->pEntitySystem->QueryProximity(query);

	for (int i = 0; i < query.nCount; ++i)
	{
		EntityId id = query.pEntities[i]->GetId();
		CItem* pItem = static_cast<CItem*>(pItemSystem->GetItem(id));
	
		if(pItem)
		{
			if(!pItem->IsMounted() && (pItem->GetOwnerId() == 0))
			{
				ColorF itemColor = pItem->GetSilhouetteColor();
				SetSilhouette(pItem, itemColor.r, itemColor.g, itemColor.b, itemColor.a, 2.5f);
			}
		}

	}

}

//---------------------------------------------------------------------
void CHUDSilhouettes::SetSilhoutteInCombatMode(IEntity* pClientEntity, IEntity* pTargetEntity)
{
}

//------------------------------------------------------------------
void CHUDSilhouettes::SetSilhouetteInInfiltrationMode(IEntity* pClientEntity, IEntity* pTargetEntity)
{
	IAIObject* pAIObject = pTargetEntity->GetAI();
	
	if(pAIObject != NULL)
	{
		if(pAIObject->IsHostile(pClientEntity->GetAI(), false))
		{
			
			/*
			int targetAIGroup = pAIObject->GetGroupId();
			int	maxAlertness = 0;
			IAISystem* pAISystem = gEnv->pAISystem;
			
			int groupCount = pAISystem->GetGroupCount(targetAIGroup, IAISystem::GROUP_ENABLED);
			for (int i = 0; i < n; i++)
			{
				IAIObject*	pMember = pAISystem->GetGroupMember(groupID, i, IAISystem::GROUP_ENABLED);
				if (pMember && pMember->GetProxy())
					maxAlertness = max(maxAlertness, pMember->GetProxy()->GetAlertnessState());
			}

			//SAIDetectionLevels detectionLevels;
			//gEnv->pAISystem->GetDetectionLevels(0, detectionLevels);
			//float currentAwareness = max(detectionLevels.vehicleThreat, detectionLevels.puppetThreat);

			//SetSilhouette(pTargetEntity, currentAwareness,0,0,1.0f, 2.5f);

			*/

			//NOTE: Consider using awareness instead, or a combination of alertness and awareness
			//      to get the puttet state
			//      It seems that the IDLE state it doesn't match always the behaviour you see on screen
			const int alertness = pAIObject->GetProxy() ? pAIObject->GetProxy()->GetAlertnessState() : 0;

			//Always blue in infiltration mode
			SetSilhouette(pTargetEntity, 0.10588f, 0.10588f, 0.89411f, 1.0f, 2.5f, false);	

			SDebugInfiltrationInfo debugIcon;
			debugIcon.fTime = 2.0f;

			switch(alertness)
			{
				//case 0: //IDLE, NOT ALERTED
					//debugIconColor.Set(0.10588f, 0.89411f, 0.10588f);
					//break;
				
				case 1: //ALERTED
					debugIcon.bEnabled = true;
					debugIcon.r = 0.7411f; debugIcon.g = 0.7411f; debugIcon.b = 0.10588f; debugIcon.a = 1.0f;
					break;

				case 2: //COMBAT
					debugIcon.bEnabled = true;
					debugIcon.r = 0.89411f; debugIcon.g = 0.10588f; debugIcon.b = 0.10588f; debugIcon.a = 1.0f;
					break;
			}

			SetDebugInfiltrationInfo(pTargetEntity, debugIcon);
		}
	}
}

//-----------------------------------------------------
void CHUDSilhouettes::SetSilhouetteInTacticalMode(IEntity* pClientEntity, IEntity* pTargetEntity)
{
	IAIObject* pAIObject = pTargetEntity->GetAI();

	//Benito - Hacky work-around to prevent the fp weapon being highlighted, this code will go away anyways
	if (((pClientEntity->GetPos() - pTargetEntity->GetPos()).len2() < 5.0f) && (pTargetEntity->GetPhysics() == NULL))
		return;

	if(pAIObject != NULL)
	{
		if(pAIObject->IsHostile(pClientEntity->GetAI(), false))
			SetSilhouette(pTargetEntity, 0.89411f,0.89411f,0.10588f,1.0f, 2.5f);	
	}
		SetSilhouette(pTargetEntity, 0.89411f,0.89411f,0.10588f,1.0f, 2.5f);	
}

//------------------------------------------------------
void CHUDSilhouettes::SetDebugInfiltrationInfo(IEntity *pEntity, const SDebugInfiltrationInfo& debugIconInfo)
{
	if(!pEntity)
		return;

	SSilhouette *pSilhouette = NULL;

	// First pass: only if the silhoutte is there we can add the icon
	for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
	{
		SSilhouette *pSil = &(*iter);
		if(pEntity->GetId() == pSil->uiEntityId)
		{
			pSilhouette = pSil;
			break;
		}
	}

	//We have a silhoutte already, add the debug icon info
	if(pSilhouette)
	{
		pSilhouette->debugInfiltrationInfo = debugIconInfo;
	}
}

//------------------------------------------------------
void CHUDSilhouettes::SetDebugCombatInfo(IEntity *pEntity, const SDebugCombatInfo& debugCombatInfo)
{
	if(!pEntity)
		return;

	SSilhouette *pSilhouette = NULL;

	// First pass: only if the silhoutte is there we can add the icon
	for(TSilhouettesVector::iterator iter=m_silhouettesVector.begin(); iter!=m_silhouettesVector.end(); ++iter)
	{
		SSilhouette *pSil = &(*iter);
		if(pEntity->GetId() == pSil->uiEntityId)
		{
			pSilhouette = pSil;
			break;
		}
	}

	//We have a silhoutte already, add the debug icon info
	if(pSilhouette)
	{
		pSilhouette->debugCombatInfo = debugCombatInfo;
	}
}

//------------------------------------------------------
void CHUDSilhouettes::DrawDebugInfiltrationInfo(const SSilhouette& silhoutte)
{
	IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(silhoutte.uiEntityId);

	if(pTargetEntity)
	{
		const Vec3 conePos = pTargetEntity->GetPos() + (Vec3Constants<float>::fVec3_OneZ * 2.0f);
		const float coneRadius = 0.12f;
		const float coneHeight = 0.25f;

		ColorB coneColor;
		coneColor.r = (uint8) (silhoutte.debugInfiltrationInfo.r * 255.0f);
		coneColor.g = (uint8) (silhoutte.debugInfiltrationInfo.g * 255.0f);
		coneColor.b = (uint8) (silhoutte.debugInfiltrationInfo.b * 255.0f);
		coneColor.a = (silhoutte.debugInfiltrationInfo.fTime < 1.0f) ? (uint8) (silhoutte.debugInfiltrationInfo.fTime * 128) : 128;

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(conePos, -Vec3Constants<float>::fVec3_OneZ, coneRadius, coneHeight, coneColor);
	}
}

//------------------------------------------------------
void CHUDSilhouettes::DrawDebugCombatInfo(const SSilhouette& silhoutte)
{
	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(silhoutte.uiEntityId);
	IActor* pClientActor = g_pGame->GetIGameFramework()->GetClientActor();

	const CActor* pCActor = static_cast<const CActor*>(pActor);

	if(pCActor && pClientActor)
	{
		Vec3 iconPos = pCActor->GetEntity()->GetPos() + (Vec3Constants<float>::fVec3_OneZ * 2.0f);

		ColorB color;
		color.r = 255;
		color.g = 0;
		color.b = 0;
		color.a = (silhoutte.debugCombatInfo.fTime < 1.0f) ? (uint8) (silhoutte.debugCombatInfo.fTime * 192) : 192;

		//Draw icon
		int characterType = pCActor->GetActorSpecies();

		if(characterType == eGCT_HUMAN)
		{
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(iconPos, 0.075f, color);
		}
		else if(characterType == eGCT_SPOTTER)
		{
			iconPos -= Vec3(0.0f, 0.0f, 1.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(iconPos, 0.075f, color);
		}
		else if(characterType == eGCT_PINGER)
		{
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawCylinder(iconPos, Vec3(0.0f, 0.0f, 1.0f), 0.075f, 0.15f, color);
		}

		Vec3 playerToIconDir = iconPos - pClientActor->GetEntity()->GetPos();
		playerToIconDir.Normalize();

		if(playerToIconDir.Dot(pClientActor->GetEntity()->GetWorldTM().GetColumn1()) < 0.1f)
			return;

		//Draw health bar
		/*IUIDraw* pUIDraw = g_pGame->GetIGameFramework()->GetIUIDraw();

		if(pUIDraw)
		{
			float health = (float)pOldActor->GetHealth();
			float maxHelath = (float)pOldActor->GetMaxHealth();
			float healthFraction = CLAMP(health / maxHelath, 0.0f, 1.0f);

			const float barWidth = 35.0f;
			const float barHeight = 6.0f;

			Vec3 screenPos;
			gEnv->pRenderer->ProjectToScreen(iconPos.x, iconPos.y, iconPos.z - 0.25f, &screenPos.x, &screenPos.y, &screenPos.z);
			screenPos.x = screenPos.x / 100.0f * 800.0f;
			screenPos.y = screenPos.y / 100.0f * 600.0f;

			pUIDraw->PreRender();
			gEnv->pRenderer->Draw2dImage(screenPos.x, screenPos.y, barWidth, barHeight, 0, 0, 0, 1, 1, 0, 0.5f, 0.5f, 0.5f, color.a);
			gEnv->pRenderer->Draw2dImage(screenPos.x, screenPos.y, barWidth * healthFraction, barHeight, 0, 0, 0, 1, 1, 0, color.r, color.g, color.b, color.a, 0.9f);
			pUIDraw->PostRender();

		}*/

		//pAuxRenderer->DrawCone(conePos, -Vec3Constants<float>::fVec3_OneZ, coneRadius, coneHeight, coneColor);

	}
}

//-------------------------------------------------------------
std::map<EntityId, Vec3>::iterator CHUDSilhouettes::GetFGSilhouette(EntityId id)
{
	std::map<EntityId, Vec3>::iterator returnVal = m_silhouettesFGVector.end();

	std::map<EntityId, Vec3>::iterator it = m_silhouettesFGVector.begin();
	std::map<EntityId, Vec3>::iterator end = m_silhouettesFGVector.end();
	for(; it != end; ++it)
	{
		if(it->first == id)
			returnVal = it;
	}
	return returnVal;
}
