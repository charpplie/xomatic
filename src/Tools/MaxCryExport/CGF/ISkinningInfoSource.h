//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISKINNINGINFOSOURCE_H__
#define __ISKINNINGINFOSOURCE_H__

#include "ISkinningInfo.h"

class ISkinningInfoSource
{
public:
	typedef ISkinningInfo::VertexBoneLinkInfo VertexBoneLinkInfo;
	typedef ISkinningInfo::Error Error;
	typedef ISkinningInfo::BoneOutOfRangeWarning BoneOutOfRangeWarning;

	virtual ~ISkinningInfoSource() {}

	virtual int GetNumBonesForVertex(int nVertex) = 0;
	virtual void GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Point3& v3WorldPosition, VertexBoneLinkInfo& info) = 0;
	virtual Error* GetError() = 0;
	virtual int GetNumBoneOutOfRangeWarnings() = 0;
	virtual void GetBoneOutOfRangeWarning(int nIndex, BoneOutOfRangeWarning& warning) = 0;
	virtual bool GetBoneInitialPosition(INode* pNode, Matrix3& initialTransform) = 0;
};

#endif //__ISKINNINGINFOSOURCE_H__
