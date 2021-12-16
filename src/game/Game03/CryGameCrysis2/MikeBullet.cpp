/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: MikeBullet

-------------------------------------------------------------------------
History:
- 25:1:2010   : Created by Filipe Amim

*************************************************************************/
#include "StdAfx.h"
#include <IRenderAuxGeom.h>
#include "Game.h"
#include "MikeBullet.h"


namespace
{
	const char* temp_burnEffectName;
	const char* temp_burnDecalMaterialName;
	int temp_burnEffectDebugDraw;
	float temp_burnEffectBuildUp;
	float temp_burnEffectDissipation;
	float temp_burnEffectRadius;

	const char g_defaultBurnEffectName[] = "Crysis2_weapon_mike.default_impact";
	const char g_defaultBurnDecalMaterialName[] = "Materials/decals/burnt/mike_default";


	Matrix33 OrthoNormalVector(const Vec3& vector, float rotation = 0.0f)
	{
		if (vector.x == 0 && vector.y == 0)
			return Matrix33(IDENTITY);
		Matrix33 matrixResult;
		Vec3 z = vector;
		Vec3 x = z.cross(Vec3(0, 0, 1));
		Vec3 y = z.cross(x);
		matrixResult.SetColumn(0, x.GetNormalized());
		matrixResult.SetColumn(1, y.GetNormalized());
		matrixResult.SetColumn(2, z);

		Matrix33 rotationMatrix = Matrix33::CreateRotationAA(rotation, vector);
		matrixResult = rotationMatrix * matrixResult;

		return matrixResult;
	}
}



CBurnEffectManager::CBurnEffectManager()
{
	REGISTER_CVAR(temp_burnEffectName, g_defaultBurnEffectName, VF_NULL, "");
	REGISTER_CVAR(temp_burnDecalMaterialName, g_defaultBurnDecalMaterialName, VF_NULL, "");
	REGISTER_CVAR(temp_burnEffectDebugDraw, 0, VF_NULL, "");
	REGISTER_CVAR(temp_burnEffectBuildUp, 0.2f, VF_NULL, "");
	REGISTER_CVAR(temp_burnEffectDissipation, 0.2f, VF_NULL, "");
	REGISTER_CVAR(temp_burnEffectRadius, 0.75f, VF_NULL, "");
}



CBurnEffectManager::~CBurnEffectManager()
{
	gEnv->pConsole->UnregisterVariable("temp_burnEffectName", true);
	gEnv->pConsole->UnregisterVariable("temp_burnDecalMaterialName", true);
	gEnv->pConsole->UnregisterVariable("temp_burnEffectDebugDraw", true);
	gEnv->pConsole->UnregisterVariable("temp_burnEffectBuildUp", true);
	gEnv->pConsole->UnregisterVariable("temp_burnEffectDissipation", true);
	gEnv->pConsole->UnregisterVariable("temp_burnEffectRadius", true);
}



void CBurnEffectManager::AddBurnPoint(const EventPhysCollision& pCollision)
{
	TBurnPoints::iterator best = FindClosestBurnPoint(pCollision.pt);

	if (best != m_burnPoints.end())
	{
		best->m_accumulation += temp_burnEffectBuildUp;
	}
	else
	{
		SBurnPoint burnPoint;
		burnPoint.m_effect = 0;
		burnPoint.m_pDecal = 0;
		burnPoint.m_pDecalMaterial = 0;
		burnPoint.m_accumulation = temp_burnEffectBuildUp;
		burnPoint.m_radius = temp_burnEffectRadius;
		burnPoint.m_position = pCollision.pt;
		CreateBurnEffect(pCollision, &burnPoint);
		m_burnPoints.push_back(burnPoint);
	}
}



void CBurnEffectManager::Update(float deltaTime)
{
	TBurnPoints::iterator it = m_burnPoints.begin();
	for (; it != m_burnPoints.end();)
	{
		SBurnPoint& burnPoint = *it;
		burnPoint.m_accumulation -= deltaTime * temp_burnEffectDissipation;

		if (burnPoint.m_accumulation < 0)
		{
			DestroyBurnEffect(&burnPoint);
			it = m_burnPoints.erase(it);
		}
		else
		{
			UpdateBurnEffect(&burnPoint);
			++it;
		}
	}

	if (temp_burnEffectDebugDraw)
		DebugDraw();
}



void CBurnEffectManager::Reset()
{
	while (!m_burnPoints.empty())
	{
		DestroyBurnEffect(&*m_burnPoints.begin());
		m_burnPoints.erase(m_burnPoints.begin());
	}
}



CBurnEffectManager::TBurnPoints::iterator CBurnEffectManager::FindClosestBurnPoint(const Vec3& point)
{
	if (m_burnPoints.empty())
		return m_burnPoints.end();
	float minDist = m_burnPoints[0].m_position.GetSquaredDistance(point);
	TBurnPoints::iterator best = m_burnPoints.end();
	if (minDist <= cry_sqr(m_burnPoints[0].m_radius))
		best = m_burnPoints.begin();
	for (TBurnPoints::iterator it = m_burnPoints.begin()+1; it != m_burnPoints.end(); ++it)
	{
		float dist = it->m_position.GetSquaredDistance(point);
		if (dist < minDist && dist <= cry_sqr(it->m_radius))
		{
			minDist = dist;
			best = it;
		}
	}
	return best;
}



void CBurnEffectManager::CreateBurnEffect(const EventPhysCollision& pCollision, CBurnEffectManager::SBurnPoint* pBurnPoint)
{
	Vec3 surfaceNormal = pCollision.n;
	Vec3 hitDir(ZERO);
	if (pCollision.vloc[0].GetLengthSquared() > 1e-6f)
	{
		hitDir = pCollision.vloc[0].GetNormalized();
	}
	Vec3 surfacePosition = pCollision.pt;
	Vec3 halfVector = (surfaceNormal + (-hitDir)).GetNormalized();


	IParticleEffect* pParticleEffect = gEnv->pParticleManager->FindEffect(temp_burnEffectName);
	if (pParticleEffect)
	{
		Matrix34 loc;
		loc.SetIdentity();
		loc.SetTranslation(surfacePosition);
		loc.SetRotation33(OrthoNormalVector(surfaceNormal));
		IParticleEmitter* pEffect = pParticleEffect->Spawn(false, loc);
		pEffect->AddRef();
		pBurnPoint->m_effect = pEffect;
		UpdateBurnEffect(pBurnPoint);
	}


	pBurnPoint->m_pDecal = static_cast<IDecalRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_Decal));
	SDecalProperties decalProperties;
	decalProperties.m_projectionType = SDecalProperties::eProjectOnTerrainAndStaticObjects;
	decalProperties.m_pos = surfacePosition;
	decalProperties.m_normal = halfVector;
	decalProperties.m_pMaterialName = temp_burnDecalMaterialName;
	decalProperties.m_radius = temp_burnEffectRadius;
	decalProperties.m_explicitRightUpFront = OrthoNormalVector(halfVector, cry_frand()*2*3.141592f);
	decalProperties.m_sortPrio = 255;
	pBurnPoint->m_pDecal->SetDecalProperties(decalProperties);

	IMaterial* pMaterial = pBurnPoint->m_pDecal->GetMaterial();
	if (pMaterial)
	{
		IMaterial* pClonedMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMaterial(pMaterial);
		pBurnPoint->m_pDecal->SetMaterial(pClonedMaterial);
		pBurnPoint->m_pDecalMaterial = pClonedMaterial;

		Matrix34 wtm(decalProperties.m_explicitRightUpFront);
		wtm.SetTranslation(surfacePosition);

		pBurnPoint->m_pDecal->SetMatrix(wtm);
	}
	else
	{
		pBurnPoint->m_pDecal->ReleaseNode();
		pBurnPoint->m_pDecal = 0;
		pBurnPoint->m_pDecalMaterial = 0;
	}
	
	UpdateBurnEffect(pBurnPoint);
}



void CBurnEffectManager::UpdateBurnEffect(CBurnEffectManager::SBurnPoint* pBurnPoint)
{
	float effectIntensity = SATURATE(pBurnPoint->m_accumulation);
	SpawnParams params;
	params.fStrength = effectIntensity;
	if (pBurnPoint->m_effect)
		pBurnPoint->m_effect->SetSpawnParams(params);

	float alpha = effectIntensity;
	if (pBurnPoint->m_pDecal)
		pBurnPoint->m_pDecalMaterial->SetGetMaterialParamFloat("opacity", alpha, false);
}



void CBurnEffectManager::DestroyBurnEffect(CBurnEffectManager::SBurnPoint* pBurnPoint)
{
	if (pBurnPoint->m_effect)
	{
		pBurnPoint->m_effect->Activate(false);
		pBurnPoint->m_effect->Release();
		pBurnPoint->m_effect = 0;
	}

	if (pBurnPoint->m_pDecal)
	{
		pBurnPoint->m_pDecal->ReleaseNode();
	}
}



void CBurnEffectManager::DebugDraw()
{
	SAuxGeomRenderFlags oldRenderFlags = gEnv->pRenderer->GetIRenderAuxGeom()->GetRenderFlags();
	SAuxGeomRenderFlags renderFlags = oldRenderFlags;
	renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
	renderFlags.SetDepthWriteFlag(e_DepthWriteOff);
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(renderFlags);

	for (size_t i = 0; i < m_burnPoints.size(); ++i)
	{
		const SBurnPoint& burnPoint = m_burnPoints[i];
		ColorB color;
		color.r = (unsigned char)(SATURATE(burnPoint.m_accumulation * 0.5f) * 255.0f);
		color.g = (unsigned char)(SATURATE(burnPoint.m_accumulation * 0.2f) * 255.0f);
		color.b = (unsigned char)(SATURATE(burnPoint.m_accumulation * 0.05f) * 255.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(
			burnPoint.m_position, burnPoint.m_radius, color);
	}

	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(oldRenderFlags);
}




void CMikeBullet::HandleEvent(const SGameObjectEvent& event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);
	CBullet::HandleEvent(event);

	if (event.event == eGFE_OnCollision)
	{
		const EventPhysCollision& collision = *reinterpret_cast<EventPhysCollision*>(event.ptr);
		g_pGame->GetBurnEffectManager()->AddBurnPoint(collision);
	}
}
