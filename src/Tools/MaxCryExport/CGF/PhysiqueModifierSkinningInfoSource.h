//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __PHYSIQUEMODIFIERSKINNINGINFOSOURCE_H__
#define __PHYSIQUEMODIFIERSKINNINGINFOSOURCE_H__

#include "SkinningInfoSourceBase.h"

class PhysiqueModifierSkinningInfoSource : public SkinningInfoSourceBase
{
public:
	PhysiqueModifierSkinningInfoSource(INode* pNode, NameList& boneList, bool bAllowBlending);
	virtual ~PhysiqueModifierSkinningInfoSource();

	virtual int GetNumBonesForVertex(int nVertex);
	virtual void GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Point3& v3WorldPosition, VertexBoneLinkInfo& info);
	virtual bool GetBoneInitialPosition(INode* pNode, Matrix3& initialTransform);

private:
	INode* pNode;
	Modifier* pModifier;
	IPhysiqueExport* pPhysiqueExport;
	IPhyContextExport* pPhysiqueContextExport;
};

#endif //__PHYSIQUEMODIFIERSKINNINGINFOSOURCE_H__
