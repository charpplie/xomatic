/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Laser accessory (re-factored from Crysis L.A.M.)

-------------------------------------------------------------------------
History:
- 11-6-2008   Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "Laser.h"
#include "Actor.h"
#include "Weapon.h"
#include "Game.h"
#include "ItemSharedParams.h"
#define LASER_UPDATE_TIME_FP	0.1f
#define LASER_UPDATE_TIME_TP	0.2f

CLaser::CLaser():
m_laserOn(false),
m_laserWasOn(false),
m_laserEntityId(0),
m_laserDotSlot(-1),
m_laserGeometrySlot(-1)
{
	
}

CLaser::~CLaser()
{
	DestroyLaserEntity();
}

//-----------------------------------------------------
void CLaser::Reset()
{
	CAccessory::Reset();

	TurnOffLaser();
	DrawSlot(eIGS_ThirdPersonAux, false);
	DrawSlot(eIGS_Aux1,false);
	m_laserHelperFP.clear();
}

//---------------------------------------------------
bool CLaser::Init(IGameObject * pGameObject )
{
	if(!CAccessory::Init(pGameObject))
		return false;

	CreateLaserEntity();

	return true;
}

//-------------------------------------------------
void CLaser::OnAttach(bool attach)
{
	if(attach)
	{
		TurnOnLaser();
	}
	else
	{
		TurnOffLaser();
	}
}

//-----------------------------------------------
void CLaser::OnParentSelect(bool select)
{
	if(select)
	{
		if(m_laserWasOn)
		{
			TurnOnLaser();
			m_laserWasOn = false;
		}
		else if(CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId())))
		{
			if(pParentWeapon->IsOwnerFP())
				SwitchFirstPersonGeometry(false, pParentWeapon);
		}
	}
	else
	{
		m_laserWasOn = m_laserOn;
		TurnOffLaser();
	}
}

//---------------------------------------------
void CLaser::ActivateLaser(bool activate)
{
	if(activate)
		TurnOnLaser(true);
	else
		TurnOffLaser(true);
}

//---------------------------------------------
void CLaser::TurnOnLaser(bool manual /*= false*/)
{
	if(!m_sharedparams->pLaserParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "LASER PARAMS: Item of type CLaser is missing it's laser params!");
		return;
	}
	
	if(m_laserOn == true)
		return;

	CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId()));
	if(pParentWeapon == NULL)
		return;
	int slot = pParentWeapon->IsOwnerFP() ? eIGS_FirstPerson: eIGS_ThirdPerson;

	//If not forced, check if the owner is a player, AI must request light on throw ActivateLight()
	if(manual == false)
	{
		CActor* pOwner = pParentWeapon->GetOwnerActor();
		if(pOwner && !pOwner->IsPlayer())
			return;
	}

	SetLaserEntitySlots(false, slot == eIGS_FirstPerson);

	if(slot == eIGS_FirstPerson)
	{
		const SAccessoryParams *params = pParentWeapon->GetAccessoryParams(GetEntity()->GetClass()->GetName());
		if (!params)
			return;

		m_laserHelperFP.clear();
		m_laserHelperFP = params->attach_helper.c_str();
		m_laserHelperFP.replace("_LAM","");

		if(manual)
			SwitchFirstPersonGeometry(true, pParentWeapon);
	}

	GetGameObject()->EnableUpdateSlot(this, eIUS_General);
	m_laserUpdateTimer = (slot == eIGS_FirstPerson)? LASER_UPDATE_TIME_FP: LASER_UPDATE_TIME_TP;
	m_lastLaserLength = m_sharedparams->pLaserParams->laser_range[slot];
	m_laserOn = true;

	//Turn off crosshair
	if(slot == eIGS_FirstPerson)
	{
		SAFE_HUD_FUNC(GetCrosshair()->Hide(true));
	}
}

//--------------------------------------------
void CLaser::TurnOffLaser(bool manual /*= false*/)
{
	if(m_laserOn == false)
		return;

	SetLaserEntitySlots(true);
	
	GetGameObject()->DisableUpdateSlot(this, eIUS_General);
	m_laserOn = false;

	CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId()));
	bool ownerIsFP = pParentWeapon? pParentWeapon->IsOwnerFP(): false;

	if(manual && ownerIsFP)
	{
		SwitchFirstPersonGeometry(false, pParentWeapon);
	}
	//Turn on crosshair
	if(ownerIsFP)
	{
		SAFE_HUD_FUNC(GetCrosshair()->Hide(false));
	}

}

//------------------------------------------
void CLaser::SwitchFirstPersonGeometry(bool laserOn, CItem* pParentWeapon)
{
	const SAccessoryParams *params = pParentWeapon->GetAccessoryParams(GetEntity()->GetClass()->GetName());
	if(!params)
		return;

	DrawSlot(eIGS_FirstPerson,false);
	DrawSlot(eIGS_Aux1,false);

	if(laserOn)
	{
		pParentWeapon->ResetCharacterAttachment(eIGS_FirstPerson, params->attach_helper.c_str(), params->attachToOwner);
		pParentWeapon->SetCharacterAttachment(eIGS_FirstPerson, params->attach_helper, GetEntity(), eIGS_FirstPerson, params->attachToOwner);
	}
	else
	{
		pParentWeapon->ResetCharacterAttachment(eIGS_FirstPerson, params->attach_helper.c_str(), params->attachToOwner);
		pParentWeapon->SetCharacterAttachment(eIGS_FirstPerson, params->attach_helper, GetEntity(), eIGS_Aux1, params->attachToOwner);
	}
}

//--------------------------------------------
void CLaser::CreateLaserEntity()
{
	if(m_laserEntityId)
	{
		//Check if entity is valid
		IEntity *pEntity = m_pEntitySystem->GetEntity(m_laserEntityId);
		if(!pEntity)
			m_laserEntityId = 0;
	}

	if (!m_laserEntityId)
	{
		SEntitySpawnParams spawnParams;
		spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
		spawnParams.sName = "LAMLaser";
		spawnParams.nFlags = (GetEntity()->GetFlags() | ENTITY_FLAG_NO_SAVE) & ~ENTITY_FLAG_CASTSHADOW;

		IEntity *pNewEntity =m_pEntitySystem->SpawnEntity(spawnParams);

		if(pNewEntity)
		{
			m_laserEntityId = pNewEntity->GetId();

			if(IEntity* pEntity = GetEntity())
				pEntity->AttachChild(pNewEntity);

			IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pNewEntity->GetProxy(ENTITY_PROXY_RENDER);
			IRenderNode * pRenderNode = pRenderProxy?pRenderProxy->GetRenderNode():NULL;

			if(pRenderNode)
				pRenderNode->SetRndFlags(ERF_RENDER_ALWAYS,true);
		}
	}

}

//-----------------------------------------
void CLaser::SetLaserEntitySlots(bool freeSlots, bool firstPerson /* = true */)
{
	if(!m_sharedparams->pLaserParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "LASER PARAMS: Item of type CLaser is missing it's laser params!");
		return;
	}

	IEntity* pLaserEntity = m_pEntitySystem->GetEntity(m_laserEntityId);
	if(!pLaserEntity)
		return;

	if(freeSlots)
	{
		if(m_laserDotSlot != -1)
			pLaserEntity->FreeSlot(m_laserDotSlot);
		if(m_laserGeometrySlot != -1)
			pLaserEntity->FreeSlot(m_laserGeometrySlot);

		m_laserDotSlot = m_laserGeometrySlot = -1;
	}
	else
	{
		if(firstPerson)
		{
			IParticleEffect * pEffect = gEnv->pParticleManager->FindEffect(m_sharedparams->pLaserParams->laser_dot[0].c_str());
			if(pEffect)
			{
				m_laserDotSlot = pLaserEntity->LoadParticleEmitter(-1,pEffect);
			}
		}
		else
		{
			m_laserGeometrySlot = pLaserEntity->LoadGeometry(-1, m_sharedparams->pLaserParams->laser_geometry_tp.c_str());
			IParticleEffect * pEffect = gEnv->pParticleManager->FindEffect(m_sharedparams->pLaserParams->laser_dot[1].c_str());
			pLaserEntity->SetSlotFlags(m_laserGeometrySlot, pLaserEntity->GetSlotFlags(m_laserGeometrySlot)|ENTITY_SLOT_RENDER);
			if(pEffect)
			{
				m_laserDotSlot = pLaserEntity->LoadParticleEmitter(-1,pEffect);
			}
		}
	}
}

//------------------------------------------
void CLaser::DestroyLaserEntity()
{
	if (m_laserEntityId)
		gEnv->pEntitySystem->RemoveEntity(m_laserEntityId);
	m_laserEntityId = 0;
}

//-------------------------------------------
void CLaser::Update(SEntityUpdateContext& ctx, int slot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if((slot == eIUS_General) && m_laserOn)
	{
		RequireUpdate(eIUS_General);

		m_laserUpdateTimer += ctx.fFrameTime;

		CItem* pParentWeapon = static_cast<CItem*>(m_pItemSystem->GetItem(GetParentId()));
		if(!pParentWeapon)
			return;		

		if(pParentWeapon->GetStats().fp)
			UpdateFirstPerson(pParentWeapon);
		else
			UpdateThirdPerson(pParentWeapon);

	}
}

//--------------------------------------------
void CLaser::UpdateFirstPerson(CItem* pParentWeapon)
{
	if(!m_sharedparams->pLaserParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "LASER PARAMS: Item of type CLaser is missing it's laser params!");
		return;
	}

	//if(m_laserUpdateTimer < LASER_UPDATE_TIME_FP)
		//return;

	// FP always update...
	IEntity* pLaserEntity = m_pEntitySystem->GetEntity(m_laserEntityId);

	if(!pLaserEntity)
		return;

	m_laserUpdateTimer = 0.0f;

	Vec3 laserPos, laserDir;

	GetLaserPositionAndDirectionFP(pParentWeapon, laserPos, laserDir);

	float laserLength = 0.0f;
	const float nearClipPlaneLimit = 10.0f;
	Vec3 hitPos(0,0,0);
	float dotScale = 1.0f;
	
	IPhysicalEntity* pSkipEntity = NULL;
	CActor* pOwner = pParentWeapon->GetOwnerActor();
	if(pOwner)
		pSkipEntity = pOwner->GetEntity()->GetPhysics();

	const int objects = ent_all;
	const int flags = (geom_colltype_ray << rwi_colltype_bit) | rwi_colltype_any | (10 & rwi_pierceability_mask) | (geom_colltype14 << rwi_colltype_bit);

	ray_hit hit;	
	if (gEnv->pPhysicalWorld->RayWorldIntersection(laserPos, laserDir*m_sharedparams->pLaserParams->laser_range[eIGS_FirstPerson], objects,	flags, &hit, 1, &pSkipEntity, pSkipEntity?1:0))
	{
		//Clamp distance below near clip plane limits, if not dot will be overdrawn during rasterization
		if(hit.dist>nearClipPlaneLimit)
		{
			laserLength = nearClipPlaneLimit;
			hitPos = laserPos + (nearClipPlaneLimit*laserDir);
		}
		else
		{
			laserLength = hit.dist;
			hitPos = hit.pt;
		}
		
		if(pOwner && pOwner->GetActorParams())
			dotScale *= max(0.3f,pOwner->GetActorParams()->viewFoVScale);

	}
	else
	{
		hitPos = laserPos - (laserDir*3.0f);
		laserLength = 3.0f;
	}

	//Final tweaks to positions
	if(laserLength<=0.7f)
		hitPos = laserPos+(0.7f*laserDir);

	CWeapon* pWeapon = static_cast<CWeapon*>(pParentWeapon->GetIWeapon());
	if(pWeapon && pWeapon->IsWeaponLowered())
	{
		hitPos = laserPos+(2.0f*laserDir);
		laserLength = 2.0f;
	}

	if(laserLength<=2.0f)
		dotScale *= min(1.0f,(0.35f + ((laserLength-0.7f)*0.5f)));

	//Set laser dot matrix
	Matrix34 finalMatrix = Matrix34::CreateTranslationMat(hitPos-(0.2f*laserDir));
	pLaserEntity->SetWorldTM(finalMatrix);
	Matrix34 localScale = Matrix34::CreateIdentity();
	localScale.SetScale(Vec3(dotScale,dotScale,dotScale));
	pLaserEntity->SetSlotLocalTM(m_laserDotSlot,localScale);

}

//------------------------------------------
void CLaser::UpdateThirdPerson(CItem* pParentWeapon)
{
	if(!m_sharedparams->pLaserParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "LASER PARAMS: Item of type CLaser is missing it's laser params!");
		return;
	}
	
	if(m_laserUpdateTimer < LASER_UPDATE_TIME_TP)
		return;

	m_laserUpdateTimer = Random(0.0f, LASER_UPDATE_TIME_TP * 0.4f);

	IEntity *pLaserEntity = m_pEntitySystem->GetEntity(m_laserEntityId);
	if(!pLaserEntity)
		return;

	Vec3 laserPos, laserDir;

	GetLaserPositionAndDirectionTP(pParentWeapon, laserPos, laserDir);

	//Check if weapon is DSG1 (hax)
	float dsg1Scale = 1.0f;
	if(pParentWeapon->GetEntity()->GetClass() == CItem::sDSG1Class)
		dsg1Scale = 3.0f;

	Vec3 hitPos(0,0,0);
	float laserLength = m_sharedparams->pLaserParams->laser_range[eIGS_ThirdPerson];
	bool hitSolid = false;

	IPhysicalEntity* pSkipEntity = NULL;
	if(pParentWeapon->GetOwner())
		pSkipEntity = pParentWeapon->GetOwner()->GetPhysics();

	const float range = m_sharedparams->pLaserParams->laser_range[eIGS_ThirdPerson]*dsg1Scale;

	// Use the same flags as the AI system uses for visbility.
	const int objects = ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid|ent_independent; //ent_living;
	const int flags = (geom_colltype_ray << rwi_colltype_bit) | rwi_colltype_any | (10 & rwi_pierceability_mask) | (geom_colltype14 << rwi_colltype_bit);

	ray_hit hit;
	if (gEnv->pPhysicalWorld->RayWorldIntersection(laserPos, laserDir*range, objects, flags, &hit, 1, &pSkipEntity, pSkipEntity ? 1 : 0))
	{
		laserLength = hit.dist;
		hitPos = hit.pt;
		hitSolid = true;
	}
	else
	{
		hitPos = laserPos + (laserDir * range);
		laserLength = range + 0.1f;
	}

	const CCamera& camera = gEnv->pRenderer->GetCamera();

	// Hit near plane
	if (laserDir.Dot(camera.GetViewdir()) < 0.0f)
	{
		Plane nearPlane;
		nearPlane.SetPlane(camera.GetViewdir(), camera.GetPosition());
		nearPlane.d -= camera.GetNearPlane()+0.15f;
		Ray ray(laserPos, laserDir);
		Vec3 out;
		if (Intersect::Ray_Plane(ray, nearPlane, out))
		{
			float dist = Distance::Point_Point(laserPos, out);
			if (dist < laserLength)
			{
				laserLength = dist;
				hitPos = out;
				hitSolid = true;
			}
		}
	}

	if (m_lastLaserLength > laserLength)
		m_lastLaserLength = laserLength;
	else
		m_lastLaserLength += (laserLength - m_lastLaserLength) * min(1.0f, 10.0f * gEnv->pTimer->GetFrameTime());

	//Update laser geometry scale
	// Orient the laser towards the point point.
	Matrix34 parentTMInv;
	parentTMInv = GetEntity()->GetWorldTM().GetInverted();

	Vec3 localDir = parentTMInv.TransformPoint(hitPos);
	float finalLaserLen = localDir.NormalizeSafe();
	Matrix33 rot;
	rot.SetIdentity();
	rot.SetRotationVDir(localDir);
	pLaserEntity->SetLocalTM(rot);

	const float assetLength = 2.0f;
	finalLaserLen = CLAMP(m_lastLaserLength,0.01f,m_sharedparams->pLaserParams->laser_max_len*dsg1Scale);
	float scale = finalLaserLen / assetLength; 

	// Scale the laser based on the distance.
	Matrix33 scl;
	scl.SetIdentity();
	scl.SetScale(Vec3(1,scale,1));
	pLaserEntity->SetSlotLocalTM(m_laserGeometrySlot, scl);


	// Set Dot matrix
	if (hitSolid)
	{
		Matrix34 mt = Matrix34::CreateTranslationMat(Vec3(0,finalLaserLen,0));
		pLaserEntity->SetSlotLocalTM(m_laserDotSlot, mt);
	}
	else
	{
		Matrix34 scaleMatrix;
		scaleMatrix.SetIdentity();
		scaleMatrix.SetScale(Vec3(0.001f,0.001f,0.001f));
		pLaserEntity->SetSlotLocalTM(m_laserDotSlot, scaleMatrix);
	}

}

//--------------------------------------------
void CLaser::OnEnterFirstPerson()
{
	CAccessory::OnEnterFirstPerson();

	if(m_laserOn)
	{
		TurnOffLaser();
		TurnOnLaser();
	}
}

//---------------------------------------------
void CLaser::OnEnterThirdPerson()
{
	CAccessory::OnEnterThirdPerson();

	if(m_laserOn)
	{
		TurnOffLaser();
		TurnOnLaser();
	}
}

//--------------------------------------------
void CLaser::GetLaserPositionAndDirectionFP(CItem* pParentWeapon, Vec3& pos, Vec3& dir)
{
	pos = pParentWeapon->GetSlotHelperPos(eIGS_FirstPerson,m_laserHelperFP.c_str(),true);
	Quat   lamRot = Quat(pParentWeapon->GetSlotHelperRotation(eIGS_FirstPerson,m_laserHelperFP.c_str(),true));
	dir = -lamRot.GetColumn0();

	if(m_sharedparams->pLaserParams->foward_dir_Y)
		dir = lamRot.GetColumn1();

	CActor *pActor = pParentWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : NULL;
	if (pMC)
	{ 
		SMovementState info;
		pMC->GetMovementState(info);

		CWeapon* pWeapon = static_cast<CWeapon*>(pParentWeapon->GetIWeapon());
		if(pWeapon && (pWeapon->IsReloading() || (!pActor->CanFire() && !pWeapon->IsZoomed())))
			return;

		if(dir.Dot(info.fireDirection)<0.985f)
			return;

		CCamera& camera = gEnv->pSystem->GetViewCamera();
		pos = camera.GetPosition();
		dir = camera.GetMatrix().GetColumn1();
		dir.Normalize();
	}
}

//--------------------------------------------
void CLaser::GetLaserPositionAndDirectionTP(CItem* pParentWeapon, Vec3& pos, Vec3& dir)
{
	pos = GetEntity()->GetWorldPos(); 
	dir = GetEntity()->GetWorldRotation().GetColumn1(); 

	//If character not visible, laser is not correctly updated
	if(CActor* pOwner = pParentWeapon->GetOwnerActor())
	{
		ICharacterInstance* pCharacter = pOwner->GetEntity()->GetCharacter(0);
		IMovementController* pMC = pOwner->GetMovementController();
		if(pMC && pCharacter)
		{
			SMovementState state;
			pMC->GetMovementState(state);
			if (!pCharacter->IsCharacterVisible())
			{
				pos = state.weaponPosition;
			}
			else if(!pOwner->IsPlayer())
			{
				const float angleMin = DEG2RAD(3.0f);
				const float angleMax = DEG2RAD(7.0f);
				const float thr = cosf(angleMax);
				float dot = dir.Dot(state.aimDirection);
				if (dot > thr)
				{
					float a = acos_tpl(dot);
					float u = 1.0f - clamp((a - angleMin) / (angleMax - angleMin), 0.0f, 1.0f);
					dir = dir + u * (state.aimDirection - dir);
					dir.Normalize();
				}
			}
		}
	}

	pos += (dir*0.10f);
}