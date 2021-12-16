//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxObject.h"
#include "MaxMaterial.h"
#include "MaxMesh.h"
#include "ModifierUtils.h"

MaxObject::MaxObject(IErrorReporter* pErrorReporter, INode* pMaxNode, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, std::map<Mtl*, MaxMaterial*>& materialMap, float fMorphMinOffset)
:	pMaxNode(pMaxNode),
	pMaterial(0),
	pMesh(0),
	pPositionController(0),
	pRotationController(0),
	pScaleController(0),
	pParent(0)
{
	this->CreateMaterial(materialMap);
	this->pMesh = MaxMesh::Create(pErrorReporter, pMaxNode, this->pMaterial, bGenerateDefaultUVs, bAllowBlending, BoneList, fMorphMinOffset);
	this->CreateControllers();
}

MaxObject::~MaxObject()
{
	if (this->pMaterial)
		this->pMaterial->Release();
	delete this->pMesh;
	delete this->pPositionController;
	delete this->pRotationController;
	delete this->pScaleController;
}

INode* MaxObject::GetMaxNode()
{
	return this->pMaxNode;
}

IController* MaxObject::GetPositionController()
{
	return this->pPositionController;
}

IController* MaxObject::GetRotationController()
{
	return this->pRotationController;
}

IController* MaxObject::GetScaleController()
{
	return this->pScaleController;
}

void MaxObject::GetTransform(Matrix34& transform)
{
	Matrix3 maxTransform = this->pMaxNode->GetObjTMAfterWSM(0);
	MaxToCryMatrix(transform, maxTransform);
}

void MaxObject::GetObjectOffsetTransform(Matrix34& transform)
{
	Matrix3 tm(1);
	Point3 pos = this->pMaxNode->GetObjOffsetPos();
	tm.PreTranslate(pos);
	Quat quat = this->pMaxNode->GetObjOffsetRot();
	PreRotateMatrix(tm, quat);
	ScaleValue scaleValue = this->pMaxNode->GetObjOffsetScale();
	ApplyScaling(tm, scaleValue);
	Matrix34 cryMatrix;
	MaxToCryMatrix(transform, tm);
}

ISourceObject* MaxObject::GetParent()
{
	return this->pParent;
}

void MaxObject::SetParent(MaxObject* pObject)
{
	this->pParent = pObject;
}

std::string MaxObject::GetName()
{
	return this->pMaxNode->GetName();
}

ISourceMaterial* MaxObject::GetMaterial()
{
	return this->pMaterial;
}

ISourceMesh* MaxObject::GetMesh()
{
	return this->pMesh;
}

void MaxObject::CreateMaterial(std::map<Mtl*, MaxMaterial*>& materialMap)
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

void MaxObject::CreateControllers()
{
	Control* pControl = this->pMaxNode->GetTMController();
	if (pControl != 0 && pControl->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID,0))
	{
		if (pControl->GetPositionController() != 0)
			this->pPositionController = new MaxController(pControl->GetPositionController());
		if (pControl->GetRotationController() != 0)
			this->pRotationController = new MaxController(pControl->GetRotationController());
		if (pControl->GetScaleController() != 0)
			this->pScaleController = new MaxController(pControl->GetScaleController());
	}
}

MaxObjectArray::MaxObjectArray()
{
}

MaxObjectArray::~MaxObjectArray()
{
}

int MaxObjectArray::Count()
{
	return int(this->objects.size());
}

ISourceObject* MaxObjectArray::Get(int i)
{
	return this->objects[i];
}

//////////////////////////////////////////////////////////////////////////
MaxObject* MaxObjectArray::FindObject( INode *pMaxNode )
{
	int num = (int)objects.size();
	for (int i = 0; i < num; i++)
	{
		if (objects[i]->GetMaxNode() == pMaxNode)
			return objects[i];
	}
	return 0;
}

MaxObject* MaxObjectArray::AddObject(IErrorReporter* pErrorReporter, INode* pMaxNode, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, std::map<Mtl*, MaxMaterial*>& materialMap, float fMorphMinOffset,bool bRecursive)
{
	MaxObject* pObject = FindObject(pMaxNode);
	if (pObject)
		return pObject;
	
	pObject = new MaxObject(pErrorReporter, pMaxNode, bGenerateDefaultUVs, bAllowBlending, BoneList, materialMap, fMorphMinOffset);
	this->objects.push_back(pObject);
	if (bRecursive)
	{
		for (int i = 0; i < pMaxNode->NumberOfChildren(); i++)
		{
			INode *pChildNode = pMaxNode->GetChildNode(i);
			MaxObject *pChildObj = AddObject(pErrorReporter,pChildNode,bGenerateDefaultUVs,bAllowBlending,BoneList,materialMap,fMorphMinOffset,bRecursive);
			pChildObj->SetParent(pObject);
		}
	}
	return pObject;
}
