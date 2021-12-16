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
#pragma once

#ifndef COVER_AND_LEAN_MATH_H
#define COVER_AND_LEAN_MATH_H


#include "Actor.h"
#include "CoverAndLean.h"


struct SSegment
{
	Vec3 m_vertices[2];
	Vec3 m_normal;
};

struct EventPhys;



Vec3 HorizontalNormal(const Vec3& normal);
bool IsBetweenVectors(const Vec3& minVec, const Vec3& maxVec, const Vec3& testVec);
Vec3 ClosestPointToPlane(const Plane& plane, const Vec3& point);
Vec3 ClosestPointToLine(const Vec3& origin, const Vec3& direction, const Vec3& point);
Vec3 LineIntersect2D(const Vec3& origin1, const Vec3& direction1, const Vec3& origin2, const Vec3& direction2);
Plane TransformPlane(const Matrix34& tMat, const Plane& plane);


bool SplitPolygon(const CCoverAndLean::SQuadPolygon& polygon, const Plane& splitPlane, SSegment* resultedSegment);
int GetClosestBoxFaceIndex(const CCoverAndLean::SBox& box, const Vec3& point);
float GetBoxDepthFromFace(const CCoverAndLean::SBox& box, int faceIdx);
float GetBoxDepthFromFaceAndRay(const CCoverAndLean::SBox& box, int faceIdx, const Vec3& rayPoint, const Vec3& rayDir);
void CreateBox(const Vec3& localBoxMin, const Vec3& localBoxMax, Matrix34& local, CCoverAndLean::SBox* resultBox);
bool RaycastCoverProxies(CActor* pActor, const Vec3& pos, const Vec3& dir, float length, ray_hit* rayHit, int coverSurfaceIdx);
bool CylinderCastTest(const Vec3& pos, const Vec3& dir, float length, float radius);

void DetectCoverBox(CActor* pActor, const Vec3& rayStart, const Vec3& rayDirection, float rayLength, int coverSurfaceIdx, CCoverAndLean::SBox* pBox, Vec3* intersectionPoint);

int RWIResultCoverLean(const EventPhys *pEvent);
#endif
