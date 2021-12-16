/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Climb-able Ledge system 

-------------------------------------------------------------------------
History:
- 20:04:2009: Created by Michelle Martin

*************************************************************************/

#include "StdAfx.h"
#include "LedgeManager.h"
#include "IEntitySystem.h"
#include "IRenderAuxGeom.h"

CLedgeManager::CLedgeManager()
{
	Reset();
}

CLedgeManager::~CLedgeManager()
{

}

// ---------------------------------------------------

void CLedgeManager::Reset()
{
	m_ledges.clear();
	m_ledges.reserve(10);  // do statistics on levels to find a good value for this
}

// ---------------------------------------------------

void CLedgeManager::AddLedge( EntityId entityId, float fWidth )
{
	// check if this ledge already exists
	// and only update it's size if so
	int iIdx = GetLedgeIndex(entityId);

	if (iIdx != -1)
	{
		if (fWidth <= 0.0f)
		{
			RemoveLedgeByEntityId(entityId);
		}
		else
		{
			m_ledges[iIdx].m_fWidth = fWidth;
		}
	}
	else
	{
		if (fWidth <= 0.0f)
			return;

		SLedge newLedge(entityId, fWidth);
		m_ledges.push_back(newLedge);
	}
}

// ---------------------------------------------------

void CLedgeManager::RemoveLedgeByEntityId( EntityId entityId )
{
	LedgeContainer::iterator it = GetLedgeIterator(entityId);
	if (it != m_ledges.end())
	{
		m_ledges.erase(it);
	}
}

// ---------------------------------------------------

void CLedgeManager::RemoveLedgeByIndex( int iIdx )
{
	if (iIdx < 0 || iIdx > m_ledges.size())
		return;

	LedgeContainer::iterator it = m_ledges.begin();
	it += iIdx;
	m_ledges.erase(it);
}

// ---------------------------------------------------

int CLedgeManager::GetLedgeIndex( EntityId entityId )
{
	int iIndex = -1;

	LedgeContainer::iterator it = m_ledges.begin();
	LedgeContainer::iterator endIt = m_ledges.end();

	for (int i = 0; it != endIt; ++it, ++i)
	{
		if ((*it).m_entityId == entityId)
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

// ---------------------------------------------------

LedgeContainer::iterator CLedgeManager::GetLedgeIterator( EntityId entityId )
{
	LedgeContainer::iterator it = m_ledges.begin();
	LedgeContainer::iterator endIt = m_ledges.end();

	for ( ; it != endIt; ++it)
	{
		if ((*it).m_entityId == entityId)
		{
			return it;
		}
	}

	return endIt;
}

// ---------------------------------------------------

int CLedgeManager::FindNearestLedge( const Vec3 &vPos, const Vec3 &vDir, float fMaxDistance /*= 2.0f*/, float fAngleRange /*= DEG2RAD(35.0f)*/ )
{
	int iIdx = -1;
	float fClosestDistance = fMaxDistance + 1.0f;
	const float fMaxDistSquared = fMaxDistance* fMaxDistance;
	const float fCosMaxAngle = cosf(fAngleRange);

	LedgeContainer::iterator it = m_ledges.begin();
	LedgeContainer::iterator endIt = m_ledges.end();
	for (int i = 0; it != endIt; ++it, ++i)
	{
		Vec3 vPosToLedge;
		if (FindVectorToClosestPointOnLedge(vPos, *it, &vPosToLedge) == false)
		{
			continue;
		}

		float fDist = vPosToLedge.GetLengthSquared();
		if (fDist > fMaxDistSquared)
		{
			continue;
		}

		fDist = sqrtf(fDist);
		if (fDist > fClosestDistance)
		{
			continue;
		}

		vPosToLedge /= fDist;		//vPosToLedge is now normalized
		float fCosAngle = vDir * vPosToLedge;

		// Explanation: (Please do not delete this comment)
		//	The item can be skipped if the angle is too big.
		//	Since only the cosine of angles are compared,
		//	bigger angles result in smaller values (hence the less_than comparison)
		if (fCosAngle < fCosMaxAngle)
		{
			continue;
		}

		iIdx = i;
		fClosestDistance = fDist;
	}

	return iIdx;
}

// ---------------------------------------------------

SLedge CLedgeManager::GetLedgeByIndex( int iIdx ) const
{
	SLedge retVal;

	if (iIdx < 0 || (iIdx > m_ledges.size() - 1))
		return retVal;

	return m_ledges.at(iIdx);
}

// ---------------------------------------------------

bool CLedgeManager::FindVectorToClosestPointOnLedge( const Vec3 &vPoint, const SLedge &ledge, Vec3 *vOutput ) const
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(ledge.m_entityId);
	if (!pEntity)
	{
		return false;
	}

	Quat qEntRot = pEntity->GetWorldRotation();
	Vec3 vXDir(1.0f, 0.0f, 0.0f);
	vXDir = qEntRot * vXDir;

	Vec3 vEntityPos = pEntity->GetWorldPos();
	Vec3 vPosToLedge = vPoint - vEntityPos;

	float fD = vPosToLedge * vXDir;
	if (fabsf(fD) > ledge.m_fWidth * 0.5f)
	{
		fD = ((fD > 0.0f)? 1.0f : -1.0f) * ledge.m_fWidth * 0.5f;
	}

	Vec3 vClosestPoint = vEntityPos + fD * vXDir;
	vPosToLedge = vClosestPoint - vPoint;

	*vOutput = vPosToLedge;
	return true;
}

// ---------------------------------------------------

// ---------------------------------------------------
// ---------------------------------------------------

void SLedge::DebugRender(const ColorB &color)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
	if (!pEntity)
		return;

	//Vec3 vEntityPos = pEntity->GetWorldPos();
	Matrix34 entMatrix = pEntity->GetWorldTM();
	entMatrix.AddTranslation(Vec3(0, 0, 0.025f));  // raise a little bit to avoid clipping
	AABB bBox;
	pEntity->GetLocalBounds(bBox);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(bBox, entMatrix, true, color, eBBD_Faceted);

/*
	Vec3 vXDir(1.0f, 0.0f, 0.0f);
	vXDir = pEntity->GetWorldRotation() * vXDir;
	Vec3 pos = pEntity->GetWorldPos() + Vec3(0, 0, 0.05f);
//	ColorB color(200, 0, 200);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, color, pos + vXDir * m_fWidth * 0.5f, color);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, color, pos - vXDir * m_fWidth * 0.5f, color);
*/
}

// ---------------------------------------------------

bool SLedge::IsValid() const
{
	if (m_entityId < 0)
		return false;

	if (m_fWidth <= 0.0f)
		return false;

	return true;
}

// ---------------------------------------------------
