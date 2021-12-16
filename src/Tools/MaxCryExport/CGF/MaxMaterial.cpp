//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxMaterial.h"
#include "../CryShader/CryShader.h"

MaxMaterialArray::MaxMaterialArray()
{
}

MaxMaterialArray::~MaxMaterialArray()
{
	for (std::vector<MaxMaterial*>::iterator itMaterial = this->materials.begin(); itMaterial != this->materials.end(); ++itMaterial)
	{
		if ((*itMaterial))
			(*itMaterial)->Release();
	}
}

int MaxMaterialArray::Count()
{
	return int(this->materials.size());
}

ISourceMaterial* MaxMaterialArray::Get(int i)
{
	return this->materials[i];
}

void MaxMaterialArray::AddMaterial(MaxMaterial* pMaterial)
{
	this->materials.push_back(pMaterial);
}

MaxMaterial* MaxMaterialArray::GetMaxMaterial(int i)
{
	return this->materials[i];
}

MaxMaterial::MaxMaterial(Mtl* pMaxMaterial, std::map<Mtl*, MaxMaterial*>& materialMap)
:	pMaxMaterial(pMaxMaterial),
	pSubMaterials(0),
	refCount(0)
{
	CreateSubMaterials(materialMap);
}

MaxMaterial::~MaxMaterial()
{
	if (this->pSubMaterials != 0)
		delete this->pSubMaterials;
}

std::string MaxMaterial::GetName()
{
	return this->pMaxMaterial->GetName().data();
}

ISourceMaterialArray* MaxMaterial::GetSubMaterials()
{
	return this->pSubMaterials;
}

CrytekShader* MaxMaterial::GetCrytekShader()
{
	CrytekShader* pCryShader = 0;

	if (this->pMaxMaterial->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
	{
		StdMat2 *pStdMat = (StdMat2*)this->pMaxMaterial;
		Shader *pShader = pStdMat->GetShader();
		if (pShader && pShader->ClassID() == CrytekShaderClassID)
			pCryShader = (CrytekShader*)pShader;
	}

	return pCryShader;
}

Mtl* MaxMaterial::GetMaxMaterial()
{
	return this->pMaxMaterial;
}

void MaxMaterial::AddRef()
{
	++this->refCount;
}

void MaxMaterial::Release()
{
	--this->refCount;
	if (this->refCount <= 0)
		delete this;
}

bool MaxMaterial::IsMultiMaterial()
{
	return this->pMaxMaterial->IsMultiMtl();
}

void MaxMaterial::CreateSubMaterials(std::map<Mtl*, MaxMaterial*>& materialMap)
{
	// Check whether this material has sub-materials.
	if (this->pMaxMaterial->ClassID() == Class_ID(MULTI_CLASS_ID,0))
	{
		// Create a container for the sub-materials.
		this->pSubMaterials = new MaxMaterialArray();

		// Create material objects for all the sub-materials.
		for (int i = 0; i < this->pMaxMaterial->NumSubMtls(); ++i)
		{
			if (this->pMaxMaterial->GetSubMtl(i) != 0)
			{
				// Check whether the material has already been created - if so, we should re-use
				// that instance rather than create a new one. This makes it easier to detect
				// that it is the same material later.
				std::map<Mtl*, MaxMaterial*>::iterator itMaterialEntry = materialMap.find(this->pMaxMaterial->GetSubMtl(i));
				if (itMaterialEntry == materialMap.end())
				{
					// Create the material object.
					itMaterialEntry = materialMap.insert(std::make_pair(this->pMaxMaterial->GetSubMtl(i), new MaxMaterial(this->pMaxMaterial->GetSubMtl(i), materialMap))).first;
				}
				this->pSubMaterials->AddMaterial((*itMaterialEntry).second);
				(*itMaterialEntry).second->AddRef();
			}
			else
			{
				// Materials seem to be able to have null sub-materials. Personally I would prefer
				// just to skip them, but I want to make sure that the ID is the same, so we add a null
				// material here.
				this->pSubMaterials->AddMaterial(0);
			}
		}
	}
}
