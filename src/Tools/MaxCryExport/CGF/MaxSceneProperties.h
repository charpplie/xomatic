//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXSCENEPROPERTIES_H__
#define __MAXSCENEPROPERTIES_H__

#include <string>

#include "ISceneProperties.h"

class Interface;

class MaxSceneProperties : public ISceneProperties
{
public:
	MaxSceneProperties(Interface* pMaxInterface);
	virtual ~MaxSceneProperties();

	virtual int PropertyCount();
	virtual void GetProperty(int i, std::string& sName, std::string& sValue);

private:
	void VariantToString (const PROPVARIANT* pProp, TCHAR* szString, int bufSize);

	Interface* pMaxInterface;
};

#endif //__MAXSCENEPROPERTIES_H__
