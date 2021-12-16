//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXSKINNINGINFO_H__
#define __MAXSKINNINGINFO_H__

#include "ISkinningInfo.h"

class ISkinningInfoSource;

class MaxSkinningInfo : public ISkinningInfo
{
public:
	MaxSkinningInfo(ISkinningInfoSource* pSource, INode* pMaxNode, Point3 *Vertices, int* arrVertexMap, int nVertexMapSize);
	virtual ~MaxSkinningInfo();

	virtual int GetNumVertices();
	virtual void GetVertex(int i, Vec3& point);
	virtual int GetNumBonesForVertex(int nVertex);
	virtual void GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Vec3& v3WorldPosition, VertexBoneLinkInfo& info);
	virtual Error* GetError();
	virtual int GetNumBoneOutOfRangeWarnings();
	virtual void GetBoneOutOfRangeWarning(int nIndex, BoneOutOfRangeWarning& warning);
	virtual bool GetBoneInitialPosition(IBone* pBone, Matrix34& initialTransform);

private:
	ISkinningInfoSource* pSource;
	INode* pMaxNode;
	Point3 *Vertices;
	int* arrVertexMap;
	int nVertexMapSize;
};

#endif //__MAXSKINNINGINFO_H__
