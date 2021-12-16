//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISKELETON_H__
#define __ISKELETON_H__

class IBoneArray;

class ISkeleton
{
public:
	virtual ~ISkeleton() {}
	virtual IBoneArray* GetBones() = 0;
};

#endif //__ISKELETON_H__
