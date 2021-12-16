//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ASSETFILEWRITER_H__
#define __ASSETFILEWRITER_H__

#include "CryHeaders.h"
#include <stdio.h>
#include <string>

class ISkinningInfo;
class ISkeleton;
class IProgressMonitor;
class IErrorReporter;
class IChunkList;

class AssetWriter
{
public:
	AssetWriter(FILE* f, IErrorReporter* pErrorReporter, IProgressMonitor* pProgressMonitor);

	bool SaveBoneInfo(ISkeleton* pSkeleton, ISkinningInfo* pSkinningInfo, const Matrix34& objectTransform, const std::string& sNodeName);
	bool SaveBoneInitialPos(ISkinningInfo* pSkinningInfo, ISkeleton* pSkeleton, int nChunkIdMesh, IChunkList* pChunkList);

private:
	static unsigned CalculateBoneInitPos(ISkinningInfo* pSkinningInfo, ISkeleton* pSkeleton, Matrix34* pBoneInitPos);

	FILE* f;
	IErrorReporter* pErrorReporter;
	IProgressMonitor* pProgressMonitor;
};

#endif //__ASSETFILEWRITER_H__
