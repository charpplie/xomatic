/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  
-------------------------------------------------------------------------
History:
- 08:06:2007   : Created by Benito G.R.

*************************************************************************/
#include "StdAfx.h"
#include "C4Projectile.h"
#include "Player.h"
#include "GameRules.h"

int CC4Projectile::s_attachNameID = 0;

CC4Projectile::CC4Projectile():
m_stuckAttIndex(-1),
m_stuckEntityID(0),
m_stuck(false),
m_armed(false),
m_uniqueAttachment(false),
m_teamId(0)
{
}

//-------------------------------------------
CC4Projectile::~CC4Projectile()
{
	if(m_stuckAttIndex >= 0)
	{
		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_stuckEntityID))
		{
			if(ICharacterInstance* pCharacter = pEntity->GetCharacter(0))
			{
				if(IAttachmentManager* pAttachManager = pCharacter->GetIAttachmentManager())
				{
					if(IAttachment* pAttachment = pAttachManager->GetInterfaceByIndex(m_stuckAttIndex))
					{
						pAttachment->ClearBinding();

						if(m_uniqueAttachment)
						{
							pAttachManager->RemoveAttachmentByInterface(pAttachment);
						}
					}
				}
			}
		}
	}
}

//------------------------------------------
void CC4Projectile::HandleEvent(const SGameObjectEvent &event)
{
	if (CheckAnyProjectileFlags(ePFlag_destroying))
		return;

	CProjectile::HandleEvent(event);

	if (event.event == eGFE_OnCollision)
	{
		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;

		if (gEnv->bServer && !m_stuck)
			Stick(pCollision);
	}
}

//--------------------------------------------
void CC4Projectile::ProcessEvent(SEntityEvent &event)
{
	BaseClass::ProcessEvent(event);

	if(event.event == ENTITY_EVENT_TIMER && event.nParam[0] == ePTIMER_ACTIVATION)
	{
		m_armed = true;
	}
}

//--------------------------------------------
void CC4Projectile::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CProjectile::Launch(pos, dir, velocity, speedScale);

	OnLaunch();

	if(m_pAmmoParams->armTime > 0.f)
	{
		GetEntity()->SetTimer(ePTIMER_ACTIVATION, (int)(m_pAmmoParams->armTime*1000.f));
	}
	else
	{
		m_armed = true;
	}
}

void CC4Projectile::SetParams(EntityId ownerId, EntityId hostId, EntityId weaponId, int damage, float damageFallOffStart, float damageFallOffAmount, float damageFalloffMin, int hitTypeId, int8 bulletPierceabilityModifier)
{
	// if this is a team game, record which team placed this claymore...
	if(gEnv->bServer)
	{
		if(CGameRules* pGameRules = g_pGame->GetGameRules())
		{
			m_teamId = pGameRules->GetTeam(ownerId);
			pGameRules->SetTeam(m_teamId, GetEntityId());
		}
	}

	CProjectile::SetParams(ownerId, hostId, weaponId, damage, damageFallOffStart, damageFallOffAmount, damageFalloffMin, hitTypeId, bulletPierceabilityModifier);
}

//-----------------------------------------------
//This function is only executed on the server
void CC4Projectile::Stick(EventPhysCollision *pCollision)
{
	assert(pCollision);
	int trgId = 1;
	int srcId = 0;
	IPhysicalEntity *pTarget = pCollision->pEntity[trgId];

	if (pTarget == GetEntity()->GetPhysics())
	{
		trgId = 0;
		srcId = 1;
		pTarget = pCollision->pEntity[trgId];
	}

	//Do not stick to breakable glass
	if(ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(pCollision->idmat[trgId]))
	{
		if(pSurfaceType->GetBreakability()==1)
		{
			return;
		}
	}

	IEntity *pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;

	if (pTarget && (!pTargetEntity || (pTargetEntity->GetId() != m_ownerId)))
	{
		//Special cases
		if(pTargetEntity)
		{
			//Stick to actors using a character attachment
			CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId()));
			
			if(pActor) 
			{
				if(!gEnv->bMultiplayer && pActor->GetHealth() > 0 )
				{
					StickToCharacter(pCollision, pActor, srcId, trgId);
				}

				return;
			}

			//Do not stick to small objects...
			pe_params_part pPart;
			pPart.ipart = 0;
			if(pTarget->GetParams(&pPart) && pPart.pPhysGeom && pPart.pPhysGeom->V<0.15f)
			{
				return;
			}

			//Do not attach to items
			if(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pTargetEntity->GetId()))
			{
				return;
			}

			Matrix34 mat = pTargetEntity->GetWorldTM();
			mat.Invert();
			Vec3 pos = mat.TransformPoint(pCollision->pt);
			mat.SetIdentity();
			mat.SetRotation33(Matrix33::CreateOrientation(-pCollision->n,GetEntity()->GetWorldTM().TransformVector(Vec3(0,0,1)),gf_PI));
			mat.SetTranslation(pos);

			//Dephysicalize and stick
			GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

			StickToEntity(pTargetEntity,mat);

			if(gEnv->bMultiplayer && GetWeapon()->IsServer())
			{
				Quat rot(Matrix33::CreateOrientation(-pCollision->n,GetEntity()->GetWorldTM().TransformVector(Vec3Constants<float>::fVec3_OneZ),gf_PI*0.5f));
				GetGameObject()->InvokeRMI(CC4Projectile::ClStickToEntity(),ProjectileStickToEntity(pTargetEntity->GetId(),pos,rot),eRMI_ToAllClients);
			}
		}
		else
		{
			StickToStaticObject(pCollision,pTarget);
		}
	}
}

//---------------------------------------------------------------------
//This function is only executed on the server
void CC4Projectile::StickToStaticObject(EventPhysCollision *pCollision, IPhysicalEntity* pTarget)
{
	m_stuck = true;
	
	Vec3 currentPos = GetEntity()->GetWorldPos();
	Matrix34 mat;
	mat.SetRotation33(Matrix33::CreateOrientation(-pCollision->n, GetEntity()->GetWorldTM().GetColumn2(), gf_PI));

	Matrix34 rotMat;
	rotMat.SetRotationX(gf_PI/2.f);
	
	AABB aabb;
	GetEntity()->GetLocalBounds(aabb);
	
	
	mat.SetTranslation(Vec3(0.f,0.f,0.f));
	mat = mat * rotMat; //rotate by 90 degrees to get collision normal as up direction

	Matrix34 inverse(mat.GetInverted());
	Vec3 diff = pCollision->pt - currentPos;
	Vec3 dim = (aabb.max - aabb.min) / 2.f;
	float maxScale = 0.f;

	diff = inverse * diff;

	for (int n = 0;  n < 3; n++)
	{
		float scale = cry_fabsf(diff[n] / dim[n]);
		maxScale = max(scale, maxScale);
	}

	diff *= (1.f - (1.f/maxScale));
	diff = mat * diff;

	currentPos += diff;

	mat.SetTranslation(currentPos);
	GetEntity()->SetWorldTM(mat);

	GetGameObject()->SetAspectProfile(eEA_Physics, ePT_Static);

	CHANGED_NETWORK_STATE(this, eEA_GameServerStatic);
}

//------------------------------------------------------------------------
void CC4Projectile::StickToEntity(IEntity* pEntity, Matrix34 &localMatrix)
{	
	m_stuck = true;
	GetEntity()->SetLocalTM(localMatrix);
	pEntity->AttachChild(GetEntity());
}

//------------------------------------------------------------------------
void CC4Projectile::StickToCharacter(EventPhysCollision* pCollision, CActor* pActor, int sourceIndex, int targetIndex)
{
	EntityId actorID = pActor->GetEntityId();

	if(CActor* pOwner = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_ownerId)))
	{
		if(pOwner->IsFriendlyEntity(actorID))
		{
			return;
		}
	}

	ICharacterInstance* pCharacter = pActor->GetEntity()->GetCharacter(0);
	
	if(pCharacter)
	{
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();

		if(pAttachmentManager)
		{		
			if(!AttachToHelper(pAttachmentManager, pActor, pCollision->vloc[sourceIndex]))
			{
				CreateAttachment(pCollision, pActor, pCharacter, pAttachmentManager, targetIndex);
			}
		}
	}
}

void CC4Projectile::CreateAttachment(EventPhysCollision* pCollision, CActor* pActor, ICharacterInstance* pCharacter, IAttachmentManager* pAttachmentManager, int targetIndex)
{
	int attachJoint = pCollision->partid[targetIndex];
	ISkeletonPose* pSkeleton = pCharacter->GetISkeletonPose();
	const char* boneName = pSkeleton->GetJointNameByID(attachJoint);
	Matrix34 worldTM = pActor->GetEntity()->GetWorldTM();

	if(!boneName[0])
	{
		//Collision gave us an invalid joint id, do a search for the nearest bone

		int numJoints = pSkeleton->GetJointCount();
		int nearestJoint = 0;
		float nearestDist = FLT_MAX;

		Vec3 localSpaceCollision = worldTM.GetInverted() * pCollision->pt;

		for (int i = 0; i < numJoints; i++)
		{
			QuatT joint = pSkeleton->GetAbsJointByID(i);
			
			float dist = (joint.t - localSpaceCollision).GetLengthSquared();

			if(dist < nearestDist)
			{
				nearestDist = dist;
				nearestJoint = i;
			}
		}

		attachJoint = nearestJoint;
		boneName = pSkeleton->GetJointNameByID(nearestJoint);
	}

	QuatT joint = pSkeleton->GetAbsJointByID(attachJoint);
	Matrix34 invTM = worldTM * Matrix34(joint);

	invTM.Invert();
	Vec3 pos = invTM * pCollision->pt;
	Quat rot = Quat(invTM) * GetEntity()->GetRotation();

	char attachName[16] = "";		
	sprintf(attachName, "C4_%d", s_attachNameID++);

	IAttachment* pCharacterAttachment = pAttachmentManager->CreateAttachment(attachName, CA_BONE, boneName, false, false, true); 
	if(pCharacterAttachment)
	{
		pCharacterAttachment->SetAttRelativeDefault(QuatT(rot, pos));

		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());

		pCharacterAttachment->AddBinding(pEntityAttachment);

		m_stuck = true;
		m_stuckEntityID = pActor->GetEntityId();
		m_stuckAttIndex = pAttachmentManager->GetIndexByName(attachName);
		m_uniqueAttachment = true;

		GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);
	}
}

bool CC4Projectile::AttachToHelper(IAttachmentManager* pAttachmentManager, CActor* pActor, Vec3 c4Direction)
{
	int attachIndex = pAttachmentManager->GetIndexByName("c4_attach");

	if(attachIndex >= 0) 
	{
		int selectedAttachment = attachIndex;
		int backAttachIndex = pAttachmentManager->GetIndexByName("c4_attach_back");

		if(backAttachIndex >= 0)
		{
			//Select one of the attachment points
			Vec3 charOrientation = pActor->GetAnimatedCharacter()->GetAnimLocation().GetColumn1();

			charOrientation.z = 0;
			c4Direction.z = 0;

			charOrientation.NormalizeSafe();
			c4Direction.NormalizeSafe();

			if(c4Direction.Dot(charOrientation) > 0.f)
			{
				selectedAttachment = backAttachIndex;
			}
		}

		IAttachment* pSelectedAttachment = pAttachmentManager->GetInterfaceByIndex(selectedAttachment);

		if(pSelectedAttachment && !pSelectedAttachment->GetIAttachmentObject()) //Check if there's already one attached
		{
			CEntityAttachment *pEntityAttachment = new CEntityAttachment();
			pEntityAttachment->SetEntityId(GetEntityId());

			pSelectedAttachment->AddBinding(pEntityAttachment);
			pSelectedAttachment->HideAttachment(0);

			m_stuck = true;
			m_stuckEntityID = pActor->GetEntityId();
			m_stuckAttIndex = selectedAttachment;
			m_uniqueAttachment = false;

			GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);
		}

		return true;
	}

	return false;
}

bool CC4Projectile::Detonate()
{
	if(m_armed)
	{
		if(m_stuck)
		{
			IEntity* pEntity = GetEntity();

			Vec3 pos = pEntity->GetWorldPos();
			Matrix34 mtx = pEntity->GetWorldTM();
			Vec3 up = mtx.GetColumn2();

			Explode(true, true, pos, up, -up);
		}
		else
		{
			Explode(true);
		}
	}
	
	return m_armed;
}

//-------------------------------------------------------------------------
IMPLEMENT_RMI(CC4Projectile, ClStickToEntity)
{
	if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.targetId))
	{
		Matrix34 localMatrix;
		localMatrix.SetRotation33(Matrix33(params.localRotation));
		localMatrix.SetTranslation(params.localCollisonPos);
		StickToEntity(pEntity,localMatrix);
	}

	return true;
}

//------------------------------------------------------------------------
bool CC4Projectile::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags)
{
	if(aspect == eEA_GameServerStatic)
	{
		Vec3 position = GetEntity()->GetWorldPos();
		Quat rotation = GetEntity()->GetWorldRotation();

		ser.Value("rot", rotation, 'ori1');
		ser.Value("pos", position, 'wrl3');
		ser.Value("stuck", m_stuck, 'bool');
		
		if(m_stuck && ser.IsReading())
		{
			Matrix34 mat;
			mat.SetRotation33(Matrix33(rotation));
			mat.SetTranslation(position);
			GetEntity()->SetWorldTM(mat);
		}

	}

	return BaseClass::NetSerialize(ser, aspect, profile, pflags);
}

void CC4Projectile::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	if(gEnv->bMultiplayer && gEnv->bServer)
	{
		IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_ownerId);

		if(!pActor || pActor->GetHealth() <= 0)
		{
			Destroy();
		}
	}

	BaseClass::Update(ctx, updateSlot);
}