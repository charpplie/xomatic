//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXMATERIAL_H__
#define __MAXMATERIAL_H__

#include "ISourceMaterial.h"
#include "ISourceMaterialArray.h"

class Mtl;
class MaxMaterial;

class MaxMaterialArray : public ISourceMaterialArray
{
public:
	MaxMaterialArray();
	virtual ~MaxMaterialArray();

	virtual int Count();
	virtual ISourceMaterial* Get(int i);

	void AddMaterial(MaxMaterial* pMaterial);
	MaxMaterial* GetMaxMaterial(int i);

private:
	std::vector<MaxMaterial*> materials;
};

class MaxMaterial : public ISourceMaterial
{
public:
	explicit MaxMaterial(Mtl* pMaxMaterial, std::map<Mtl*, MaxMaterial*>& materialMap);
	virtual ~MaxMaterial();

	virtual std::string GetName();
	virtual ISourceMaterialArray* GetSubMaterials();
	virtual CrytekShader* GetCrytekShader();

	// TEMPORARARY workaround - TAKE THIS OUT ONCE CODE IS SUFFICIENTLY REFACTORED.
	virtual Mtl* GetMaxMaterial();

	void AddRef();
	void Release();

	bool IsMultiMaterial();

private:
	void CreateSubMaterials(std::map<Mtl*, MaxMaterial*>& materialMap);

	Mtl* pMaxMaterial;
	MaxMaterialArray* pSubMaterials;
	int refCount;
};

#endif //__MAXMATERIAL_H__
