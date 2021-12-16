/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Manager for camera raycasts

-------------------------------------------------------------------------
History:
- 12:2008 : Created By Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "CameraRayScan.h"

//this is used to make sure the camera has some extra space inside the rayscan range
const static float RAY_SCAN_BUFFER_SCALE = 1.1f;
const static float RAY_SCAN_OFFSET_DISTANCE = 0.2f;
//raycast settings
const int CCameraRayScan::g_objTypes = (ent_all | ent_water) & ~(ent_living | ent_independent | ent_rigid);
const int CCameraRayScan::g_geomFlags = geom_colltype0|geom_colltype_player|rwi_stop_at_pierceable;
//raycast vectors
const static Vec3 g_vRayUpOffset(0,0,0.1f);
const static Vec3 g_vRayUpDir(0,0,0.1f);

CCameraRayScan::CCameraRayScan()
{
	m_rayCastWrapper.SetReceiver(this);
	OnDRWReset();
}

CCameraRayScan::~CCameraRayScan()
{
}

void CCameraRayScan::OnDataReceived(const EventPhysRWIResult *pRWIResult)
{
	ray_hit *pHit = (ray_hit*)(pRWIResult->pForeignData);
	if(pHit)
	{
		if(pRWIResult->nHits > 0)
		{
			//get new hit
			*pHit = pRWIResult->pHits[0];
		}
		else
		{
			//reset old data
			pHit->dist = 0.0f;
			pHit->pCollider = NULL;
		}
	}
}

void CCameraRayScan::OnDataReceived(const EventPhysPWIResult *pPWIResult)
{
	float *pHitDist = (float*)(pPWIResult->pForeignData);
	if(pHitDist)
		*pHitDist = pPWIResult->dist;
}

void CCameraRayScan::OnDRWReset()
{
	//clear results
	for(int i = 0; i < eNUM_RAYS; ++i)
	{
		m_rayData[i].dist = 0.0f;
		m_rayData[i].pCollider = NULL;
	}
}

ray_hit * CCameraRayScan::GetHit(ECameraRays nr)
{
	if(nr >= 0 && nr < eNUM_RAYS)
		return &(m_rayData[nr]);

	ray_hit *pHit = NULL;
	ray_hit *pCurHit = m_rayData;
	//int nHits = 0;
	for(int i = 0; i < eNUM_RAYS; ++i)
	{
		if(pCurHit->pCollider != NULL)
		{
			//count hits
			//nHits++;
			//if(i == eRAY_CENTER) //mid-point counts double
			//	nHits++;
			if(!pHit || pCurHit->dist < pHit->dist)
				pHit = pCurHit; //shortest distance hit
		}
		pCurHit++;
	}

	//if(nHits > 1) //only return if at least 2 rays hit something
	return pHit;

	return NULL;
}

const Vec3& CCameraRayScan::GetRayDir(ECameraRays nr) const
{
	if(nr >= 0 && nr < eNUM_RAYS)
		return m_rayDirs[nr];
	return Vec3Constants<float>::fVec3_Zero;
}

void CCameraRayScan::ShootRays(const Vec3 &rayPos, const Vec3 &rayDir, IPhysicalEntity **pSkipEnts, int numSkipEnts)
{
	//shoot rays for all ray_hits
	Vec3 tempPos = rayPos;
	Vec3 tempDir = rayDir;
	const Vec3 dirNorm = rayDir.normalized();
	const Vec3 right = dirNorm.Cross(Vec3Constants<float>::fVec3_OneZ);
	const Vec3 rightOff = right * 0.15f;
	const Vec3 rightDir = right * 0.15f;
	const float len = rayDir.len() * RAY_SCAN_BUFFER_SCALE; //add some distance to be sure that the view is free

	const Vec3 rayPos2 = rayPos + (dirNorm * RAY_SCAN_OFFSET_DISTANCE); //move the rays away from the head to prevent clipping

	//center ray
	m_rayDirs[eRAY_CENTER] = dirNorm;
	//this is currently only used to improve actor-hit detection, can it be replaced ?
	m_rayCastWrapper->CastRay(rayPos2, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_CENTER]));

	tempDir = (dirNorm - rightDir + g_vRayUpDir).normalized();
	tempPos = rayPos2 - rightOff + g_vRayUpOffset;
	m_rayDirs[eRAY_TOP_LEFT] = tempDir;
	m_rayCastWrapper->CastRay(tempPos, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_TOP_LEFT]));

	tempDir = (dirNorm + rightDir + g_vRayUpDir).normalized();
	tempPos = rayPos2 + rightOff + g_vRayUpOffset;
	m_rayDirs[eRAY_TOP_RIGHT] = tempDir;
	m_rayCastWrapper->CastRay(tempPos, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_TOP_RIGHT]));

	tempDir = (dirNorm - rightDir - g_vRayUpDir).normalized();
	tempPos = rayPos2 - rightOff - g_vRayUpOffset;
	m_rayDirs[eRAY_BOTTOM_LEFT] = tempDir;
	m_rayCastWrapper->CastRay(tempPos, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_BOTTOM_LEFT]));

	tempDir = (dirNorm + rightDir - g_vRayUpDir).normalized();
	tempPos = rayPos2 + rightOff - g_vRayUpOffset;
	m_rayDirs[eRAY_BOTTOM_RIGHT] = tempDir;
	m_rayCastWrapper->CastRay(tempPos, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_TOP_RIGHT]));

	tempDir = (dirNorm + g_vRayUpDir).normalized();
	tempPos = rayPos2 + g_vRayUpOffset * 2.0f;
	m_rayDirs[eRAY_TOP_CENTER] = tempDir;
	m_rayCastWrapper->CastRay(tempPos, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_TOP_CENTER]));

	tempDir = (dirNorm - g_vRayUpDir).normalized();
	tempPos = rayPos2 - g_vRayUpOffset * 2.0f;
	m_rayDirs[eRAY_BOTTOM_CENTER] = tempDir;
	m_rayCastWrapper->CastRay(tempPos, tempDir * len, g_objTypes, g_geomFlags, pSkipEnts, numSkipEnts, (void*)(&m_rayData[(int)eRAY_BOTTOM_CENTER]));
}

void CCameraRayScan::ShootRay(const Vec3 &rayPos, const Vec3 &rayDir, ray_hit *pHit, int objTypes, int geomFlags, IPhysicalEntity **pSkipEnts, int numSkipEnts)
{
	m_rayCastWrapper->CastRay(rayPos, rayDir, objTypes, geomFlags, pSkipEnts, numSkipEnts, (void*)pHit);
}

void CCameraRayScan::ShootSphere(const Vec3 &spherePos, const Vec3 &sphereDir, float fRadius, float *pDist, int objTypes /* = g_objTypes */, int geomFlags /* = g_geomFlags */, IPhysicalEntity **pSkipEnts /* = NULL */, int numSkipEnts /* = 0 */)
{
	primitives::sphere	sph;
	sph.r				= fRadius;
	sph.center	= spherePos;
	m_rayCastWrapper->CastPrimitive(sph.type, &sph, sphereDir, objTypes, geomFlags, NULL, pSkipEnts, numSkipEnts, pDist);
}