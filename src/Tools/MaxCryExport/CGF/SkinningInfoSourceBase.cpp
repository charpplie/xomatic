//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "SkinningInfoSourceBase.h"

SkinningInfoSourceBase::SkinningInfoSourceBase(NameList& boneList)
:	boneList(boneList)
{
	// Initially there is no error.
	this->bError = false;
}

SkinningInfoSourceBase::Error* SkinningInfoSourceBase::GetError()
{
	if (this->bError)
		return &this->error;

	return 0;
}

int SkinningInfoSourceBase::GetNumBoneOutOfRangeWarnings()
{
	return int(this->boneOutOfRangeWarnings.size());
}

void SkinningInfoSourceBase::GetBoneOutOfRangeWarning(int nIndex, BoneOutOfRangeWarning& warning)
{
	warning = this->boneOutOfRangeWarnings[nIndex];
}

void SkinningInfoSourceBase::SetError(const std::string& sDescription)
{
	this->error.sDescription = sDescription;
	this->bError = true;
}

void SkinningInfoSourceBase::SetBoneOutOfRangeWarning(int nVertex, const std::string& sBoneName)
{
	BoneOutOfRangeWarning warning;
	warning.nVertex = nVertex;
	warning.sBoneName = sBoneName;
	this->boneOutOfRangeWarnings.push_back(warning);
}

int SkinningInfoSourceBase::LookupBone(INode* pBoneNode, int nVertex)
{
	int nBoneID = this->boneList.GetID(pBoneNode, false);
	if (nBoneID == -1 && pBoneNode)
		this->SetBoneOutOfRangeWarning(nVertex, pBoneNode->GetName());
	return nBoneID;
}
