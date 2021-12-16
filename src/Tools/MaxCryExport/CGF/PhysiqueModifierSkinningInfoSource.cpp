//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include <StdAfx.h>
#include "PhysiqueModifierSkinningInfoSource.h"
#include "ModifierUtils.h"

PhysiqueModifierSkinningInfoSource::PhysiqueModifierSkinningInfoSource(INode* pNode, NameList& boneList, bool bAllowBlending)
:	SkinningInfoSourceBase(boneList)
{
	// Store the node.
	this->pNode = pNode;

	// Find the physique modifier for the node.
	this->pModifier = FindPhysiqueModifier(this->pNode->GetObjectRef());
	if (this->pModifier == 0)
	{
		this->SetError("Cannot find skinning info - node has no Physique modifier.");
		return;
	}

	// Get an export helper interface from the physique modifier.
	this->pPhysiqueExport = (IPhysiqueExport*)(this->pModifier->GetInterface(I_PHYINTERFACE));
	if (this->pPhysiqueExport == 0)
	{
		this->SetError("Cannot get export helper interface from physique modifier.");
		return;
	}

	// Get the context data for the modifier on this node.
	this->pPhysiqueContextExport = this->pPhysiqueExport->GetContextInterface(this->pNode);
	if (this->pPhysiqueContextExport == 0)
	{
		this->SetError("Cannot get context data for physique modifier.");
		return;
	}

	// Allow blending for the physique modifier.
	this->pPhysiqueContextExport->AllowBlending(bAllowBlending);
	this->pPhysiqueContextExport->ConvertToRigid();
}

PhysiqueModifierSkinningInfoSource::~PhysiqueModifierSkinningInfoSource()
{
	if (this->pPhysiqueExport != 0 && this->pPhysiqueContextExport != 0)
		this->pPhysiqueExport->ReleaseContextInterface(this->pPhysiqueContextExport);
	if (this->pPhysiqueExport != 0 && this->pModifier != 0)
		this->pModifier->ReleaseInterface(I_PHYINTERFACE, this->pPhysiqueExport);
}

int PhysiqueModifierSkinningInfoSource::GetNumBonesForVertex(int nVertex)
{
	if (this->bError)
		return -1;

	// Get the information about this vertex.
	IPhyVertexExport* pVertex = this->pPhysiqueContextExport->GetVertexInterface(nVertex);
	if (!pVertex)
		return -1;

	// Find the type of vertex this is.
	int nVertexType = pVertex->GetVertexType();
	int nNumNodes = -1;
	switch (nVertexType)
	{
	case RIGID_NON_BLENDED_TYPE:
		nNumNodes = 1;
		break;

	case RIGID_BLENDED_TYPE:
		nNumNodes = ((IPhyBlendedRigidVertex*)pVertex)->GetNumberNodes();
		break;

	default:
		this->SetError("There are unsupported Physique links - check \"Force Rigid\" checkbox.");
		nNumNodes = -1;
		break;
	}

	// Free the vertex interface.
	if (this->pPhysiqueContextExport != 0)
		this->pPhysiqueContextExport->ReleaseVertexInterface(pVertex);

	return nNumNodes;
}

void PhysiqueModifierSkinningInfoSource::GetBoneLinkInfoForVertex(int nVertex, int nBoneLink, const Point3& v3WorldPosition, VertexBoneLinkInfo& info)
{
	if (this->bError)
		return;

	// Get the information about this vertex.
	IPhyVertexExport* pVertex = this->pPhysiqueContextExport->GetVertexInterface(nVertex);
	if (!pVertex)
		return;

	// Find the type of vertex this is.
	int nVertexType = pVertex->GetVertexType();
	switch (nVertexType)
	{
	case RIGID_NON_BLENDED_TYPE:
		{
			IPhyRigidVertex* pRigidVertex = (IPhyRigidVertex*)pVertex;

			// Set the link parameters.
			info.nBoneID = this->LookupBone(pRigidVertex->GetNode(), nVertex);
			info.fBlendingWeight = 1.0f;
			info.offset.x = pRigidVertex->GetOffsetVector().x;
			info.offset.y = pRigidVertex->GetOffsetVector().y;
			info.offset.z = pRigidVertex->GetOffsetVector().z;
		}
		break;

	case RIGID_BLENDED_TYPE:
		{
			IPhyBlendedRigidVertex* pBlendedVertex = (IPhyBlendedRigidVertex*)pVertex;

			// Set the link parameters.
			info.nBoneID = this->LookupBone(pBlendedVertex->GetNode(nBoneLink), nVertex);
			info.fBlendingWeight = pBlendedVertex->GetWeight(nBoneLink);
			info.offset.x = pBlendedVertex->GetOffsetVector(nBoneLink).x;
			info.offset.y = pBlendedVertex->GetOffsetVector(nBoneLink).y;
			info.offset.z = pBlendedVertex->GetOffsetVector(nBoneLink).z;
		}
		break;

	default:
		this->SetError("There are unsupported Physique links - check \"Force Rigid\" checkbox.");
		break;
	}

	if (this->pPhysiqueContextExport != 0)
		this->pPhysiqueContextExport->ReleaseVertexInterface(pVertex);
}

bool PhysiqueModifierSkinningInfoSource::GetBoneInitialPosition(INode* pNode, Matrix3& initialTransform)
{
	return (MATRIX_RETURNED == this->pPhysiqueExport->GetInitNodeTM(pNode, initialTransform));
}
