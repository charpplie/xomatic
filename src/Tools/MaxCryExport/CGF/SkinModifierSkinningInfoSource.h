//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __SKINMODIFIERSKINNINGINFOSOURCE_H__
#define __SKINMODIFIERSKINNINGINFOSOURCE_H__

#include "SkinningInfoSourceBase.h"
#include <iskin.h>

class SkinModifierSkinningInfoSource : public SkinningInfoSourceBase
{
public:
	SkinModifierSkinningInfoSource(INode* pNode, NameList& boneList);
	virtual ~SkinModifierSkinningInfoSource();

	virtual int GetNumBonesForVertex(int nVertex);
	virtual void GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Point3& v3WorldPosition, VertexBoneLinkInfo& info);
	virtual bool GetBoneInitialPosition(INode* pNode, Matrix3& initialTransform);

private:
	Modifier* pModifier;
	ISkin* pSkin;
	ISkinContextData* pSkinContext;
};

#endif //__SKINMODIFIERSKINNINGINFOSOURCE_H__
