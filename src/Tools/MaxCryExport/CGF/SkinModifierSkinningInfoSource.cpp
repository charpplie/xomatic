//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "ModifierUtils.h"
#include "SkinModifierSkinningInfoSource.h"

SkinModifierSkinningInfoSource::SkinModifierSkinningInfoSource(INode* pNode, NameList& boneList)
:	SkinningInfoSourceBase(boneList)
{
	// Find the skin modifier for the node.
	this->pModifier = FindSkinModifier(pNode->GetObjectRef());
	if (this->pModifier == 0)
	{
		this->SetError("Cannot find skinning info - node has no Skin modifier.");
		return;
	}

	// Look for a skin interface on the skin modifier.
	this->pSkin = (ISkin*)this->pModifier->GetInterface(I_SKIN);
	if (this->pSkin == 0)
	{
		this->SetError("Cannot get skin interface from Skin modifier.");
		return;
	}

	// Look for the context data for the skin modifier on the node.
	this->pSkinContext = this->pSkin->GetContextInterface(pNode);
	if (this->pSkinContext == 0)
	{
		this->SetError("Cannot get context data for Skin modifier.");
		return;
	}
}

SkinModifierSkinningInfoSource::~SkinModifierSkinningInfoSource()
{
	if (this->pModifier != 0 && this->pSkin != 0)
		this->pModifier->ReleaseInterface(I_SKIN, this->pSkin);
}

int SkinModifierSkinningInfoSource::GetNumBonesForVertex(int nVertex)
{
	if (this->bError)
		return -1;

	return this->pSkinContext->GetNumAssignedBones(nVertex);
}

void SkinModifierSkinningInfoSource::GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Point3& v3WorldPosition, VertexBoneLinkInfo& info)
{
	if (this->bError)
		return;

	info.nBoneID = this->LookupBone(this->pSkin->GetBone(this->pSkinContext->GetAssignedBone(nVertex, nBoneLink)), nVertex);
	info.fBlendingWeight = this->pSkinContext->GetBoneWeight(nVertex, nBoneLink);
	Point3 offset = v3WorldPosition * this->boneList.invTM[info.nBoneID];
	info.offset.x = offset.x;
	info.offset.y = offset.y;
	info.offset.z = offset.z;
}

bool SkinModifierSkinningInfoSource::GetBoneInitialPosition(INode* pNode, Matrix3& initialTransform)
{
	return (SKIN_OK == this->pSkin->GetBoneInitTM(pNode, initialTransform));
}
