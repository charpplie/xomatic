//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISOURCEMATERIALARRAY_H__
#define __ISOURCEMATERIALARRAY_H__

class ISourceMaterial;

class ISourceMaterialArray
{
public:
	virtual ~ISourceMaterialArray() {}

	virtual int Count() = 0;
	virtual ISourceMaterial* Get(int i) = 0;
};

#endif //__ISOURCEMATERIALARRAY_H__
