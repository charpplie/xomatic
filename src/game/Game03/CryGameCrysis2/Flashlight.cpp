/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Flashlight accessory (re-factored from Crysis L.A.M.)

-------------------------------------------------------------------------
History:
- 11-6-2008   Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "Flashlight.h"
#include "Actor.h"
#include "ItemSharedParams.h"

CFlashLight::CFlashLight():
m_flashlightOn(false),
m_flashlightWasOn(false),
m_aiDynamicSpotLightUpdateTime(0.0f)
{
	m_lightID[0] = m_lightID[1] = -1;
}

CFlashLight::~CFlashLight()
{

}

//-----------------------------------------------------
void CFlashLight::Reset()
{
	CAccessory::Reset();

	TurnOffFlashlight();
}

//-------------------------------------------------
void CFlashLight::OnAttach(bool attach)
{
	if(attach)
	{
		TurnOnFlashlight();
	}
	else
	{
		TurnOffFlashlight();
	}
}

//-----------------------------------------------
void CFlashLight::OnParentSelect(bool select)
{
	if(select)
	{
		if(m_flashlightWasOn)
		{
			TurnOnFlashlight();
			m_flashlightWasOn = false;
		}
	}
	else
	{
		m_flashlightWasOn = m_flashlightOn;
		TurnOffFlashlight();
	}
}

//---------------------------------------------
void CFlashLight::ActivateLight(bool activate)
{
	if(activate)
		TurnOnFlashlight(true);
	else
		TurnOffFlashlight();
}

//---------------------------------------------
void CFlashLight::TurnOnFlashlight(bool force /*= false*/)
{
	if(!m_sharedparams->pFlashlightParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "FLASHLIGHT PARAMS: Item of type CFlashlight is missing it's flashlight params!");
		return;
	}

	if(m_flashlightOn == true)
		return;

	CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId()));
	if(pParentWeapon == NULL)
		return;

	const SAccessoryParams *params = pParentWeapon->GetAccessoryParams(GetEntity()->GetClass()->GetName());
	if (!params)
		return;

	int slot = pParentWeapon->IsOwnerFP() ? eIGS_FirstPerson: eIGS_ThirdPerson;

	//If not forced, check if the owner is a player, AI must request light on throw ActivateLight()
	if(force == false)
	{
		CActor* pOwner = pParentWeapon->GetOwnerActor();
		if(pOwner && !pOwner->IsPlayer())
			return;
	}

	Vec3 color = m_sharedparams->pFlashlightParams->light_color[slot] * m_sharedparams->pFlashlightParams->light_diffuse_mul[slot];
	float specular = 1.0f/m_sharedparams->pFlashlightParams->light_diffuse_mul[slot];

	CryFixedStringT<32> helper = params->attach_helper.c_str();
	if(slot==eIGS_FirstPerson)
		helper.append("_light");
	Vec3 dir(-1,0,0);
	Vec3 localOffset(0.0f,0.0f,0.0f);


	//Assets don't have same orientation for pistol/rifle.. 8/
	dir = m_sharedparams->pFlashlightParams->light_dir[slot];
	dir.Normalize();

	bool fakeLight = false;
	bool castShadows = false;

	//Some MP/SP restrictions for lights
	IRenderNode *pCasterException = NULL;
	if(CActor *pOwner = pParentWeapon->GetOwnerActor())
	{
		if(gEnv->bMultiplayer)
		{
			if(!pOwner->IsClient())
				fakeLight = true;
			else
				castShadows = true;
		}
		else
		{
			if(pOwner->IsPlayer())
				castShadows = true;
		}

		if(castShadows)
		{
			if(IEntityRenderProxy* pRenderProxy = static_cast<IEntityRenderProxy*>(pOwner->GetEntity()->GetProxy(ENTITY_PROXY_RENDER)))
				pCasterException = pRenderProxy->GetRenderNode();
		}
	}

	m_lightID[slot] = pParentWeapon->AttachLightEx(	slot, 
																									0, 
																									true, 
																									fakeLight, 
																									castShadows, 
																									pCasterException, 
																									m_sharedparams->pFlashlightParams->light_range[slot], 
																									color, 
																									specular, 
																									m_sharedparams->pFlashlightParams->light_texture[slot], 
																									m_sharedparams->pFlashlightParams->light_fov[slot], 
																									helper.c_str(), 
																									localOffset, 
																									dir, 
																									m_sharedparams->pFlashlightParams->light_material[slot].c_str(), 
																									m_sharedparams->pFlashlightParams->light_hdr_dyn[slot]);

	GetGameObject()->EnableUpdateSlot(this, eIUS_General);
	m_aiDynamicSpotLightUpdateTime = Random(0.0f, 0.2f);
	m_lastLightRadius = m_sharedparams->pFlashlightParams->light_range[slot];
	m_flashlightOn = true;
}

//--------------------------------------------
void CFlashLight::TurnOffFlashlight()
{
	if(m_flashlightOn == false)
		return;

	CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId()));
	if(pParentWeapon == NULL)
		return;

	for(int i = 0; i < 2; i++)
	{
		if (m_lightID[i] != -1)    
		{
			pParentWeapon->AttachLightEx(i, m_lightID[i], false, true);           
			m_lightID[i] = -1;    
		}   
	}

	GetGameObject()->DisableUpdateSlot(this, eIUS_General);
	m_flashlightOn = false;
}

//-------------------------------------------
void CFlashLight::Update(SEntityUpdateContext& ctx, int slot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if(!m_sharedparams->pFlashlightParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "FLASHLIGHT PARAMS: Item of type CFlashlight is missing it's flashlight params!");
		return;
	}

	const float updateRate = 0.4f;
	if((slot == eIUS_General) && m_flashlightOn)
	{
		RequireUpdate(eIUS_General);

		m_aiDynamicSpotLightUpdateTime += ctx.fFrameTime;

		if(m_aiDynamicSpotLightUpdateTime < updateRate)
			return;
	
		m_aiDynamicSpotLightUpdateTime = Random(0.0f, 0.1f);

		CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId()));
		if(!pParentWeapon)
			return;
	
		int slot = (m_lightID[0] != -1)? eIGS_FirstPerson: eIGS_ThirdPerson;
		const float range = m_sharedparams->pFlashlightParams->light_range[slot];

		Vec3 flPos, flDir;
		GetFlashlightPositionAndDirection(flPos, flDir);

		// Use the same flags as the AI system uses for visibility.
		const int objects = ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid|ent_independent; //ent_living;
		const int flags = (geom_colltype_ray << rwi_colltype_bit) | rwi_colltype_any | (10 & rwi_pierceability_mask) | (geom_colltype14 << rwi_colltype_bit);

		IPhysicalEntity* pSkipEntity = NULL;
		if(pParentWeapon->GetOwner())
			pSkipEntity = pParentWeapon->GetOwner()->GetPhysics();

		float dist = m_sharedparams->pFlashlightParams->light_range[slot];
		ray_hit hit;
		if (gEnv->pPhysicalWorld->RayWorldIntersection(flPos, flDir*range, objects, flags,&hit, 1, &pSkipEntity, pSkipEntity ? 1 : 0))
		{
			dist = hit.dist;
		}

		if(m_lastLightRadius < dist)
			dist += (dist-m_lastLightRadius) * min(1.0f, (ctx	.fFrameTime * 5.0f));

		m_lastLightRadius = dist;

		if (slot == eIGS_ThirdPerson)
		{
			pParentWeapon->SetLightRadius(dist, m_lightID[eIGS_ThirdPerson]);
		}

		float lightAIRange = min(dist, m_sharedparams->pFlashlightParams->light_range[slot]);
		UpdateAIFlashlight(pParentWeapon, flPos, flDir, lightAIRange, m_sharedparams->pFlashlightParams->light_fov[slot]);
	}
}

//--------------------------------------------
void CFlashLight::OnEnterFirstPerson()
{
	CAccessory::OnEnterFirstPerson();

	if(m_flashlightOn)
	{
		TurnOffFlashlight();
		TurnOnFlashlight();
	}
}

//---------------------------------------------
void CFlashLight::OnEnterThirdPerson()
{
	CAccessory::OnEnterThirdPerson();

	if(m_flashlightOn)
	{
		TurnOffFlashlight();
		TurnOnFlashlight();
	}
}

//--------------------------------------------
void CFlashLight::GetFlashlightPositionAndDirection(Vec3& pos, Vec3& dir)
{
	pos = GetEntity()->GetWorldPos();
	dir = GetEntity()->GetWorldRotation().GetColumn1();

	pos += (dir*0.10f);
}

//---------------------------------------------
void CFlashLight::UpdateAIFlashlight(CItem* pParentWeapon, const Vec3& pos, const Vec3& dir, float lightRange, float fov)
{
	if (!gEnv->pAISystem)
		return;

	IAIObject* pUserAI = NULL;
	CActor* pActor = pParentWeapon->GetOwnerActor();
	if (pActor)
		pUserAI = pActor->GetEntity()->GetAI();

	if (lightRange > 0.0001f)
	{
		gEnv->pAISystem->DynSpotLightEvent(pos, dir, lightRange, DEG2RAD(fov)/2, AILE_FLASH_LIGHT, pUserAI->GetEntityID(), 1.0f);
		if (pUserAI)
			gEnv->pAISystem->DynOmniLightEvent(pUserAI->GetPos() + dir*0.75f, 1.5f, AILE_FLASH_LIGHT, pUserAI->GetEntityID(), 2.0f);
	}
}