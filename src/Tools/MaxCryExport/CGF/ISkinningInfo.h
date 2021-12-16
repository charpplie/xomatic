//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISKINNINGINFO_H__
#define __ISKINNINGINFO_H__

class IBone;

#include <string>

class ISkinningInfo
{
public:
	class VertexBoneLinkInfo
	{
	public:
		Vec3 offset;
		int nBoneID;
		float fBlendingWeight;
	};

	class Error
	{
	public:
		std::string sDescription;
	};

	class BoneOutOfRangeWarning
	{
	public:
		int nVertex;
		std::string sBoneName;
	};

	virtual ~ISkinningInfo() {}

	virtual int GetNumVertices() = 0;
	virtual void GetVertex(int i, Vec3& point) = 0;
	virtual int GetNumBonesForVertex(int nVertex) = 0;
	virtual void GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Vec3& v3WorldPosition, VertexBoneLinkInfo& info) = 0;
	virtual Error* GetError() = 0;
	virtual int GetNumBoneOutOfRangeWarnings() = 0;
	virtual void GetBoneOutOfRangeWarning(int nIndex, BoneOutOfRangeWarning& warning) = 0;
	virtual bool GetBoneInitialPosition(IBone* pBone, Matrix34& initialTransform) = 0;
};

#endif //__ISKINNINGINFO_H__
