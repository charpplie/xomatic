//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxSkeleton.h"
#include "MaxMaterial.h"
#include "MaxMesh.h"
#include "ModifierUtils.h"
#include "MaxPhysicsNode.h"

MaxSkeleton::MaxSkeleton()
{
	this->pBoneArray = new BoneArray();
}

MaxSkeleton::~MaxSkeleton()
{
	this->pBoneArray->DeleteChildren();
	delete this->pBoneArray;
	this->pBoneArray = 0;
}

IBoneArray* MaxSkeleton::GetBones()
{
	return this->pBoneArray;
}

void MaxSkeleton::AddBone(Bone* pBone)
{
	this->pBoneArray->AddBone(pBone);
}

MaxSkeleton::Bone::Bone(IErrorReporter* pErrorReporter, const std::string& sName, INode* pMaxNode, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, std::map<Mtl*, MaxMaterial*>& materialMap, MaxPhysicsNode* pPhysicsNode, float fMorphMinOffset)
:	pMaxNode(pMaxNode),
	pMaterial(0),
	pMesh(0),
	pPhysicsNode(pPhysicsNode),
	sName(sName),
	pParent(0)
{
	this->CreateMaterial(materialMap);
	this->pMesh = MaxMesh::Create(pErrorReporter, pMaxNode, this->pMaterial, bGenerateDefaultUVs, bAllowBlending, BoneList, fMorphMinOffset);
	this->pChildren = new BoneArray();
}

MaxSkeleton::Bone::~Bone()
{
	if (this->pMaterial)
		this->pMaterial->Release();
	delete this->pPhysicsNode;
	delete this->pChildren;
}

ISourceMaterial* MaxSkeleton::Bone::GetMaterial()
{
	return this->pMaterial;
}

ISourceMesh* MaxSkeleton::Bone::GetMesh()
{
	return this->pMesh;
}

IPhysicsNode* MaxSkeleton::Bone::GetPhysicsNode()
{
	return this->pPhysicsNode;
}

bool MaxSkeleton::Bone::MarkedNonPhysical()
{
	bool bMarkedNonPhysical = false;
	TSTR PropStr; 
	this->pMaxNode->GetUserPropBuffer(PropStr);
	if (strstr(PropStr,"nonphysical"))
			bMarkedNonPhysical = true;
	return bMarkedNonPhysical;
}

std::string MaxSkeleton::Bone::GetUserProperty()
{
	TSTR property;
	this->pMaxNode->GetUserPropBuffer(property);
	return std::string((char*)property);
}

IBoneArray* MaxSkeleton::Bone::GetChildren()
{
	return this->pChildren;
}

std::string MaxSkeleton::Bone::GetBoneName()
{
	return this->sName;
}

IBone* MaxSkeleton::Bone::GetParent()
{
	return this->pParent;
}

void MaxSkeleton::Bone::GetTransform(Matrix34& matrix)
{
	Matrix3 tm = this->pMaxNode->GetNodeTM(0);
	MaxToCryMatrix(matrix, tm);
}

void MaxSkeleton::Bone::AddChild(Bone* pBone)
{
	this->pChildren->AddBone(pBone);
}

INode* MaxSkeleton::Bone::GetMaxNode()
{
	return this->pMaxNode;
}

void MaxSkeleton::Bone::SetParent(Bone* pParent)
{
	this->pParent = pParent;
}

void MaxSkeleton::Bone::CreateMaterial(std::map<Mtl*, MaxMaterial*>& materialMap)
{
	this->pMaterial = 0;
	if (this->pMaxNode->GetMtl() != 0)
	{
		// Check whether the material has already been created - if so, we should re-use
		// that instance rather than create a new one. This makes it easier to detect
		// that it is the same material later.
		std::map<Mtl*, MaxMaterial*>::iterator itMaterialEntry = materialMap.find(this->pMaxNode->GetMtl());
		if (itMaterialEntry == materialMap.end())
		{
			// Create the material object.
			itMaterialEntry = materialMap.insert(std::make_pair(this->pMaxNode->GetMtl(), new MaxMaterial(this->pMaxNode->GetMtl(), materialMap))).first;
		}
		this->pMaterial = (*itMaterialEntry).second;
	}

	if (this->pMaterial)
		this->pMaterial->AddRef();
}

MaxSkeleton::BoneArray::BoneArray()
{
}

MaxSkeleton::BoneArray::~BoneArray()
{
	// Don't delete children - this should have been done already if necessary.
}

int MaxSkeleton::BoneArray::Count()
{
	return int(this->bones.size());
}

IBone* MaxSkeleton::BoneArray::Get(int nIndex)
{
	return this->bones[nIndex];
}

void MaxSkeleton::BoneArray::AddBone(Bone* pBone)
{
	this->bones.push_back(pBone);
}

void MaxSkeleton::BoneArray::DeleteChildren()
{
	for (std::vector<Bone*>::iterator itBone = this->bones.begin(); itBone != this->bones.end(); ++itBone)
		delete *itBone;
}
