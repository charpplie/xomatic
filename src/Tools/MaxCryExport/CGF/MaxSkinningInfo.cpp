//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "ModifierUtils.h"
#include "MaxSkinningInfo.h"
#include "MaxSkeleton.h"
#include "ISkinningInfoSource.h"

MaxSkinningInfo::MaxSkinningInfo(ISkinningInfoSource* pSource, INode* pMaxNode, Point3 *Vertices, int* arrVertexMap, int nVertexMapSize)
:	pSource(pSource),
	pMaxNode(pMaxNode),
	Vertices(Vertices),
	arrVertexMap(arrVertexMap),
	nVertexMapSize(nVertexMapSize)
{
}

MaxSkinningInfo::~MaxSkinningInfo()
{
}

int MaxSkinningInfo::GetNumVertices()
{
	return this->nVertexMapSize;
}

void MaxSkinningInfo::GetVertex(int i, Vec3& point)
{
	point.x = this->Vertices[i].x;
	point.y = this->Vertices[i].y;
	point.z = this->Vertices[i].z;
}

int MaxSkinningInfo::GetNumBonesForVertex(int nVertex)
{
	return this->pSource->GetNumBonesForVertex(this->arrVertexMap[nVertex]);
}

void MaxSkinningInfo::GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Vec3& v3WorldPosition, VertexBoneLinkInfo& info)
{
	Point3 tempPoint;
	tempPoint.x = v3WorldPosition.x;
	tempPoint.y = v3WorldPosition.y;
	tempPoint.z = v3WorldPosition.z;
	this->pSource->GetBoneLinkInfoForVertex(this->arrVertexMap[nVertex], nBoneLink, tempPoint, info);
}

MaxSkinningInfo::Error* MaxSkinningInfo::GetError()
{
	return this->pSource->GetError();
}

int MaxSkinningInfo::GetNumBoneOutOfRangeWarnings()
{
	return this->pSource->GetNumBoneOutOfRangeWarnings();
}

void MaxSkinningInfo::GetBoneOutOfRangeWarning(int nIndex, BoneOutOfRangeWarning& warning)
{
	this->pSource->GetBoneOutOfRangeWarning(nIndex, warning);
}

bool MaxSkinningInfo::GetBoneInitialPosition(IBone* pBone, Matrix34& initialTransform)
{
	Matrix3 maxTransform;
	bool bSuccess = this->pSource->GetBoneInitialPosition(static_cast<MaxSkeleton::Bone*>(pBone)->GetMaxNode(), maxTransform);
	if (bSuccess)
		MaxToCryMatrix(initialTransform, maxTransform);
	return bSuccess;
}
