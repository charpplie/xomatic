//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISOURCEMATERIAL_H__
#define __ISOURCEMATERIAL_H__

#include <string>

class ISourceMaterialArray;
class CrytekShader;

class ISourceMaterial
{
public:
	virtual ~ISourceMaterial() {}

	virtual std::string GetName() = 0;
	virtual ISourceMaterialArray* GetSubMaterials() = 0;
	virtual CrytekShader* GetCrytekShader() = 0;

	// TEMPORARARY workaround - TAKE THIS OUT ONCE CODE IS SUFFICIENTLY REFACTORED.
	virtual Mtl* GetMaxMaterial() = 0;
};

#endif //__ISOURCEMATERIAL_H__
