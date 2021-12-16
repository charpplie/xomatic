//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __SKINNINGINFOSOURCEBASE_H__
#define __SKINNINGINFOSOURCEBASE_H__

#include "ISkinningInfoSource.h"
#include "NameList.h"

class SkinningInfoSourceBase : public ISkinningInfoSource
{
public:
	SkinningInfoSourceBase(NameList& boneList);

	virtual Error* GetError();
	virtual int GetNumBoneOutOfRangeWarnings();
	virtual void GetBoneOutOfRangeWarning(int nIndex, BoneOutOfRangeWarning& warning);

protected:
	void SetError(const std::string& sDescription);
	void SetBoneOutOfRangeWarning(int nVertex, const std::string& sBoneName);
	int LookupBone(INode* pBoneNode, int nVertex);

	Error error;
	bool bError;
	std::vector<BoneOutOfRangeWarning> boneOutOfRangeWarnings;
	NameList& boneList;
};

#endif //__SKINNINGINFOSOURCEBASE_H__
