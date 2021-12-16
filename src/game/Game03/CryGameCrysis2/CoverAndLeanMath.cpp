/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 5:11:2009: Created by Filipe Amim

*************************************************************************/
#include "StdAfx.h"
#include "CoverAndLeanMath.h"
#include "GameCVars.h"



CCoverAndLean::SQuadPolygon::SQuadPolygon()
	:	m_plane(Vec3(ZERO), 0)
{
	m_vertices[0].Set(0, 0, 0);
	m_vertices[1].Set(0, 0, 0);
	m_vertices[2].Set(0, 0, 0);
	m_vertices[3].Set(0, 0, 0);
}



CCoverAndLean::SBox::SBox()
	:	m_localMin(ZERO)
	,	m_localMax(ZERO)
	,	m_local(IDENTITY)
{
}



Vec3 HorizontalNormal(const Vec3& normal)
{
	Vec3 result = normal;
	result.z = 0;
	CRY_ASSERT_MESSAGE(!result.IsZero(), "normal vector is zero, cannot be projected onto xoy");
	result.Normalize();
	return result;
}



bool IsBetweenVectors(const Vec3& minVec, const Vec3& maxVec, const Vec3& testVec)
{
	float a = testVec.Dot(minVec);
	float b = maxVec.Dot(minVec);
	float c = testVec.Cross(minVec).z;
	float d = maxVec.Cross(minVec).z;
	return (a >= b && (c*d)>0);
}



Vec3 ClosestPointToPlane(const Plane& plane, const Vec3& point)
{
	return point - plane.n * plane.DistFromPlane(point);
}



Vec3 ClosestPointToLine(const Vec3& origin, const Vec3& direction, const Vec3& point)
{
	return origin + direction * direction.Dot(point-origin);
}



Vec3 LineIntersect2D(const Vec3& origin1, const Vec3& direction1, const Vec3& origin2, const Vec3& direction2)
{
	Lineseg line1(origin1, origin1+direction1);
	Lineseg line2(origin2, origin2+direction2);
	float t1, t2;
	Intersect::Lineseg_Lineseg2D(line1, line2, t1, t2);
	return origin1 + direction1*t1;
}



Plane TransformPlane(const Matrix34& tMat, const Plane& plane)
{
	const Vec3 planePosition = tMat * (plane.n * (-plane.d));
	const Vec3 planeNormal = Matrix33(tMat) * plane.n;
	return Plane::CreatePlane(planeNormal, planePosition);
}




static void GetBoxPolygon(const Vec3& boxMin, const Vec3& boxMax, int faceIndex, const Matrix34& local, CCoverAndLean::SQuadPolygon* polygon)
{
	const int i = faceIndex % 3;
	const int j = (faceIndex+1) % 3;
	const int k = (faceIndex+2) % 3;
	const float signal = (float)(faceIndex/3);

	Vec3 normal(ZERO);
	normal[i] = 1.0f;
	normal *= signal * 2 -1;
	polygon->m_plane = TransformPlane(local, Plane::CreatePlane(normal, LERP(boxMin, boxMax, signal)));

	for (int e = 0; e < 4; ++e)
	{
		const float jSigs[4] = {0, 0, 1, 1};
		const float kSigs[4] = {0, 1, 1, 0};

		polygon->m_vertices[e][i] = LERP(boxMin[i], boxMax[i], signal);
		polygon->m_vertices[e][j] = LERP(boxMin[j], boxMax[j], jSigs[e]);
		polygon->m_vertices[e][k] = LERP(boxMin[k], boxMax[k], kSigs[e]);
		polygon->m_vertices[e] = local * polygon->m_vertices[e];
	}
}


bool SplitPolygon(const CCoverAndLean::SQuadPolygon& polygon, const Plane& splitPlane, SSegment* resultedSegment)
{
	int splitVertex = 0;

	for (int v1 = 0; v1 < 4 && splitVertex < 2; ++v1)
	{
		int v2 = (v1+1)%4;

		Lineseg line(polygon.m_vertices[v1], polygon.m_vertices[v2]);
		Vec3 ipoint;
		bool splitSegment = Intersect::Segment_Plane(line, splitPlane, ipoint, false);
		if (!splitSegment)
			continue;

		resultedSegment->m_vertices[splitVertex++] = ipoint;
	}

	if (splitVertex == 2)
	{
		resultedSegment->m_vertices[0] = resultedSegment->m_vertices[0];
		resultedSegment->m_vertices[1] = resultedSegment->m_vertices[1];
		resultedSegment->m_normal = polygon.m_plane.n;
		return true;
	}

	return false;
}



int GetClosestBoxFaceIndex(const CCoverAndLean::SBox& box, const Vec3& point)
{
	CRY_FIXME(03, 11, 2009, "filipe - change the starting point of min as a numeric_limits max for floats instead of a hight number");
	float minDistance = 10000.0f;
	const float epsilon = 0.001f;
	int bestIndex = -1;
	for (int i = 0; i < 6; ++i)
	{
		float distance = box.m_worldPolygons[i].m_plane.DistFromPlane(point);
		if (distance >= -epsilon)
		{
			if (bestIndex != -1)		// closest point needs to be on face, not edge or vertex
				return -1;
			if (distance < minDistance)
			{
				minDistance = distance;
				bestIndex = i;
			}
		}
	}
	return bestIndex;
}


float GetBoxDepthFromFace(const CCoverAndLean::SBox& box, int faceIdx)
{
	int otherSizeFaceIdx = (faceIdx + 3) % 6;
	float distanceBetweenPlanes = -box.m_worldPolygons[faceIdx].m_plane.d - box.m_worldPolygons[otherSizeFaceIdx].m_plane.d;
	return abs(distanceBetweenPlanes);
}


float GetBoxDepthFromFaceAndRay(const CCoverAndLean::SBox& box, int faceIdx, const Vec3& rayPoint, const Vec3& rayDir)
{
	Vec3 p;
	int otherSizeFaceIdx = (faceIdx + 3) % 6;
	Intersect::Ray_Plane(Ray(rayPoint, rayDir), box.m_worldPolygons[otherSizeFaceIdx].m_plane, p, false);
	return p.GetDistance(rayPoint);
}


void CreateBox(const Vec3& localBoxMin, const Vec3& localBoxMax, Matrix34& local, CCoverAndLean::SBox* resultBox)
{
	resultBox->m_localMin = localBoxMin;
	resultBox->m_localMax = localBoxMax;
	resultBox->m_local = local;

	for (int p = 0; p < 6; ++p)
	{
		GetBoxPolygon(localBoxMin, localBoxMax, p, local, &resultBox->m_worldPolygons[p]);
	}
}



CRY_TODO(17, 02, 2010, "filipe - cover detection is hightly hacked and not optimized. need refactor and optimizations.");


static float ClosestDistanceToStaticSubObject(const Vec3& referencePoint, const Matrix34& instancePose, const IStatObj::SSubObject* pStaticSubObject)
{
	Matrix34 local = instancePose * pStaticSubObject->localTM;
	Vec3 localBoxMin = -pStaticSubObject->helperSize*0.5f;
	Vec3 localBoxMax = pStaticSubObject->helperSize*0.5f;

	Vec3 localReferencePoint = local.GetInverted().TransformPoint(referencePoint);

	CRY_TODO(17, 02, 2010, "filipe - quick dirty implementation of closest point to box. Need a proper implementation.");
	return localReferencePoint.GetDistance((localBoxMin+localBoxMax)*0.5f);
}


 
static const IStatObj::SSubObject* GetClosestCoverAndLeanProxy(const Vec3& referencePoint, const Matrix34& instancePose, IStatObj* staticObject)
{
	if (!staticObject)
		return 0;

	const IStatObj::SSubObject* best = 0;
	float bestDistance = 10000.0f;

	for (int i = 0; i < staticObject->GetSubObjectCount(); ++i)
	{
		const IStatObj::SSubObject* subObject = staticObject->GetSubObject(i);
		if (subObject->nType != STATIC_SUB_OBJECT_DUMMY)
			continue;
		if (strstr(subObject->name, "$cover") == 0)
			continue;

		float distance = ClosestDistanceToStaticSubObject(referencePoint, instancePose, subObject);

		if (distance < bestDistance)
		{
			bestDistance = distance;
			best = subObject;
		}
	}

	if (!best)
		return GetClosestCoverAndLeanProxy(referencePoint, instancePose, staticObject->GetParentObject());

	return best;
}



static const IStatObj::SSubObject* GetCoverAndLeanProxy(const ray_hit& rayHit)
{
	IPhysicalEntity* pCollider = rayHit.pCollider;
	pe_params_part pp; pp.partid = rayHit.partid;
	if (pCollider->GetParams(&pp) && pp.pPhysGeom->pGeom)
	{
		IStatObj* pStatObj = (IStatObj*) pp.pPhysGeom->pGeom->GetForeignData(0);

		pe_status_pos colliderPose;
		Matrix34 colliderLocalMatrix;
		colliderPose.pMtx3x4 = &colliderLocalMatrix;
		pCollider->GetStatus(&colliderPose);

		return GetClosestCoverAndLeanProxy(rayHit.pt, *colliderPose.pMtx3x4, pStatObj);
	}
	return 0;
}

struct RWIResultStruct
{
	bool			rayPending;
	bool			coverAndleanSurface;

	int				coverSurfaceIdx;

	ray_hit		hitInfo;

	RWIResultStruct() : rayPending(false), coverAndleanSurface(false)
	{	}

};

static RWIResultStruct sHitInfo;


int RWIResultCoverLean(const EventPhys *pEvent)
{
	int ret = 0;

	const EventPhysRWIResult* pResult = static_cast<const EventPhysRWIResult*>(pEvent);

	if (pResult->iForeignData == PHYS_FOREIGN_ID_USER + 1338)
	{
		int coverSurfaceIdx = (int)pResult->pForeignData;

		sHitInfo.rayPending = false;
		sHitInfo.coverAndleanSurface = false;
		int numHits = pResult->nHits;

		if (numHits > 0)
		{
			int i = pResult->pHits[0].dist < 0.0f ? 1 : 0;
			for (; numHits; ++i, --numHits)
			{
				if (pResult->pHits[i].surface_idx == sHitInfo.coverSurfaceIdx || GetCoverAndLeanProxy(pResult->pHits[i])!=0)
				{
					sHitInfo.hitInfo = pResult->pHits[i];
					sHitInfo.coverAndleanSurface = true;
				}
			}
		}

		ret = 1;
	}

	return ret;
}



bool RaycastCoverProxies(CActor* pActor, const Vec3& pos, const Vec3& dir, float length, ray_hit* rayHit, int coverSurfaceIdx)
{
	const int flags = rwi_colltype_any | rwi_ignore_back_faces | rwi_queue;
	const int entityTypes = ent_static | ent_sleeping_rigid | ent_rigid;
	const int maxHits = 4;

	if (!sHitInfo.rayPending)
	{
		sHitInfo.rayPending = true;
		sHitInfo.coverSurfaceIdx = coverSurfaceIdx;

		IPhysicalEntity *pSkipList = pActor->GetEntity()->GetPhysics();
		gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir * length, entityTypes, flags, 0, maxHits, &pSkipList, 1, 0, PHYS_FOREIGN_ID_USER + 1338);
	}

	//const int maxHits = 4;
	//ray_hit hits[maxHits];
	//int numHits = gEnv->pPhysicalWorld->RayWorldIntersection(
	//	pos, dir * length, entityTypes, flags,
	//	hits, maxHits, pActor->GetEntity()->GetPhysics());

	//if (g_pGameCVars->pl_cover_and_lean.debug_draw != 0)
	//{
	//	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(
	//		pos, ColorB(64, 0, 0),
	//		pos + dir*length, ColorB(255, 64, 64));
	//}

	//int i = hits[0].dist < 0.0f ? 1 : 0;
	//for (; numHits; ++i, --numHits)
	//{
	//	if (hits[i].surface_idx == coverSurfaceIdx || GetCoverAndLeanProxy(hits[i])!=0)
	//	{
	//		*rayHit = hits[i];
	//		return true;
	//	}
	//}

	if (sHitInfo.coverAndleanSurface)
	{
		*rayHit = sHitInfo.hitInfo;
	}

	return sHitInfo.coverAndleanSurface;
}


bool CylinderCastTest(const Vec3& pos, const Vec3& dir, float length, float radius)
{
	const int entityTypes = ent_static | ent_sleeping_rigid | ent_rigid;
	const int geometryTypes = geom_colltype0;

	primitives::cylinder cyl;
	cyl.r = radius;
	cyl.axis = dir;
	cyl.hh = length;
	cyl.center = pos + dir*length*0.5f;

	float n = 0.0f;
	geom_contact *contacts;
	intersection_params params;
	params.bStopAtFirstTri = false;
	params.bNoBorder = true;
	params.bNoAreaContacts = true;
	n = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(primitives::cylinder::type, &cyl, Vec3(ZERO), 
		entityTypes, &contacts, 0, geometryTypes, &params, 0, 0);

	int ret = (int)n;
	bool result = false;
	geom_contact* currentc = contacts;

	for (int i=0; i<ret; i++)
	{
		geom_contact* contact = currentc;
		if (contact)
		{
			IPhysicalEntity *pCollider = gEnv->pPhysicalWorld->GetPhysicalEntityById(contact->iPrim[0]);
			if (pCollider)
			{
				result = true;
				break;
			}
		}
		++currentc;
	}

	if (ret)
	{
		WriteLockCond lockColl(*params.plock, 0);
		lockColl.SetActive(1);
	}

	return result;
}




void DetectCoverBox(CActor* pActor, const Vec3& rayStart, const Vec3& rayDirection, float rayLength, int coverSurfaceIdx, CCoverAndLean::SBox* pBox, Vec3* intersectionPoint)
{
	ray_hit rayHit;
	if (!RaycastCoverProxies(pActor, rayStart, rayDirection, rayLength, &rayHit, coverSurfaceIdx))
		return;

	IPhysicalEntity* pCollider = rayHit.pCollider;
	pe_status_pos colliderPose;
	Matrix34 colliderLocalMatrix;
	colliderPose.pMtx3x4 = &colliderLocalMatrix;
	pCollider->GetStatus(&colliderPose);

	if (const IStatObj::SSubObject* pSubObject = GetCoverAndLeanProxy(rayHit))
	{
		Matrix34 local = (*colliderPose.pMtx3x4) * pSubObject->localTM;
		Vec3 localBoxMin = -pSubObject->helperSize*0.5f;
		Vec3 localBoxMax = pSubObject->helperSize*0.5f;
		CreateBox(localBoxMin, localBoxMax, local, pBox);
	}
	else
	{
		pe_params_part colliderPart;
		colliderPart.partid = rayHit.partid;
		colliderPart.ipart = rayHit.ipart;
		pCollider->GetParams(&colliderPart);

		if (colliderPart.pPhysGeom->pGeom->GetType() == GEOM_BOX)
		{
			primitives::box prim;
			colliderPart.pPhysGeom->pGeom->GetPrimitive(0, &prim);

			Matrix34 local = (*colliderPose.pMtx3x4) * prim.Basis;
			Vec3 localBoxMin = prim.Basis.GetInverted() * prim.center - prim.size;
			Vec3 localBoxMax = prim.Basis.GetInverted() * prim.center + prim.size;
			CreateBox(localBoxMin, localBoxMax, local, pBox);
		}
		else
		{
			GameWarning("Detected cover and lean proxy is not a primitive box");
		}
	}

	*intersectionPoint = rayHit.pt;
}
