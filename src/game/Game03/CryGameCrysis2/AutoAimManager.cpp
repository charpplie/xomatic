/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

20:11:2009 - Benito G.R.
*************************************************************************/

#include "StdAfx.h"
#include <ICryAnimation.h>
#include "Game.h"
#include "AutoAimManager.h"
#include "GameRules.h"
#include "IAIObject.h"

CAutoAimManager::CAutoAimManager()
: m_externalSnapTarget(0)
{
	assert((kMaxAutoaimTargets > 0) && (kMaxAutoaimTargets < 256));
	
	m_autoaimTargets.reserve(kMaxAutoaimTargets);

	REGISTER_CVAR(g_autoAimManagerDebug, 0, 0, "Debug auto aim manager");
}

CAutoAimManager::~CAutoAimManager()
{
	if (gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("g_autoAimManagerDebug", true);
	}
}

void CAutoAimManager::Update(float dt)
{
	const int numAutoaimTargets = m_autoaimTargets.size();

	//Update the autoaim positions for the entities
	for(int i = 0; i < numAutoaimTargets; i++)
	{
		UpdateCharacterTargetInfo(m_autoaimTargets[i]);
	}

#ifndef _RELEASE
	if (g_autoAimManagerDebug)
	{
		DebugDraw();
	}
#endif
}

void CAutoAimManager::UpdateCharacterTargetInfo(SAutoaimTarget& aaTarget)
{
	IEntity * pEntity = gEnv->pEntitySystem->GetEntity(aaTarget.entityId);
	CRY_ASSERT(pEntity);

	const Matrix34& worldTM = pEntity->GetWorldTM();
	
	Vec3 characterPos = worldTM.GetTranslation();
	Quat characterRot(worldTM);

	Vec3 primaryOffset = aaTarget.fallbackOffset;
	Vec3 secondaryOffset = aaTarget.fallbackOffset;

	ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
	ISkeletonPose* pSkeletonPose = pCharacter ? pCharacter->GetISkeletonPose() : NULL;

	if (pSkeletonPose)
	{
		//Need this because of decouple catch-up movement ;/
		//GetStatus( ) requires a ReadLock, which slows down this update, if it becomes a problem
		//use IanimatedCharacter and retrieve animation location from it, or something
		if (IPhysicalEntity* pCharacterPhysics = pSkeletonPose->GetCharacterPhysics())
		{
			pe_status_pos statusPos;
			if (pCharacterPhysics->GetStatus(&statusPos))
			{
				//Z of character physics is displaced by capsule, use entity one
				characterPos.Set(statusPos.pos.x, statusPos.pos.y, characterPos.z);		
				characterRot = statusPos.q;
			}
		}
		primaryOffset		= (aaTarget.primaryBoneId >= 0)		? pSkeletonPose->GetAbsJointByID(aaTarget.primaryBoneId).t		: primaryOffset;
		secondaryOffset = (aaTarget.secondaryBoneId >= 0) ? pSkeletonPose->GetAbsJointByID(aaTarget.secondaryBoneId).t	: secondaryOffset;
	}

	aaTarget.primaryAimPosition = characterPos + (characterRot * primaryOffset);
	aaTarget.secondaryAimPosition = characterPos + (characterRot * secondaryOffset);
}

bool CAutoAimManager::RegisterAutoaimTarget(EntityId entityId, const SAutoaimTargetRegisterParams& registerParams)
{
	if (IsEntityRegistered(entityId))
	{
		GameWarning("Trying to register entity more than once in auto aim manager!");
		return false;
	}

	if (!IsSpaceAvailable())
	{
		GameWarning("Maximum number of entities reached for auto aim manager (%d)!", kMaxAutoaimTargets);
		return false;
	}

	RegisterCharacterTargetInfo(entityId, registerParams);
	return true;

}

void CAutoAimManager::RegisterCharacterTargetInfo(EntityId entityId, const SAutoaimTargetRegisterParams& registerParams)
{
	SAutoaimTarget aimTarget;
	aimTarget.entityId = entityId;
	aimTarget.fallbackOffset = registerParams.fallbackOffset;
	aimTarget.primaryBoneId = registerParams.primaryBoneId;
	aimTarget.physicsBoneId = registerParams.physicsBoneId;
	aimTarget.secondaryBoneId = registerParams.secondaryBoneId;
	aimTarget.innerRadius = registerParams.innerRadius;
	aimTarget.outerRadius = registerParams.outerRadius;
	aimTarget.snapRadius = registerParams.snapRadius;
	aimTarget.secondaryRadius = registerParams.secondaryRadius;
	aimTarget.teamNum = (int8)g_pGame->GetGameRules()->GetTeam(entityId);
	if (!gEnv->bMultiplayer)
	{
		IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(entityId);
		IEntity* pClientEntity = gEnv->pEntitySystem->GetEntity(g_pGame->GetIGameFramework()->GetClientActorId());
		if (pTargetEntity && pClientEntity)
		{
			bool isHostile = pTargetEntity->GetAI() ? pTargetEntity->GetAI()->IsHostile(pClientEntity->GetAI(), false) : false;
			if (isHostile)
				aimTarget.SetFlag(eAATF_AIHostile);
		}

		//Instance properties, other stuff could be added here easily (grab enemy, sliding hit, etc)
		SmartScriptTable props;
		SmartScriptTable propsPlayerInteractions;
		IScriptTable* pScriptTable = pTargetEntity->GetScriptTable();
		if (pScriptTable && pScriptTable->GetValue("Properties", props))
		{
			if (props->GetValue("PlayerInteractions", propsPlayerInteractions))
			{
				int stealhKill = 0;
				if (propsPlayerInteractions->GetValue("bStealthKill", stealhKill) && (stealhKill != 0))
				{
					aimTarget.SetFlag(eAATF_StealthKillable);
				}
				int canBeGrabbed = 0;
				if (propsPlayerInteractions->GetValue("bCanBeGrabbed", canBeGrabbed) && (canBeGrabbed != 0))
				{
					aimTarget.SetFlag(eAATF_CanBeGrabbed);
				}
			}
		}
	}

	m_autoaimTargets.push_back(aimTarget);
}

void CAutoAimManager::UnregisterAutoaimTarget(EntityId entityId)
{	
	TAutoaimTargets::iterator it = m_autoaimTargets.begin();
	TAutoaimTargets::iterator end = m_autoaimTargets.end();

	for (; it != end; ++it)
	{
		if (it->entityId == entityId)
		{
			m_autoaimTargets.erase(it);
			return;
		}
	}

	GameWarning("Trying to unregister non registered entity from autoaim manager");
}


bool CAutoAimManager::IsEntityRegistered(EntityId entityId) const
{
	const int numAutoaimTargets = (int)m_autoaimTargets.size();

	for(int i = 0; i < numAutoaimTargets; i++)
	{
		if(m_autoaimTargets[i].entityId == entityId)
		{
			return true;
		}
	}

	return false;
}

bool CAutoAimManager::IsSpaceAvailable() const
{
	return (m_autoaimTargets.size() < kMaxAutoaimTargets);
}

const SAutoaimTarget* CAutoAimManager::GetTargetInfo( EntityId targetId ) const
{
	const int numAutoaimTargets = (int)m_autoaimTargets.size();

	for(int i = 0; i < numAutoaimTargets; i++)
	{
		if(m_autoaimTargets[i].entityId == targetId)
		{
			return &(m_autoaimTargets[i]);
		}
	}

	return NULL;
}

void CAutoAimManager::DebugDraw()
{
	IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();
	const int numAutoaimTargets = m_autoaimTargets.size();

	const Vec3 viewPos = gEnv->pSystem->GetViewCamera().GetPosition();

	SAuxGeomRenderFlags oldFlags = pRenderAux->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone); 
	pRenderAux->SetRenderFlags(newFlags);

	const ColorB enemyColor(255,0,0,128);
	const ColorB friendlyColor(0,255,0,128);
	const ColorB followColorInner(255,255,0,64);
	const ColorB followColorOuter(255,255,0,0);
	const ColorB snapColor(255,255,255,64);

	for(int i = 0; i < numAutoaimTargets; i++)
	{
		const SAutoaimTarget& aaTarget = m_autoaimTargets[i];

		Vec3 dirToTarget = aaTarget.primaryAimPosition - viewPos;
		dirToTarget.NormalizeSafe();
		
		pRenderAux->DrawSphere(aaTarget.primaryAimPosition, aaTarget.innerRadius, aaTarget.HasFlagSet(eAATF_AIHostile) ? enemyColor : friendlyColor);
		pRenderAux->DrawSphere(aaTarget.secondaryAimPosition, aaTarget.secondaryRadius, aaTarget.HasFlagSet(eAATF_AIHostile) ? enemyColor : friendlyColor);
		DrawDisc(aaTarget.primaryAimPosition, dirToTarget, aaTarget.innerRadius, aaTarget.outerRadius, followColorInner, followColorOuter);
		DrawDisc(aaTarget.primaryAimPosition, dirToTarget, aaTarget.outerRadius, aaTarget.snapRadius, followColorOuter, snapColor);
	}

	pRenderAux->SetRenderFlags(oldFlags);

	const float white[4] = {1.0f, 1.0f, 1.0f, 0.75f};
	gEnv->pRenderer->Draw2dLabel(50.0f, 50.0f, 1.5f, white, false, "Number of targets: %d", numAutoaimTargets);
}

void CAutoAimManager::DrawDisc(const Vec3& center, Vec3 axis, float innerRadius, float outerRadius, const ColorB& innerColor, const ColorB& outerColor)
{
	axis.NormalizeSafe(Vec3Constants<float>::fVec3_OneZ);
	Vec3 u = ((axis * Vec3Constants<float>::fVec3_OneZ) > 0.5f) ? Vec3Constants<float>::fVec3_OneY : Vec3Constants<float>::fVec3_OneZ;
	Vec3 v = u.cross(axis);
	u = v.cross(axis);

	float sides = cry_ceilf(3.0f * (float)g_PI * outerRadius);
	//sides = 20.0f;
	float step = 1.0f / sides;
	for (float i = 0.0f; i < 0.99f; i += step)
	{
		float a0 = i * 2.0f * (float)g_PI;
		float a1 = (i+step) * 2.0f * (float)g_PI;
		Vec3 i0 = center + innerRadius * (u*cry_cosf(a0) + v*cry_sinf(a0));
		Vec3 i1 = center + innerRadius * (u*cry_cosf(a1) + v*cry_sinf(a1));
		Vec3 o0 = center + outerRadius * (u*cry_cosf(a0) + v*cry_sinf(a0));
		Vec3 o1 = center + outerRadius * (u*cry_cosf(a1) + v*cry_sinf(a1));
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, innerColor, i1, innerColor, o1, outerColor);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle(i0, innerColor, o1, outerColor, o0, outerColor);
	}
}




